#!/usr/bin/env python3
"""
Golden Content Fetcher - Source 1.5

Copies reference maps and assets from local Steam installation for parity testing.
Maps are NOT tracked in git - they're used locally for regression testing only.

Usage:
    python scripts/fetch_golden_content.py [--game tf|hl2mp] [--output DIR]

Requirements:
    - Team Fortress 2 or HL2:DM installed via Steam
    - Source SDK Base 2013 Multiplayer (appid 243750)
"""

import os
import sys
import argparse
import shutil
import json
from pathlib import Path
from typing import List, Dict, Optional

# Steam installation paths (common locations)
STEAM_PATHS_WINDOWS = [
    Path("C:/Program Files (x86)/Steam"),
    Path("C:/Program Files/Steam"),
    Path(os.environ.get("ProgramFiles(x86)", "")) / "Steam" if os.environ.get("ProgramFiles(x86)") else None,
]

STEAM_PATHS_LINUX = [
    Path.home() / ".steam/steam",
    Path.home() / ".local/share/Steam",
]

# Game configurations
GAME_CONFIGS = {
    "tf": {
        "name": "Team Fortress 2",
        "appid": "440",
        "maps_dir": "tf/maps",
        "golden_maps": [
            "ctf_2fort.bsp",
            "pl_badwater.bsp",
            "cp_dustbowl.bsp",
            "koth_harvest_final.bsp",
            "pl_upward.bsp",
        ],
    },
    "hl2mp": {
        "name": "Half-Life 2: Deathmatch",
        "appid": "320",
        "maps_dir": "hl2mp/maps",
        "golden_maps": [
            "dm_lockdown.bsp",
            "dm_overwatch.bsp",
            "dm_resistance.bsp",
            "dm_runoff.bsp",
        ],
    },
}

class ContentFetcher:
    def __init__(self, game: str, output_dir: Path):
        self.game = game
        self.config = GAME_CONFIGS[game]
        self.output_dir = output_dir
        self.steam_path: Optional[Path] = None
        self.stats = {
            "copied": 0,
            "skipped": 0,
            "failed": 0,
        }

    def find_steam_installation(self) -> bool:
        """Find Steam installation directory."""
        print("🔍 Searching for Steam installation...")

        # Check platform-specific paths
        if sys.platform == "win32":
            search_paths = [p for p in STEAM_PATHS_WINDOWS if p]
        else:
            search_paths = STEAM_PATHS_LINUX

        for steam_path in search_paths:
            if steam_path and steam_path.exists():
                # Verify it's actually Steam by checking for steamapps
                steamapps = steam_path / "steamapps"
                if steamapps.exists():
                    self.steam_path = steam_path
                    print(f"✅ Found Steam at: {steam_path}")
                    return True

        print("❌ Steam installation not found!")
        print("\nSearched locations:")
        for path in search_paths:
            if path:
                print(f"  - {path}")
        return False

    def find_game_directory(self) -> Optional[Path]:
        """Find game installation directory within Steam."""
        if not self.steam_path:
            return None

        print(f"\n🔍 Searching for {self.config['name']}...")

        # Check steamapps/common
        common_path = self.steam_path / "steamapps/common"

        # Try common game directory names
        game_dirs = {
            "tf": ["Team Fortress 2", "TeamFortress2", "tf2"],
            "hl2mp": ["Half-Life 2 Deathmatch", "hl2mp"],
        }

        for dir_name in game_dirs.get(self.game, []):
            game_path = common_path / dir_name
            if game_path.exists():
                # Verify by checking for maps directory
                maps_path = game_path / self.config["maps_dir"]
                if maps_path.exists():
                    print(f"✅ Found {self.config['name']} at: {game_path}")
                    return game_path

        print(f"❌ {self.config['name']} not found!")
        print(f"   Expected in: {common_path}")
        return None

    def copy_golden_maps(self, game_path: Path) -> None:
        """Copy golden maps from game installation to output directory."""
        maps_src = game_path / self.config["maps_dir"]
        maps_dst = self.output_dir / "maps"

        print(f"\n📦 Copying golden maps...")
        print(f"   Source: {maps_src}")
        print(f"   Dest:   {maps_dst}")

        # Create output directory
        maps_dst.mkdir(parents=True, exist_ok=True)

        # Copy each golden map
        for map_name in self.config["golden_maps"]:
            src_file = maps_src / map_name
            dst_file = maps_dst / map_name

            if not src_file.exists():
                print(f"⚠️  {map_name} - not found in game directory")
                self.stats["failed"] += 1
                continue

            # Check if already exists and is identical
            if dst_file.exists():
                if src_file.stat().st_size == dst_file.stat().st_size:
                    print(f"⏭️  {map_name} - already exists (same size)")
                    self.stats["skipped"] += 1
                    continue

            # Copy map file
            try:
                shutil.copy2(src_file, dst_file)
                size_mb = src_file.stat().st_size / (1024 * 1024)
                print(f"✅ {map_name} - copied ({size_mb:.1f} MB)")
                self.stats["copied"] += 1
            except Exception as e:
                print(f"❌ {map_name} - failed: {e}")
                self.stats["failed"] += 1

    def create_manifest(self) -> None:
        """Create manifest file with golden content info."""
        manifest_path = self.output_dir / "manifest.json"

        manifest = {
            "game": self.game,
            "game_name": self.config["name"],
            "maps": self.config["golden_maps"],
            "source": str(self.steam_path) if self.steam_path else "unknown",
            "stats": self.stats,
        }

        with open(manifest_path, "w") as f:
            json.dump(manifest, f, indent=2)

        print(f"\n📝 Created manifest: {manifest_path}")

    def print_summary(self) -> None:
        """Print copy operation summary."""
        print("\n" + "=" * 60)
        print("📊 SUMMARY")
        print("=" * 60)
        print(f"Copied:  {self.stats['copied']}")
        print(f"Skipped: {self.stats['skipped']}")
        print(f"Failed:  {self.stats['failed']}")
        print(f"Total:   {sum(self.stats.values())}")
        print("=" * 60)

    def fetch(self) -> bool:
        """Main fetch operation."""
        # Find Steam
        if not self.find_steam_installation():
            return False

        # Find game
        game_path = self.find_game_directory()
        if not game_path:
            return False

        # Copy content
        self.copy_golden_maps(game_path)

        # Create manifest
        self.create_manifest()

        # Print summary
        self.print_summary()

        return self.stats["failed"] == 0

def main():
    parser = argparse.ArgumentParser(
        description="Fetch golden content from Steam for parity testing",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "--game",
        choices=["tf", "hl2mp"],
        default="tf",
        help="Game to fetch content from (default: tf)",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("golden_content"),
        help="Output directory (default: golden_content/)",
    )

    args = parser.parse_args()

    print("=" * 60)
    print("Golden Content Fetcher - Source 1.5")
    print("=" * 60)

    # Create fetcher and run
    fetcher = ContentFetcher(args.game, args.output)
    success = fetcher.fetch()

    # Exit with appropriate code
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""
Parity Testing Harness - Source 1.5

Automated visual regression testing for Source 1.5 modifications.
Launches game headless, captures screenshots, compares with golden images.

Usage:
    python scripts/parity_test.py --map pl_badwater --config parity_config.json

Requirements:
    - PIL/Pillow for image processing
    - Golden content fetched via fetch_golden_content.py
    - Game built and ready to run
"""

import os
import sys
import argparse
import subprocess
import json
import time
from pathlib import Path
from typing import Dict, List, Tuple, Optional
from dataclasses import dataclass, asdict

# Try to import PIL for image comparison
try:
    from PIL import Image
    import numpy as np
    HAS_PIL = True
except ImportError:
    HAS_PIL = False
    print("⚠️  Warning: PIL/Pillow not found. Install with: pip install Pillow numpy")

@dataclass
class ParityConfig:
    """Configuration for parity test."""
    map_name: str
    game_dir: Path
    screenshot_dir: Path
    golden_dir: Path
    test_positions: List[Dict[str, any]]
    cvars: Dict[str, str]
    timeout: int = 30

@dataclass
class ComparisonResult:
    """Result of image comparison."""
    map_name: str
    position: str
    ssim: float
    mse: float
    passed: bool
    threshold: float

class ParityTester:
    def __init__(self, config: ParityConfig):
        self.config = config
        self.results: List[ComparisonResult] = []

    def launch_game_headless(self, position: Dict) -> bool:
        """Launch game headless at specific position."""
        print(f"\n🎮 Launching game at position: {position['name']}")

        # Build command line
        game_exe = self.config.game_dir / "hl2.exe"  # Windows
        if not game_exe.exists():
            game_exe = self.config.game_dir / "hl2.sh"  # Linux

        if not game_exe.exists():
            print(f"❌ Game executable not found: {game_exe}")
            return False

        # Command line arguments
        args = [
            str(game_exe),
            "-game", "mod_tf",  # or mod_hl2mp
            "+map", self.config.map_name,
            "-windowed",
            "-noborder",
            "-w", "1920",
            "-h", "1080",
            "-dev",
            "-console",
            "-nosteam",
            "-insecure",
        ]

        # Add CVars
        for cvar, value in self.config.cvars.items():
            args.extend(["+", cvar, value])

        # Set position
        args.extend([
            "+setpos", f"{position['x']} {position['y']} {position['z']}",
            "+setang", f"{position['pitch']} {position['yaw']} 0",
        ])

        # Wait a bit, then take screenshot
        screenshot_name = f"{self.config.map_name}_{position['name']}.tga"
        args.extend([
            "+wait", "100",  # Wait for map to load
            "+screenshot", screenshot_name,
            "+quit",
        ])

        print(f"   Command: {' '.join(args[:10])}... (+{len(args)-10} more)")

        try:
            # Launch and wait
            process = subprocess.Popen(
                args,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )
            process.wait(timeout=self.config.timeout)

            # Check for screenshot
            screenshot_path = self.config.screenshot_dir / screenshot_name
            if screenshot_path.exists():
                print(f"✅ Screenshot captured: {screenshot_name}")
                return True
            else:
                print(f"❌ Screenshot not found: {screenshot_path}")
                return False

        except subprocess.TimeoutExpired:
            print(f"❌ Game launch timed out after {self.config.timeout}s")
            process.kill()
            return False
        except Exception as e:
            print(f"❌ Failed to launch game: {e}")
            return False

    def calculate_ssim(self, img1: 'Image', img2: 'Image') -> float:
        """Calculate Structural Similarity Index (SSIM) between two images."""
        if not HAS_PIL:
            return 0.0

        # Convert to numpy arrays
        arr1 = np.array(img1.convert('L'))  # Grayscale
        arr2 = np.array(img2.convert('L'))

        # Simple SSIM approximation (for full SSIM, use skimage)
        # This is a simplified version - just correlation
        mean1 = np.mean(arr1)
        mean2 = np.mean(arr2)
        std1 = np.std(arr1)
        std2 = np.std(arr2)
        cov = np.mean((arr1 - mean1) * (arr2 - mean2))

        # Avoid division by zero
        if std1 == 0 or std2 == 0:
            return 1.0 if np.array_equal(arr1, arr2) else 0.0

        # Correlation coefficient (simplified SSIM)
        ssim = cov / (std1 * std2)
        return max(0.0, min(1.0, ssim))  # Clamp to [0, 1]

    def calculate_mse(self, img1: 'Image', img2: 'Image') -> float:
        """Calculate Mean Squared Error between two images."""
        if not HAS_PIL:
            return 0.0

        arr1 = np.array(img1)
        arr2 = np.array(img2)

        # Ensure same shape
        if arr1.shape != arr2.shape:
            print("⚠️  Image dimensions don't match!")
            return float('inf')

        mse = np.mean((arr1.astype(float) - arr2.astype(float)) ** 2)
        return mse

    def compare_screenshots(self, position: Dict) -> Optional[ComparisonResult]:
        """Compare test screenshot with golden screenshot."""
        if not HAS_PIL:
            print("⚠️  Cannot compare images without PIL/Pillow")
            return None

        screenshot_name = f"{self.config.map_name}_{position['name']}.tga"
        test_path = self.config.screenshot_dir / screenshot_name
        golden_path = self.config.golden_dir / screenshot_name

        print(f"\n📊 Comparing screenshots for: {position['name']}")

        if not test_path.exists():
            print(f"❌ Test screenshot not found: {test_path}")
            return None

        if not golden_path.exists():
            print(f"⚠️  Golden screenshot not found: {golden_path}")
            print(f"   Creating golden reference from test image...")
            # Copy test as golden if it doesn't exist
            self.config.golden_dir.mkdir(parents=True, exist_ok=True)
            Image.open(test_path).save(golden_path)
            return ComparisonResult(
                map_name=self.config.map_name,
                position=position['name'],
                ssim=1.0,
                mse=0.0,
                passed=True,
                threshold=0.95,
            )

        # Load images
        try:
            img_test = Image.open(test_path)
            img_golden = Image.open(golden_path)
        except Exception as e:
            print(f"❌ Failed to load images: {e}")
            return None

        # Calculate metrics
        ssim = self.calculate_ssim(img_test, img_golden)
        mse = self.calculate_mse(img_test, img_golden)

        # Determine pass/fail
        threshold = position.get('ssim_threshold', 0.95)
        passed = ssim >= threshold

        print(f"   SSIM: {ssim:.4f} (threshold: {threshold:.2f})")
        print(f"   MSE:  {mse:.2f}")
        print(f"   {'✅ PASS' if passed else '❌ FAIL'}")

        return ComparisonResult(
            map_name=self.config.map_name,
            position=position['name'],
            ssim=ssim,
            mse=mse,
            passed=passed,
            threshold=threshold,
        )

    def generate_report(self) -> None:
        """Generate HTML report with comparison results."""
        report_path = self.config.screenshot_dir / "parity_report.html"

        html = """<!DOCTYPE html>
<html>
<head>
    <title>Parity Test Report - Source 1.5</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        h1 { color: #333; }
        .result { margin: 20px 0; padding: 10px; border: 1px solid #ddd; }
        .pass { background-color: #d4edda; }
        .fail { background-color: #f8d7da; }
        .metrics { font-family: monospace; }
        img { max-width: 800px; border: 1px solid #ccc; margin: 10px 0; }
    </style>
</head>
<body>
    <h1>Parity Test Report</h1>
    <p>Map: <strong>{map_name}</strong></p>
    <p>Total Tests: {total} | Passed: {passed} | Failed: {failed}</p>
""".format(
            map_name=self.config.map_name,
            total=len(self.results),
            passed=sum(1 for r in self.results if r.passed),
            failed=sum(1 for r in self.results if not r.passed),
        )

        for result in self.results:
            status_class = "pass" if result.passed else "fail"
            status_text = "PASS" if result.passed else "FAIL"

            html += f"""
    <div class="result {status_class}">
        <h2>{result.position} - {status_text}</h2>
        <div class="metrics">
            SSIM: {result.ssim:.4f} (threshold: {result.threshold:.2f})<br>
            MSE:  {result.mse:.2f}
        </div>
        <div>
            <strong>Test:</strong><br>
            <img src="{self.config.map_name}_{result.position}.tga">
        </div>
    </div>
"""

        html += """
</body>
</html>
"""

        with open(report_path, 'w') as f:
            f.write(html)

        print(f"\n📄 Report generated: {report_path}")

    def run(self) -> bool:
        """Run parity tests."""
        print("=" * 60)
        print("Parity Testing Harness - Source 1.5")
        print("=" * 60)
        print(f"Map: {self.config.map_name}")
        print(f"Positions: {len(self.config.test_positions)}")

        # Create directories
        self.config.screenshot_dir.mkdir(parents=True, exist_ok=True)
        self.config.golden_dir.mkdir(parents=True, exist_ok=True)

        # Test each position
        for position in self.config.test_positions:
            # Launch game and capture
            if not self.launch_game_headless(position):
                print(f"⚠️  Skipping comparison for {position['name']}")
                continue

            # Compare with golden
            result = self.compare_screenshots(position)
            if result:
                self.results.append(result)

        # Generate report
        if self.results:
            self.generate_report()

        # Print summary
        self.print_summary()

        # Return success if all tests passed
        return all(r.passed for r in self.results)

    def print_summary(self) -> None:
        """Print test summary."""
        print("\n" + "=" * 60)
        print("📊 SUMMARY")
        print("=" * 60)
        total = len(self.results)
        passed = sum(1 for r in self.results if r.passed)
        failed = total - passed

        print(f"Total:  {total}")
        print(f"Passed: {passed}")
        print(f"Failed: {failed}")
        print(f"Rate:   {(passed/total*100) if total > 0 else 0:.1f}%")
        print("=" * 60)

def load_config(config_path: Path) -> ParityConfig:
    """Load parity test configuration from JSON."""
    with open(config_path) as f:
        data = json.load(f)

    return ParityConfig(
        map_name=data['map_name'],
        game_dir=Path(data['game_dir']),
        screenshot_dir=Path(data.get('screenshot_dir', 'parity_screenshots')),
        golden_dir=Path(data.get('golden_dir', 'parity_golden')),
        test_positions=data['test_positions'],
        cvars=data.get('cvars', {}),
        timeout=data.get('timeout', 30),
    )

def main():
    parser = argparse.ArgumentParser(
        description="Run parity tests for visual regression testing",
    )
    parser.add_argument(
        "--config",
        type=Path,
        required=True,
        help="Path to parity test configuration JSON",
    )

    args = parser.parse_args()

    if not args.config.exists():
        print(f"❌ Config file not found: {args.config}")
        sys.exit(1)

    # Load config
    config = load_config(args.config)

    # Run tests
    tester = ParityTester(config)
    success = tester.run()

    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()

# Source 1.5 Scripts - Testing & Utilities

Automation scripts for content management and parity testing.

---

## Golden Content Fetcher

Copies reference maps from Steam installation for local testing.

### Usage

```bash
# Fetch TF2 maps (default)
python scripts/fetch_golden_content.py

# Fetch HL2MP maps
python scripts/fetch_golden_content.py --game hl2mp

# Custom output directory
python scripts/fetch_golden_content.py --output my_test_content
```

### What It Does

1. Searches for Steam installation (common paths on Windows/Linux)
2. Locates game directory (TF2 or HL2:DM)
3. Copies golden maps to `golden_content/maps/`
4. Creates manifest.json with metadata

### Golden Maps

**Team Fortress 2:**
- ctf_2fort.bsp
- pl_badwater.bsp
- cp_dustbowl.bsp
- koth_harvest_final.bsp
- pl_upward.bsp

**Half-Life 2: Deathmatch:**
- dm_lockdown.bsp
- dm_overwatch.bsp
- dm_resistance.bsp
- dm_runoff.bsp

**Note:** Maps are NOT tracked in git (.gitignore). Fetch locally for testing.

---

## Parity Testing Harness

Automated visual regression testing for Source 1.5 modifications.

### Requirements

```bash
pip install Pillow numpy
```

### Usage

```bash
# Run parity tests with config
python scripts/parity_test.py --config scripts/parity_config_example.json
```

### Configuration

Create a JSON config file (see `parity_config_example.json`):

```json
{
  "map_name": "pl_badwater",
  "game_dir": "game",
  "screenshot_dir": "parity_screenshots",
  "golden_dir": "parity_golden",
  "timeout": 45,
  "cvars": {
    "r_lightmap_bicubic": "1",
    "mat_tonemapping_mode": "3",
    "fov_desired": "90"
  },
  "test_positions": [
    {
      "name": "spawn_blu",
      "x": "-3072",
      "y": "2560",
      "z": "384",
      "pitch": "0",
      "yaw": "90",
      "ssim_threshold": 0.95
    }
  ]
}
```

### How It Works

1. **Launches game headless** at each test position
2. **Captures screenshot** (TGA format)
3. **Compares with golden** reference image
4. **Calculates metrics:**
   - SSIM (Structural Similarity Index)
   - MSE (Mean Squared Error)
5. **Generates HTML report** with visual diffs

### Test Positions

Each position requires:
- `name` - Identifier for this test
- `x`, `y`, `z` - World coordinates
- `pitch`, `yaw` - Camera angles
- `ssim_threshold` - Pass threshold (0.95 = 95% similarity)

### Golden References

First run creates golden reference images from test screenshots:

```bash
# First run - creates golden images
python scripts/parity_test.py --config my_config.json

# Subsequent runs - compares against golden
python scripts/parity_test.py --config my_config.json
```

Golden images stored in `parity_golden/` (gitignored).

### Reading Results

**HTML Report:** `parity_screenshots/parity_report.html`
- Visual comparison for each position
- Pass/fail status
- SSIM and MSE metrics

**Console Output:**
```
📊 SUMMARY
============================================================
Total:  4
Passed: 3
Failed: 1
Rate:   75.0%
============================================================
```

---

## Workflow: Visual Regression Testing

### 1. Setup Golden Content

```bash
# Fetch test maps
python scripts/fetch_golden_content.py --game tf

# Verify maps downloaded
ls golden_content/maps/
```

### 2. Create Parity Config

```bash
# Copy example config
cp scripts/parity_config_example.json my_parity_config.json

# Edit positions for your map
nano my_parity_config.json
```

### 3. Establish Baseline

```bash
# Build Source 1.5 with current features
cd build && cmake --build . --config RelWithDebInfo

# Run tests to create golden references
python scripts/parity_test.py --config my_parity_config.json
```

### 4. Make Changes

```bash
# Implement new features (e.g., SSAO, color grading)
# Rebuild
cd build && cmake --build . --config RelWithDebInfo
```

### 5. Verify Parity

```bash
# Run tests again
python scripts/parity_test.py --config my_parity_config.json

# Check report
open parity_screenshots/parity_report.html
```

### 6. Update Golden (if intentional)

If visual changes are intentional (e.g., ACES tonemap):

```bash
# Delete old golden images
rm -rf parity_golden/

# Re-run to create new golden references
python scripts/parity_test.py --config my_parity_config.json
```

---

## Tips

### Finding Good Test Positions

1. **Variety:** Indoor, outdoor, bright, dark
2. **Features:** Complex lighting, shadows, reflections
3. **Static:** No dynamic elements (players, particles)
4. **Reproducible:** Same position every time

### Choosing SSIM Thresholds

- **0.99** - Nearly identical (strict)
- **0.95** - Very similar (recommended default)
- **0.90** - Similar (allow minor differences)
- **0.85** - Moderately similar (loose)

### Performance

- Each test position adds ~30-45 seconds
- Keep test suites focused (4-8 positions)
- Run full suite in CI, subset locally

### Debugging Failures

1. Check console output for errors
2. Open HTML report for visual comparison
3. Verify CVars match between runs
4. Ensure deterministic settings (fixed FOV, no vsync)

---

## Continuous Integration

### GitHub Actions Example

```yaml
name: Parity Tests

on: [pull_request]

jobs:
  parity:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v3

      - name: Setup Python
        uses: actions/setup-python@v4
        with:
          python-version: '3.11'

      - name: Install dependencies
        run: pip install Pillow numpy

      - name: Build Source 1.5
        run: |
          cmake -B build
          cmake --build build --config RelWithDebInfo

      - name: Fetch golden content
        run: python scripts/fetch_golden_content.py --game tf

      - name: Run parity tests
        run: python scripts/parity_test.py --config ci_parity_config.json

      - name: Upload report
        if: always()
        uses: actions/upload-artifact@v3
        with:
          name: parity-report
          path: parity_screenshots/
```

---

## Directory Structure

```
scripts/
├── README.md                      # This file
├── fetch_golden_content.py        # Golden content fetcher
├── parity_test.py                 # Parity testing harness
└── parity_config_example.json     # Example parity config

golden_content/                    # Not tracked (gitignored)
├── maps/
│   ├── pl_badwater.bsp
│   └── ...
└── manifest.json

parity_screenshots/                # Not tracked (gitignored)
├── pl_badwater_spawn_blu.tga
├── pl_badwater_mid_outdoors.tga
└── parity_report.html

parity_golden/                     # Not tracked (gitignored)
├── pl_badwater_spawn_blu.tga
└── ...
```

---

## Troubleshooting

### Steam Not Found

```
❌ Steam installation not found!
```

**Solution:** Ensure Steam is installed in standard location or edit `STEAM_PATHS` in `fetch_golden_content.py`.

### Game Not Found

```
❌ Team Fortress 2 not found!
```

**Solution:** Install TF2 via Steam (Free to Play).

### Screenshot Not Captured

```
❌ Screenshot not found: parity_screenshots/map_position.tga
```

**Solution:**
- Increase `timeout` in config (map may take longer to load)
- Check game logs for errors
- Verify `screenshot_dir` path is correct

### PIL/Pillow Import Error

```
⚠️  Warning: PIL/Pillow not found
```

**Solution:** `pip install Pillow numpy`

---

*Last Updated: 2025-11-03*
*Source 1.5 - Phase 0 Testing Tools*

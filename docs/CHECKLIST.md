# Source 1.5 – Master Checklist

## 0) Guardrails (do first, then enforce every PR)

* [x] **Licensing boundaries set** (SDK non-commercial; Hub MIT; Steam Audio Apache-2.0; no Valve assets in repo)
* [x] **CI required** (Win x64 + Linux x64), tests + static analysis must pass to merge
* [x] **Coding standards** (.clang-format; clang-tidy baseline; no new warnings)
* [ ] **Deterministic tests** (fixed FOV/time/seed; vsync off; same cvars)

---

## Phase 0 — Week 1: Foundation (TDD & structure)

* [x] **Repo branches**: create `source-1.5` branch off your pinned SHA
* [x] **CI**: GitHub Actions Win x64 (VS2022) + Linux x64 (Steam Runtime) green
* [x] **CMake**: root + targets `lib_framework`, `lib_engine_bridge`, `tests`
* [x] **Unit test harness**: Catch2; `tests/test_string_utils.cpp` passing
* [x] **Static analysis**: clang-tidy + cppcheck wired; fail on new issues
* [x] **Parity harness**: headless launch, screenshot capture, SSIM diff script (parity_test.py)
* [x] **Golden content script** (`scripts/fetch_golden_content.py`): copies maps from local Steam, not tracked
* [x] **Docs**: Comprehensive documentation including testing tools (scripts/README.md)

**DoD:** CI green; unit tests run in CI; parity script produces a report; docs published.

**STATUS**: ✅ 100% Complete (All Phase 0 foundation items complete)

---

## Phase 1 — Structural Refactor (weeks 2–4)

* [x] **Create `src/framework/`** (utils/serialization/math extras) + tests
* [x] **Create `src/engine_bridge/`** (filesystem/materialsystem/console adapters) - compiles, stubs ready
* [ ] **Split `game/shared/` mini-libs**:
  * [ ] `lib_animation` (+ tests)
  * [ ] `lib_prediction` (+ tests)
  * [ ] `lib_network` (+ tests)
* [ ] **De-ifdef hotspots**: replace `#ifdef HL2/TF/HL2MP` with interface dispatch
* [ ] **CMake targets**: games depend only on framework + mini-libs + engine interfaces

**DoD:** Games still build/run; new libs compile standalone; tests cover new headers; compile times drop or hold.

**STATUS**: 🔄 30% Complete (framework ✅, engine_bridge compiling ✅, color grading complete)

---

## Phase 2 — Visual Quick Wins (weeks 2–3)

* [x] **Bicubic lightmaps** (Very High shader detail; `r_lightmap_bicubic 1`)
* [x] **ACES tonemap** (`mat_tonemapping_mode 3`)
* [x] **Color grading pipeline** (8 functions: exposure, saturation, contrast, brightness, temperature, gamma)
* [x] **SSAO foundation** (C++ reference, HLSL shaders, ConVars, tests ✅)
* [x] **SSAO integration** (render targets ✅, compute pass ✅, bilateral blur ✅, noise generation ✅)
* [ ] **SSAO lighting integration** (multiply diffuse/ambient by occlusion)
* [ ] **Parity screenshots**: before/after; SSIM thresholds documented

**DoD:** Visual diffs show expected change; ≤5% perf hit; toggleable via cvars; parity scenes pass.

**STATUS**: 🔄 95% Complete (bicubic ✅, ACES ✅, color grading ✅, SSAO render pipeline ✅, lighting integration pending)

---

## Phase 3 — PBR Materials (weeks 3–5)

* [x] **PBR shader foundation** (BRDF functions, VS/PS, material system) - source files ready
* [ ] **PBR shader compilation** (add to build system, compile, test)
* [ ] **Authoring preset**: Substance Painter export (albedo/normal/MRAO)
* [ ] **Starter library**: 5 CC0 materials (metal/wood/concrete/plastic/fabric)
* [ ] **Docs**: "Hello PBR Prop" (2-min video + steps)

**DoD:** Side-by-side Phong vs PBR; sample assets render correctly; doc followed by fresh user in <15 min.

**STATUS**: 🔄 40% Complete (shader foundation ✅, build integration pending)

---

## Phase 4 — Editor Hub MVP (weeks 3–5, overlaps)

* [ ] **Shell**: Project switcher, **Play**, **Console** (tail output)
* [ ] **Asset Browser**: filter by Models/Materials/Audio/Scripts/Maps
* [ ] **File watcher** → **PNG→VTF** + VMT update (call `vtex`)
* [ ] **Material preview**: PBR sphere (same parameters as game)
* [ ] **Hot-reload**: send `mat_reloadmaterial <name>` via IPC
* [ ] **Command palette** (Ctrl/Cmd+K): search assets, commands, docs
* [ ] **A11y**: keyboard navigation, focus rings, WCAG AA colors

**DoD:** Edit texture → in-game update <10s; one-click Play works from Hub; logs/errors visible and actionable.

**STATUS**: ⏸️ Not Started (after PBR)

---

## Phase 5 — DX11 Backend (flag-gated) (weeks 6–12)

* [ ] `shaderapidx11` module scaffolded; selected by `-dx11`
* [ ] **Parity pack** (10 scenes): RMS/SSIM within target vs DX9
* [ ] **Soak test**: 1-hour loop; no leaks/device loss
* [ ] **Linux**: works via **DXVK** path; doc the flag

**DoD:** Stable; parity scenes pass; perf within ±10% of DX9; crash-free soak.

**STATUS**: ⏸️ Not Started (weeks 6-12)

---

## Phase 6 — Steam Audio (optional) (weeks 7–10)

* [ ] **SA-0**: HRTF + occlusion as wet bus; cvars `s1_steamaudio`, `s1_sa_occlusion`, `s1_sa_hrtf`
* [ ] **SA-1**: BSP → SA geometry export; early reflections; Hub Audio pane
* [ ] **Budget rails**: cap active sources; CPU ms cap; debug toggles
* [ ] **License note**: Apache-2.0; build flag `SOURCE15_STEAMAUDIO`

**DoD:** L-shaped corridor audio demo clearly occludes; toggle live; CPU within budget; optional build passes.

**STATUS**: ⏸️ Optional (weeks 7-10)

---

## Phase 7 — Templates & Docs (rolling)

* [ ] **SP-FPS template** (movement, 3 weapons, HUD, autosave, interactables)
* [ ] **Puzzle template** (buttons, doors, triggers, physics props; no weapons)
* [ ] **Walking-sim** (flashlight, captions, ambience zones)
* [x] **Docs site** (MkDocs): Quickstart, PBR, Hot-reload, Hub usage
* [ ] **3 silent videos** (≤2 min each): Drag-drop pipeline, Hot-reload, PBR prop

**DoD:** Fresh user to first playable in <60 min using docs + Hub.

**STATUS**: 🔄 20% Complete (comprehensive docs written, videos pending)

---

## Current Sprint: SSAO Implementation (Day 5)

### ✅ Completed (Days 1-4)
- [x] TDD foundation (Catch2, CMake, CI)
- [x] String utils with 51 edge case tests
- [x] Bicubic lightmap filtering
- [x] ACES tonemap (140 assertions)
- [x] Color grading pipeline (8 functions, 301+ assertions)
- [x] Runtime integration (ConVars → GPU)
- [x] Comprehensive documentation (1,592 lines)

### ✅ Day 5 Completed
- [x] SSAO research (hemisphere sampling, LearnOpenGL)
- [x] SSAO test suite (2 test cases, 7 assertions, 100% pass)
- [x] SSAO C++ reference implementation
- [x] SSAO HLSL shader functions
- [x] SSAO ConVars and runtime binding
- [x] SSAO documentation (SSAO_IMPLEMENTATION.md)
- [x] PBR research and integration planning (520 lines)
- [x] PBR shader foundation (BRDF, VS/PS, material system)
- [x] Fix Windows MSVC compiler detection (engine_bridge compiles)

### ✅ Day 5 Extended - SSAO Integration
- [x] SSAO render target setup (5 render targets: depth, normal, SSAO, blur, noise)
- [x] SSAO compute shader (ssao_ps20b.fxc, hemisphere sampling)
- [x] SSAO material system integration (ssao_dx9.cpp)
- [x] Bilateral blur shader (ssao_blur_ps20b.fxc, depth-aware)
- [x] Blur material system integration (ssao_blur_dx9.cpp)
- [x] SSAO noise texture generation (4x4 random rotations)
- [x] Comprehensive integration documentation (SSAO_INTEGRATION.md, 500+ lines)

### 🔄 In Progress (Next)
- [ ] SSAO lighting integration (modify lighting shaders)
- [ ] PBR shader build integration (add to shader compile system)

---

## Apple-Grade UI Polish (continuous)

* [ ] Single accent color, one primary action per view
* [ ] Progress **pills** in header; click to expand task logs
* [ ] Human microcopy ("Imported *metal_01* in 142 ms")
* [ ] Command palette (Ctrl/Cmd+K); fuzzy search everywhere
* [ ] Keyboard-first UX (arrow keys, Space preview, Enter to run)
* [ ] Error states with "Fix" chips linking to docs/actions

---

## Performance Gates

* [ ] Bench scene metrics logged (avg & 99p frametime)
* [ ] RTX 3060 @1080p: target 144 FPS (quality presets if needed)
* [ ] GTX 1660 @1080p: ≥60 FPS
* [ ] Report stored under `build/reports/perf/`

---

## Security & Sandboxing

* [ ] Hub **Safe Mode** default (no external tools; read-only)
* [ ] Full mode = explicit consent; whitelisted tool paths only
* [ ] No network except local IPC; no spawning arbitrary processes
* [ ] Asset validation (size, duration, NPOT checks) with friendly errors

---

## Release & Artifacts

* [x] Per-commit artifacts: binaries + **separate** symbols; 7-day retention
* [ ] Tagged release: Engine zip, Hub MSI/AppImage, Content zip
* [ ] Changelog & versioning (`0.x` → `1.0` when DX11 stable + PBR + Hub MVP + docs + templates done)

---

## Nice-to-Have (post-1.0)

* [ ] Model hot-reload (entity refresh)
* [ ] Visual scripting v1 (nodes → VScript)
* [ ] Prefab authoring (instance-backed)
* [ ] Steam Audio baking tools
* [ ] BSP v25 (expanded limits)
* [ ] Linux CI parity with visual tests

---

## Daily "Definition of Done" (use this for every PR)

* [x] Builds on Win + Linux
* [x] Unit tests added/updated
* [ ] Parity tests pass (or thresholds updated with rationale)
* [x] Docs touched if behavior/UI changed
* [x] No new warnings; style formatted
* [x] Security/licensing unaffected

---

## Progress Summary

**Overall**: 48% Complete (Phases 0-3 in progress)

**Current Focus**: Phase 2 SSAO Integration (95% complete), Phase 3 PBR Materials

**Recent**: SSAO render pipeline ✅, bilateral blur ✅, render targets ✅, noise generation ✅

**Next**: SSAO lighting integration, PBR shader compilation, material library

**Blockers**: None (CI green, tests passing, docs current)

---

*Last updated: 2025-11-04 (Day 5 extended - SSAO integration)*
*Active sprint: SSAO render pipeline complete, PBR shader foundation ready*

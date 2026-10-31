# OGL Firestorm Fork – verified starting point (2026-10-31)

## Baseline
- Official upstream: FirestormViewer/phoenix-firestorm
- Release branch: Firestorm_7.2.5, commit 4ac201fdc561
- Fork: marlin198323-glitch/ogl-viewer; branch ogl-develop
- Primary workstation: Linux Mint 22.3, 28 logical CPUs, 31 GiB RAM, RTX 4070 SUPER
- Legacy native viewer: separate nexverse-viewer checkout, develop-current; this fork has not edited or overwritten it.
- Python toolchain: isolated venv .venv-firestorm, autobuild 3.10.2
- Baseline target: ReleaseOS with OpenSim support
- Grid login advertised by public /get_grid_info: http://hg.stadt-nexverse.de:80/ (HTTP 200). OGL supports this **exact HTTP endpoint** after an explicit in-session warning confirmation. HTTP credentials are exposed to interception; use a separate test account.

## 10 implementation blocks
1. Clone stable Firestorm release and register upstream/fork remotes. DONE
2. Prepare isolated Python build environment and inspect system requirements. DONE (libpulse-dev and GCC 11 installed; the successful baseline was compiled using GCC 13).
3. Verify reproducible ReleaseOS configure/build on Linux. DONE (Firestorm baseline 1223/1223; branded OGL 824/824; runtime staging and live Linux Mint GUI startup PASS).
4. Verify authentic OpenSim/OpenGenesisLINK login and full 3D region rendering. IN PROGRESS (local login PASS; pinned HTTP grid with explicit per-process warning enabled; real authenticated login and in-world graphics still pending user acceptance).
5. Implement isolated OGL branding, launcher and user-data separation. IN PROGRESS (OGL title/icon/welcome and isolated data PASS on Linux Mint; full custom UI and cross-platform branding OPEN).
6. Verify full avatar wearables, appearance, attachments, animations. OPEN
7. Measure texturing/mesh/cache/sky pipeline vs upstream on repeat visits. OPEN
8. Integrate versioned OGL module API into the viewer safely. IN PROGRESS (trusted RTX capability module loads in real OGL Linux viewer; future generic manager, trust controls and GPU rendering integration OPEN)
9. Add OGL-specific UI, grid settings, optional plugins, updater, packaging. OPEN
10. CI, compatibility, notices, regression/performance/security checks and release gates. IN PROGRESS (14/14 Python checks and 4/4 native/module checks PASS; actual HTTP login and in-world acceptance OPEN)

## Non-goals / security boundaries
- Never overwrite the legacy viewer, its running binary, cache or local changes.
- Do not remove upstream copyright, third-party licenses or required notices.
- The example module runs as native code and is not sandboxed; no untrusted modules.
- Do not store login passwords or secrets in source code; use proven credential storage.
- A passing module ABI smoke test is not evidence of a functional Firestorm-based OGL viewer.
- Only release after a real grid login, world rendering and user acceptance test.

## 2026-10-31 addendum: grid/profile

- Fixed Stadt NexVerse profile: DONE (central header, pinned startup,
  other grids excluded, selected grid logging confirmed on Linux Mint).
- Exact-grid HTTP transport exception: DONE (interactive approval for
  one process only; authentication boundary rechecks grid and consent).
- Authentication and in-world visual acceptance: ready for **manual**
  test with a dedicated test account; HTTP is unencrypted and carries risk.
- RTX capability module: DONE for hardware discovery, but ray-traced
  shadows/reflections/GI remain NOT IMPLEMENTED.

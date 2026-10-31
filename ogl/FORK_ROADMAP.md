# OGL Firestorm Fork – verified starting point (2026-10-31)

## Baseline
- Official upstream: FirestormViewer/phoenix-firestorm
- Release branch: Firestorm_7.2.5, commit 4ac201fdc561
- Fork: marlin198323-glitch/ogl-viewer; branch ogl-develop
- Primary workstation: Linux Mint 22.3, 28 logical CPUs, 31 GiB RAM, RTX 4070 SUPER
- Legacy native viewer: separate nexverse-viewer checkout, develop-current; it has uncommitted work and must not be overwritten
- Python toolchain: isolated venv .venv-firestorm, autobuild 3.10.2
- Baseline target: ReleaseOS with OpenSim support
- Grid login advertised by public /get_grid_info: http://hg.stadt-nexverse.de:80/ (HTTP 200). HTTPS port 443 was not reachable. Do not test real credentials over this unsecured endpoint before TLS is resolved.

## 10 implementation blocks
1. Clone stable Firestorm release and register upstream/fork remotes. DONE
2. Prepare isolated Python build environment and inspect system requirements. DONE (libpulse-dev and GCC 11 installed; the successful baseline was compiled using GCC 13).
3. Verify reproducible ReleaseOS configure/build on Linux. DONE (OpenSim CMake configure PASS; firestorm-bin compile/link 1223/1223 and Linux runtime staging PASS; live test window visible).
4. Verify authentic OpenSim/OpenGenesisLINK login and full 3D region rendering. IN PROGRESS (local login window PASS; real account authentication and in-world graphics blocked pending secure HTTPS grid login).
5. Implement isolated OGL branding, launcher and user-data separation. IN PROGRESS (separate desktop test launcher, settings/cache/log profile PASS; Firestorm UI/icon still present, full OGL branding OPEN).
6. Verify full avatar wearables, appearance, attachments, animations. OPEN
7. Measure texturing/mesh/cache/sky pipeline vs upstream on repeat visits. OPEN
8. Integrate versioned OGL module API into the viewer safely. OPEN (standalone proof of concept only)
9. Add OGL-specific UI, grid settings, optional plugins, updater, packaging. OPEN
10. CI, compatibility, notices, regression/performance/security checks and release gates. OPEN

## Non-goals / security boundaries
- Never overwrite the legacy viewer, its running binary, cache or local changes.
- Do not remove upstream copyright, third-party licenses or required notices.
- The example module runs as native code and is not sandboxed; no untrusted modules.
- Do not store login passwords or secrets in source code; use proven credential storage.
- A passing module ABI smoke test is not evidence of a functional Firestorm-based OGL viewer.
- Only release after a real grid login, world rendering and user acceptance test.

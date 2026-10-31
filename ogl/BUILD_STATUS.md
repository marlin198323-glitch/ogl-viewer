# OGL Firestorm Fork – verified Linux Mint state (2026-10-31)

## Passed

- Official Firestorm release baseline: `Firestorm_7.2.5`,
  base commit `4ac201fdc561`; fork `marlin198323-glitch/ogl-viewer`,
  development branch `ogl-develop`.
- OGL branding implementation committed as `5e66e2df`.
- CMake OpenSim configuration: `OPENSIM:BOOL=ON`.
- Branded channel: `OGL Viewer Development`.
- Full OGL recompile/link: 824/824 Ninja steps, exit 0.
- Linux manifest resource staging: exit 0.
- OGL logo in login skin and Linux desktop resources; staged source
  and destination SHA-256 checks match.
- Linux Mint X11 start: window title
  `OGL Viewer Development 7.2.5.8`, login screen reached
  `STATE_LOGIN_WAIT`.
- Linux OpenGL: NVIDIA RTX 4070 SUPER, OpenGL 4.6, driver 615.71.09.
- CEF login background loads
  `https://stadt-nexverse.de/service/welcome`.
- Branded runtime writes `~/.local/share/ogl-firestorm-test/logs/OGL.log`.
- Start log contained zero ERROR/FATAL messages at first check.
- Static OGL branding/security tests: 6/6 passing.
- Standalone OGL module tests: 3/3 passing (RTX capability, missing-driver
  fallback, basic ABI handshake). RTX module loaded by the actual viewer;
  RTX 4070 SUPER reported hardware_ready=1 / rendering_active=0.
  This is not an active ray-tracing renderer.
- Legacy `nexverse-viewer` project was not overwritten or merged.

## Run on Linux Mint

- Desktop shortcut: `~/Schreibtisch/OGL-Viewer-Test.desktop`.
- App menu: `OGL Viewer (Entwicklung)`.
- Command: `bash ~/Schreibtisch/OGL-Viewer/ogl-firestorm/ogl/scripts/run-viewer-test.sh`.
- Rebuild and test: `bash ~/Schreibtisch/OGL-Viewer/ogl-firestorm/ogl/scripts/build-test.sh --run`.
- Read-only local report: `python3 ~/Schreibtisch/OGL-Viewer/ogl-firestorm/ogl/scripts/ogl-test-report.py`.

Build and staging logs:
`~/Schreibtisch/OGL-Viewer/.ogl-branding-build.log` and
`~/Schreibtisch/OGL-Viewer/.ogl-branding-stage.log`.
Launch log:
`~/.local/state/ogl-firestorm-test/launch-*.log`.

## Open acceptance gates

1. Actual password authentication was **not** tested. The grid advertises
   `http://hg.stadt-nexverse.de:80/` as its login endpoint and
   `https://hg.stadt-nexverse.de` was not available at the last check.
   OGL's interactive login button now rejects grids without HTTPS
   login URIs. A real-grid HTTPS deployment and controlled login test
   are still required.
2. Avatar wearables, attachments, mesh completeness, sky, sun,
   animations, teleport, chat/voice and world rendering require a
   verified in-world OpenSim session. Do not call these accepted.
3. The modular C ABI handshake runs independently; Firestorm does
   not yet load in-viewer OGL modules.
4. Design is early OGL branding, not a fully reworked responsive
   OGL UI. Fonts, ultra-wide scaling and input ergonomics still
   need manual UI acceptance.
5. Production installers, source/license redistribution review,
   multi-platform CI and updater are not released.

The initial authenticated viewer must never be represented as
production-ready until these gates pass.

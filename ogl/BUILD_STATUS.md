# OGL Firestorm Linux Mint test status – 2026-10-31

## VERIFIED: Firestorm-based OGL test viewer runs on the user's PC

- Genuine GitHub Firestorm fork: `marlin198323-glitch/ogl-viewer`, branch `ogl-develop`.
- Release baseline: Firestorm 7.2.5 (`Firestorm_7.2.5`, base SHA `4ac201fdc561`).
- Built on Linux Mint 22.3 with GCC 13.3, 6 parallel jobs.
- OpenSim CMake configuration PASS: `-DOPENSIM:BOOL=ON`,
  `-DVIEWER_CHANNEL:STRING=Firestorm-PrivateOGL`.
- Full `firestorm-bin` compiled and linked: 1223/1223 steps, exit code 0.
- Runtime staging `copy_l_viewer_manifest` PASS, exit code 0.
- Actual Linux X11 GUI startup PASS: visible window
  `Firestorm-PrivateOGL 7.2.5.263032345` at 1024x738.
- Startup logs reached `STATE_LOGIN_WAIT`; fonts and media CEF plugin
  initialized; no ERROR/FATAL lines found in the initial window log.
- Local C ABI module example CTest passed 1/1, but module loader is **not**
  connected to the Firestorm viewer yet.
- User's older `nexverse-viewer` checkout with uncommitted work was not edited.

## User launch

- Linux Mint desktop icon: `~/Schreibtisch/OGL-Firestorm-Test.desktop`
- Menu: `OGL Viewer (Firestorm-Test)`
- Shell: `bash ~/Schreibtisch/OGL-Viewer/ogl-firestorm/ogl/scripts/run-viewer-test.sh`
- Isolated settings/cache: `~/.local/share/ogl-firestorm-test`
- Launcher logs: `~/.local/state/ogl-firestorm-test/launch-*.log`
- Build log: `~/Schreibtisch/OGL-Viewer/.ogl-firestorm-full-build.log`
- Packaging log: `~/Schreibtisch/OGL-Viewer/.ogl-firestorm-stage.log`

## Important limitations before grid/in-world testing

- This is a Firestorm-based OGL **engineering test**, not fully branded
  OGL. Firestorm logo, menus and welcome content remain on the baseline.
- The viewer still lacks the production OGL integrated module loader.
- Public `http://hg.stadt-nexverse.de/get_grid_info` advertised
  `http://hg.stadt-nexverse.de:80/` as the grid login endpoint.
  Do **not** enter real credentials until HTTPS is available and validated.
- A visible login window does **not** verify any authenticated Grid session,
  region rendering, avatar appearance, texture completeness, teleport or voice.
- The launcher filters desktop protocol-registration calls out of a staged
  copy of the original runtime wrapper and uses a private user directory.
  Upstream source files remain unchanged.
- Source copyright, licensing and third-party notices must remain intact.

## Repeat after a code change

```bash
cd "$HOME/Schreibtisch/OGL-Viewer/ogl-firestorm"
cmake --build build-linux-x86_64 --target firestorm-bin --parallel 6
cmake --build build-linux-x86_64 --target copy_l_viewer_manifest --parallel 5
bash ogl/scripts/run-viewer-test.sh
```

The current live test window can be closed normally before repeating startup.

# OGL Firestorm first-build status – 2026-10-31

## Verified

- Genuine GitHub fork `marlin198323-glitch/ogl-viewer`, development branch `ogl-develop`.
- Firestorm official release source branch `Firestorm_7.2.5` at `4ac201fdc561`.
- Existing `nexverse-viewer` checkout and its uncommitted avatar work preserved.
- Local Python venv with Autobuild 3.10.2 installed (no global pip changes).
- Standalone Linux shared-library module ABI example compiles; CTest 1/1 PASS.
- ReleaseOS CMake configuration with explicit `-DOPENSIM:BOOL=ON` and
  `-DVIEWER_CHANNEL:STRING=Firestorm-PrivateOGL` PASS.
- Large WebRTC package installed after cleanup of an interrupted unregistered extraction.
- Initial two-job build completed 126 of 1355 Ninja steps before the
  deliberate 100-second execution limit. No compiler diagnostics were found in the
  captured log. This does **not** certify full build success.
- Build log: `~/Schreibtisch/OGL-Viewer/.ogl-firestorm-first-build.log`.
- Configure log: `~/Schreibtisch/OGL-Viewer/.ogl-opensim-configure.log`.

## Missing/risks

- `libpulse-dev` is not installed; Firestorm configured with dummy volume catcher.
- GCC 13.3 is installed; upstream Linux guide recommends GCC 11. A reproducible
  GCC 11 build should be tested, but GCC 13 compatibility is not ruled out.
- OGL branding, module loader within Firestorm, live-grid login, world and
  avatar rendering are not implemented or verified.
- Public grid info returns `<login>http://hg.stadt-nexverse.de:80/</login>`;
  HTTPS port 443 was unreachable. Resolve secure login before testing real credentials.

## Resume on Linux Mint

Run the prerequisite report:

```sh
cd "$HOME/Schreibtisch/OGL-Viewer/ogl-firestorm"
bash ogl/scripts/doctor.sh
```

After installing missing system libraries, continue the existing build:

```sh
cd "$HOME/Schreibtisch/OGL-Viewer/ogl-firestorm"
cmake --build build-linux-x86_64 --parallel 2
```

The command continues the incremental CMake/Ninja build; it does not
automatically start a usable OGL application or release an installer.
Avoid aggressive parallelism while unrelated OGL builds run on the same host.

# OGL Viewer – Firestorm fork

This folder contains OGL-specific extensions based on the official upstream
Firestorm 7.2.5 release. A functional OpenSim Firestorm baseline was already
compiled and GUI-launched on Linux Mint. The next OGL branding iteration now
modifies product identity, welcome page, icons and interactive HTTPS login
validation; these changes require a fresh compiled runtime acceptance.

- Upstream: https://github.com/FirestormViewer/phoenix-firestorm
- Fork: https://github.com/marlin198323-glitch/ogl-viewer
- Development branch: `ogl-develop`
- Existing separate viewer: `../nexverse-viewer` (do not overwrite).
- First target: a working, repeatable Linux OpenSim build before branding.
- Grid connection must be confirmed against the actual login endpoint.

The `modules/` directory is an **experimental OGL ABI prototype**. It is not
loaded by Firestorm and is not a production plugin system. Only run
modules from trusted developers: a native shared library executes arbitrary
code in the viewer process.

Build the self-contained module handshake test:

```sh
cmake -S ogl/modules -B build-ogl-modules -G Ninja
cmake --build build-ogl-modules
ctest --test-dir build-ogl-modules --output-on-failure
```

For full Firestorm Linux build instructions see `doc/building_linux.md`.
Use OpenSim configuration `ReleaseOS` rather than Second Life only
`ReleaseFS_open`. OGL-specific names, settings paths, and integrations
must be implemented and tested separately. Preserve upstream license
notices, third-party notices, and appropriate source-code obligations.

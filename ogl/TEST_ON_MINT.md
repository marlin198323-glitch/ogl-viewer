# OGL Viewer (Firestorm baseline) – local Linux Mint test

This runs the **real Firestorm-based OpenSim build**, not the older native
NexVerse viewer. It is a pre-branding engineering test: the window may still
show Firestorm assets and labels. OGL native modules are a standalone proof of
concept and are **not** loaded inside the viewer yet.

## Build and stage

From `~/Schreibtisch/OGL-Viewer/ogl-firestorm`:

```bash
bash ogl/scripts/doctor.sh
cmake --build build-linux-x86_64 --target firestorm-bin --parallel 6
cmake --build build-linux-x86_64 --target copy_l_viewer_manifest --parallel 6
bash ogl/scripts/install-test-shortcut.sh
```

Then double-click `~/Schreibtisch/OGL-Firestorm-Test.desktop`, or start
the menu entry `OGL Viewer (Firestorm-Test)`, or run:

```bash
bash ogl/scripts/run-viewer-test.sh
```

## Isolation and log files

- Dedicated settings, cache, crash files: `~/.local/share/ogl-firestorm-test`
- Launcher diagnostics: `~/.local/state/ogl-firestorm-test/launch-*.log`
- Does not replace or update `~/Schreibtisch/OGL-Viewer/nexverse-viewer`
- Test launcher removes automatic protocol handler registration from a local
  **staged copy** of Firestorm's wrapper; upstream sources remain unchanged.
- Do not log in with real account credentials until the advertised HTTP grid
  login address is secured via HTTPS.

## Manual acceptance order

1. Login window appears and scales correctly across displays.
2. Viewer shows OpenSim grid selection and does not load saved credentials from
   any existing Firestorm install.
3. No immediate crash, missing library or OpenGL initialization error.
4. Test a real grid login only after a secure TLS login endpoint is verified.
5. In-world: verify avatar appearance, attachments, textures, sky, sun, meshes,
   sound, cache behavior, teleports and region changes against Firestorm.
6. Document visual issues and inspect logs before changing renderer defaults.

Passing steps 1–3 **does not** imply steps 4–5 are complete or that this is
a production-ready branded OGL release.

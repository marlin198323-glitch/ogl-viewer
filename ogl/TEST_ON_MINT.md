# OGL Viewer on Linux Mint – interactive engineering test

The `ogl-develop` branch is a genuine Firestorm-based OpenSim fork, not the
previous independently written native `nexverse-viewer`. The baseline is
Firestorm 7.2.5, with early OGL branding applied.

## Launch the compiled application

In the Linux Mint desktop, double-click
`~/Schreibtisch/OGL-Viewer-Test.desktop` or select
**OGL Viewer (Entwicklung)** from the application menu.

Alternatively use:

```bash
bash "$HOME/Schreibtisch/OGL-Viewer/ogl-firestorm/ogl/scripts/run-viewer-test.sh"
```

To rebuild, stage the matching resources, verify tests and start:

```bash
cd "$HOME/Schreibtisch/OGL-Viewer/ogl-firestorm"
bash ogl/scripts/build-test.sh --run
```

Optional local diagnostic:

```bash
python3 ogl/scripts/ogl-test-report.py
```

## Isolation

- Dedicated settings/cache: `~/.local/share/ogl-firestorm-test`
- Viewer app log: `~/.local/share/ogl-firestorm-test/logs/OGL.log`
- Launcher diagnostics: `~/.local/state/ogl-firestorm-test/launch-*.log`
- The previous native viewer and its cache have not been overwritten.
- The test wrapper suppresses desktop protocol-registration commands
  from the staged Firestorm launcher.

## Acceptance

1. OGL title, icon and NexVerse welcome appear.
2. Test narrow/standard/ultrawide window sizes and read text labels.
3. Verify startup without crash or missing font, texture or CEF plugin.
4. Stadt NexVerse currently uses HTTP. OGL permits the exact fixed URI
   only after you explicitly choose **Über HTTP anmelden** (or its English
   equivalent). HTTP traffic is unencrypted: use a separate test account
   whenever possible; never paste credentials into chat or debugging logs.
5. Test authentication, region entry and rendering manually after consent.
   An automatic HTTP login without a fresh per-process consent remains blocked.
6. Compare textures, mesh, avatar wearables, sky/sun, animations,
   camera, movement, cache reuse and teleport with upstream Firestorm.

This remains a pre-release OGL development viewer. The standalone
module ABI prototype is not yet installed into the viewer process.

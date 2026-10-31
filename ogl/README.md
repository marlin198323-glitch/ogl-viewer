# OGL Viewer – Firestorm fork

This folder contains OGL-specific extensions based on the official upstream
Firestorm 7.2.5 release. A functional OpenSim Firestorm baseline was already
compiled and GUI-launched on Linux Mint. The next OGL branding iteration now
modifies product identity, welcome page, icons and a pinned HTTP login
consent flow; every other insecure login endpoint stays blocked.

- Upstream: https://github.com/FirestormViewer/phoenix-firestorm
- Fork: https://github.com/marlin198323-glitch/ogl-viewer
- Development branch: `ogl-develop`
- Existing separate viewer: `../nexverse-viewer` (do not overwrite).
- First target: a working, repeatable Linux OpenSim build before branding.
- Grid connection must be confirmed against the actual login endpoint.

The `modules/` directory defines an experimental OGL native C ABI. The
first bundled NVIDIA RTX capability module is now loaded by the real Linux
OGL Viewer at startup and safely unloaded on shutdown. This only reports
Vulkan ray-tracing hardware support; it **does not render RTX effects yet**.
Other arbitrary native modules are not loaded. Native code is not sandboxed.
See [RTX_ROADMAP.md](RTX_ROADMAP.md) for implementation and release gates.

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

## Dedicated Stadt NexVerse grid

This OGL fork is pinned to Stadt NexVerse. The grid profile has one source
of truth: `indra/newview/oglnexversegrid.h`. The login UI cannot select
other grids; upstream and remote grid lists are not loaded. See
[`NEXVERSE_GRID.md`](NEXVERSE_GRID.md) for verified endpoints and TLS
limitations.

The public Stadt NexVerse login currently uses HTTP. OGL permits exactly
that grid URI only after an explicit, temporary warning confirmation.
Automatic HTTP login without approval is blocked. HTTP does **not** encrypt
credentials; use a separate test account when possible.

## Öffentliche Grafikauslieferung

Der OGL Viewer ist nicht an den Rechner eines Entwicklers gebunden:
passende Vulkan-RT-GPUs werden nach unterstützten Funktionen statt nach
Grafikkartenmarken geprüft. Das optionale, allgemeine Qualitätsprofil
„NexVerse Cinematic“ liegt im Viewerpaket und wird bei Bedarf ohne
Überschreiben persönlicher Einstellungen bereitgestellt. Die GPU-RT-
Funktionen sind weiter in Entwicklung; es werden noch keine RTX-Pixel
berechnet. [Kompatibilitätsmatrix](PUBLIC_GRAPHICS_COMPATIBILITY.md).


## Raytracing-Entwicklung: 10 Blöcke

Die aktuelle RTX-Entwicklungsrunde baut einen **wirklich ausgeführten**
Vulkan-Ray-Query-Compute-Shader und testet ein 8x8-Schattenraster
mit 64 Strahlen. Bis die echte OpenSim-Weltgeometrie und das OpenGL-
Compositing angeschlossen sind, werden im Viewer selbst weiter keine
RTX-Schatten angezeigt. [Testbericht und nächste Schritte](
RTX_10_BLOCKS_2026_10_31.md).


Die **zweite RTX-10er-Runde** prüft bereits die komplette isolierte
Datenkette `OGL-Geometrie-ABI → GPU-BLAS/TLAS → Ray-Query-Maske` mit
64 und 1.024 Beispiel-Dreiecken. Die Live-Render-Integration fehlt
weiterhin. [Abnahmebericht](RTX_10_BLOCKS_2026_10_31_P2.md).


Die **dritte RTX-10er-Runde** ergänzt eine getrennt ladbare Vulkan-GPU-
Probe, die nur im expliziten Entwicklungsmodus nach dem Empfang einer
CPU-Szenenprobe eine echte GPU-Ray-Query-Diagnose berechnet.
Der öffentliche Viewer aktiviert sie nicht automatisch. Die
3D-Welt erhält **noch keine sichtbaren RTX-Schatten**.
[Technischer Abnahmebericht](RTX_10_BLOCKS_2026_10_31_P3.md).


Die **vierte RTX-10er-Runde** verlagert die experimentelle Vulkan-GPU-Probe
aus dem Viewer-Render-Thread in eine begrenzte, herstellerunabhängige
asynchrone Warteschlange. Teleports, Fehlerfälle, Hardware-Fallback und
Shutdown sind abgesichert. **Noch keine sichtbaren Raytracing-Schatten.**
[Details](RTX_10_BLOCKS_2026_10_31_P4.md).


In der **fünften RTX-10er-Runde** bleiben Vulkan-Gerät und beim
Sonnenlichtwechsel die GPU-BLAS/TLAS im optionalen Diagnose-Worker
bestehen. Identische Inputs nutzen gecachte Ray-Query-Daten. Der
Endnutzer-Viewer ist weiterhin **OpenGL ohne sichtbare RTX-Schatten**.
[Technischer GPU-Wiederverwendungsnachweis](RTX_10_BLOCKS_2026_10_31_P5.md).

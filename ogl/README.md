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

## Cross-platform rule and future plugins

Every new OGL Firestorm capability must work on Linux, Windows and
macOS, with OS-specific adapters and safe fallbacks where necessary.
The development Linux Mint machine must not become a runtime assumption.
The full policy and architecture are documented in
[PLUGIN_SYSTEM_CROSS_PLATFORM_ARCHITECTURE.md](PLUGIN_SYSTEM_CROSS_PLATFORM_ARCHITECTURE.md).

A cross-OS C-ABI handshake is available, and the portable .ogltheme
checker validates metadata in archives without installing them.
**There is no installable third-party plugin manager or live theme
switching in the viewer yet.** Full Windows/macOS builds and signing
must be verified before claiming cross-platform release readiness.

## CPU/ARM64 and installable themes foundation

The second plugin-system phase implements **runtime CPU capabilities**
and a tested, portable changed-buffer primitive with scalar, SSE2,
separately compiled AVX2 and ARM64 NEON backends. Optional AVX2
requires both CPU features and operating-system XMM/YMM state support.
Six OS/architecture CI module test targets are configured. A portable
theme command-line tool can validate, install, list, select, reset and
remove declarative packages in a per-user directory, without touching
viewer files. **The selection is not yet used by the Firestorm UI.**
Details: [CPU and extension phase 2](CPU_AND_EXTENSION_SYSTEM_PHASE2.md).

## Optional fast texture cache write deduplication

The actual Firestorm fast texture cache now links the same
portable scalar/SSE2/AVX2/NEON CPU comparison backend as the
native module tests. An opt-in experiment can skip byte-identical
fast-cache entries with OGL_FASTCACHE_SKIP_IDENTICAL=1; default
is deliberately disabled because an extra disk read may cost
more than the skipped write. The original cache format and
behavior remain unchanged. The test report exposes aggregate
counts but does not prove an end-to-end speedup.

See [P37 Cache Test](CPU_FASTCACHE_OPT_IN_2026_11_01_P37.md).

## Mandatory OGL Viewer updater (planned)

NexVerse requires a mandatory Viewer updater. Unsupported older
releases must be denied grid login by the server, not only shown a
client-side warning. Existing Velopack sources are not a completed
NexVerse release/update service. See the
[20-block updater plan](GOAL_MANDATORY_VIEWER_UPDATER.md)
and [project handoff for future chats](PROJECT_HANDOFF.md).

## Dedicated Stadt NexVerse grids (public and Beta)

This OGL fork ships exactly two compiled-in login choices:
Stadt NexVerse (hg.stadt-nexverse.de:80) and
Stadt NexVerse Beta (beta.stadt-nexverse.de).
The sole source of truth is indra/newview/oglnexversegrid.h.
The current selection is saved, and other grid imports/overrides are
blocked. See NEXVERSE_GRID.md for URLs and safety rules.

The public OpenSim login uses the existing exact-URI HTTP consent flow.
The Beta entry is selectable, but Beta authentication is deliberately
blocked until its HTTPS certificate and actual login protocol are
verified. Never transmit credentials to an unverified endpoint.
See GOAL_DUAL_GRID_AND_VIEWER_ROADMAP_2026_11_01.md.

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


Die **sechste RTX-10er-Runde** überträgt alle 64 echten GPU-Ray-Query-
Pixelwerte der 8×8-Debug-Maske über die asynchrone Modulverbindung
an eine sichere Firestorm-Host-Schnittstelle. Pixelvalidierung,
Szenenrevisionen und OpenGL-Fallback sind getestet. Es wird
**noch kein Schattenbild in der 3D-Welt angezeigt**.
[Technischer Bericht](RTX_10_BLOCKS_2026_10_31_P6.md).


Die **siebte RTX-10er-Runde** ergänzt perspektivische Kamera-Ray-Queries
mit anschließendem sekundärem GPU-Schattenstrahl zur Sonne. Ein
synthetischer Vulkan-Test mit Boden/Hindernis beweist den Unterschied
zwischen unverschatteter und verschatteter Oberfläche. Dieser Shader
ist als separater, standardmäßig deaktivierter Entwicklermodus
integriert. Die reale OpenSim-Welt wird **noch nicht** mit sichtbaren
Raytracing-Schatten dargestellt.
[GPU-Test und technische Grenzen](RTX_10_BLOCKS_2026_10_31_P7.md).


Die **achte RTX-10er-Runde** bindet die exakte Firestorm-Kamera
(Rechts-/Obenachsen, Roll, vertikaler FOV, Aspect) an Vulkan an.
Der asynchrone GPU-Scheduler kann bei neuer Perspektive denselben
OpenSim-Mesh-Snapshot erneut berechnen, ohne alte Masken zu zeigen.
Dies bleibt ein ausdrücklich aktivierter Diagnosemodus.
**Keine sichtbaren Raytracing-Schatten im Weltbild.**
[GPU- und Kameratestbericht](RTX_10_BLOCKS_2026_10_31_P8.md).


Die **neunte RTX-10er-Runde** bindet erstmals einen kleinen echten
OpenGL-Deferred-Depth-Ausschnitt an die versionierte RTX-Modul-
Diagnoseschnittstelle an. Die 8×8-Probe aus dem Bildzentrum ist
nur mit `OGL_RTX_DEPTH_PROBE=1` aktiv und wird noch **nicht**
an Vulkan als Schatten-Datenquelle übergeben. Kein sichtbares
Raytracing im Weltbild.
[Technischer Bericht und lokaler Test](RTX_10_BLOCKS_2026_10_31_P9.md).


Die **zehnte RTX-10er-Runde** korrigiert die räumliche Zuordnung
der Tiefenabfrage: Ein GPU-Blit reduziert Firestorms echte
Deferred-Tiefe auf ein 8×8-Raster über das **gesamte Sichtfeld**.
Dessen Pixelmitten entsprechen den 8×8-Kamera-Strahlpositionen;
damit wird ein künftiger gemeinsamer Depth-/Ray-Query-Pfad
möglich. Die Daten werden **noch nicht im Vulkan-Schatten-Shader
verwendet**; sichtbares Raytracing bleibt deaktiviert.
[Technischer Bericht](RTX_10_BLOCKS_2026_10_31_P10.md).


Die **elfte RTX-10er-Runde** führt die realen 8×8-OpenGL-Tiefenwerte
erstmals als Eingabe an einen neuen Vulkan-Sonnenstrahl-Shader.
Nahe/Ferne-Clipping und der Depth-Cache sind integriert;
synthetische GPU-Szenen mit Hindernis bestehen den Ray-Query-Test.
Der Standard-Viewer benutzt weiterhin ausschließlich OpenGL;
**noch keine sichtbaren RTX-Schatten**.
[Technische Abnahme](RTX_10_BLOCKS_2026_10_31_P11.md).


Die **zwölfte RTX-10er-Runde** zeigt die 8×8-GPU-Schattenmaske
optional als ausdrücklich gekennzeichnete Diagnosevorschau
im OGL Viewer an. Der neue Dev-Starter
`bash ogl/scripts/run-rtx-preview-test.sh` aktiviert nur
den kontrollierten Testmodus. Die UI enthält den Hinweis
**„NOT WORLD SHADOWS“**. Normaler OpenGL-Betrieb und die
3D-Weltbeleuchtung bleiben unverändert.
[Technischer Bericht und Abnahme](RTX_10_BLOCKS_2026_10_31_P12.md).


Die **dreizehnte RTX-10er-Runde** erweitert den optionalen
OpenSim-GPU-Scene-Snapshot auf bis zu 8.192 Dreiecke und
ergänzt eine begrenzte Triangulierung realer regionaler
Terrain-Höhen. Damit können mehr tatsächliche Schattenwerfer
auf der Vulkan-GPU geprüft werden. Die rote/türkise Anzeige
`NOT WORLD SHADOWS` bleibt **korrekt**, denn eine
hochaufgelöste Integration in den Sonnenlicht-Renderer fehlt
weiterhin. [Technischer Bericht](RTX_10_BLOCKS_2026_10_31_P13.md).


Die **vierzehnte RTX-10er-Runde** unterscheidet bei der
8×8-Vulkan-Diagnose gültige OpenGL-Oberflächen und
Himmel/Hintergrund. Neue Tiefen- und GPU-Tests verhindern, dass
ein Nullwert fälschlich als Grafikfehler ausgegeben wird. In der
Debug-Vorschau erscheinen Himmel grau, unverschattete Flächen
türkis und verschattete Flächen rot – weiterhin ausdrücklich
**NOT WORLD SHADOWS**.
[Abnahme und Entwicklungsgrenzen](RTX_10_BLOCKS_2026_10_31_P14.md).


**RTX-Runde 15:** Die optionale Vulkan-GPU-Diagnose vergleicht
erstmals die tatsächlichen OpenGL-Depth-Werte mit einem
eigenen GPU-Kamera-Primärstrahl in die unvollständige
OpenSim-BLAS. Neue Live-Zähler `primary_hits` und
`depth_matches` helfen einzuordnen, weshalb die 8×8-
Sonnenverschattung manchmal `0/64` ergibt.
[Technischer Bericht](RTX_10_BLOCKS_2026_10_31_P15.md).
Es gibt weiterhin keine sichtbare RTX-Weltbeleuchtung.


**Dynamische Raytracing-Datenstrukturen (31.10.2026):**
BLAS-Refit und TLAS-Transform-Refit laufen als Vulkan-GPU-
Entwicklertests. Eine begrenzte CPU-Objektregistry übernimmt
UUIDs, lokale Dreiecke und pro-Frame-Transformationen aus dem
Firestorm-Scanner. Die produktive GPU-TLAS ist jedoch noch
nicht mehrinstanzig; es fehlen vollständige Regionsdaten,
animierte Avatare und GPU-Realtime-Synchronisation.
[Technische Details](RTX_DYNAMIC_ACCELERATION_STRUCTURES_2026_10_31.md).


**OpenGL/Vulkan-Depth-Abgleich:** Der aktuelle NexVerse-
Live-Test meldete zahlreiche GPU-Primärtreffer, aber keine
einzige passende OpenGL-Tiefe. Die neue Diagnose trennt,
ob die GPU eine Oberfläche vor oder hinter der GL-Tiefe
trifft. Hardware-GPU-Tests für beide Fälle sind vorhanden;
ein weiterer Live-Test entscheidet über die nächste
Korrektur. [Technische Auswertung](RTX_LIVE_DEPTH_MISMATCH_2026_10_31.md).


**Vordergrund-Mesh-Verbesserung:** Nach einem Live-Test mit
35 Vulkan-Primärtreffern *hinter* den sichtbaren GL-Oberflächen
sammelt der optionale RTX-Scanner nahe, im Blick liegende
Objekte zuerst, begrenzt ihre einzelnen Dreieckbudgets
und entnimmt Mesh-Face-Dreiecke über die gesamte Indexliste.
[Details und Abnahme](RTX_FOREGROUND_MESH_CAPTURE_2026_10_31.md).
Noch keine vollständige Regions-BVH und keine RTX-Weltschatten.

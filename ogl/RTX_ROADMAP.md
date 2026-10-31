# OGL RTX – optional Vulkan ray tracing backend

## Implemented and tested (2026-10-31)

- Module `ogl_rtx.so` uses the OGL versioned native C ABI v1.
- Dynamically opens the Vulkan loader and queries NVIDIA GPU properties,
  `VK_KHR_acceleration_structure`, `VK_KHR_ray_query`,
  `VK_KHR_ray_tracing_pipeline`, extension dependencies and feature flags.
- Confirmed on RTX 4070 SUPER, NVIDIA 615.71.09:
  `acceleration=1 query=1 pipeline=1 hardware_ready=1`.
- Actual OGL Viewer now loads the bundled `./lib/ogl_rtx.so` on startup,
  checks the exact module identity, logs status and unloads on shutdown.
- Driver missing / no RTX GPU is a clean, supported OpenGL fallback.
- The Firestorm/OpenGL graphics pipeline remains in control.
- **No ray tracing lighting, shadows, reflections or GI is rendered yet.**
  The value `rendering_active=0` is intentionally explicit and accurate.
- No third-party user plugin paths or unknown native modules are allowed.
- Frame-data bridge v1 implemented and compiled in the real OGL world
  deferred lighting path: viewport dimensions, camera position/direction,
  and sun direction are sent through the RTX module C ABI.
- Native RTX module API test submits synthetic camera/sun frames and checks
  acceptance/validation. **Actual world frame transfer has yet to be
  confirmed after an authenticated OpenSim login**.
- Real OpenSim world-volume geometry exchange now implemented: sample
  loaded world-space triangles from nearby static prim/mesh volume faces,
  bounded to 1,024 triangles and 96 objects every 180 render frames.
  Rigged attachments/avatars are excluded; the sample resets when empty.
  The OGL native module validates finite coordinates and holds the last
  non-empty sample in **CPU memory only**. No on-disk geometry caching.
- Vulkan logical device test now **passes on RTX 4070 SUPER** with
  VK_KHR_ray_query, VK_KHR_acceleration_structure and buffer device address
  enabled. This proves actual logical GPU device creation rather than
  only enumerating extensions.
- **Still missing:** GPU transfer of sampled scene triangles, BLAS/TLAS
  building, ray-query shader dispatch, shadow mask, denoising, and
  OpenGL/Vulkan compositing. There are **no raytraced image pixels yet**.
- The new in-world triangle capture requires a fresh viewer login to
  confirm it reports a nonzero triangle count in OGL.log; offline tests
  and C++ compilation do not replace this live acceptance.
- No OpenSim server configuration change is required for viewer-local RTX;
  the viewer reuses geometry/material/lighting already supplied by OpenSim.
  Advanced effects still depend on asset and material fidelity.


## Required renderer phases

1. GPU capability module and in-viewer lifecycle. DONE (Linux).
2. Camera/sun/viewport data bridge v1: IMPLEMENTED and native-tested;
   actual in-world delivery still pending login acceptance. Vulkan
   device/queue, synchronization and surface transfer: NOT STARTED.
3. Transfer bounded static, non-rigged OpenSim volume triangles to CPU
   module: IMPLEMENTED, native-tested; live in-world triangle count pending.
   Full terrain, animated/rigged avatars, attachments and GPU BLAS/TLAS:
   NOT STARTED. Next: persistent Vulkan device and GPU vertex buffers.
4. Render **one** measurable effect first: ray-traced sun/moon contact
   shadows. Implement denoising, temporal accumulation, fallback,
   performance budget and memory limits. NOT STARTED.
5. Optional PBR reflections (water, glass, metals) and local ray-traced
   ambient occlusion; account for transparent and animated content.
   NOT STARTED.
6. Optional ray-traced global illumination, multiple bounces and
   upscaling. Compare image quality/FPS and GPU load across GTX,
   NVIDIA RTX, AMD Vulkan RT and low-end GPUs; default off unless
   explicitly enabled and supported. NOT STARTED.
7. Build, test and package Vulkan-enabled backends for Linux and
   Windows, including signed/verified module distribution and stable
   versioned APIs. NOT STARTED.

## Integration details

Firestorm renders through OpenGL. Vulkan ray tracing requires a separate
hardware acceleration pipeline and scene data, as documented by Khronos.
A shared library loading successfully does **not** automatically enable
the GPU's RT cores for Firestorm drawing.

To build/launch on Linux Mint:

```bash
cd "$HOME/Schreibtisch/OGL-Viewer/ogl-firestorm"
bash ogl/scripts/build-test.sh
bash ogl/scripts/run-viewer-test.sh
python3 ogl/scripts/ogl-test-report.py
```

The local Vulkan C headers are kept outside the Git repo at
`~/Schreibtisch/OGL-Viewer/.vulkan-sdk-local/headers/include` from
the Khronos `Vulkan-Headers` project tag `v1.3.275` and can later
be replaced with the system's `libvulkan-dev` package. No proprietary
NVIDIA toolkit or user passwords are stored in Git.

Upstream Khronos references:
- https://docs.vulkan.org/guide/latest/extensions/ray_tracing.html
- https://docs.vulkan.org/samples/latest/samples/extensions/ray_tracing_basic/README.html

## 31.10.2026 – erster echter Vulkan-BLAS-/TLAS-Aufbau

Ein neuer, **hardwareunabhängig implementierter** GPU-Test befindet sich in
`ogl/modules/rtx/vulkan_blas_smoke.cpp`. Er erstellt ein logisches Vulkan-RT-
Gerät auf einer nach Vulkan-Features ausgewählten GPU, kopiert ein Dreieck
in einen Device-Address-Puffer und führt auf der GPU folgendes aus:

- BLAS-Build über `vkGetAccelerationStructureBuildSizesKHR`,
  `vkCreateAccelerationStructureKHR` und `vkCmdBuildAccelerationStructuresKHR`.
- Synchronisierter TLAS-Build aus einer Instanz, die auf die GPU-Adresse
  der zuvor gebauten BLAS zeigt.
- GPU-Adressen und abgeschlossene Queue-Befehle werden verifiziert.
- Vulkan-/RT-freie Rechner bekommen ein CTest **SKIP**; OpenGL bleibt nutzbar.

**Lokal bestanden:** GeForce RTX 4070 SUPER: BLAS 2.560 Byte, BLAS-Scratch
1.920 Byte, TLAS 2.048 Byte, TLAS-Scratch 2.176 Byte (treiberspezifische
Zahlen; keine verbindlichen Speichervorgaben für andere GPUs).

**Nicht erledigt:** Reale OpenSim-Szenendreiecke sind noch nicht mit diesem
GPU-Builder verbunden. **Dieser historische Abschnitt** wurde inzwischen
um einen isolierten Ray-Query-GPU-Test ergänzt: 64 Teststrahlen erzeugen
korrekt eine 8x8-Okklusionsmaske. Die echte NexVerse-Szenengeometrie bleibt
aber weiterhin unverbunden und im OpenGL-Bild erscheint keine RTX-Maske.
Der Viewer benutzt weiterhin OpenGL. Ein isolierter GPU-Test ist keine
Raytracing-Freigabe für Endnutzer.


## 31.10.2026 – Ray-Query-Compute-Shader und Szenenrevisionen

- Shader `ogl/modules/rtx/shaders/ray_query_shadow.comp`, übersetzt nach
  SPIR-V und mit `spirv-val` auf Vulkan 1.2 geprüft.
- BLAS für ein Dreieck und TLAS mit Instanz auf der GPU aufgebaut.
- 64 Ray Queries ergeben eine **korrekte 8x8-Schattenmaske**:
  **36 Treffer / 28 Nichttreffer**, nach der GPU-Ausführung ausgelesen.
- Vorstufe für echte Shadow-Maps: `VK_KHR_ray_query`, Compute-Pipeline,
  Descriptor-Set mit TLAS und Ergebnis-Storage-Buffer sind funktionsfähig.
- SPIR-V als öffentliches Viewer-Asset unter
  `indra/newview/app_settings/ogl/rtx/ray_query_shadow.comp.spv`.
- Geometrie-API erhält eine Revisionsnummer und bounded Snapshot-Kopie.
  Unveränderte Dreiecke erhöhen die Revision nicht; Teleport/Leerszene
  löst ein klares Zurücksetzen aus.
- Gemeinsame finite/degenerate-Validierung ist im Viewer-Export und
  RTX-Modul integriert. Die Modultests prüfen auch 1.024 Dreiecke.
- Für GPU-freie Rechner bleiben alle nichtgrafischen Tests aktiv; Vulkan-
  RT-Tests melden SKIP, der OpenGL-Viewer bleibt unabhängig.
- Dokumentierte Einzelabnahme:
  [`RTX_10_BLOCKS_2026_10_31.md`](RTX_10_BLOCKS_2026_10_31.md).

**Nicht erledigt:** Live-BLAS aus der geladenen NexVerse-Region, GPU-
Szenenupdates, weltweite Sonnen-/Materialdaten, Echtzeit-Shadow-Denoiser
und das eigentliche OpenGL-Compositing. `rendering_active=0` bleibt
deshalb korrekt. Endnutzer sehen noch kein hardwareberechnetes RTX-Bild.


## 31.10.2026 – zweite 10er-Runde: OGL-ABI → Vulkan-GPU

- Der eigenständige Vulkan-Builder kann einen Snapshot über die gleiche
  native C-ABI annehmen, die der Viewer für seine OpenSim-Dreiecke benutzt.
  Es werden synthetische Testdreiecke durch das OGL-Modul eingereicht,
  kopiert und anschließend für BLAS, TLAS und die 64 GPU-Strahlen verwendet.
- Zwei GPU-Tests bestehen: 64 Dreiecke und 1.024 Dreiecke.
  Die resultierende 8×8-Testmaske bleibt korrekt (36/28).
- Kritischer GPU-Sicherheitsfehler behoben: Eine TLAS hat eine Instanz,
  auch wenn die BLAS 1.024 Dreiecke enthält. BLAS-Primitive-Count darf
  nicht als TLAS-Instanzanzahl verwendet werden.
- GPU-freie Systeme überspringen die Vulkan-Tests kontrolliert;
  reiner OpenGL-Betrieb bleibt erhalten.
- Die Szene meldet CPU-Snapshot-Anzahl und Revision ohne Objektnamen;
  eine spätere Leerszene oder ein Teleport erzeugt ein
  `CPU scene cleared`-Ereignis. Der Diagnosebericht erkennt den
  chronologisch letzten Zustand.
- Ausführlicher Nachweis:
  [`RTX_10_BLOCKS_2026_10_31_P2.md`](RTX_10_BLOCKS_2026_10_31_P2.md).

**Nicht als erledigt melden:** Der produktive Viewer baut noch keine
GPU-BLAS aus der *aktuell verbundenen* OpenSim-Region auf; diese
Schnittstelle ist bislang nur isoliert mit Testdaten abgenommen.
Es gibt noch keine Raytracing-Schatten im sichtbaren OGL-Viewer,
kein GPU/GL-Compositing und keinen Endnutzer-RTX-Schalter.


## 31.10.2026 – dritte RTX-10er-Runde: optionaler Live-GPU-Probe-Pfad

- Die bisherige Vulkan-BLAS/TLAS-/Ray-Query-Pipeline wurde in die optionale
  Bibliothek `ogl_gpu_probe.so` / `ogl_gpu_probe.dll` mit eigener
  C-ABI ausgelagert und in Linux-/Windows-Paketpfade integriert.
- Das RTX-Modul kann bei explizitem Entwickler-Opt-in
  (`OGL_RTX_SCENE_PROBE=1`) *seinen eigenen* Scene-Snapshot übergeben:
  BLAS/TLAS-Build, 64 GPU-Ray-Queries und rein numerische Diagnose.
- Push Constants nutzen Scene-XY-Grenzen und die Viewer-Sonnenrichtung;
  die alte deterministische Shader-Regression mit 36/28 Treffern bleibt.
- Ein isolierter End-to-End-Modultest mit 64 Dreiecken lieferte 15/64
  GPU-Treffer. Weitere Tests prüfen fehlendes GPU-Backend und fehlende
  Vulkan-Treiber ohne Beeinträchtigung von OpenGL.
- Die synchronen Probe-Aufrufe sind bewusst **aus** für Endnutzer.
  Rate-Limit: 60 Sekunden; GPU-Fehler deaktivieren die Probe bis Neustart.
- [`RTX_10_BLOCKS_2026_10_31_P3.md`](RTX_10_BLOCKS_2026_10_31_P3.md).

**Noch nicht erreicht:** Live-Login-Szene mit tatsächlichen NexVerse-Dreiecken
auf dem GPU-Renderer abgenommen, dauerhafte BLAS/TLAS-Weltverwaltung,
sichtbare Raytracing-Schatten und OpenGL/Vulkan-Komposition.
`rendering_active=0` darf noch nicht geändert werden.


## 31.10.2026 – vierte 10er-Runde: Vulkan-Probe asynchron

- `ogl_async_gpu_queue.h`: ein dedizierter Worker für die bereits
  existierende Vulkan-BLAS/TLAS-/Ray-Query-Probe. CPU-Snapshots werden
  begrenzt kopiert und nur nach explizitem Entwickler-Opt-in verarbeitet.
- `ogl_rtx_submit_geometry` führt **keine** synchronen Vulkan-Aufrufe
  mehr aus; es übermittelt lediglich den neuesten CPU-Snapshot.
- Veraltete Warteschlangeneinträge und Ergebnisse nach Teleport werden
  verworfen. Ein GPU-Fehler stoppt weitere Tests.
- Die OGL-Logausgabe bleibt ausschließlich im Viewer-Thread. Worker
  wird vor dem Entladen seiner GPU-Bibliothek sauber beendet.
- Zusätzliche native Tests prüfen No-GPU-Fallback, Verzögerungen,
  Deduplizierung, Szenenwechsel und Shutdown.
- Details: [`RTX_10_BLOCKS_2026_10_31_P4.md`](RTX_10_BLOCKS_2026_10_31_P4.md).

**Unverändert:** Die Vulkan-Probe baut GPU-BLAS/TLAS pro Testauftrag
neu auf. Kein persistentes Vulkan-Gerät und kein sichtbares RT-Bild.
`rendering_active=0` bleibt korrekt. Öffentliche Nutzer benötigen
weder Opt-in noch einen Vulkan-RT-fähigen Rechner.


## 31.10.2026 – fünfte RTX-10er-Runde: GPU-Device und BLAS/TLAS wiederverwenden

- `VulkanContext` bleibt **pro GPU-Worker-Thread** zwischen mehreren
  Diagnosejobs erhalten. Vulkan-Instanz und logisches Gerät werden nicht
  bei jeder Änderung neu erstellt.
- Ein byteidentischer CPU-Dreiecks-Snapshot mit unveränderter Sonne und
  Shaderressource nutzt die gecachten Ray-Query-Ergebnisse.
- Bei **reinem Lichtwechsel** bleiben bestehende GPU-BLAS/TLAS erhalten;
  der Ray-Query-Compute-Pass wird mit neuem Licht neu ausgeführt.
- Bei geänderter Geometrie werden taskbezogene GPU-AS-Ressourcen
  sauber freigegeben und neu aufgebaut; das Vulkan-Gerät bleibt erhalten.
- ABI v2 der Diagnose meldet Geräteinitialisierungen, BLAS-Neuaufbauten
  und Cache-Treffer. Die Statistik erscheint nur bei Entwickler-Opt-in
  im anonymisierten Viewer-Log.
- Test mit 1 und 1.024 Dreiecken auf RTX 4070 SUPER:
  **1 Geräteinitialisierung, 3 BLAS-Builds, 2 Cache-Hits**.
- 32 Python- und 14 native Tests, darunter GPU-freie Fallbacktests;
  vollständiger Linux-OGL-Build erfolgreich.
- Details: [`RTX_10_BLOCKS_2026_10_31_P5.md`](RTX_10_BLOCKS_2026_10_31_P5.md).

**Noch offen:** Sichtbare RT-Schatten in der 3D-Welt, OpenGL/Vulkan-
Compositing, inkrementelle Mesh-Updates innerhalb einer BLAS,
Multi-GPU-Auswahl sowie vollständige Windows-Viewer-/AMD-/Intel-Abnahme.
`rendering_active=0` bleibt korrekt. Die GPU-Diagnose ist kein
Produktiv-Renderer.


## 31.10.2026 – sechste 10er-Runde: vollständige Ray-Query-Debug-Maske

- Die Vulkan-Compute-Abnahme gibt jetzt nicht mehr nur die Zahl der
  Strahlentreffer zurück, sondern ein vollständiges **8×8-Raster aus
  64 tatsächlich auf der GPU berechneten Pixelwerten**.
- Diagnose-ABI **v3**: `mask_width`, `mask_height` und
  `shadow_mask[64]`; derselbe Maskeninhalt wird bei identischen
  Szenen-/Sonnen-/Shaderdaten aus dem bisherigen GPU-Cache übernommen.
- Der asynchrone GPU-Worker validiert jedes Pixel sowie die
  Übereinstimmung von Pixelmaske und Trefferzahl. Fehlerhafte
  Ergebnisse deaktivieren den Diagnosepfad (OpenGL-Fallback).
- `ogl_rtx_copy_shadow_debug` ist eine eigene, versionierte
  read-only C-API im RTX-Modul. Die Firestorm-Hostseite stellt
  `ogl_rtx_copy_debug_shadow_mask` zur zukünftigen Diagnoseanzeige bereit.
- Die Debug-Maske wird bei Mesh-Änderung, Teleport oder Sonnenwechsel
  verworfen; auf eine alte GPU-Antwort wird niemals zurückgegriffen.
- GPU-Regressionen prüfen das exakte Dreiecks-Maskenmuster mit
  36 Treffern sowie den Bytevergleich bei Cache-Treffern.
- Kompletter Linux-Viewer-Build mit beiden Vulkan-SOs und der
  SPIR-V-Ressource erfolgreich.
- [`RTX_10_BLOCKS_2026_10_31_P6.md`](RTX_10_BLOCKS_2026_10_31_P6.md).

**Weiterhin kein sichtbares Raytracing:** Das Raster stammt von
einer kleinen Testszene, nicht aus dem perspektivisch korrekten
Framebuffer. Kein Schatten-Compositing und kein Endnutzer-RTX-Schalter.
`rendering_active=0` bleibt unverändert.


## 31.10.2026 – siebte RTX-10er-Runde: kamerabasierte GPU-Sonnenschatten

- Neuer Vulkan-Compute-Shader `camera_sun_shadow.comp` erzeugt aus
  Kamera-Position, Vorwärtsvektor und Blickfeld eine 8×8-Matrix
  perspektivischer Primärstrahlen.
- Zu jeder getroffenen Oberfläche folgt ein zweiter GPU-Ray-Query-
  Strahl in Sonnenrichtung. Erst dieser zeigt die geometrische
  **Sonnenverschattung der sichtbaren Oberfläche**.
- Separater GPU-Test auf der RTX 4070 SUPER: Testboden **0/64**
  verschattete Bildpunkte, mit erhöhtem Hindernis **2/64**.
  Kamerabewegungen verändern das Ergebnis, ohne BLAS/TLAS neu aufzubauen.
- Neues optionales ABI `ogl_gpu_probe_camera_run`; Kamera- und
  Viewportwerte sind Bestandteil des GPU-Cache-Schlüssels.
- Neuer kombinierter Entwickler-Modus:
  `OGL_RTX_SCENE_PROBE=1 OGL_RTX_CAMERA_PROBE=1`.
  Bleibt standardmäßig vollständig aus, OpenGL ist unverändert.
- Der Maskenstatus wird bei Kamerabewegung/Viewportänderung
  invalidiert und das Ergebnis über die asynchrone Queue verarbeitet.
- Beide SPIR-V-Shader werden für Linux und Windows paketiert;
  GLSL und SPIR-V werden in Linux-CI geprüft.
- Details: [`RTX_10_BLOCKS_2026_10_31_P7.md`](RTX_10_BLOCKS_2026_10_31_P7.md).

**Noch keine Sichtfreigabe:** Die neue perspektivische Berechnung
ist bisher ein 8×8-Test mit synthetischer Geometrie und vereinfachter
Kameraausrichtung. Kein OpenGL-Compositing, keine Vollbild-Raytracing-
Schatten und kein produktiver RTX-Schalter. `rendering_active=0`.


## 31.10.2026 – achte RTX-10er-Runde: präzise Firestorm-Kameradaten

- Die Renderpipeline nutzt für GPU-Kamerastrahlen jetzt unmittelbar
  `LLCamera::getAtAxis()`, `-getLeftAxis()`, `getUpAxis()`,
  `getView()` (vertikaler FOV in Radiant) und `getAspect()`.
  Feste 60°-Näherung und world-up-abgeleitete Achsen entfallen.
- Frame-ABI auf `OGL_RTX_FRAME_VERSION=2` erweitert und durch eine
  herstellerunabhängige Prüffunktion abgesichert.
- Vulkan-Push-Constants benutzen korrekte Firestorm-Kamerabasis;
  FOV, Seitenverhältnis und Rollwinkel sind Teil des GPU-Cache-Schlüssels.
- Wichtiger Fehler behoben: Mesh-Revisionsnummer und GPU-Auftragssequenz
  sind getrennt; bei reiner Kamerabewegung kann ein neuer GPU-Job mit
  unverändertem Mesh erzeugt werden.
- Gemeinsamer GPU-Scheduler für Frame- und Mesh-Updates mit weiterhin
  standardmäßiger 60-Sekunden-Grenze für Entwicklerdiagnosen.
- Der neue GPU-Modultest beweist zwei Kameradurchläufe bei derselben
  Geometrie mit veralteter Maske zuverlässig invalidiert.
- ABI-Prüfungen für Kamera-Roll, FOV, Portrait, Ultrawide und ungültige
  Werte. Vollständiger Linux-C++-Build grün.
- [`RTX_10_BLOCKS_2026_10_31_P8.md`](RTX_10_BLOCKS_2026_10_31_P8.md).

**Weiterhin offen:** echte Tiefen-/Normalen-Daten aus der
Deferred-Pipeline, vollständige OpenSim-Szene, bildrichtige
OpenGL/Vulkan-Überblendung, Qualität und Laufzeitmessungen.
`rendering_active=0` bleibt wahr.


## 31.10.2026 – neunte RTX-10er-Runde: echte Deferred-Tiefenprobe

- Firestorm stellt den OpenGL-Tiefenpuffer in
  `mRT->deferredScreen.getDepth()` bereit. Dessen Quelle wurde im
  originalen FBO-/Deferred-Renderpfad verifiziert.
- Neue `ogl_rtx_depth_debug`-C-ABI (v1) überträgt maximal
  64 normierte Tiefenwerte aus einem **zusammenhängenden
  8×8-Bildzentrums-Ausschnitt** samt Renderzielgröße und Koordinaten.
- Optionaler Frame-Readback per separatem READ-FBO, vollständige
  Wiederherstellung von OpenGL-Read-FBO und Pixel-Pack-Zustand.
- **Ausgeschaltet, außer bei Entwickler-Opt-in**
  `OGL_RTX_DEPTH_PROBE=1`; höchstens einmal pro Minute.
  `glReadPixels` ist noch synchron: nicht für Echtzeit freigeben.
- RTX-Modul validiert Werte und verwirft vorige Depth-Proben
  bei jedem neuen Kamera-Frame. Reduzierte Renderauflösungen
  werden anhand des Kameraseitenverhältnisses unterstützt.
- Portable C++-Validierung und Modul-ABI-Test erweitert.
  Diagnosebericht erkennt einen echten Deferred-Depth-Readback.
- 36 Python-/19 native Tests und kompletter Linux-Viewer gebaut.
- [`RTX_10_BLOCKS_2026_10_31_P9.md`](RTX_10_BLOCKS_2026_10_31_P9.md).

**Echte eingeloggte OpenSim-Szene noch nicht abgenommen.** Die
Tiefenwerte werden noch nicht als Input in Vulkan benutzt. Das
8×8-Zentrum ist nicht mit dem 8×8-Vollbild-Ray-Query-Raster
deckungsgleich. Kein OpenGL/Vulkan-Compositing und keine
sichtbaren RTX-Weltschatten: `rendering_active=0`.

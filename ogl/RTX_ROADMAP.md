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

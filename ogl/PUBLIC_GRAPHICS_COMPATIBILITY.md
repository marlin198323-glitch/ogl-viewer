# OGL Viewer – Grafiksystem für öffentliche Nutzer

Stand: 2026-10-31. Der Viewer gehört **allen kompatiblen Nutzern**; die
Entwicklung oder Qualität darf nicht an den Rechner des Entwicklers
gekoppelt werden.

## Architektur

1. OGL lädt pro Start ein **mitgeliefertes** natives Vulkan-RT-Modul.
   Unter Linux: `lib/ogl_rtx.so`; unter Windows: `ogl_rtx.dll` neben
   der ausführbaren Datei. Das Modul ist optional.
2. Erkennung prüft je physischem Vulkan-Gerät die tatsächlich vorhandenen
   `VK_KHR_acceleration_structure`-,
   `VK_KHR_ray_query`- oder
   `VK_KHR_ray_tracing_pipeline`-Funktionen sowie
   `bufferDeviceAddress`. **Kein GPU-Modell oder Hersteller wird
   hartcodiert.** Wenn mehrere GPUs existieren, wird ein geeignetes
   diskretes Gerät bevorzugt.
3. Ohne passende Vulkan-RT-Funktionen, DLL/SO oder Vulkan-Laufzeit
   arbeitet der Viewer unverändert mit OpenGL. Keine RT-Abhängigkeit
   beim Programmstart oder beim Login.
4. Das native Grafikprofil `NexVerse Cinematic` ist in
   `indra/newview/app_settings/ogl/NexVerse%20Cinematic.xml`
   enthalten. Der Firestorm-Presetmanager kopiert es beim ersten
   Aufruf der Grafikvoreinstellungen **nur wenn es fehlt** in das
   jeweilige Nutzerprofil. Keine persönlichen Pfade, Kennwörter oder
   GPU-spezifischen Werte.
5. Das Preset wird **nicht automatisch aktiviert**. Es schaltet
   ausschließlich vorhandene PBR-/OpenGL-Effekte und gilt als
   Qualitätsoption, nicht als Raytracing.
6. Für die spätere echte Raytracing-Version sind Einstellungen wie
   `Aus`, `Automatisch`, `Qualität`, `Ultra` vorgesehen. Nur
   nach Verifikation eines funktionierenden Renderpfads darf dort
   „Raytracing aktiv“ stehen.
7. Die Grafikstatusanzeige bezeichnet die Hardware als
   `Vulkan RT bereit` statt fälschlich NVIDIA-spezifisch
   zu sein. `rendering_active=0` bleibt so lange wahr,
   bis nachweislich RT-berechnete Bilder dargestellt werden.

## Plattformmatrix

| Plattform/GPU | Status |
|---|---|
| Linux + NVIDIA RTX, getesteter Rechner | C++ Viewer, Gerätetest, Modul geladen und OpenGL-Fallback bestanden |
| Linux + AMD mit Vulkan Ray Query / Raytracing | Herstellerneutrale Erkennung implementiert; GPU-Abnahme offen |
| Linux + Intel mit Vulkan RT | Herstellerneutrale Erkennung implementiert; GPU-Abnahme offen |
| Windows 64-Bit + geeigneter Vulkan-RT-GPU | DLL-Ladeweg und CMake/Manifest-Integration implementiert; echter Viewer-Build/Windows-Abnahme offen |
| Kein Vulkan RT / zu alter Treiber | OpenGL-Fallback vorgesehen; Linux-Ausfalltest bestanden |
| macOS | OGL/OpenGL-Basismodus; Vulkan-RT-Modul auf macOS derzeit nicht unterstützt |

**Wichtig:** Eine Vulkan-geeignete GPU reicht allein nicht, um RT-Effekte
in der Welt anzuzeigen. Geometrie, BLAS/TLAS, Ray-Query-Shader und
Ergebnis-Compositing sind weiterhin in Entwicklung. GPUs mit mehreren
Grafikchips müssen künftig gegen die tatsächlich verwendete OpenGL-GPU
abgeglichen werden. Feature-Erkennung allein gewährleistet noch keine
Multi-GPU-Kopplung.

## Freigabekriterien

- [x] Herstellerneutrale GPU-Funktionsprüfung und logischer Vulkan-RT-Gerätetest
- [x] Optionales Grafikprofil im Releasepaket statt im Entwicklerkonto
- [x] Grafikeinstellungen bleiben bis zur manuellen Aktivierung unverändert
- [x] Vulkan-RT-Modul wird bei verfügbaren SDK-Headern mit dem Viewer gebaut
- [x] Linux-OpenGL-Fallback- und ABI-Tests
- [x] Quellcode und Modul unter Linux erfolgreich gebaut
- [x] Linux-/Windows-CI für portable Vulkan-RT-Module
- [ ] Vollständiger Windows-Viewer-Start und Installer-Abnahme
- [ ] AMD/Intel-RT-Hardware geprüft
- [ ] Multi-GPU-Identitätsabgleich (OpenGL ↔ Vulkan)
- [x] Isolierter Vulkan-GPU-BLAS- und TLAS-Build mit Testdreieck und Instanz
- [x] OGL-Geometrie-ABI mit 64 und 1.024 synthetischen Dreiecken an Vulkan-BLAS/TLAS angebunden
- [x] Opt-in-Developer-Probe: vorhandene RTX-Modul-Snapshots an GPU BLAS/TLAS übergeben
- [x] Entwicklerprobe aus Render-Thread gelöst: begrenzte, asynchrone CPU-Snapshot-Queue
- [x] GPU-Gerät pro Worker wiederverwendet; BLAS/TLAS bei Lichtwechsel erhalten
- [x] Identische Szene/Sonne/Shader: Ray-Query-Ergebnis ohne GPU-Neuaufbau wiederverwendet
- [ ] Inkrementelle BLAS-/TLAS-Mesh-Updates und produktiver Echtzeit-Renderer
- [x] Isolierter 8x8-Vulkan-Compute-Test: 64 Ray Queries, 36 Treffer, 28 Misses
- [x] GPU-8×8-Ray-Hit-Maske validiert bis zur Firestorm-Debug-Hostschnittstelle übertragen
- [x] Synthetische Kameraszene: primäre GPU-Strahlen + sekundäre Sonnenschatten geprüft
- [x] Exakte Firestorm-Kamerabasis, Roll, FOV und Aspect in GPU-Shader übernehmen
- [x] Tatsächliche Firestorm-Deferred-Depth-Textur als ausgerichtetes 8×8-Vollbild-Diagnoseraster abtasten
- [ ] Echte Deferred-Depth-Live-Daten in einer angemeldeten NexVerse-Region abnehmen
- [x] Vulkan-Diagnose verarbeitet 64 ausgerichtete OpenGL-Tiefenwerte für Sonnenstrahlen
- [ ] Vulkan erhält vollständige Tiefen- und Normalen-Renderziele ohne synchronen CPU-Readback
- [x] Bounded Entwickler-Szenensnapshot bis 8.192 Dreiecke inklusive grobem Terrain einbinden
- [x] Vulkan-BLAS-Refit und transform-only-TLAS-Refit mit echten GPU-Abnahmetests
- [x] UUID-basierte CPU-Objektverwaltung mit Renderframe-Transformaktualisierung
- [ ] Vollständige dynamische OpenSim-Geometrie mit echten Terrain-Patches, Avataren, Materialsicherheit und Updates erfassen
- [ ] Mehrinstanzige Vulkan-TLAS mit eigenständigen gecachten Prim-/Mesh-/Avatar-BLAS und GPU-synchronen Realtime-Updates
- [x] Abgesicherten 8×8-Vulkan-Ergebnis-Preview als klar gekennzeichnetes Entwickler-UI hinzufügen
- [x] Himmel/Oberflächen getrennt klassifizieren und GPU-Messwerte zum exakt gleichen Kamera-Snapshot zuordnen
- [x] Kamera-Primärstrahlen mit OpenGL-Depth abgleichen, fehlende Mesh-Abdeckung getrennt von echten Schatten prüfen
- [ ] Bildschirmfüllende Ray-Query-Schattenmaske in OpenGL einfügen
- [ ] Performance-/Stabilitäts- und Abnahmetests bei verschiedenen Auflösungen
- [ ] Installer, automatische Updates und Signaturen für Endnutzer
- [ ] Finales Lizenz-/Third-Party-Notices- und Release-Audit

Die CI-Vorprüfung unter
`.github/workflows/ogl-portable-graphics.yml` kompiliert die
kleinen nativen Grafikmodule auf Linux und Windows. Sie ist
**keine** Freigabe des vollständigen Viewer-Installers.

Keine Änderungen am Stadt-NexVerse-OpenSim-Server nötig:
Raytracing ist eine Funktion des jeweiligen Viewers und seiner GPU.

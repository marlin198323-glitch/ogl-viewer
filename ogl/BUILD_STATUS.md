# OGL Viewer – Linux Mint: aktueller Build- und Abnahmestand

Stand: 31.10.2026 | Branch `ogl-develop` | Firestorm 7.2.5 / OpenSim

## Bereits verifiziert

- Firestorm-basierter OGL Viewer auf Linux Mint kompiliert und per X11
  mit eigener Verknüpfung gestartet.
- NVIDIA RTX 4070 SUPER mit OpenGL 4.6 und Vulkan-Raytracing-Erweiterungen
  erkannt. `hardware_ready=1`, aber `rendering_active=0`:
  **keine** echten Raytracing-Schatten oder Reflexionen implementiert.
- Stadt NexVerse ist das fest eingebaute, einzige auswählbare Grid.
  Seine GridInfo veröffentlicht `http://hg.stadt-nexverse.de:80/`.
- NexVerse-Welcome-Seite: `https://stadt-nexverse.de/service/welcome`.
- Eigenes Datenverzeichnis: `~/.local/share/ogl-firestorm-test`.
- OGL-Branding, Grafik-Icons, Login-Modusname und UI-Ressourcen aufgebaut.
- Nutzerseitiger Wunsch: bestehende **HTTP-Anmeldung** statt HTTPS.
  OGL verlangt beim ersten manuellen Login eine ausdrückliche Warnungs-
  bestätigung ("Über HTTP anmelden"), ausschließlich für das fest
  hinterlegte Stadt-NexVerse-Grid.
- Die Freigabe gilt nur für den laufenden Prozess. Der eigentliche
  Authentifizierungsaufruf prüft sie ebenfalls. Automatische HTTP-Anmeldung
  ohne erneute Bestätigung und fremde HTTP-Ziele bleiben gesperrt.
- Vollständiger C++-Build für diese Freigabe erfolgreich.
- Historischer Zwischenstand: 14/14 Python-Vertrags-/Sicherheitstests
  und 4/4 native Modul-/Allowlist-Tests bestanden; aktueller Stand siehe oben.

## NexVerse-Login und RTX-Bridge – aktueller UI-Stand

- Die Login-Leiste wurde in OGL-Dunkelblau mit Cyan-Akzenten gestaltet:
  Logo, native Beschriftungen, klarere Eingabefelder, cyanfarbener
  „Welt betreten“-Button, Stadt-NexVerse-Grid-Auswahl und lesbare Links.
- Welcome-Seite bleibt unverändert im eingebetteten Browser.
- OGL nutzt die neue Leiste unabhängig von historischen Firestorm/Phoenix-
  Einstellungen; deutscher Buttontext ist separat geprüft.
- Die native Grafikstatus-Zeile unterscheidet zwischen RTX-Kartenunterstützung
  und tatsächlich aktivem Raytracing: Bei RTX 4070 SUPER erscheint
  „RTX bereit | Raytracing noch aus“.
- Das Modul kann Kamera-, Sonnen- und Bildgrößendaten aus Firestorms
  Deferred-Renderpfad annehmen; kein in-world-Frame wurde bisher
  mit einer echten Anmeldung abgenommen.
- **Keine sichtbaren RTX-Schatten, Reflexionen oder GI**: Die erforderliche
  Mesh-Geometrie, GPU-Beschleunigungsstrukturen und Vulkan-Ausgabe fehlen.
- 16/16 Python/UI-Vertragstests, 5/5 native Modul- und API-Tests bestanden;
  OGL C++-Build und lokale Ressourcenbereitstellung ebenfalls erfolgreich.

## 31.10.2026 – RTX-Geometry-Brücke und echte Vulkan-Geräteinitialisierung

- Nutzer-Screenshot und OGL.log bestätigen erfolgreiches Betreten von Stadt
  NexVerse (STATE_STARTED), sichtbare Gebäude, Avatar, Himmel und Texturen.
  Die bisherigen sichtbaren Schatten sind **OpenGL, nicht RTX**.
- OGL entnimmt nun statische OpenSim Volume-Face-Dreiecke aus der geladenen
  Szene und reicht höchstens 1.024 Dreiecke alle 180 Frames per versioniertem
  RTX-Modul-ABI weiter. Rigged-Avatare/Attachments sind noch ausgeschlossen.
  Bei leeren oder wechselnden Szenen wird die alte Geometrie verworfen.
- Der Vulkan-Ray-Query-Gerätetest besteht auf der RTX 4070 SUPER:
  VkDevice erstellt, benötigte RT-Features angefordert, sauber freigegeben.
- Die Viewer-CPU-Geometriebrücke und native Modulvalidierung sind gebaut;
  die tatsächliche Dreiecksanzahl aus einem frischen In-World-Lauf steht
  noch aus. Diagnose: `python3 ogl/scripts/ogl-test-report.py`.
- Keine GPU-BLAS/TLAS, Ray-Query-Ausführung, RT-Schatten oder Reflexionen.
  `rendering_active=0` bleibt korrekt. Es sind keine Serveränderungen nötig.
- 16 Python-Vertragstests, 6 native/Vulkan-/Fallback-Tests bestanden.

## Einschränkungen

- HTTP **verschlüsselt den Logintransport nicht**. Anmeldedaten oder
  wiederverwendbare Zugangsinformationen können abgefangen und verändert
  werden. Nach Möglichkeit ausschließlich ein separates Testkonto nutzen.
- Die Freigabe ersetzt keine TLS-Konfiguration. Unter `ogl/server/`
  liegt weiterhin die optionale, validierte Caddy-HTTPS-Vorlage.
  Auf dem öffentlichen Grid-Server wurde **nichts** geändert.
- Der tatsächliche authentifizierte Login, die 3D-Welt, Avatar-Wearables,
  Texturen, Meshes, Himmel, Sonne und Teleport müssen noch manuell
  abgenommen werden.
- RTX-Modul ist hardwareseitig aktiv, aber noch kein Raytracing-Renderer.
- Das bestehende native `nexverse-viewer`-Projekt ist unverändert.

## Start und Diagnose

```bash
cd "/home/jam/Schreibtisch/OGL-Viewer/ogl-firestorm"
bash ogl/scripts/run-viewer-test.sh
python3 ogl/scripts/ogl-test-report.py
```

Desktop: `~/Schreibtisch/OGL-Viewer-Test.desktop`.

Vollständiger Build einschließlich Ressourcen:
```bash
bash ogl/scripts/build-test.sh
```

Log: `~/.local/share/ogl-firestorm-test/logs/OGL.log`.
Es dürfen keine realen Zugangsdaten in den Chat oder öffentliche
Fehlerberichte eingefügt werden.

## Nächste Abnahme

1. Viewer starten und "Stadt NexVerse" als festes Grid prüfen.
2. **Mit einem separaten Testkonto** bei der HTTP-Warnung
   "Über HTTP anmelden" wählen; alternativ "Abbrechen" testen.
3. Prüfen, ob der OpenSim-Login erfolgreich die 3D-Welt öffnet.
4. Danach reale Region, Avatar/Attachments, Texturen/Meshes,
   Himmel/Sonne, Kamera, Bewegung und Teleport vergleichen.
5. Raytracing-Effekte und echtes Cache-Profiling separat implementieren.

## 31.10.2026 – Vulkan-BLAS/TLAS-GPU-Smoke-Test

- Eigenständiger CUDA-unabhängiger Vulkan-GPU-Test erzeugt eine BLAS für ein
  Dreieck und eine TLAS mit einer Instanz, die auf die BLAS zeigt.
- Beide Aufbauten werden auf der RTX 4070 SUPER wirklich durch Vulkan-
  Kommandos abgearbeitet (Queue-Submit/Wait + gültige Device-Adressen).
- Feature-/Treiber-unabhängiger CI-Fallback: bei fehlendem RT-Support
  wird der Test übersprungen statt den OpenGL-Build zu blockieren.
- Keine Raytracing-Effekte im Viewer: Real-World-Geometrieanbindung,
  GPU-Ray-Query-Shader und Schatten-Compositing stehen noch aus.


## 31.10.2026 – 10er RTX-Block: echtes GPU-Ray-Query-Testergebnis

- Vulkan-Compute-Shader mit 8x8 Strahlen auf GPU kompiliert und ausgeführt.
  Ergebnis: 36 korrekt verdeckte, 28 sichtbare Ray-Samples.
- CPU-Geometrie: 1.024 Dreiecke im Belastungstest, Revisionsnummern,
  Bounds-Checks, copy-out und Invalidierungsprüfung.
- GLSL-/SPIR-V-Validierung und plattformübergreifende Paketintegration.
- Linux OGL-Viewer C++-Build PASS; SPIR-V und RT-Modul im lokalen
  Testpaket verifiziert. OpenGL bleibt unverändert aktiv.
- Tests: 24 Python-Vertragsprüfungen und 8 native C++/Vulkan-Tests.
- Trotz funktionierender Ray Queries ist die reale OpenSim-Welt noch
  nicht an den GPU-BLAS-/TLAS-Builder angeschlossen. Keine sichtbare
  Raytracing-Darstellung im Viewer. Siehe
  [`RTX_10_BLOCKS_2026_10_31.md`](RTX_10_BLOCKS_2026_10_31.md).


## 31.10.2026 – zweite 10er-Runde: Vulkan-Modul-Geometrie auf GPU

- Portable Vulkan-Testpipeline lädt ein OGL-Modul als SO/DLL und
  verarbeitet dessen validierte Dreiecks-Snapshots auf der GPU.
- Tests auf NVIDIA RTX 4070 SUPER: 64 und 1.024 Dreiecke;
  jeweils BLAS/TLAS erfolgreich, GPU-Ray-Query-8×8-Maske 36/28.
- TLAS-PrimitiveCount-Fehler bei größeren BLAS korrigiert.
- Diagnoselogs enthalten nur Dreiecksanzahl/Revision und beachten
  eine spätere Teleport-Invalidierung.
- Nachweis: 30 Python-Tests und 10 native Vulkan-/C++-Tests;
  normaler Linux-Viewer kompiliert, Shader und Modul ausgeliefert.
- **Noch kein sichtbares Raytracing im Viewer**: Live-Weltgeometrie
  ist nicht an einen persistenten GPU-Renderer angeschlossen.
  Siehe `RTX_10_BLOCKS_2026_10_31_P2.md`.


## 31.10.2026 – dritte 10er-Runde: RTX GPU-Live-Probe (opt-in)

- C-ABI-GPU-Bibliothek und Shader-Push-Constants mit echten Szenengrenzen
  und Sonnenrichtung integriert; weiterhin nicht der sichtbare Renderer.
- Die GPU-Probe wird bei `OGL_RTX_SCENE_PROBE=1` an CPU-Szenensnapshots
  gebunden und verarbeitet genau diese (statt erneut erzeugter Fixtures).
- Probe-Test auf RTX 4070 SUPER: 64 Szene-Dreiecke / 64 GPU-Ray-Queries,
  15 Treffer, ohne jemals OpenGL zu überschreiben.
- Fallbacktests für fehlende Vulkan-Features und fehlende Probe-DLL/SO.
  Default-OpenGL ist unverändert. Rate-Limit und Fail-Closed für Probe.
- [`RTX_10_BLOCKS_2026_10_31_P3.md`](RTX_10_BLOCKS_2026_10_31_P3.md).


## 31.10.2026 – vierte RTX-10er-Runde (Async GPU Probe)

- OpenGL-Render-Thread wird nicht mehr durch den BLAS/TLAS-/Ray-Query-
  Diagnoseaufruf blockiert; begrenzte Queue mit einem GPU-Worker.
- Mehrfache Szenensnapshots: latest-wins, stale results invalidiert.
- GPU-/Thread-Fehler: diagnostischen GPU-Pfad abschalten, OpenGL aktiv.
- Ausdrückliches Entwickler-Opt-in bleibt Pflicht.
- Kompletter Linux-Viewer C++-Build inklusive beider separaten SOs
  und SPIR-V-Ressource erfolgreich.
- Native GPU- und CPU-Regressionen, sowie No-Vulkan-Fallback getestet.
- Bericht: [`RTX_10_BLOCKS_2026_10_31_P4.md`](RTX_10_BLOCKS_2026_10_31_P4.md).


## 31.10.2026 – fünfte RTX-10er-Runde: persistentes Vulkan-Gerät

- Kontext und logisches Vulkan-Gerät bleiben während mehrerer Jobs
  desselben Workers erhalten; geänderte Meshes lösen BLAS/TLAS-Neubau aus.
- Unveränderte Geometrie und neue Sonnenrichtung: dieselbe BLAS/TLAS,
  neuer GPU-Ray-Query-Pass, keine weitere Geräteinitialisierung.
- Bei identischen Inputs: vollständiger Diagnose-Cache-Hit.
- Diagnose-ABI v2 enthält `vulkan_device_creations`, `blas_builds`,
  `cache_hits`. Das Viewer-Protokoll unterscheidet GPU-Wiederverwendung
  vom Textur-/Mesh-Cache.
- Neuer GPU-Lifecycle-Test bis 1.024 Dreiecke; kompletter Linux-Build
  inklusive beider RTX-SOs erfolgreich. OpenGL-Fallback unverändert.
- [`RTX_10_BLOCKS_2026_10_31_P5.md`](RTX_10_BLOCKS_2026_10_31_P5.md).


## 31.10.2026 – sechste RTX-10er-Runde: GPU-Maske bis Firestorm Host

- Versionierte GPU-Probe-ABI v3 mit 64 Pixelwerten (8×8, nur Debug).
- GPU-Pixelmaske wird durch die asynchrone Queue und das RTX-Modul
  bis zur Firestorm-Host-Abfrage validiert transportiert.
- Szene/Teleport/Sonne: vorhandene Maske invalidieren; keine
  stale Schattenproben oder fehlgeschlagene GPU-Weitergabe.
- Abnahmen: synthetische Dreiecksmaske, Cache-Bytevergleich,
  korruptes GPU-Pixel, falsche Trefferzahl, ungültige ABI-Version.
- Vollständiger Linux-Viewer C++-Build, beide RTX-SOs und Shader
  im Paket bestätigt; normales OpenGL-Rendering bleibt aktiv.
- [`RTX_10_BLOCKS_2026_10_31_P6.md`](RTX_10_BLOCKS_2026_10_31_P6.md).


## 31.10.2026 – siebte RTX-10er-Runde: Kamera + Sonnenschatten

- Zweistufige GPU-Ray-Query-Pipeline (Kamera-Oberfläche und
  anschließend Oberfläche-Sonnenstrahl) in separatem Shader.
- Hardwaretest: ohne Blocker 0/64, mit geometrischem Blocker
  2/64 verschattete Testbildpunkte; GPU-Szenencache überprüft.
- Entwickler-Modus in OGL-RTX-Modul und asynchrone Queue integriert.
  Die normale Vulkan-Testprobe bleibt unverändert verfügbar.
- SPIR-V als zweite RTX-Ressource paketiert. Vollständiger Linux-
  Viewer C++-Build mit beiden Shadern und beiden RTX-Bibliotheken grün.
- Kameraausrichtung nur angenähert, tatsächliche OpenSim-Region noch
  nicht mit GPU-Schatten und Renderbild abgenommen.
- [`RTX_10_BLOCKS_2026_10_31_P7.md`](RTX_10_BLOCKS_2026_10_31_P7.md).


## 31.10.2026 – achte RTX-Runde: reale Kamerabasis

- FOV, Aspect, Right- und Up-Achsen werden aus der laufenden Firestorm-
  Kamera abgerufen, nicht aus fester 60-Grad-Kamera oder world-up.
- Vulkan-Kamera-Rays und Cache-Schlüssel verwenden diese Daten.
- Asynchroner GPU-Auftrag nun von Mesh-Revisionsnummer getrennt;
  Kameraänderungen können ohne Meshänderung neu berechnet werden.
- Vollständiger Linux-Viewer-Build mit neuer Kamera-ABI v2 erfolgreich.
- Native Regressionen prüfen live zwei GPU-Aufträge bei einer
  unveränderten Mesh-Version, Roll, FOV, Seitenverhältnis und No-GPU.
- [`RTX_10_BLOCKS_2026_10_31_P8.md`](RTX_10_BLOCKS_2026_10_31_P8.md).


## 31.10.2026 – neunte RTX-Runde: OpenGL-Deferred-Depth-Bridge

- Versionierte 8×8-Depth-Diagnosestruktur und portable
  Validierung der tatsächlichen OpenGL-Tiefe.
- Firestorm-Host kann nur im expliziten Entwicklermodus
  (`OGL_RTX_DEPTH_PROBE=1`) 64 Depth-Floats der
  Bildschirmmitte erfassen und an das RTX-Modul weiterreichen.
- Render-/Packzustand bleibt wiederhergestellt; 60-Sekunden-
  Limit verhindert permanente synchrone Readback-Stalls.
- Framewechsel invalidiert alte Proben. Fallback für
  fehlendes GPU-Modul sowie für ungültige Daten besteht.
- Vollständiger Linux-Viewer-Build; 36 Python- und 19
  native Tests bestanden. OpenSim-Live-Tiefentest steht aus.
- [`RTX_10_BLOCKS_2026_10_31_P9.md`](RTX_10_BLOCKS_2026_10_31_P9.md).

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

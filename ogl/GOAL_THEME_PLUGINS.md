# /goal Viewer-Designs und Plugin-System

OGL Firestorm | Stand 01.11.2026
Alle Phasen: gleicher C++-Viewer fuer Linux, Windows und macOS;
CPU-/GPU-Faehigkeitserkennung mit Standardpfad fuer aeltere Hardware.
Keine automatische Ausfuehrung fremder Plugins.

## Phase 1: Integrierte Theme-Verwaltung – 20 Bloecke

1. Originale Firestorm-Skins-Einstellungen untersucht.
2. Originale Skin-Auswahl und Farbmodus erhalten.
3. Eigenen Reiter OGL Designs im vorhandenen Einstellungen-Fenster
   ergaenzt. Originale Skins-XML bleibt bytegenau unveraendert.
4. Installierte Theme-Liste in existierenden Skins-Tab aufgenommen.
5. Nutzerverzeichnisse von portablem Installer fuer Linux/Windows/macOS uebernommen.
6. Themenkennung auf sichere ASCII-Zeichen begrenzt.
7. Symlink-/Verzeichnis-Manipulationen der Installationen abgefangen.
8. Theme-JSON vor Nutzung auf maximal 16 KiB begrenzt.
9. Manifest-Typ, Schema, Name, Version und ID geprueft.
10. Exakte Plattform-Menge Linux/Windows/macOS geprueft.
11. Nur fest definierte Farbtokens zu realen Firestorm-Farben abgebildet.
12. Hex-Farben vor Vorschau/Aktivierung geprueft.
13. Echte farbige Hintergrund-, Text- und Chat-Vorschau im neuen Reiter.
    Separates Demo-Design Ocean Blue fuer sichtbaren Unterschied.
14. Vorschau als rein lesenden Vorgang ausgefuehrt, ohne Renderer-Mutation.
15. Aktivieren-Button mit Firestorms echter LLUIColorTable verbunden.
16. Bereits bestehende Nutzerfarben vor erster Aktivierung gesichert.
17. Original-Farben-Ruecksetzen ohne Pauschal-Loeschung aller Farben implementiert.
18. Theme-Status/Fehler mit DE/EN-XUI-Strings angezeigt;
    Originaldesign bleibt der Fallback.
19. GUI-, Farbtoken-, Pfad- und Ruecksetztests sowie nativen Viewer-Compile durchgefuehrt.
20. Cross-OS-Module und GitHub-Freigabe fuer die Phase validieren.

**Wichtig:** Diese Phase konsumiert bereits **installierte**
.ogltheme-Pakete aus dem bestehenden sicheren Python-Installer
und aktiviert deren zulassige Farbwerte im Firestorm.
Der Button "Open theme folder" ist ein Speicherortzugang,
KEIN nativer ZIP-Importdialog. Installation erfolgt vorerst
mit dem getesteten Kommandozeilenwerkzeug. Die bereits
gespeicherte Auswahl wird beim Oeffnen der Skins-Seite
angezeigt; fuer die visuelle Aktivierung ist der
Button "Activate" erforderlich. Neustart wird empfohlen.
Das originale Firestorm-Design ist Standard und jederzeit
ueber "Restore original" wiederherstellbar.

## Phase 2: Native Paketinstallation im Viewer

20 Bloecke: native .ogltheme ZIP-Sicherheitspruefung,
Dateiauswahl, kontrollierte Asset-Extraktion, atomare
Installation/Rollback, Paketlisten-/Loeschen-UI,
sprachuebergreifende XUI-Texte, Tests mit manipulierten ZIPs,
Windows-/macOS-/Linux-Builds. Ohne externe Interpreterpflicht.

## Phase 3: Theme-Engine und visuelle Live-Vorschau

20 Bloecke: Farb-Token-System, Design-Templates, live
umschaltbare Skins soweit technisch moeglich, bestehende
Einstellungen schuetzen, fuer Login, Chat und Menues,
Regressionen bei Login/Voice/PBR/OpenSim und mehreren DPI.

## Phase 4: Abgesicherte Funktionsplugins und Releases

20 Bloecke: manifestbasierte Capabilities, Signaturen,
Sandbox/Prozesstrennung, Benutzerfreigaben, API-Version,
Widerruf, sichere Updates, Packaging fuer Linux/Windows/macOS
sowie x64/ARM64. Kein blindes Laden fremder DLL/SO/DYLIB.

### Entwicklertest (Linux)

    cd "/home/jam/Schreibtisch/OGL-Viewer/ogl-firestorm"
    python3 -m unittest discover -s ogl/tests -p 'test_*.py' -q
    bash ogl/scripts/build-test.sh
    bash ogl/scripts/run-viewer-test.sh

Unter "Einstellungen > OGL Designs" vorhandene Themes
auswaehlen, Farb-Vorschau ansehen, Activate und Restore original
pruefen. Visuelle Laufzeitabnahme am Bildschirm steht aus.

### Beispiel: Ein Design installieren und im Viewer aktivieren

    cd "/home/jam/Schreibtisch/OGL-Viewer/ogl-firestorm"
    cd ogl/plugins/examples/ocean-blue
    python3 -m zipfile -c /tmp/ogl-ocean-blue.ogltheme theme.json
    cd ../../../..
    python3 ogl/plugins/tools/manage_themes.py install /tmp/ogl-ocean-blue.ogltheme

Danach Firestorm starten und unter Einstellungen > OGL Designs
"OGL Ocean Blue" waehlen. Vorschau kontrollieren; mit
"Design aktivieren" die Farbeinstellungen speichern.
Mit "Original wiederherstellen" die VORHERIGEN eigenen
Firestorm-Farbwerte wiederherstellen. Ein Viewer-Neustart
ist fuer durchgehend konsistente Farben sinnvoll.
Bei nicht existierenden installierten Designs bleibt
automatisch das Firestorm-Original gewaehlt.
Die XUI-Dateien des Original-Firestorm-Designs werden
NICHT ueberschrieben.

### Grenzen dieser Phase

- GUI-Kontrolle betrifft Farben, nicht komplette XML-Layouts.
- .ogltheme ZIPs werden per vorhandenem sicheren CLI-Installer
  installiert. Der native Datei-Dialog ist Phase 2.
- Bild-Assets eines Theme-Pakets werden noch nicht als
  dynamische Viewer-Skin-Assets registriert.
- Ausfuehrbare Erweiterungen (DLL/.so/.dylib) bleiben verboten.
- Linux-Nativbuild und plattformneutrale Paket-CI sind erforderlich;
  ein erfolgreicher Test ist noch keine visuelle Laufzeitabnahme.

# /goal Viewer-Designs und Plugin-System

OGL Firestorm | Stand 01.11.2026
Alle Phasen: gleicher C++-Viewer fuer Linux, Windows und macOS;
CPU-/GPU-Faehigkeitserkennung mit Standardpfad fuer aeltere Hardware.
Keine automatische Ausfuehrung fremder Plugins.

## Aktueller Stand 04.11.2026 – P03 17/20

**P03 Block 17 (Code 59bc92d3,
Linux-End-to-End-QA bestanden):**
Im echten OGL Viewer
7.2.5.210 wurde ein
gültiges ZIP (nur
theme.json) nach
Dateiauswahl und
Installationsklick
im isolierten
Userverzeichnis
installiert und
anschließend
mit dem Entfernen-
Button gelöscht.
Fehlerursache des
früheren Import-
Abbruchs war
linkname="" als
nicht-null Zeiger
in minizip-ng;
diese regulären
ZIP-Einträge sind
jetzt zugelassen.
Echte Symlinks,
Zusatzdateien,
Pfadtraversal und
doppelte JSON-Keys
wurden in vier
zusätzlichen nativen
GUI-Tests abgelehnt,
ohne Datenänderung.
Linux-Vollbuild,
312 Python-/41
native Tests bestanden.
Keine Signaturprüfung,
Plugin-/Bilddateien
oder Windows-/macOS-
Vollviewer-Abnahme.
P03 17/20, nächster
Block 18: Altlasten/
Skin-Overrides und
Duplikate bereinigen.

Block 16 (Code 1f174bc7):
OGL-Design-Default
über XUI auf Deutsch/
Englisch lokalisiert,
Textflächen responsiv,
rechte Buttons rechts
verankert und
Farbswatches aus
ausgegrauten Buttons
in echte, beschriftete
nicht-interaktive
Farb-Panels umgebaut.
Laufender GUI-Test
unter Linux EN
1920x1080/140%,
DE 1440x900/100%
visuell erfolgreich.
311 Python-/41 native
Tests, Linux-Vollbuild
7.2.5.207, zwei XUI-
Paketvergleiche
erfolgreich.
Block 17 folgt:
native ZIP-Import-
und Manipulations-
Abnahme. Keine
vollständige P03-
Freigabe behaupten.

Ab Code 73565590:
Der schon vorhandene
OGL-Designs-Reiter
enthält jetzt auch
native Schaltflächen
"Design installieren"
und "Entfernen".
Ein vom Benutzer
ausgewähltes lokales
unsigned .ogltheme-ZIP
kann ausdrücklich
nativ installiert
werden, sofern es
exakt eine überprüfte
theme.json mit erlaubten
Farbwerten enthält.
Keine Plugins, Skripte,
XUI-Overrides,
Bild-Assets oder
System-/Netzwerkrechte.
Minizip-ng-C++ prüft
CRC, Größen, Metadaten,
Duplikate, Archiveinträge,
Compression-/ZIP-Typ
und Farberlaubnis
vor dem Schreibzugriff.
Kanonisches JSON,
atomisches Staging/Rename
und kein Überschreiben.
Für inaktive intakte
Pakete sicheres nicht-
rekursives Entfernen.
Bestehende Vorschau,
Aktivierung, OGL Standard
und gespeicherte
Benutzerfarben bleiben.

Linux-Vollviewer
7.2.5.204 sowie
308 Python-/41 native
Tests bestanden.
DE-Preferences-
Theme-Panel und
nativer Dateidialog
real angezeigt; EN/DE
Login-GUI grün.
Eine vollständige
native Datei-Auswahl/
Import/Entfernen-
E2E-Abnahme fehlt,
ebenso Signaturen,
Bild-Assets und
Windows-/macOS-
Vollviewer-Gates.
Nächster Block P03/16:
HiDPI, Sprachen,
Chat-/Menü-Integration,
danach bösartige
ZIPs und E2E-Tests.
Der folgende Abschnitt
P03/1–8 ist historisch.

## Aktueller Stand 04.11.2026 – P03 Blöcke 1–8/20

Der folgende ältere Phase-1-Text
belegt historische Vorarbeit,
nicht den heutigen Standard.
OGL Standard hat inzwischen
eigene zentralisierte blau-
türkise Akzentfarben im
bestehenden colors.xml.
Das EN/DE-Theme-Panel zeigt
OGL Standard; gesicherte
individuelle Nutzerfarben
und das Classic-Beispiel
bleiben als Rücksetz-/Fallback
erhalten. Strikter C++/
Python-SemVer-Vertrag.
Native .ogltheme-Dateiauswahl
im bestehenden Preferences-
Panel ist hinzugefügt,
INSTALLIERT/EXTRAHIERT
jedoch noch kein Archiv.
Der Python-Theme-Installer
ist mit ZIP-Sicherheits-
prüfungen und gesonderten
Tests gehärtet. Der native
Validator und die atomische
Installation sind kommende
P03-Blöcke 9–10 sowie
weitere Sicherheitsaufgaben.
Linux-Vollviewer 7.2.5.202,
306 Python-/41 native Tests,
DE-/EN-GUI und CLI-Import
im isolierten /tmp bestanden.
Kein echter In-World-
Theme-/Windows-/macOS-
Vollviewer-Test behauptet.
Commit c4a615ba.

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

## Verbindliche neue Branding- und Dateivorgabe

Die nachfolgenden Phasen sollen die Oberfläche vollständig
auf OGL/NexVerse umstellen: **kein sichtbarer Firestorm-
Altstil, keine Firestorm-Marken im normalen Betrieb**.
Die vorhandene Phase-1-Farbwahl und die frühere
Original-Firestorm-Rücksetzung bleiben als historische
Zwischenstandsangabe dokumentiert. Das langfristige
Nutzer-Standarddesign ist **OGL/NexVerse**; ein interner
sicherer Skin-/OpenGL-Fallback und verpflichtende
Urheber-/Lizenzhinweise bleiben erhalten.

Das Login wird kompakter und moderner gestaltet; Anzeigen
"Vulkan RT bereit" und "Effekt aus" aus dem Login entfernen
und nur noch in freiwilligen Diagnosen zulassen.
Navigation, Chat, Login und übrige Fenster verwenden
gemeinsame Design- und Layoutregeln.

Strikt vorhandene Dateien unter denselben Pfaden
aktualisieren. **Keine neuen Projektdateien** oder
parallelen *_neu, *_v2, .bak, Kopien und Overlays ohne
ausdrückliche Zustimmung; keine Daten/Secrets blind
überschreiben. Verwaiste UI-Ressourcen/Altcode in jeder
20-Block-Runde identifizieren und nach Tests bereinigen.

Der Detailplan ist in der Hauptroadmap Phase H (20 Blöcke)
verbindlich dokumentiert.

## Phase 2: Native Paketinstallation im Viewer

20 Bloecke: native .ogltheme ZIP-Sicherheitspruefung,
Dateiauswahl, kontrollierte Asset-Extraktion, atomare
Installation/Rollback, Paketlisten-/Loeschen-UI,
sprachuebergreifende XUI-Texte, Tests mit manipulierten ZIPs,
Windows-/macOS-/Linux-Builds. Ohne externe Interpreterpflicht.

## Phase 3: Theme-Engine und visuelle Live-Vorschau

20 Bloecke: Farb-Token-System, Design-Templates, live
umschaltbare Skins soweit technisch moeglich, bestehende
Einstellungen schuetzen, fuer Login, Chat und Menues;
OGL/NexVerse als ausschliesslich sichtbares Produktdesign,
nicht das Firestorm-Original. Regressionen bei
Login/Voice/PBR/OpenSim und mehreren DPI.

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

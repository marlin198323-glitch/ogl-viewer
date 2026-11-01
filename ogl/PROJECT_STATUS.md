# OGL Firestorm – aktueller Projektstand

Stand: 01.11.2026
Repository: https://github.com/marlin198323-glitch/ogl-viewer
Branch: ogl-develop
Verbindlicher Auftrag: GOAL_DUAL_GRID_AND_VIEWER_ROADMAP_2026_11_01.md
Übernahme in neuen Chats: PROJECT_HANDOFF.md und AGENTS.md (Repo-Wurzel)

## Aktiver /goal-Ausführungsstand (maßgeblich)

**Nächster Block:** `/goal P01 Block 7/20` – Eingabefelder, Skalierung
und echte HiDPI-/Login-Regression. **Keine manuelle Abnahme erforderlich.**
Nach technischer Fertigstellung einer Phase sofort
Abschlussmeldung mit Commit/Tests und, soweit im laufenden
Arbeitsauftrag möglich, direkter Start der Folgephase.
Ohne laufenden Chat findet keine Hintergrundentwicklung statt.

| Phase | Status | Fortschritt | Nächster Schritt |
| --- | --- | --- | --- |
| P01 – Branding und modernes Login | aktiv | 6/20 | Block 7: Felder und DPI prüfen |
| P02 – OGL-Navigation, Fenster, Chat und Avatare | offen | 0/20 | nach Abschluss der Vorphase |
| P03 – Theme-System und sichere Designinstallation | offen | 0/20 | nach Abschluss der Vorphase |
| P04 – Renderer-Abstraktion und OpenGL-Fallback | offen | 0/20 | nach Abschluss der Vorphase |
| P05 – Vulkan als vollständige auswählbare 3D-Engine | offen | 0/20 | nach Abschluss der Vorphase |
| P06 – Vulkan-Raytracing und Grafikqualität | offen | 0/20 | nach Abschluss der Vorphase |
| P07 – CPU/GPU-Leistung, Textur-, Mesh- und Avatarcache | offen | 0/20 | nach Abschluss der Vorphase |
| P08 – Beta-Protokoll und getrennte NexVerse-Anmeldung | offen | 0/20 | nach Abschluss der Vorphase |
| P09 – Abgesicherter Plugin-Manager | offen | 0/20 | nach Abschluss der Vorphase |
| P10 – Automatisierte signierte GitHub-Releases | offen | 0/20 | nach Abschluss der Vorphase |
| P11 – Clientseitiger Pflicht-Updater und Update-UI | offen | 0/20 | nach Abschluss der Vorphase |
| P12 – Serverseitige Mindestversion für beide Grids | offen | 0/20 | nach Abschluss der Vorphase |
| P13 – Portal-Releases, End-to-End-QA und Dauerbereinigung | offen | 0/20 | nach Abschluss der Vorphase |

## P01 – technische Arbeitsrunde vom 01.11.2026

**Verifizierter Code-Commit:** `9c29aba6` auf `ogl-develop`.
**Sequenziell abgeschlossene Blöcke: 1–6 (6/20).** Das XUI-/C++-
Login und die bestehenden OGL-Branding-Elemente wurden inventarisiert,
die zwei festgelegten Grids mit den vorhandenen Sicherheitsverträgen
abgeglichen, sichtbare alte Login-Auswahl/RT-Debugtexte identifiziert,
eine gemeinsame Login-Farbpalette in der vorhandenen `colors.xml`
hinterlegt, die bestehende NexVerse-Welcome-URL beibehalten und
die untere Loginleiste von 152 auf 140 Einheiten verkleinert.

**Weitere bereits technisch bearbeitete Punkte (nicht als
sequenzieller 7–20-Fortschritt verbucht):** Login-RTX-Debugstatus samt
Abhängigkeit entfernt (11–13), alte Phoenix/Firestorm-Modusauswahl im
OGL-Login ausgeblendet, Sprachtexte Englisch/Deutsch aktualisiert (15),
Tastaturzugänglichkeit der beiden Speichern-Checkboxen verbessert (14),
Konto- und Passwort-Hinweislinks auf die verifizierte NexVerse-
Website statt Second Life/Firestorm geführt (10), altes
`showLoginWidgets()`-Layoutziel für OGL korrigiert. Es gibt **noch
keine dokumentierte direkte Passwort-zurücksetzen-URL**; die
aktuelle sichere Zwischenlösung öffnet lediglich die Portal-Startseite.

**Prüfungen lokal Linux Mint:** `bash ogl/scripts/build-test.sh`
erfolgreich, vollständiges Viewer-Binary kompiliert und
Release-Staging erzeugt, **120/120 Python-Tests** und
**41/41 native CTest-Tests** erfolgreich; `git diff --check` ohne
Fehler. Die Beta-Login-Sperre, feste Grid-Auswahl und
explizite HTTP-Einwilligung bleiben in den Sicherheitstests grün.
Keine neue Projektdatei angelegt. Duplikat-/Altbestand-Audit:
`git ls-files` auf *_neu, *_v2, *.bak und Kopie-Namen geprüft;
gefundenes upstream-notwendiges `client_list_v2.xml` sowie
Funktionsressourcen mit „copy“ nicht blind gelöscht.

**Noch offen:** Blöcke 7–20 inklusive tatsächlicher GUI-Prüfung auf
720p–4K/HiDPI, Speichern und Wiederherstellen der Zugangsdaten,
sichtbarer Gridwechsel, bestätigtes öffentliches Grid-Login,
vollständiger sichtbarer OGL-Rebranding-Audit, UI-/Sicherheitsregression
und relevante Windows-/macOS-CI. Ein lokaler Modul-/Linux-Build
ist kein Beleg für plattformübergreifende Installer. **P01 bleibt
aktiv**, kein Phasenabschluss und noch kein Übergang nach P02.

**Nächster Arbeitsschritt:** `/goal P01 Block 7/20` –
Login-Eingaben auf kleinen/großen Anzeigen und HiDPI sowie
Breiten-/Fokusregression technisch absichern. Danach 8–20
sequenziell abschließen und erst mit grünen Gates P02 starten.

**Bestehende bereits geprüfte Teilfunktionen:** Öffentliche und
Beta-Grid-Auswahl, OGL-Farbtheme-Panel und experimentelle
Vulkan-/CPU-Module sind zu übernehmen. Diese Bestände bedeuten
nicht, dass ganze operative Phasen P01–P13 erledigt wären.

**Abschluss-Gate je Phase:** 20/20 nachweisbar bearbeitet,
relevante Tests/Builds/CI grün, unverzichtbare externe
Sicherheits- oder Zugriffsgates eingehalten, Änderungen nach
`ogl-develop` gepusht, Statusdatei aktualisiert. Keine
Benutzerabnahme als automatische Pausensperre. Unerreichbare
Live-Services bleiben transparent blockiert, nicht „grün“.

## Historischer Fachphasenstand A–I (Referenz)

| Phase | Stand | Nächster überprüfbarer Schritt |
| --- | --- | --- |
| Phase A – zwei feste Grids | 19 von 20 Blöcken im Code/Build geprüft; visuelle Abnahme ausstehend | Beide Login-Grid-Einträge und öffentlichen OpenSim-Login im laufenden Viewer testen |
| Phase B – Beta-Protokoll | Geplant | TLS-Zertifikat und tatsächliches Login-Protokoll verifizieren; ohne Bestätigung keine Passwortübertragung |
| Phase C – Designs, Templates, Chat | Designsystem Phase 1 mit Farbwahl implementiert; native Installation und weitere Templates offen | Nutzerabnahme der OGL-Designs und später sichere GUI-Paketinstallation |
| Phase D – Plugin-Manager | Grundlage vorhanden, vollständiger sicherer Manager geplant | Versionierte Plugin-API, Rechte, Isolation und Signaturen konzipieren |
| Phase E – CPU/GPU, Cache, Grafik | Verschiedene Tests/experimentelle Module implementiert, Gesamtziel offen | Grafik-/Cache-Livebenchmark und vollständige Plattform-Builds prüfen |
| Phase F – verpflichtender Updater | Geplant, 0 von 20 Blöcken implementiert | Gemeinsame Client-/Server-Mindestversions-API und Releasekanäle festlegen |
| Phase G – OpenGL-/Vulkan-Grafik-Engine | Geplant, 0 von 20 Blöcken als vollständiger Renderer implementiert | Architektur für Renderer-Auswahl und OpenGL-Fallback prüfen |
| Phase H – OGL-Navigation, Login und vollständiges Rebranding | Geplant; frühere UI-Arbeiten vorhanden, aber Ziel noch nicht erreicht | Firestorm-UI-Reste erfassen; Loginanzeigen "Vulkan RT bereit" und "Effekt aus" entfernen; moderne Navigation planen |
| Phase I – GitHub-Releases, Portal und Bereinigung | Geplant; Repository besitzt Build-/CI-Grundlagen | Releases/Signierung/Portal-API prüfen und Quellbaum-Audit in jede Runde integrieren |

## Neue verbindliche Anforderungen (01.11.2026)

- **Grafikeinstellung:** Nutzer kann zwischen OpenGL (kompatibler
  Standard für ältere Hardware) und Vulkan (zusätzlicher vollwertiger
  Renderer) wählen. Vulkan-RT-Effekte sind optional und separat.
  **Stand:** Vulkan-Welt-Renderer und Auswahl noch nicht realisiert.
- **OGL-Produktdesign:** Alle sichtbaren Firestorm-Branding-/
  Navigation-/Login-/Dialogreste sollen durch ein eigenständiges
  modernes OGL/NexVerse-Design ersetzt werden. Insbesondere
  "Vulkan RT bereit" und "Effekt aus" im Login entfernen.
  Technische Debugdaten in Grafik-/Diagnosebereich verlagern.
  **Stand:** UI-Umbau noch nicht abgeschlossen.
- **Releasequelle:** GitHub Releases soll plattform-/architektur-
  passende geprüfte, signierte OGL-Versionen automatisch
  veröffentlichen. Das NexVerse-Portal bezieht aktuelle
  Release-Informationen mit offiziellem API-Zugriff, Cache,
  Integritätsprüfung und sicheren Downloadlinks.
  **Stand:** noch keine durchgängige Release-/Portal-Lösung.
- **Dateiregel und Bereinigung:** ausschließlich vorhandene
  Projektdateien in-place aktualisieren, keine neuen
  Projektdateien oder duplizierte Backups/Overlays ohne
  ausdrückliche Freigabe. Zu jedem 20er-Block verwaiste
  Inhalte, Dateiduplikate und alten Quellcode prüfen und
  nach Tests gezielt bereinigen. CI-Release-Dateien
  sind keine Quellbaum-Ersatzdateien.
- Urheber-/Lizenzhinweise erhalten, Nutzerdaten schützen.
- Verbindlicher Detailplan: Hauptroadmap, Phasen G/H/I
  mit jeweils 20 geplanten Entwicklungsblöcken.

## Fest bestätigte technische Grundlage

- Der Quellcode für die zwei festen NexVerse-Grids liegt auf ogl-develop.
  Stadt NexVerse Beta ist auswählbar; der eigentliche Beta-Login bleibt
  bis zur abgesicherten Zertifikats- und Protokollintegration gesperrt.
- Zum Grid-Umbau wurde auf Linux Mint ein vollständiger Viewer-Build
  einschließlich 119 Python-Tests und 41 nativen Tests erfolgreich
  gemeldet. Die sechs Varianten der Modul-/CPU-CI waren erfolgreich.
  Diese Angaben beziehen sich auf den damaligen Code-Teststand
  und sind keine pauschale Freigabe aller OS-Installer.
- Im OGL-Designsystem sind Theme-Auswahl, Farb-Vorschau,
  Farb-Aktivierung und Wiederherstellung eingebaut; das
  originale Firestorm-Design bleibt der Standard.
- Die Pflichtupdate-Policy ist ausschließlich als Anforderung
  und 20-Block-Plan dokumentiert. Es ist noch **kein**
  produktiv durchsetzender Viewer-/Server-Updater vorhanden.
- Ein früherer OpenGL/Vulkan-RTX-Entwicklungsstand enthält
  Diagnosen; das ist **keine** fertig auswählbare Vulkan-
  Grafik-Engine und kein Nachweis sichtbarer Weltschatten.
- Die frühere Aussage "originales Firestorm-Design als Standard"
  beschreibt den bestehenden Zwischenstand, **nicht** das neue
  Branding-Ziel: künftig rein sichtbare OGL-/NexVerse-Oberfläche.

## Betriebssysteme und Architektur

Verbindliche Ziele: Linux, Windows, macOS sowie x86-64
(AMD/Intel) und ARM64. Gemeinsame Funktionalität, automatische
CPU-/GPU-Erkennung und kompatibler OpenGL-Standardpfad.
Ein grüner Modul-Test ist **nicht** dasselbe wie ein gebauter,
signierter und manuell getesteter vollständiger Viewer.

## Verbindliche Pflicht-Updater-Anforderung

Sobald eine neue Mindestversion für Stadt NexVerse oder
NexVerse Beta vorgeschrieben ist, muss eine ältere Version
vor dem Server-Login aktualisiert werden. Ablehnen/Abbrechen
erlaubt keinen Zugang; der Server prüft die Mindestversion
selbst. Der genaue 20-Block-Plan steht in
GOAL_MANDATORY_VIEWER_UPDATER.md.

## Vorgehen für jede weitere 20-Block-Runde

1. AGENTS.md, PROJECT_HANDOFF.md, diese Statusdatei und
   die relevante /goal-Roadmap lesen.
2. Über Linux Mint Commander git status, aktuellen Branch
   und den letzten Commit prüfen; lokalen Quellstand schützen.
3. Einen genauen 20-Block-Umfang mit Abnahmekriterien wählen.
4. Implementieren und nach Möglichkeit den echten Viewer
   bauen, Python-/native Tests und nötige Plattform-CI prüfen.
5. Alle tatsächlichen Ergebnisse dokumentieren; manuelle
   Sicht-/Login-Tests ausdrücklich als offen kennzeichnen.
6. Fertige, geprüfte Änderungen sofort auf ogl-develop pushen.
7. **Diese Datei aktualisieren:** Status je Phase, erledigte
   Blöcke, Teststände, neuer Commit, offene Fehler, konkrete
   nächste Schritte. Kein willkürliches Markieren als fertig.

Wird ein neues Chatfenster eröffnet, muss mindestens die
Repository-URL plus Branch angegeben und der Assistent zum
Einlesen der obigen Projektdateien aufgefordert werden.
Projektdateien allein übertragen den Kontext nicht
automatisch in jede neue Chat-Unterhaltung.

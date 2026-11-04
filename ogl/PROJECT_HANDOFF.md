# OGL Firestorm Fork – Informationen für einen neuen Chat

Aktuelles Projekt: https://github.com/marlin198323-glitch/ogl-viewer
Branch: ogl-develop
Git-Remote: https://github.com/marlin198323-glitch/ogl-viewer.git

Linux-Mint-PC: jam-MS-7D28
Desktop-Commander-ID: 3a1e9ba9-ea8a-4650-b8f7-84f8f653f20d
Projektpfad: /home/jam/Schreibtisch/OGL-Viewer/ogl-firestorm

Teststart: bash ogl/scripts/run-viewer-test.sh
Build und Tests: bash ogl/scripts/build-test.sh

## Verbindliche Projektquellen

- **Hauptprojektplan mit sämtlichen Arbeitsvorgaben:**
  [GOAL_DUAL_GRID_AND_VIEWER_ROADMAP_2026_11_01.md](GOAL_DUAL_GRID_AND_VIEWER_ROADMAP_2026_11_01.md)
- **Aktueller Umsetzungsstand / nächste offene Schritte:**
  [PROJECT_STATUS.md](PROJECT_STATUS.md)
- **Pflichtupdate-Anforderung und 20-Blöcke-Plan:**
  [GOAL_MANDATORY_VIEWER_UPDATER.md](GOAL_MANDATORY_VIEWER_UPDATER.md)
- **Hinweise für Coding-Assistenten im Repository:** ../AGENTS.md

Lese diese Dateien zu Beginn jeder neuen Projektunterhaltung;
bei jedem Entwicklungsschritt die Statusdatei mit tatsächlichen
Test-/Abnahmeergebnissen aktualisieren. Das vorhandene GitHub-Repository
ist die verbindliche Projektquelle. Die Dateien werden in neuen
Chats nicht ohne Repositoryzugriff oder ausdrücklichen Hinweis
automatisch geladen.

## Automatischer /goal-Phasenlauf

**Maßgeblich sind P01–P13 im bestehenden Hauptprojektplan.**
Nicht mehr frei zwischen den alten Fachkapiteln A–I
wechseln. Neuer Einstieg: P02 Block 7/20.
P01 unter Linux 20/20 abgeschlossen nach ausdrücklicher
Nutzerbestätigung vom 02.11.2026:
Live-Login, 3D-Welt, Shutdown und Speicherung/
Wiederherstellung der Anmeldedaten funktionieren.
Der Assistent hat die Live-Anmeldung nicht mit
eigenen Zugangsdaten reproduziert; Linux-Vollbuild
7.2.5.180, 270 Python-/41 native Tests und
Prelogin-GUI-Smokes waren separat erfolgreich.
Windows/macOS noch nicht verfügbar für
Vollviewer-Abnahme, vor Release in P13 nachholen.
Beta-TLS-Zertifikatsproblem bleibt P08.
P02 Block 1/20: Menüs/Floater/Chat inventarisiert,
13 Menü-/11 Toolbar-Sprachlayouts und
258 EN-Floater als XML erfolgreich geparst.

Den letzten tatsächlich erledigten Block und Commit immer
in [PROJECT_STATUS.md](PROJECT_STATUS.md) fortschreiben.

Bei `/goal` die aktive Phase in 20 Blöcken durchziehen,
betroffene Tests und Builds ausführen, nach erfolgreicher
technischer Prüfung auf `ogl-develop` pushen.
Unmittelbar mit `/goal Pxx abgeschlossen | 20/20 |
Tests ... | GitHub ...` informieren und ohne Rückfrage
die Folgephase während der aktiven Unterhaltung bearbeiten,
sofern keine echten Blocker entgegenstehen.

**Kein manueller Abnahmestopp zwischen Phasen.** Fehlen
Zertifikate, API-Verträge, Signaturen, Serverrechte oder
Live-Testergebnisse, dies klar als Blocker führen und
soweit möglich unabhängige Arbeiten fortführen.
Kein Anspruch auf Arbeit im Hintergrund zwischen Chats.

Nur bestehende Projektdateien direkt überarbeiten,
ohne neue Dateien oder redundante Kopien; zu jedem
20er-Block veraltete Pfade und Code kontrollieren.

## Arbeitsregeln

- Nur ogl-viewer auf Branch ogl-develop bearbeiten, nicht das ältere
  Repo nexverse-viewer.
- Zuerst git status und letzten Commit prüfen.
- /goal phasenweise, jeweils 20 Entwicklungsblöcke.
- Ergebnisse testen und direkt auf ogl-develop veröffentlichen.
- Linux, Windows und macOS, AMD/Intel x86-64 und ARM64 bedenken.
- CPU-/GPU-Erkennung und OpenGL-Standardpfad für ältere Hardware.
- Sichtbares Branding/Navigation/Login vollständig OGL/NexVerse,
  kein sichtbarer Firestorm-Altstil; Copyright/Lizenzen erhalten.
  Die bereits installierte originale Firestorm-Skin-/Farbwahl
  ist bisheriger Implementierungsstand, nicht das neue UI-Ziel.
- OpenGL bleibt Standard für ältere Hardware; zusätzlich Vulkan
  als vollständige anwählbare Grafik-Engine entwickeln,
  separate GPU-abhängige RT-Effekte und sichere Rückfallfunktion.
- Login optisch modernisieren und "Vulkan RT bereit" / "Effekt aus"
  aus der Loginoberfläche entfernen.
- GitHub-Releases nach erfolgreichem CI-/Signaturprozess
  automatisch bereitstellen; Stadt-NexVerse-Portal liest
  passende stabile Releases von GitHub, mit Cache/Integritätsprüfung.
- Keine neuen Projektdateien ohne explizite Zustimmung.
  Vorhandene Dateien in-place ändern, keine *_neu/_v2/.bak/
  Kopie-Dateien. Bei jeder 20-Block-Phase Altbestand,
  verwaiste Ressourcen, Duplikate und toten Code prüfen
  und sicher bereinigen, Nutzerdaten/Lizenzen erhalten.
- Feste Grid-Auswahl: Stadt NexVerse und Stadt NexVerse Beta.
- Beta-Anmeldung bleibt bis zur bestätigten HTTPS-/Protokoll-
  Unterstützung gesperrt.
- Verbindlich geplant: ein Pflicht-Updater, der vor der
  Anmeldung prüft, ob die Server-Mindestversion erfüllt ist.
  Veraltete Versionen erhalten keine Serveranmeldung, wenn
  das neue Update noch nicht installiert wurde.

**Letzte unabhängige P02-Vorarbeit (02.11.2026):**
`912a7320` aktualisiert die Einstellung des eingebetteten
Browsers auf offizielle HTTPS-NexVerse-Hosts mit exakter
Host-/Portprüfung und Bewahrung der Nicht-OGL-Policy.
Zehn existierende Sprach-Overlays + zwei Regressionstests.
Linux-Vollbuild, 222 Python-/41 native Tests und isolierter
EN-Xvfb-GUI-Lauf mit Exit 0 bestanden.
**P01 bleibt 18/20, P02 formal 0/20; nächster Block P01 19/20.**
Details in `ogl/PROJECT_STATUS.md`.

**Letzte unabhängige P02-Vorarbeit (02.11.2026,
f1552bb1):** Alte externe Firestorm-/BugSplat-Crashuploads
in den drei Plattformstartpfaden und im separaten
Crash-Logger für OGL gesperrt; Crash-Upload-Tab verborgen.
Linux-Vollbuild und 227 Python-/41 native Tests erfolgreich;
DE-/EN-Xvfb-Login-Smokes sauber. Native vollständige
Windows-/macOS-GUI-/Build-Nachweise fehlen.
**Formaler Stand P01 18/20, P02 0/20;
nächster Block P01 19/20.**
Details in ogl/PROJECT_STATUS.md.

**Letzter Entwicklungsstand (02.11.2026):**
bf031d29: alte Firestorm-Wiki-Hilfelinks aus 13
Setup-Sprachlayouts entfernt, Chat-Command-Hilfe
in fünf Sprachen bereinigt, Second-Life-E-Mail-
Kontolink im OGL-Kanal gesperrt. Linux-Vollbuild,
230 Python-/41 native Tests, DE-/EN-Xvfb-Login
und aktualisierte XUI-Staging-Dateien geprüft.
**Formale P01 18/20, P02 0/20; nächster Block
P01 19/20**, keine echten In-World-/Windows-/
macOS-Vollviewer-Gates behaupten. Details im Status.

Letzte /goal-20er-Runde (02.11.2026):
ef3dc0e5: Altes Second-Life-DeprecatedHelp-Untermenü
aus EN und elf lokalisierten Menüdateien entfernt.
Der aktive NexVerse-Help- und Script-Library-Pfad
bleibt erhalten. Windows-cp1252-Regressionstest-
Fehler auf explizites UTF-8 korrigiert und unter
ASCII-Systemkodierung lokal bestätigt. Linux-Vollbuild,
232 Python-/41 native Tests, 80 Branding-Tests,
13/13 XUI-Menüs im Build und isolierte DE-/EN-Login-
Smokes bestanden. Formale Phase P01 18/20,
P02 0/20; nächster /goal P01 Block 19/20.
Kein echter Live-Login oder Windows-/macOS-Vollviewer-
Test behauptet. Details in PROJECT_STATUS.md.

Letzte unabhängige /goal-20er-Runde vom 02.11.2026:
6b6779fc: aktiver FS-Chat, Nearby-Toast und
Benachrichtigungsansichten verwenden das vorhandene
OGL-Systemicon statt des Second-Life-Logos.
Demo-Name und Demo-Zeit im Chatkopf entfernt.
Radar-/RLVa-Branches sowie Nicht-OGL-Fallback bleiben
erhalten. Linux-Vollbuild, 237 Python-/41 native Tests,
85 Branding-Tests auch unter ASCII, drei geprüfte
Staging-Assets und DE-/EN-Xvfb-Login-Smokes bestanden.
Kein Live-Grid-Chat oder kompletter Windows-/macOS-
Viewer-Abnahmelauf behauptet.
Formaler Stand P01 18/20, P02 0/20, nächster
operativer Block P01 19/20.
Details im Status.

**Letzte unabhängige /goal-20er-Runde (02.11.2026,
bde46c18):** Aktive Chat-Zeitstempel verschieben
bei langen History-Werten die Avatar-/Namensleiste
nicht mehr. Feste rechtsbündige Anzeige mit Ellipsen
und vollständigem Tooltip. DE-/JA-/PT-/TR-
Demo-Zeitwerte entfernt, toten C++-Layoutzeiger
bereinigt. 240 Python-/41 native Tests und
88 Branding-Tests auch unter ASCII, Linux-Vollbuild,
sieben Chat-XUI-Stagingdateien und isolierte
DE-/EN-GUI-Starts geprüft. Keine echten
In-World-/Windows-/macOS-Vollviewer-Gates
behaupten. Formal P01 18/20, P02 0/20;
nächster /goal P01 Block 19/20.
Detailnachweise in PROJECT_STATUS.md.

**Letzte unabhängige /goal-Runde vom 02.11.2026,
cfa671e4:** Im aktiven FSFloaterIM keine
Firestorm-Supportgruppen-Präfixe (OS/Version/
Skin/RLVa), Schalter, Popup oder Sysinfo-
Bestätigungswege mehr im OGL-Kanal.
Normale IM-Funktionen und Nicht-OGL-Kanäle
erhalten. 20 technische Prüfpunkte,
245 Python-/41 native Tests, 93 Branding-
Tests auch unter ASCII, vollständiger Linux-
Viewer-Build, DE-/EN-Xvfb-Login-Smokes
bestanden; GitHub-Code direkt gepusht.
Live-In-World- und Windows-/macOS-Vollviewer-
Gates bleiben offen. Formal P01 18/20,
P02 0/20, nächster operativer Block P01 19/20.
Mehr in PROJECT_STATUS.md.

Letzte unabhängige /goal-Runde (02.11.2026,
8deebedc): Negative Unread-Zähler in Nearby-,
Muted- und IM-Chat auf null begrenzt.
Vorhandener initial ausgeblendeter 20-Pixel-
Hinweis nicht unnötig überschrieben.
20 Prüfpunkte, Linux-Build 7.2.5.166,
250 Python-/41 native Tests, 98 Chat-/
Branding-Tests, DE-/EN-Xvfb-GUI-Smokes
bestanden. Beta-TLS-Hostnamenfehler
per curl bestätigt. P01 18/20, P02 0/20;
P01 Block 19/20 operativer Einstieg.
In-World-/Windows-/macOS-Vollviewer-Gates offen.

Letzte unabhängige /goal-20er-Runde (02.11.2026,
a462330e): Doppelte Verringerung der
Chat-Namensspaltenbreite im Draw-Pfad
entfernt; bestehende XUI trennt 72px
Zeitstempel vom Namen. Tote Layout-
Zustände und Aufrufe bereinigt,
Avatar-/Name-Cache, RLVa und Teleport-
Trennzeile erhalten. Linux-Vollviewer
7.2.5.168 gebaut, 253 Python-/41 native
Tests, 101 Branding-/Chatlayout-Tests
und isolierte DE-/EN-Xvfb-GUI-Smokes
bestanden. Kein Live-/In-World-/voller
Windows-/macOS-Viewer-Test behauptet.
Formal P01 18/20, P02 0/20;
nächster /goal-Einstieg P01 Block 19/20.
Mehr in PROJECT_STATUS.md.

Letzte unabhängige /goal-Runde (02.11.2026,
72c8beed): Neun bestehende
Sprach-strings.xml korrigiert, um bei
Spanisch, Russisch, Türkisch, Chinesisch
den alten Namen Firestorm/Second Life
nicht anzuzeigen; aktive Grid-/System-
Bezeichnung nutzt [CURRENT_GRID].
Drei Regressionstests, 20 Prüfpunkte,
vollständiger Linux-Viewer-Build,
256 Python-/41 native Tests,
104 Branding-Tests, 13 exakt
staged Locale-XMLs und vier
GUI-Login-Smokes (ES/RU/TR/ZH)
bestanden. Vollständige In-World-/
Windows-/macOS-Gates weiter offen,
Beta-TLS-Sperre unverändert.
Formal P01 18/20, P02 0/20.
Nächster Einstieg P01 Block 19/20.

Letzte unabhängige /goal-20er-Runde
(02.11.2026, 4ff7e7cb): P01-Login-Sicherheit,
verwaiste Startup-Credentials ohne
Dropdown-Auswahl können nicht mehr den
Passwortfülltext als echte Eingabe
verwenden; Passwortstatus/Augenknöpfe/
Loginbutton zurückgesetzt, Einwort-
OpenSim-Konto angezeigt. Gültige
gespeicherte Konten bleiben unberührt.
259 Python-/41 native Tests, 107 Branding-
und 49 Gridtests unter ASCII, Linux-
Vollbuild 7.2.5.172, DE-/EN-Xvfb-
Voranmelde-GUI erfolgreich.
Kein echter gespeicherter Login/Windows-/
macOS-Vollviewer-Test behaupten.
P01 formal 18/20, P02 0/20,
nächster Einstieg P01 Block 19/20.

**Letzte unabhängige /goal-Runde (02.11.2026,
8e9e773e):** Legacy-Grid-Hilfe-Callback
unter OGL auf offizielles NexVerse-Portal
begrenzt, alter Grid-Builder-Callback vor
Fremd-URL gesperrt. Gilt auch für alte UI-
Callbacks; zwei feste Grids bleiben erhalten.
20 technische Prüfpunkte, Linux-Vollviewer
7.2.5.174, 262 Python-/41 native Tests,
110 Branding-/49 Gridtests, 23 Login-
Sprachlayouts und isolierte DE-720p-
/EN-1080p-Starts bestanden. Keine
authentifizierte oder Windows-/macOS-
Vollviewer-Abnahme. Formal P01 18/20,
P02 0/20, nächster /goal P01 Block 19/20.

Letzte unabhängige /goal-20er-Runde
(02.11.2026, be18b791): OGL-Anmeldung
cached bei getFields nur ein wirklich
manuell eingegebenes Passwort als
temporäres Retry-Passwort. Gespeicherte
Authenticator-Platzhalter werden nicht
übernommen; bei Kontowechsel wird der
vorherige Klartext-Retry gelöscht.
Leerer Startup-Retry erzwingt manuelle
Neueingabe, tatsächlich manuell
eingegebene Passwörter können nach
fehlgeschlagener Anmeldung restauriert
werden. 20 technische Prüfpositionen,
Linux-Vollbuild 7.2.5.176, 265 Python-
/41 native Tests, 113 Branding-
/49 Gridtests, DE-720p-/EN-1080p-GUI
bestanden. Weder echte Credentials
noch Grid-Live-Login getestet.
Formal P01 18/20, P02 0/20.
Nächster operativer /goal P01 Block 19/20.

Letzte unabhängige /goal-20er-Runde
(02.11.2026, bfd5e2d9): OGL-
Benutzernamenwechsel löscht Passwort-/
Retry-Zustand auch bei leerem Feld und
synchronisiert die Augenknöpfe.
Drei neue/zwei angepasste Tests,
Linux-Vollbuild 7.2.5.178,
268 Python-/41 native Tests, 116
Branding-/49 Gridtests, DE-/EN-GUI
bestanden. Formal P01 18/20,
P02 0/20; Live-/Windows-/macOS-
Vollviewer-Abnahme weiterhin offen.

Letzte P01-19-Vorprüfung (02.11.2026,
263a224e): Live-Grid-GET HTTP 200/XML;
Beta-TLS-Hostnamefehler 60. Native
Grid-Credential-Prüfung sperrt mehrere
@-Trenner; zehn neue C++-Prüfvarianten
und zwei Python-Vertragstests. CI-Push-/
PR-Trigger für Grid-, Login- und
Startup-Code über sechs Plattformen.
Linux-Vollbuild 7.2.5.180,
270 Python-/41 CTests, 116 Branding-
/51 Gridtests und DE-/EN-Xvfb-GUI
grün. 20 technische Prüfpunkte, aber
keine echte Live-/In-World- oder volle
Windows-/macOS-Abnahme. Formal P01
18/20, P02 0/20; P01 19 weiter offen.

Windows-Installer-Vorbereitung 02.11.2026:
Der aktive sechsfache OGL-Grafik-Workflow enthält
einen bewusst per Push-Committext [windows-installer]
auslösbaren separaten Windows-2022-x64-Vollviewer-
Build (OpenSim, unsigniert, NSIS) mit geplanten
GitHub-Artefakten. NSIS-Pfaderkennung und
OGL-Installer-/Programmkennung wurden in
bestehenden Python-Dateien angepasst.
Kein .exe als erfolgreich nachgewiesen melden,
bevor GitHub Actions das vollständige Artefakt
wirklich produziert hat. Windows-Installation
und GUI-Test auf echter Hardware bleiben offen;
P01 Linux 20/20, P02 1/20 unverändert.

**P02 Block 2/20 abgeschlossen (02.11.2026,
Code 9c10de4e):** Kanonische sechs
Hauptnavigationen für 13 XUI-Locale-Dateien:
Avatar / Chat-Kontakte / Welt / Bauen /
Entdecken / Hilfe. Untergeordnete
RLVa-/Advanced-/Developer-/Admin-Menüs
erhalten; spanische acht zusätzliche
V1-Hauptmenüs entfernt, fehlende
lokale Content-Labels Dänisch Opdag und
Türkisch Keşfet ergänzt.
Drei neue Regressionstests,
Linux-Vollbuild 7.2.5.186,
276/276 Python- und 41/41 native Tests,
ES-/DA-/TR-GUI-Starts sowie
3/3 paketierte XUI-Dateien bestätigt.
P01 Linux 20/20, P02 jetzt 2/20;
nächster Block 3/20, Bestandshauptmenüs
und Toolbars ohne Duplikate konsolidieren.
Windows-/macOS-Vollviewer und Beta-TLS
bleiben in den entsprechenden Phasen
offen. Ältere Statuszahlen sind historisch.

**P02 Block 3/20 abgeschlossen
(02.11.2026, 4a2f34cc):**
Elf Sprachlayouts mit doppelten
Avatar-/Develop-Admin-Status-
Befehlen bereinigt, nur Develop
behält die originalen Admin-Callbacks
und Strg+Alt+G. Dreizehn Menüs geprüft;
ein gemeinsames EN-Toolbar-Basislayout
plus zehn Sprach-Overlays ohne
parallele Toolbar-Instanzen.
279 Python-/41 native Tests,
125 Branding-/Navigationstests,
Linux-Vollbuild 7.2.5.188,
13/13 bytegleiche Menü-Stagingdateien,
DE-/EN-/ES-GUI-Smokes erfolgreich.
Aktiv P02 Block 4/20; P01 Linux
weiterhin 20/20. Windows-x64-
Installer nicht als erfolgreich
melden, bis GitHub ein .exe-Artefakt
ausweist.

P02 Block 4/20 abgeschlossen
(04.11.2026, fc097c18):
Spanisches Entwicklermenü um ein
doppeltes World- und ein leeres
Logging-Untermenü bereinigt;
Simulator-Diagnose und fünf Logging-
Stufen erhalten. Leere DE/PT/ZH-
Toolbar-Platzhalter gelöscht,
lokalisierte Buttons unverändert.
Vollviewer 7.2.5.190, 282 Python-
/41 native Tests, 128 Brandingtests,
13 Menü-/11 Toolbar-XMLs,
vier XUI-Staging-Vergleiche und
vier ES/DE/PT/ZH-GUI-Smokes grün.
Formal P02 4/20, nächster Block 5.
Windows-Installer-CI hatte NSIS-
Paketerzeugung im Log, aber kein
verifiziertes Download-Artefakt;
alter Firestorm-OGL-Kanal noch offen.

**P02 Block 5/20 abgeschlossen
(04.11.2026, 8c274734):**
Der alte Firestorm-Supporthinweis
im Infofenster wird nur für OGL
durch aktuelle GitHub-Viewerinfos
ersetzt; OGL_System_Icon zeigt
im bestehenden "Über"-Kopf ein
OGL-Symbol, die originale
Kopieren-Schaltfläche hat
lokalisierten OGL-Text. Zwei
neue String-Namen je bereits
bestehender XUI-Sprache (13)
und drei Regressionstests.
Linden-/Firestorm-Credits,
Lizenznachweise und andere
Channels bleiben erhalten.
Linux-Vollbuild 7.2.5.192,
285 Python-/41 native Tests,
131 Brandingtests, 14/14
geänderte XUI-Dateien bytegleich
im Viewer-Paket, DE-/EN-Login-
GUI-Smokes bestanden. Kein
In-World-About-Klick getestet.
P02 5/20, nächster Block 6/20:
bestehende Avatar-/Chat-Kopfbereiche
vereinheitlichen. Windows-Setup
weiter ohne Download-Artefakt.

**P02 Block 6/20 abgeschlossen (04.11.2026,
Code 8960ecec):**
Japanisches Header-Overlay ohne
Test-Namen, spanischer Header-
Label wieder im_header.
Aktiver FSChatHistoryHeader folgt
für Zeitstempel der show_time-
Einstellung auch im erweiterten
Modus, bei Teleport-Trennern
verborgen. Verspätete Avatar-
Cache-Namen eines anderen
Absenders werden verworfen.
RLVa-Privatsphäre, runde Icons
und echte Namen bleiben erhalten.
Drei Regressionstests ergänzt.
Linux-Vollbuild 7.2.5.194,
288 Python-/41 native Tests,
134 Brandingtests, 7 Chat-Header-
XMLs und 2 JA/ES-Stagingdateien
validiert; JA-/ES-/DE-Login-GUI
jeweils Exit 0. Kein Live-Chat-
Test mit Nutzerkonto.
P02 jetzt 6/20; als Nächstes
Block 7: IM-/Gruppenfenster-
Kopfzeilen/Status/Navigation.

## Aktuelle Roadmaps

- ogl/GOAL_DUAL_GRID_AND_VIEWER_ROADMAP_2026_11_01.md
- ogl/PROJECT_STATUS.md
- ogl/GOAL_MANDATORY_VIEWER_UPDATER.md
- ogl/GOAL_THEME_PLUGINS.md
- ogl/PLUGIN_SYSTEM_CROSS_PLATFORM_ARCHITECTURE.md
- ogl/NEXVERSE_GRID.md

## Kompakter Starttext für einen neuen Chat

Bitte setze mein OGL Firestorm Fork Projekt fort.
GitHub: https://github.com/marlin198323-glitch/ogl-viewer
Branch: ogl-develop
Nutze den Linux-Mint-Commander jam-MS-7D28 und
/home/jam/Schreibtisch/OGL-Viewer/ogl-firestorm.
Lies zuerst AGENTS.md, ogl/PROJECT_HANDOFF.md,
ogl/PROJECT_STATUS.md und die /goal-Roadmaps.
Beginne mit der im Projektstatus markierten operativen
/goal-Phase P01–P13 und arbeite alle 20 Blöcke ab.
Nach erfolgreichem technischem Gate Abschluss melden und
ohne manuelle Abnahme zur nächsten Phase wechseln.
Bestehende Dateien am bisherigen Pfad ändern;
keine neuen Projektdateien oder Duplikate ohne Freigabe.
Zu jeder Runde Altbestand prüfen und sicher bereinigen.
Linux, Windows und macOS sowie AMD/Intel/ARM64 beachten.
OGL/NexVerse als vollständig eigenes sichtbares UI mit
moderner Navigation/Loginmaske; keine Firestorm-Altoptik.
Aus dem Login "Vulkan RT bereit" und "Effekt aus" entfernen.
OpenGL als Fallback und zusätzliche frei auswählbare
Vulkan-Grafik-Engine inklusive Hardwareprüfung entwickeln.
GitHub-Releases mit signierten Installern automatisch
bereitstellen und vom NexVerse-Portal beziehen.
Beide feste NexVerse-Grids erhalten. Ein verpflichtender
Viewer-Updater muss vom Viewer und Server unterstützt werden;
alte, nicht mehr zugelassene Versionen dürfen sich nicht
anmelden. Änderungen testen, direkt auf GitHub pushen,
den dokumentierten Projektstatus aktualisieren und
die nächsten offenen Schritte nennen.

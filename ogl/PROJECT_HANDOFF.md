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

P05 Vorarbeit 04.11.2026: Opt-in OGL_VULKAN_WORLD_BOOTSTRAP=1 im bestehenden Vulkan-Modul erzeugt ein langlebiges VkDevice und eine reale Grafik-VkQueue, unabhängig von RTX. Verifiziert: Linux-Vollbuild, 321 Python-Tests, 41 CTests und zwei echte GL-Prelogin-GUI-Starts mit an/aus. Kein Vulkan-Surface/Swapchain/World-Present; P05 gemäß Gate weiterhin 0/20. Nächster Fokus echter SDL-Vulkan-Fensterpfad und sichere GL-Rückfallebene. Keine parallelen Projektdateien erstellt.

Emoji-Reparatur 04.11.2026:
Die Emoji-Funktion war nie aus Nearby Chat oder IM entfernt.
Im Linux-Dev-Checkout liegen Emoji-Daten nicht im
Skin-Quellverzeichnis, aber im Viewer-Paket.
Die bestehende llemojidictionary.cpp nutzt nun bei
fehlgeschlagener normaler Skin-Suche einen
schreibgeschützten, sprachabhängigen Paket-Fallback.
322 Python-Tests, Linux-Vollbuild und isolierte DE/EN
GUI-Starts inklusive geladener Emoji-Zeichendatei bestanden.
Laufende User-Instanz nicht beendet; nach Update neu starten.
P05-Zähler unverändert.

**P05/1 abgeschlossen, 04.11.2026:** Eine echte VkSurfaceKHR
und VkSwapchainKHR wird nun ausschließlich bei zweifacher
Entwicklerfreigabe auf der tatsächlich vorhandenen
SDL/X11-Viewer-Window-ID aufgebaut. Presentqueue, Surface-
Caps/Formate/FIFO/Bildanzahl werden validiert; die
Vulkan-Handles noch vor dem nächsten GL-Frame wieder
freigegeben. Kein Vulkan-Frame-Acquire/Present, keine
Vulkan-Weltengine. Voll-Linux-Build und
323 Python-/41 native Tests grün. Xvfb-GUI-WSI-Erfolg,
normaler GL-Start und ungültiger Vulkan-Treiber
mit GL-Fallback alle Exit 0. **P05 1/20, neuer
Einstieg Block 2**: dauerhafter Vulkan-Fenster-/
Frame-/Swapchain-/Resize-Lebenszyklus;
Wayland/Windows/macOS WSI noch offen.

## Automatischer /goal-Phasenlauf

**Maßgeblich sind P01–P13 im bestehenden Hauptprojektplan.**
Nicht mehr frei zwischen den alten Fachkapiteln A–I
wechseln. Neuer Einstieg: P05 Block 2/20.
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

**P02 Block 7/20 abgeschlossen (04.11.2026,
Code b04c547c):**
IM-Well-Zeile zeigt nicht mehr
"TestString PleaseIgnore", sondern
sofort realen Sitzungsnamen für
P2P-, Gruppen- und Ad-hoc-Chats.
P2P-Cacheantworten nur bei
passender Teilnehmer-ID und
mit kontrollierter Verbindung,
die beim Schließen der Zeile
getrennt wird. Tooltip mit
echtem vollständigem Namen.
Einzelchat-Profilavatar rund
und quadratisch (104x104),
Gruppen-Sprecherliste unverändert.
Drei Regressionstests;
Linux-Vollbuild 7.2.5.196,
291 Python-/41 native Tests,
137 Brandingtests, 2/2 XUI
im Paket bytegleich; DE-/EN-
Prelogin-GUI-Smokes bestanden.
Kein echter In-World-IM-/Gruppen-
Chat-Test. Formal P02 7/20;
nächster Block 8: Fensterwechsel
und aktive Sitzungsanzeigen.
Windows-Installer weiter nicht
downloadbar bestätigt.

**P02 Block 8/20 abgeschlossen
(04.11.2026, 8116671a):**
IM-Sammelfenster erkennt nur
sichtbar gelesene Sitzungen
an, nicht pauschal alle Chats.
tabOpen beendet Tab-Blinken
und entfernt nur die aktuelle
Sitzungsmarkierung; versteckte
und minimierte Fenster löschen
keinen Hintergrund-Status.
Session-ID-Migration erhält
Status ohne Duplikate.
Close-Callback nutzt Floater-
Identität statt alter UUID;
Voice-Status bei Tab-Entfernung
neu initialisiert, Toolbar-
Zugriff gegen Null abgesichert.
Vier Regressionstests.
Linux-Vollbuild 7.2.5.198,
295 Python-/41 native Tests,
141 Brandingtests, DE-/EN-
Prelogin-GUI bestanden.
Keine echten In-World-
IM-/Gruppen-/Voice-Wechsel
getestet. Formal P02 8/20,
nächster Block 9: Fokus,
Nachrichteneingabe und
Unread-Counter beim Wechsel.
Windows-Installer weiter nicht
als Download verfügbar.

**P02/9–20 technisch abgeschlossen
(04.11.2026, 68e50e2b):**
IM-Fokus/aktive Sitzung beim
Tabwechsel abgesichert,
asynchrone P2P-Namensantworten
über Absender-ID geprüft,
sichtbare Demotexte aus IM-Toast
entfernt, runder Avatar.
Restliche Gruppenchat-, responsive
Feld-/Zeitstempel-, Kontakte-,
RLVa-, Voice- und DE/EN-Verträge
an bestehenden realen UI-
Funktionspfaden geprüft.
Linux-Vollbuild 7.2.5.200,
299 Python-/41 native Tests,
145 Branding-/UI-Tests,
zwei DE-/EN-GUI-Prelogin-Smokes,
gepacktes IM-Toast-XUI identisch.
P02 20/20 Linux-Code/Tests,
kein echter In-World-Chat-/Voice-
oder Windows/macOS-Viewer-Test.
P03 jetzt 0/20 eröffnet:
bestehende Themes und CLI zuerst
inventarisieren, dann native
Auswahl-/Sicherheitsarbeit.

/goal-Runde 04.11.2026:
P02/9–20 (Code 68e50e2b,
Status 7ac04220) abgeschlossen;
Linux-Vollbuild 7.2.5.200,
299 Python-/41 native Tests
sowie DE-/EN-GUI-Smokes.
P02 insgesamt 20/20 als
Linux-Code-/Regression; reale
In-World- und Windows/macOS-
GUI-Gates bleiben offen.

P03/1–8 (Code c4a615ba)
umgesetzt: vorhandenes Theme-
Panel/CLI inventarisiert,
OGL-Standard-Akzentfarben in
colors.xml zentralisiert,
originale User-Farb-Sicherung
und Classic-Fallback erhalten,
strictes C++/Python-SemVer,
EN/DE-XUI ohne Duplikate,
native .ogltheme-Dateiauswahl
ohne Installation und
ZIP-Validator zusätzlich
gegen nicht reguläre Einträge,
doppelte JSON-Keys, unzulässige
Kompression, Pfadausbruch,
Größen-/Case-Kollisionen
gehärtet. Viewer 7.2.5.202,
306 Python-/41 native Tests,
3 XUI-Paketvergleiche,
DE-/EN-GUI sowie isolierter
Theme-CLI-Lebenszyklus bestanden.
P03 nun 8/20. Als Nächstes
P03/9 Paketvertrauen/Rechte,
P03/10 native atomische
Installation. Keine echte
In-World-Theme-Abnahme.

**P03 Blöcke 9–15/20
(04.11.2026, 73565590):**
Nativer, explizit geklickter
Importer nur für lokale,
unsignierte Farbdesigns
mit exakt theme.json,
ohne Skripte/Assets.
Vorprüfung per vorhandener
minizip-ng-Bibliothek,
Manifest-/SemVer-/Farb-
Whitelists, ZIP-Typ,
Größe, CRC, Duplikate,
Symlink-/Pfad-Abwehr;
erst danach atomisches
Staging/Rename in
Nutzerverzeichnis.
Kein Überschreiben alter
Designs. Neue EN/DE-
Schaltflächen Installieren
und Entfernen; aktive/
veränderte Themes vor
Löschung geschützt.
Bestehende Liste,
Vorschau, Aktivieren,
Nutzerfarben-Reset und
Restart-Hinweise erhalten.
Linux-Vollbuild 7.2.5.204,
308 Python-/41 CTests,
DE/EN Prelogin GUI;
DE echte Preferences-
Ansicht inkl. Design-
Tab und Dateidialog.
**Kein erfolgreich
automatisierter nativer
ZIP-Import-End-to-End-
Test**, keine Signaturen,
keine Bild-Assets,
kein Windows/macOS
Full-Viewer-Test.
P03 jetzt 15/20,
weiter Block 16:
HiDPI/Sprache/Kompatibilität
und darauf native ZIP-QA.

**P03 Block 16 (04.11.2026,
Code 1f174bc7):**
OGL-Standardname in
Theme-Auswahl nach EN/DE
lokalisiert; Textfelder
bei Breitenänderung
mit left|right verankert,
rechte Buttons
right|top. Ausgegraute,
deaktivierte
Farbvorschau-Buttons
durch echte nicht
anklickbare Panels
mit setBackgroundColor
und getrennten
übersetzten Labels
ersetzt. Chat-/Menü-
Token-Verträge für
bestehendes Skin
regressiongeprüft.
Linux-Vollviewer
7.2.5.207, 311 Python/
41 native Tests,
EN/DE-XUI im Paket
identisch. EN bei
140% UI-Skalierung
1920x1080 und DE bei
100% 1440x900 real
als OGL-Preferences
geöffnet, beide Exit 0.
Keine authentifizierte
Chat-/Voice-Abnahme.
P03 16/20,
nächster Block 17:
nativer ZIP-Import-E2E
inklusive Malware-/Fehler-
Pfaden. Windows/macOS-
Gates weiter offen.

**P03 Block 17 (04.11.2026,
Code 59bc92d3, abgeschlossen):**
Frühere GTK-Dialog-Probleme
wurden durch explizites
Dateisystem-/Ort-Feld
in der isolierten
Xvfb-GUI aufgelöst.
Echter C++-Importfehler:
minizip-ng meldet
bei gewöhnlichen
ZIP-Einträgen eine
leere, nicht-null
linkname-Zeichenkette.
Die Prüfung wurde
korrigiert, echter
Symlink-/Unix-Modus-
Schutz beibehalten.
Linux-Build 7.2.5.210,
312 Python-/41 native
Tests bestanden.
Fünf native GUI-E2Es:
gültiges Ocean-Blue-
Theme selektiert,
installiert, Manifest-
ID und Nichtaktivierung
geprüft, über Viewer-
Button entfernt;
Zusatzdatei, ZIP-Symlink,
../-Pfadausbruch und
doppelter JSON-Schlüssel
jeweils bei tatsächlich
ausgewähltem Archiv
vom Viewer abgewiesen,
ohne Installation
oder Stagingreste.
Alle Szenarien Exit 0.
P03 jetzt 17/20;
weiter P03 Block 18:
Skin-Overrides/Theme-
Duplikate prüfen und
Altlasten sicher entfernen.
Windows-/macOS-Gates
und Signatur-Vertrauen
bleiben offen.

**P03 Block 18 (04.11.2026,
Commit 854e8a2d):**
Skin-/Sprach- und
Theme-Duplikate geprüft.
Einziges OGL-Theme-
Hauptlayout in EN,
einziges DE-Override,
keine OGL-Duplikate
in anderen sieben
Skins. Zwei
unterschiedliche
Theme-Beispiele.
303 Bytegleichheits-
Gruppen aus Upstream-
Sprachen/Skins dürfen
nicht ohne Nutzungs-
Nachweis gelöscht
werden. Test gegen
parallele OGL-Theme-
Layouts/IDs im
bestehenden Python-
Test ergänzt:
313/313 bestanden,
EN/DE-Paketlayout
byteidentisch.
P03 18/20; als
Nächstes Block 19
Linux-Vollbuild/CI.

**P03 Blöcke 19–20
(04.11.2026,
Linux-Code-/QA-
Abschluss 20/20):**
Vollviewer Linux
7.2.5.214 gebaut.
313 Python-/41
native Tests grün,
12 CLI-Theme-
Security- und
19 Viewer-Theme-
Tests enthalten.
DE/EN OGL-Designs
GUI mit 100/140 %
Skalierung gestartet,
beide sauber
geschlossen.
GitHub native
portable graphics
run 41891918909:
Linux/Windows/
macOS je x64/ARM
sechs Jobs erfolgreich.
Unsigned Windows-
Vollviewer-Installer-
Job skipped, keine
Windows/macOS
Full-Viewer-/Grid-
Abnahme. Unterstützter
Theme-Import nur
lokal unsigned
Farbmanifeste;
keine nativen
PNG/WebP-Assets,
Signaturen oder
ausführbaren
Plugins. Hierfür
keine Release-
Freigabe behaupten.
**Aktiv P04/1:
OpenGL-/Vulkan-
Rendererbestand
inventarisieren.**

**P04 Block 1
(04.11.2026, c2aade42):**
Renderer-Bestandsinventur:
OpenGL-Welt via
llviewerdisplay.cpp,
LLPipeline/pipeline.cpp,
LLShader und LLGL.
oglrtxhost.cpp lädt
optional Vulkan-RT-
Hardware-/Shadow-Probe,
ogl_gpu_probe als
zweites Diagnostikmodul.
Die opt-in Schatten-
Compositor-Versuche
arbeiten IN der
OpenGL-Pipeline,
sind keine vollständige
Vulkan-Weltengine.
Mac-Modulhost Stub,
keine Engine-Auswahl.
Regression in
test_public_graphics.py
ergänzt, 314 Python-
Tests bestanden.
P04 1/20, Block 2
Backend-Schnittstelle
und Fallback-Grenzen.
**P04 Block 2/20 (04.11.2026):** C++20-Backend-Vertrag in vorhandener `oglrtxhost.h`, OpenGL bleibt aktive Weltengine. Vulkan benötigt vollständig implementierte Welt-/Swapchain-Pipeline UND nutzbares Gerät; RTX-Diagnose allein ist keine Engine. Kein Umschalten oder Nutzer-Setting. Linux-Binary 7.2.5.218 seriell gelinkt, 315 Python-/41 native Tests und isolierter constexpr-C++-Test erfolgreich. Nächster Block P04/3: vorhandene Frame-Schnittstellen anbinden.

P04 Code ec52fd45 (04.11.2026): Blöcke 3–17 als Linux-Code/
Verifikationsstand: Weltframe an OpenGL-Vertrag gebunden,
Viewerstart/-shutdown-Log, plattformneutrale Hardware- und
VRAM-/Treiber-Information, sicherer OpenGL-Default,
EN/DE-Grafik-Reiter mit deaktiviertem Vulkan (noch keine
komplette Vulkan-Weltpipeline), getrenntes Requested/LastWorking,
letzter Erfolg nur nach wirklichem Swap, Rückfall eines
manuellen Vulkan-Wunsches erfolgreich im realen Linux-Viewer
getestet. Keine Änderungen an Original-Grafiksettings.
Viewer 7.2.5.219 gebaut, 319 Python-/41 native Tests,
7 C++-Szenarien, GUI/DE mit sichtbarem Renderer-Reiter.
P04/18 portable GitHub-Matrix ausstehend; 19 Cleanup-Audit,
20 Status/GitHub/Übergang P05. Windows/macOS-Vollviewer
und Vulkan-Weltrenderer sind NICHT bestätigt.

P04 Abschluss 04.11.2026: Code ec52fd45 (Blöcke 3-17)
und bereits vorhandene P04/1-2. Linux GL-Weltpfad
mit fail-closed OpenGL-Fallback, klar deaktivierter
Vulkan-Weltoption, EN/DE-Grafik-Reiter, getrennte
Rendererwunsch-/Erfolgseinstellung und tatsächliches
GL-swap-Erfolgssignal. Linux Vollbuild 7.2.5.219,
319 Python/41 CTests, 7 C++-Hardware-Szenarien,
echte DE-GUI und manueller Vulkan-Request-Fallback.
P04/18 GitHub portable graphics 41901011126
sechs Jobs Linux/Windows/macOS x64+ARM SUCCESS;
Windows-Full-Installer SKIPPED. P04/19 Diagnosehooks
haben reale Aufrufer, nicht löschen. P04/20 abgeschlossen,
Status synchronisiert; P05/1 beginnt. Vulkan-World-
Renderer, echte Swapchain und Windows/macOS-Vollviewer
NICHT fertig.

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

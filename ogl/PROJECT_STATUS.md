# OGL Firestorm – aktueller Projektstand

Stand: 01.11.2026
Repository: https://github.com/marlin198323-glitch/ogl-viewer
Branch: ogl-develop
Verbindlicher Auftrag: GOAL_DUAL_GRID_AND_VIEWER_ROADMAP_2026_11_01.md
Übernahme in neuen Chats: PROJECT_HANDOFF.md und AGENTS.md (Repo-Wurzel)

## Aktiver /goal-Ausführungsstand (maßgeblich)

**Aktiver Block:** `/goal P01 Block 19/20` (Teilprüfungen bestanden;
vollständiger Live-Login und plattformübergreifender Viewer-Nachweis
noch nicht verfügbar). **Keine manuelle Zwischenabnahme erforderlich.**
Nach technischer Fertigstellung einer Phase sofort
Abschlussmeldung mit Commit/Tests und, soweit im laufenden
Arbeitsauftrag möglich, direkter Start der Folgephase.
Ohne laufenden Chat findet keine Hintergrundentwicklung statt.

| Phase | Status | Fortschritt | Nächster Schritt |
| --- | --- | --- | --- |
| P01 – Branding und modernes Login | aktiv | 18/20 | Block 19: Live-Login und Plattformregression |
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

## CI-Nachprüfung – sechs portable Grafikplattformen grün

Nach dem Code- und Dokumentations-Push
`a8951b29`/`61b076e8` zeigte die
portable Grafikmatrix auf Windows
x64 und ARM64 einen Python-
`UnicodeDecodeError` im neuen
Login-XUI-Regressionsvertrag.
Die Ursache war ein `Path.read_text()`
ohne explizite Zeichenkodierung unter
Windows-CP1252; Linux-/macOS-Tests
waren bereits erfolgreich.
In der **bestehenden**
`ogl/tests/test_branding_contract.py`
ist nun `encoding="utf-8"` explizit
gesetzt (Fix-Commit `0fb7b3d0`).
Kein Viewer-Code und keine Login-
Funktion mussten dafür geändert werden.

**Bestätigter Nachweis:** GitHub Actions
`ogl-portable-graphics.yml`,
Lauf `41338813171`,
`0fb7b3d0`: **6/6 Jobs erfolgreich**
(Ubuntu x64/ARM64,
Windows x64/ARM64,
macOS Intel/ARM64).
Alle 198 Python-Verträge bestanden;
Linux-Vollviewer- und 41 native Tests
blieben unverändert grün.
**Wichtig:** Die portable
Grafikmodulmatrix ersetzt weiterhin
keine vollständigen Windows-/macOS-
Viewer-Builds, signierten Releases
oder In-World-UI-Abnahmen.
Die formalen Stände bleiben
**P01 18/20, P02 0/20**.

## P01 Block 19 – Alte Modusbestätigungen und Login-URLs abgesichert (20 Blöcke)

**Code-Commit:** `a8951b29` auf `ogl-develop`.
**20/20 überprüfte Entwicklungs-/QA-Positionen bestanden.**
Die formalen Phasenstände bleiben **P01 18/20**
und **P02 0/20**, weil Live-Login und
vollständige Windows-/macOS-Viewer-Tests
weiterhin nicht verifiziert sind.

**Implementierung ausschließlich in vorhandenen Dateien:**

- `indra/newview/fspanellogin.cpp`:
  Moduswechsel-Bestätigungen erhalten
  eine monotone, nur im laufenden
  Prozess gültige
  `OGLModeConfirmationSequence`. Die
  Callback prüft vor jeder UI-/Settings-
  Änderung, dass das OGL-Loginpanel
  noch existiert, die aktuelle
  Modusauswahl dem angefragten
  Preset entspricht, die bisherige
  `SessionSettingsFile` nicht
  zwischenzeitlich verändert wurde
  und der Dialog die neueste Generation
  besitzt. Die Freigabe wird vor
  Verarbeitung verbraucht. Überholte,
  wiederholte oder nach erneutem
  Moduswechsel eingetroffene
  Bestätigungen können weder einen
  Neustart noch einen Toolbar-Reset
  oder eine falsche Presetauswahl
  auslösen. Nicht-OGL-Aufrufkonvention
  und bestehende Presets bleiben
  unverändert.
- In sieben bereits vorhandenen
  `panel_fs_login.xml`-Sprachdateien
  die letzten direkten alten
  Phoenix-/Second-Life-Ziele für
  Welcome-/Registrierungs-/
  Passwort-Hilfe ersetzt: verifizierte
  NexVerse-Startseite
  `https://stadt-nexverse.de/` und
  `https://stadt-nexverse.de/service/welcome`.
  Beide lieferten bei Prüfung gültiges
  HTTPS und HTTP 200. Die kanonische
  moderne OGL-Loginmaske blieb
  unverändert; keine neue
  alternative Login-UI angelegt.
- Drei Tests für Modusbestätigungen
  in `ogl/tests/test_nexverse_grid.py`,
  ein Test für die 13 bestehenden
  Login-XUI-Lokalisierungen in
  `ogl/tests/test_branding_contract.py`
  ergänzt. Originale Einrückungen,
  interne Presetnamen und Lizenzen
  unverändert. Alte getrackte
  `_v2`-/`script_copy`-Dateien
  auf Existenz geprüft, nicht ohne
  Abhängigkeitsnachweis gelöscht.

**20 bestandene QA-Blöcke:**
01 Branch, 02 Upstream-Sync,
03 keine neuen Projektdateien,
04 zwei feste OGL-Grids,
05 monotone Dialoggeneration,
06 generationsgebundener Payload,
07 Ablehnung veralteter Bestätigung,
08 Null-Panel-Schutz,
09 Auswahl/Preset-Bindung,
10 Vergleich mit ursprünglicher
SessionSettingsFile, 11 Einmalverbrauch,
12 Abbruch ohne Änderung,
13 bestätigter Toolbar-Reset
nur gemäß Benutzerpräferenz,
14 Nicht-OGL-Kompatibilität,
15 13 valide alte Login-Overlays,
16 ausschließlich NexVerse-URLs,
17 keine alten Phoenix-/SL-Ziele,
18 sieben Dateien bytegleich
im Viewer-Staging,
19 Linux-Vollbuild + Regressionstests,
20 Hauptgrid-/Beta-GUI-Negativtests.
**20/20 bestanden.**

**Nachweise:** `bash ogl/scripts/build-test.sh`
erfolgreich; vollständiger Linux-Mint-
Viewer mit **198/198 Python-Tests**
und **41/41 nativen CTests**.
Zwei isolierte 1280×720-Xvfb-GUI-Läufe:
Hauptgrid-HTTP-Warnung mit ausschließlich
künstlichen Eingaben, sowie Beta-
Anmeldesperre. `STATE_LOGIN_WAIT`,
kein `STATE_LOGIN_AUTH_INIT`,
beide sauber mit Strg+Q und
**Exit-Code 0**. Die konkrete
Interaktion mit mehrfachen
Modusdialogen wurde durch
Quelltext-Regressionstests, nicht durch
einen visuellen End-to-End-Klicktest,
geprüft. Kein authentifizierter
Live-Login erfolgt.

**Nächster operativer Einstieg:
P01 Block 19/20.** Für den formalen
Abschluss weiterhin offen:
autorisierter Hauptgrid-Login,
echte Passwortpersistenz,
In-World-Shutdown sowie vollständige
Windows-/macOS-Viewer-Builds und
GUI-Laufzeitabnahmen.
Keine manuelle Zwischenabnahme
zwischen erfolgreichen Phasen nötig;
unabhängige P02-Arbeiten zulässig.

## P01 Block 19 – Einmalige HTTP-Warnungen und sichere Moduswechsel

**Code-Commit:** `f2630c0e` auf `ogl-develop`.
**20/20 unabhängige Entwicklungs-/Prüfpositionen bestanden.**
Diese sind **nicht** 20 zusätzlich abgeschlossene
Phasenblöcke: **P01 bleibt formal 18/20**,
**P02 0/20**.

**In bestehenden Dateien implementiert:**

- `indra/newview/fspanellogin.cpp`:
  Neben der Formularrevision erzwingt
  `sOGLHTTPWarningSequence` nun eine
  monoton fortgeschriebene Dialoggeneration.
  Ein erneuter Klick auf „Anmelden“
  verdrängt ältere offene HTTP-
  Einwilligungsdialoge, selbst wenn Konto,
  Grid und Passwortformular unverändert
  sind. Nur der neueste Dialog darf eine
  Freigabe erteilen. Bei Zustimmung wird
  die Dialoggeneration **vor** dem
  persistenten Speichern bzw. der
  einmaligen HTTP-Freigabe verbraucht;
  ein wiederholter Callback kann
  die Freigabe nicht erneut verwenden.
  Weiterhin geprüft werden exakte URI-Pins,
  Konto, Grid, Formrevision und explizite
  Zustimmung. Keine Passwörter werden
  in Dialog-Callbacks erfasst.
- Der bestehende Moduswechsel hatte
  `ResetToolbarSettings=true` schon
  **beim Öffnen** der Bestätigung gesetzt.
  Der Reset liegt nun ausschließlich in
  `onModeChangeConfirm`, `case 0`,
  nach tatsächlich bestätigtem Wechsel
  und nur bei aktivierter Einstellung
  `FSToolbarsResetOnModeChange`. Abbruch
  stellt den alten Modus wieder her,
  ohne die Werkzeugleisten zurückzusetzen.
  Die vorhandenen Preset-Dateien und
  Einstellungen bleiben erhalten.
- Drei weitere Regressionstests in
  `ogl/tests/test_nexverse_grid.py`,
  dazu zwei vorhandene Tests auf
  neue Callback-Capture-Signatur angepasst.
  **Keine neuen Projektdateien**;
  alte `_v2`-/`script_copy`-Ressourcen
  nur auf Abhängigkeiten inventarisiert,
  nicht blind gelöscht.

**20 bestandene Prüfschritte:** 01 Arbeitsbranch,
02 Remote-Synchronität, 03 keine neuen
Dateien; 04 getrennte Dialoggeneration;
05 erneute Warnung supersediert ältere;
06 Callback-Capture ohne Passwörter;
07 Generationsvergleich; 08 Formprüfung;
09 Kontoprüfung; 10 Gridprüfung;
11 exakte Login-URI-Pins;
12 Ablehnung vor Auth-Freigabe;
13 Dialoggeneration wird vor Nutzung
verbraucht; 14 nur explizites „Merken“
persistiert Zustimmung; 15 Abbruch
erteilt keine HTTP-Freigabe;
16 Moduswechsel ohne Vorab-Reset;
17 bestätigter Wechsel setzt Toolbar
nur bei aktivierter Option zurück;
18 abgebrochener Wechsel stellt
Modus ohne Reset wieder her;
19 Linux-Vollbuild und Python-/native Tests;
20 isolierte Hauptgrid-/Beta-GUI-Negativtests.
**20/20 grün.**

**Nachweise:** `bash ogl/scripts/build-test.sh`
vollständig erfolgreich, **194/194 Python-
Tests und 41/41 native CTests**. Zwei
echte isolierte 1280×720-Xvfb-
Voranmeldeläufe mit **synthetischen**
Zugangsdaten: HTTP-Warnablauf angesteuert,
Beta-Anmeldung serverseitig im Viewer
geblockt, beide ohne
`STATE_LOGIN_AUTH_INIT`, beide mit
Strg+Q und **Exit-Code 0**.
Ein interaktiver Mehrfachwarnungs-
Replaytest und ein visueller
Moduswechsel-Abbruchtest in der
angemeldeten Welt stehen noch aus;
Quelltext-/Regressionsnachweise sind
keine vollständige End-to-End-Abnahme.
Unveränderte OpenGL-Fallback-Pfade.

**P01 bleibt blockiert durch** autorisierten
Hauptgrid-Live-Login, echte
Passwortwiederherstellung, In-World-
Shutdown und vollständige Windows-/
macOS-Viewer-Build-/GUI-Nachweise.
Die portable Grafikmodul-CI ersetzt
diese nicht. **Nächster Einstieg:
P01 Block 19/20.** Keine manuelle
Zwischenabnahme erforderlich.

## P01 Block 19 – OGL-Datenschutz und konsistente Einstellungen (20 Prüfblöcke)

**Code-Commit `02af8ad4` (`ogl-develop`): 20/20 technische
Entwicklungs-/Prüfpositionen bestanden.** Die formalen
P01-Phasenblöcke bleiben **18/20**; P02 bleibt **0/20**.
Die eigentlichen Abnahmeblocker sind noch nicht erfüllt.

**In bestehenden Dateien umgesetzt:**

- `indra/newview/llstartup.cpp`: Ein bereits
  gespeichertes `AutoQueryGridStatus=true`
  kann im OGL Viewer nach dem Login **keinen**
  zusätzlichen Abruf an das alte
  Phoenix-/Second-Life-Statusfeed auslösen.
  Der HTTP-Call steht nun hinter
  `APP_NAME != "OGL"` und bleibt für
  andere Upstream-Kanäle unverändert.
  Wichtig: Dies ist eine direkt im
  Quelltext abgesicherte Sperre; kein
  tatsächlicher autorisierter Live-Login.
- Vorhandene `panel_preferences_advanced.xml`
  und `panel_preferences_alerts.xml`
  in allen 13 vorhandenen Sprachverzeichnissen:
  die Altoption für beliebige weitere
  Login-Grids bzw. das externe Statusfeed
  ist ausgeblendet und deaktiviert.
  Genau zwei feste Grids bleiben weiter
  im OGL-Login angezeigt.
- Sechs Gruppen von bestehenden
  Einstellungs-XUI-Dateien:
  UI, Chat, Sound, Benachrichtigungen,
  Advanced und Grafik. Sichtbare Titel,
  Labels und Tooltips mit „Firestorm“
  auf OGL umgestellt, ohne Konfigurations-
  Namen, Event-Callbacks, technische
  Lizenzhinweise oder Einstellungen
  umzubenennen. Englische und spanische
  Modus-Overlays zeigen „OGL“ und
  „Classic“, die bisherigen
  `settings_firestorm.xml` und
  `settings_phoenix.xml` bleiben
  kompatibel.
- Fünf neue Regressionsprüfungen in
  vorhandenen Python-Testdateien;
  keine neuen Projektdateien. Die
  inventarisierten Altdateien mit
  `_v2`/`script_copy` wurden mangels
  geprüfter Nutzungsabhängigkeiten
  nicht gelöscht.

**20 bestandene Prüfschritte:** 01 Branch/
GitHub synchron; 02 keine neuen Dateien;
03 zwei feste Grids; 04 Sperre des
Legacy-Statusabrufs im OGL-Pfad;
05 Nicht-OGL-Abwärtskompatibilität;
06 Fremdgrid-Einstellung in 13 Sprachen
versteckt; 07 Legacy-News-Schalter in
13 Sprachen deaktiviert; 08 UI-Texte;
09 Chat-Texte; 10 Sound-Texte;
11 Benachrichtigungstexte;
12 Advanced-Texte; 13 Grafikeinstellungen;
14 englischer OGL-/Classic-Modus;
15 spanischer OGL-/Classic-Modus;
16 interne Presetdateien unverändert;
17 alle aktualisierten XUI-Dateien
bytegleich im Buildpaket;
18 vollständiger Linux-Viewer-Build
und 191 Python-/41 native Tests;
19 GUI-Lauf Deutsch; 20 GUI-Lauf
Spanisch. **20/20 bestanden.**

**Verifikation:** `bash ogl/scripts/build-test.sh`
erfolgreich, vollständig gestagter
Linux-Mint-Viewer mit **191/191 Python-
Tests und 41/41 nativen CTests**.
Echte isolierte 1280×720-Xvfb-
Login-Starts auf Deutsch und Spanisch,
kein `STATE_LOGIN_AUTH_INIT`,
regulärer Strg+Q-Exit-Code **0**.
Die Sperre des Legacy-Netzwerkabrufs
ist durch Quelltext-/Regressionsprüfungen
nachgewiesen; es wurde kein
authentifizierter In-World-Datenverkehr
simuliert. Das OpenGL-Fallback wurde
nicht verändert. Portable
GitHub-Grafikmodule sind keine
vollständigen Windows-/macOS-Viewer.

**Noch offen für formalen P01-Abschluss:**
autorisierter Hauptgrid-Live-Login,
Passwortwiederherstellung nach Login,
In-World-Shutdown und vollständige
Windows-/macOS-Viewer-Build- und
GUI-Nachweise. Nächster
`/goal`-Einstieg bleibt **P01 Block 19/20**;
unabhängige P02-Vorarbeit möglich,
keine manuelle Zwischenabnahme.

## P01 Block 19 und unabhängige P02-Vorarbeit – Kontolöschschutz/Branding

**Code-Commit `6e7c5e8c` (`ogl-develop`): 20/20 überprüfte
technische Einzelpositionen bestanden.** Diese Prüfungen
sind **nicht** 20 formale Phasenblöcke. P01 bleibt
**18/20**, P02 formal **0/20**.

**In vorhandenen Dateien implementiert:**
- `fspanellogin.cpp`: Bestätigung zum Löschen
  gespeicherter Anmeldungen an die aktuelle
  Kontoauswahl, Grid-Zuordnung, Revision
  der Loginmaske und einmalige Dialoggeneration
  gebunden. Eine veraltete, mehrfach geöffnete
  oder nach Grid-/Konto-/Formwechsel
  bestätigte Anfrage kann den geschützten
  Credential-Speicher **nicht mehr** erreichen.
  Nach erfolgreicher Kontolöschung wird
  die Formularrevision erhöht und eine
  einmalige HTTP-Zustimmung ungültig.
  Fehlende Secure-Store-Einträge bleiben
  weiterhin durch Nullprüfung abgesichert.
  Nicht-OGL-Zweige bleiben unangetastet.
- In 13 vorhandenen `floater_preferences.xml`-
  Dateien den sichtbaren „Firestorm“-Tab,
  soweit dort explizit definiert, auf
  „OGL Features“ (Deutsch „OGL Funktionen“)
  umbenannt. Das interne Panel
  `panel_preference_firestorm` und seine
  Einstellungen und Callbacks bleiben erhalten.
  Das bestehende EN-`panel_preferences_firestorm.xml`
  nennt sich sichtbar „OGL Preferences“
  und enthält keine irreführende
  Firestorm-Zeichnungsdistanzbeschreibung.
- Elf bestehende
  `floater_fs_search.xml`-Fenstertitel
  verwenden „NexVerse“ statt „Firestorm“.
  Die internen Registrierungsnamen,
  Suche-/Gruppen-/Personentabs und
  erforderlichen Lizenztexte sind
  unverändert geblieben.
- Vier zusätzliche Regressionstests in
  den bestehenden Dateien
  `test_nexverse_grid.py` und
  `test_branding_contract.py`, dazu
  aktualisierte Formularrevisions-Erwartung.
  **Keine neuen Projektdateien.**

**20 nachweisbare Prüfpositionen:**
01 Git-Branch/Remote; 02 Dateiregel;
03 nur gespeichertes Konto; 04 Grid-Prüfung;
05 Formrevision bei Löschanforderung;
06 eindeutige Dialoggeneration;
07 ausgewähltes Grid gebunden;
08 unveränderte Kontoauswahl;
09 unverändertes aktuelles Grid;
10 exakte Dialog-Gridbindung;
11 Formularrevision verglichen;
12 Dialoggeneration verglichen;
13 Ablehnung vor Credential-Lesezugriff;
14 Freigabe vor Nutzung verbraucht;
15 Nullprüfung vor MFA-/Credential-Löschung;
16 HTTP-Freigabe nach Kontolöschung invalidiert;
17 OGL-Einstellungstitel aller betroffenen
13 Sprachen; 18 interne Einstellungen/Callbacks
und Panelnamen weiter vorhanden;
19 elf NexVerse-Suchfenstertitel;
20 XUI-Staging, Linux-Vollbuild und DE-/EN-
GUI-Smokes. **20/20 bestanden.**

**Testnachweis:** `bash ogl/scripts/build-test.sh`
erfolgreich; vollständiger Linux-Mint-Viewer
gebaut, **186/186 Python-Tests und 41/41
native CTests**. Alle 22 geänderten
XUI-Dateien bytegleich im Build-Staging.
Isolierte 1280×720-Xvfb-Starts in
Deutsch/Englisch: Anmeldemaske bereit,
`STATE_LOGIN_WAIT`, kein `STATE_LOGIN_AUTH_INIT`,
Strg+Q mit **Exit-Code 0**. Englischer
Negativtest zeigt HTTP-Risikodialog
mit ausschließlich synthetischen Eingaben.
**Nicht** Ende-zu-Ende getestet:
tatsächliche Kontolöschung mit realem
persistiertem Secure-Store-Konto,
autorisiertem Login und UI-Klick in
einer angemeldeten 3D-Welt.

**P01 weiterhin blockiert durch:**
autorisierter Hauptgrid-Live-Login,
Wiederherstellung real gespeicherter
Passwörter nach Anmeldung,
In-World-Shutdown, vollständige
Windows-/macOS-Viewer-Build- und
Laufzeitnachweise. Die CI-Grafikmodulmatrix
ist dafür kein Ersatz. Operativer
Wiedereinstieg bleibt **P01 Block 19/20**,
weitere unabhängige P02-Vorarbeiten erlaubt,
keine manuelle Zwischenabnahme.

## P02 unabhängige Vorarbeit – kompakte Toolbar und Chatbar (20 Prüfblöcke)

**Code-Commit:** `192bd10d` (`ogl-develop`).
**20/20 einzeln verifizierte Entwicklungs-/Prüfpositionen bestanden.**
P01 bleibt **18/20 formale Phasenblöcke**, P02 bleibt
**0/20 formale Phasenblöcke** bis zur In-World-Abnahme.

**Implementiert – ausschließlich bestehende Quelldateien:**
- Untere OGL-Chatbar in bestehender
  `skins/default/xui/en/panel_toolbar_view.xml`:
  klarer „Local Chat“-Text, korrekte Strg+H-
  Hilfe, 104-Pixel-Button; die vorhandene
  `Floater.Toggle/fs_nearby_chat`-Funktion,
  der `fs_nearby_chat_voice_monitor`, die
  Rechtschreibprüfung und die Toolbar-Position
  bleiben unangetastet. Deutsch auf „Lokaler
  Chat“ mit Strg+H-Tooltip angepasst.
- Veraltete `nearby_voice_monitor`- und
  `open_chat_bar_button_panel`-Elemente
  in den vorhandenen DE-/PT-/ZH-XUI-Overlays
  entfernt. Sie besitzen keinen entsprechenden
  Eintrag mehr in der aktuellen
  EN-Basis und dürfen nicht als tote bzw.
  doppelte UI-Kontrollen nachgeladen werden.
- Werkzeugleisten-Konfigurationsfenster
  `floater_toybox.xml` auf 820×520 statt
  975×575 Pixel gesetzt, größenveränderbar
  mit Mindestmaß 610×440 und gespeicherter
  Fensterposition; innere Toolbar-/
  Footer-Anker passend verschoben. Die
  vorhandenen „Leeren“, „Standards
  wiederherstellen“ und LockToolbars-
  Funktionen samt ihren Callbacks bewahrt.
  OGL-Fenstertitel EN/DE angepasst.
- Horizontale und vertikale
  Werkzeugleisten-Kontextmenüs in EN/DE
  auf „Customize toolbar...“ bzw.
  „Werkzeugleiste anpassen...“ umbenannt,
  ursprüngliche Alignment-/Layout-/
  Sperr-Callbacks nicht verändert. Vier
  neue Regressionstests der bestehenden
  `ogl/tests/test_branding_contract.py`.
  **Keine neuen Projektdateien angelegt.**

**20 geprüfte Blöcke:** 01 Branch/Remote-Sync,
02 Dateiregel; 03 11 XUI-Dateien valide;
04 EN-Chatbar; 05 DE-Chatbar;
06 Nahchat-Callback; 07 Voice und Spellcheck;
08 DE keine toten Widgets;
09 PT keine toten Widgets;
10 ZH keine toten Widgets;
11 kompakter Toybox-Dialog;
12 Größenanpassung/Mindestgrößen;
13 Drag&Drop-Toolbar weiter verfügbar;
14 Reset-/Leeren-Aktionen und Sperre;
15 deutsche Beschriftung/Lock-Preference;
16 DE/EN Kontextmenüs;
17 Orientierung und Layoutfunktionen;
18 alle 11 XUI-Dateien bytegleich gestaged;
19 Linux-Vollviewer-Build, Python-/native Tests;
20 deutsche und englische GUI-Smoketests.
**20/20 bestanden.**

**Nachweise:** Vollständiger Linux-Mint-
Viewer-Build mit `bash ogl/scripts/build-test.sh`
grün, **182/182 Python-Tests und
41/41 native CTests**. Isolierte
1280×720-Xvfb-Starts mit
`--set Language de` und `en`:
`ogl-viewer`-Fensterklasse, `STATE_LOGIN_WAIT`,
kein `STATE_LOGIN_AUTH_INIT`, Strg+Q,
**Exit-Code 0**. 11 geänderte XUI-Dateien
bytegleich im Viewer-Staging. Toolbars/
Toybox wurden **noch nicht in einer
authentifizierten 3D-Welt geklickt**
(kein autorisierter Hauptgrid-Login).
Die Alt-/Duplikatprüfung fand nur bereits
getrackte `_v2`-/`script_copy`-Ressourcen;
ohne Abhängigkeitsbeleg nichts gelöscht.

**Operativer Wiedereinstieg:** P01 Block 19/20.
Autorisierter Hauptgrid-Live-Login,
Passwortpersistenz, In-World-Shutdown
und vollständige Windows-/macOS-Viewer-
Build-/GUI-Nachweise bleiben Blocker.
Keine manuelle Zwischenabnahme.

## P02 unabhängige Vorarbeit – Sprachübergreifende NexVerse-Routen

**Code-Commit:** `7a3919a8` (`ogl-develop`).
**20/20 unabhängig geprüfte Arbeits-/Qualitätsblöcke bestanden.**
Formale Phasen bleiben **P01 18/20, P02 0/20**;
In-World-Bedienung, Hauptgrid-Live-Login und
vollständige Plattformviewer-Builds fehlen weiter.

**Änderungen ausschließlich an vorhandenen Pfaden:**
- Elf zusätzliche Menülokalisierungen
  `az,da,es,fr,it,ja,pl,pt,ru,tr,zh` geändert;
  die EN-/DE-Basis war bereits ausgerichtet.
  Alte Menüpunkte für Second-Life-L$-Kauf,
  Membership, SL Marketplace, LindenXchange
  und SL Community Pages aus allen 13
  Sprach-Overlays entfernt. Dänische und
  türkische doppelte `Manage My Account`-
  Elemente entfallen; stattdessen gilt
  das vorhandene funktionale NexVerse-
  Basismenü mit unveränderter Callback-ID.
- Die noch definierten `Manage Account`,
  `MarketplaceListings`, `NexVerse News`
  und `Report Bug`-Einträge aller
  Sprach-Overlays enthalten explizite
  `Advanced.ShowURL`-Callbacks zu
  `https://stadt-nexverse.de/`,
  `https://marketplace.stadt-nexverse.de/`
  und dem eigenen GitHub-Issue-Tracker.
  Der vorher in Portugiesisch enthaltene
  Link zu `secondlife.com/my/account`
  und `accounts.secondlife.com`
  wird nicht mehr angeboten.
- Vier dauerhafte Regressionstests in
  der bestehenden
  `ogl/tests/test_branding_contract.py`
  ergänzt; keine neuen Projektdateien,
  keine Credentials oder Nutzerdaten geändert.
  OpenSim-Karte, Landfunktionen,
  Missbrauchsmeldung, Chat und rechtlich
  nötige Lizenzinformationen bleiben erhalten.

**20 einzeln bestandene Positionen:**
01 Branch/Remote synchron und Dateiregel;
02 XML-Prüfung aller 13 Sprachdateien;
03 L$-Kauf entfernt; 04 Membership entfernt;
05 SL Marketplace entfernt; 06 LindenXchange
entfernt; 07 SL Community Pages entfernt;
08 Doppelkontomenüs DA/TR entfernt;
09 portugiesische SL-Konto-/Mitgliedschaftslinks
entfernt; 10 Konto-URLs aller Overlays;
11 Marketplace-URLs aller Overlays;
12 OGL-Issue-URLs aller Overlays;
13 NexVerse-News-URLs aller Overlays;
14 kein fremder Credential-/Konto-Link
in den relevanten Menüaktionen;
15 gemeinsames funktionales EN-Basismenü;
16 OpenSim-Kernfunktionen und Lizenzinfo;
17 Skriptbibliothek EN/DE/PT;
18 elf XUI-Sprachdateien bytegleich im
Viewer-Staging; 19 vollständiger Linux-
Viewer-Build, Python- und CTests;
20 echte DE- und PT-GUI-Starts mit
Sprachumschaltung, Login und sauberem Quit.
**20/20 grün.**

**Nachweise:** `bash ogl/scripts/build-test.sh`
erfolgreich: **178/178 Python-Tests,
41/41 native CTests**, Linux-Mint-
Vollviewer erfolgreich gebaut.
Zwei isolierte 1280×720-Xvfb-GUI-Läufe
mit `--set Language de` und
`--set Language pt`: Sprache in Logs
bestätigt, `STATE_LOGIN_WAIT`,
kein `STATE_LOGIN_AUTH_INIT`,
Strg+Q/Exit-Code **0**.
Die tatsächlichen Weltmenüs sind ohne
autorisierte Grid-Anmeldung noch nicht
durch Klicktests abgenommen.
Die älteren `_v2`-/`script_copy`-
Dateien wurden inventarisiert, wegen
möglicher Abhängigkeiten nicht gelöscht.

**Nächster operativer Einstieg: P01 Block 19/20**;
unabhängige P02-Arbeit weiter möglich,
kein künstlicher Phasenabschluss.

## P02 unabhängige Vorarbeit – Altmenüs und OGL-Support (20 Blöcke)

**Code-Commit:** `8b712092` (`ogl-develop`).
**20/20 überprüfte technische Einzelpositionen bestanden.**
Der Phasenstand bleibt **P01 18/20 und P02 formal 0/20**:
Vollständige In-World-/Windows-/macOS-Abnahmen fehlen.

**Konkrete Implementierung (nur bestehende Dateien):**
- Sämtliche 13 vorhandenen `menu_viewer.xml`-
  Lokalisierungen auf nicht mehr passende
  Second-Life-Events, Linden Home, Official
  Linden Blog, alte Public Issue Tracker und
  `Firestorm MoTD` geprüft und die jeweiligen
  Menüeinträge entfernt. Das verhindert, dass
  Übersetzungs-Overlays die entfernten Menüs
  erneut einblenden.
- Der vorhandene Content-/Entdecken-Bereich
  bietet nun **NexVerse News** mit dem
  offiziellen `https://stadt-nexverse.de/`-
  Link (DE: „NexVerse Nachrichten“).
  Die Hilfe bietet in EN/DE den verifizierten
  GitHub-Fehlertracker und die offizielle
  Release-Seite. Daraus folgt **keine**
  Aussage, dass bereits signierte Installer
  veröffentlicht sind.
- Die Script-Library-Menüpunkte in Deutsch
  und Portugiesisch wurden von falsch
  typisierten `menu_item_check`-Overlays
  auf `menu_item_call` korrigiert. Bestehende
  LSL-/OpenSim-Skriptfunktionen, Grid- und
  Abuse-Menüs sowie rechtliche „Über den
  Viewer“-Informationen bleiben erhalten.
- Bestehende
  `ogl/tests/test_branding_contract.py`
  um vier Tests erweitert. Keine neuen
  Quelldateien, Backups, Templates oder
  Zugangsdaten verändert.

**20 prüfbare Positionen:** 01 Branch/Sync,
02 keine neuen Projektdateien,
03 13 XML-Sprachen gültig,
04 SL Events entfernt, 05 Linden Home
entfernt, 06 Linden Blog entfernt,
07 alter Public Issue Tracker entfernt,
08 alter Firestorm MoTD entfernt
(jeweils in allen Sprachen);
09 News EN/DE eindeutig,
10 EN News-Portal, 11 DE News-Portal,
12 EN GitHub Issues, 13 DE GitHub Issues,
14 EN GitHub-Releases, 15 DE GitHub-Releases,
16 EN/DE Entdecken-Menü,
17 Skriptbibliothek EN/DE/PT korrekt aufrufbar,
18 OpenSim-Welt-/Lizenzfunktionen erhalten,
19 alle 12 geänderten Menü-Overlays
bytegleich im Viewer-Staging,
20 Linux-Vollbuild plus Python-/Native-
Tests und realer GUI-Negativlauf.
**20/20 bestanden.**

**Verifikation:** `bash ogl/scripts/build-test.sh`
erfolgreich: vollständiger Linux-Mint-
Viewer-Build, **174/174 Python-Tests,
41/41 native CTests**. Isolierter
1280×720-Xvfb-Voranmelde-Viewer:
Fensterklasse `ogl-viewer`, `STATE_LOGIN_WAIT`,
keine Authentifizierung,
Strg+Q mit Exit-Code **0**.
In-World-Klicktests der Menüs noch nicht
nachweisbar, da autorisierter Hauptgrid-
Live-Login fehlt. Vorhandene `_v2`-/`script_copy`-
Ressourcen nur inventarisiert, mangels
Abhängigkeitsnachweis nicht gelöscht.

**Nächster `/goal`-Einstieg:** P01
Block 19/20; unabhängige P02-Arbeit kann
fortgesetzt werden. Kein künstlicher
P01-/P02-Phasenabschluss und keine
manuelle Nutzerzwischenabnahme.

## P02 unabhängige Vorarbeit – neue NexVerse-Navigation (20 Blöcke)

**Code-Commit:** `17ab85cf` auf `ogl-develop`.
**20/20 nachvollziehbare Entwicklungs-/Prüfpositionen bestanden.**
P01 bleibt formell **18/20**; P02 bleibt **0/20**,
weil der authentifizierte Welt-/Chat-Funktionstest
und der P01-Abschluss noch fehlen. Dies ist ein
getesteter Teilumbau, keine vollständig abgenommene
P02-Phase.

**Implementierung:** In den bestehenden
`skins/default/xui/{en,de}/menu_viewer.xml`
haben „Konto“ und „Marketplace listings“
nun funktionsfähige `Advanced.ShowURL`-
Callbacks zur über HTTPS (HTTP 200,
Zertifikatsprüfung OK) verifizierten
`https://stadt-nexverse.de/` bzw.
`https://marketplace.stadt-nexverse.de/`.
Unpassende Second-Life-Menüfunktionen für
L$-Kauf, Second-Life-Membership,
SL-Marketplace, LindenXchange und die
SL-Community-Links sind aus den EN- und
DE-Menüs entfernt. Menüs „Communicate“,
„World“, „Help“ und die Einträge
„Nearby Chat“, „Conversations“ erhielten
einheitliche verständliche Beschriftungen
in EN/DE, **ohne** stabile Callback-Namen,
Shortcuts oder die Chat-/IM-Floater zu
verändern. Vorhandene
`ogl/tests/test_branding_contract.py`
um drei konkrete Navigationstests ergänzt.

**20 geprüfte Positionen:** 01 EN Bürgerportal,
02 DE Bürgerportal, 03 EN Marketplace,
04 DE Marketplace, 05 L$-Kauf entfernt,
06 SL Membership entfernt, 07 SL-Marketplace
entfernt, 08 LindenXchange entfernt,
09 SL Community entfernt (jeweils EN/DE),
10 EN Chat/Contacts, 11 DE Chat/Kontakte,
12 EN Local Chat, 13 DE Lokaler Chat,
14 EN Messages, 15 DE Nachrichten,
16 EN/DE Welt entdecken, 17 EN/DE Hilfe,
18 Shortcuts/Callbacks unverändert,
19 beide Menüs bytegleich gestaged,
20 Linux-Vollbuild und Python-/Native-Tests.
**20/20 bestanden.**

**Nachweise:** Linux-Mint-Vollviewer erfolgreich
via `bash ogl/scripts/build-test.sh`,
**170/170 Python-Tests und 41/41 native CTests**.
Sämtliche betroffene EN-/DE-Menü-XUI sind
valide und bytegleich im staged Viewer.
Ein echter GUI-Testlauf auf isoliertem
1280×720-Xvfb-Profil: OGL-Fensterklasse,
Anmeldemaske erreichbar, kein
`STATE_LOGIN_AUTH_INIT`, Strg+Q,
Exit-Code 0. Die neuen In-World-Menüs
konnten ohne autorisierte Anmeldung nicht
per Mausklick in einer 3D-Welt getestet werden.

## P02 unabhängige Vorarbeit – Chat und Konversationsfenster (20 Blöcke)

**Code-Commit:** `b654c299`.
**20/20 getrennte Chat-UI-Entwicklungs- und
Prüfpositionen bestanden.** Fünf bestehende
EN-XUI-Dateien aktualisiert: runde Avatar-
Porträts 26 px, kompakter 34-px-Chatkopf
mit Namens-/Zeitspalten, Nachrichtentext
mit Theme-Farbe und Abstand, skalierbare
Nahchat- und IM-/Gruppenchatfenster,
lesbare Eingabefelder und breitere
Sitzungstabs. Vorhandene Profilsymbole,
Zeitstempel, Chatkanäle, Voice-/Sende-
Buttons, User-Preferences und Avatar-
Privacy-Pfade nicht entfernt. Die
bestehenden Tests
`test_nexverse_chat_layout.py` wurden
nach der neuen XUI-Größe angepasst;
fünf zusätzliche Regressionstests in
`test_branding_contract.py`. Keine
neuen Quelldateien.

**Nachweise:** Vollständiger Linux-Build,
**167/167 Python-Tests, 41/41 native CTests**.
Alle fünf Chat-XUI-Dateien bytegleich im
Viewer-Staging. Echter GUI-Voranmeldetest
mit separatem Testprofil und Exit-Code 0.
In-World-Chat/Voice-Liveprüfung
noch ausstehend; alte `_v2`-/
`script_copy`-Ressourcen nur inventarisiert.

**Operativer Wiedereinstieg: P01 Block 19/20.**
Dort bleiben autorisierter Hauptgrid-Login,
Passwortpersistenz nach erfolgreichem Login,
In-World-Shutdown und vollständige
Windows-/macOS-Viewer-Build-/GUI-Tests
offen. Weitere unabhängige P02-Arbeiten
sind möglich, formaler P02-Start erst nach
echtem P01-Abschluss.

## P01 Block 19 – /goal-Durchlauf: Nutzerlinks und Login-Protokollhygiene

**Code-Commit:** `224ced81` (`ogl-develop`).
**20/20 separat dokumentierte Arbeits-/Prüfpositionen bestanden.**
Das sind keine 20 neuen Phasenabschlüsse; **P01 bleibt
18/20 formalen Blöcken** und bei Block 19.

**In bestehenden Dateien implementiert:**

- `llstartup.cpp`: Im OGL-Loginfehlerpfad
  wird nicht länger die ganze Serverantwort
  serialisiert, weil diese potentiell
  Sitzungs- und Authentifizierungsmetadaten
  enthalten kann. Die nutzerseitige Fehler-
  verarbeitung bleibt unverändert; Nicht-OGL
  erhält seinen bisherigen Logpfad.
- Die vorhandenen **13 Sprachdateien**
  `skins/default/xui/*/strings.xml` wurden
  gezielt aktualisiert. Die globale Support-
  Kennzeichnung ist `NexVerse Portal`, alte
  Registrierungspfade auf Firestorm werden
  durch `https://stadt-nexverse.de/` ersetzt,
  vorhandene Viewer-Download- und Sperr-
  meldungslinks zeigen auf
  `https://github.com/marlin198323-glitch/ogl-viewer/releases`.
  EN/DE beschreiben eine gesperrte OGL-
  Viewerversion statt eines alten Second-
  Life-/Firestorm-Downloads. EN verweist bei
  Versionsinformationen und Fehlermeldungen
  auf den OGL-Releases-Kanal; externe Links
  werden neutral beschrieben.
  Der Releases-Link ist eine Quelle, kein
  Nachweis vorhandener signierter Installer.
  Übersetzungen außerhalb EN/DE haben ihre
  bestehenden Fließtexte behalten; alt-
  sprachige Second-Life-Referenzen bleiben
  dort zur späteren UI-Lokalisierungsprüfung.
- In der bestehenden
  `ogl/tests/test_branding_contract.py`
  drei neue Regressionstests hinzugefügt:
  Supportkennzeichnung aller 13 Sprachdateien;
  registrierungs- und downloadbezogene Links;
  OGL-Loginfehler ohne rohe Serverantwort.
  Keine zusätzlichen Projektdateien,
  keine Nutzerprofile, Secrets oder
  Lizenzen bearbeitet.

**20 Prüfpositionen:** 01 Branch; 02 kein
Remote-Rückstand; 03 keine neuen Projektdateien;
04 XML aller 13 Sprachdateien; 05 alle 13
Supportbezeichnungen; 06 Registrierungs-URLs;
07 Download-URLs; 08 keine Firestorm-Sperrlinks;
09 keine veralteten externen Second-Life-FAQs;
10 EN Links; 11 DE Links; 12 EN Zugriffsmeldung;
13 EN Release-Notes-Quelle; 14 EN Crash-Hilfe;
15 EN externer Linkhinweis; 16 OGL-Loginfehler-
Log ohne rohe Antwort; 17 Nicht-OGL-Logging
bleibt kompatibel; 18 exakte Hauptgrid-
Endpunktbindung und Beta-Sperre; 19 voller
Linux-Viewer-Build mit Python-/nativen Tests;
20 reale GUI-Negativtests Hauptgrid/Beta.
**20/20 bestanden.**

**Messungen:** `bash ogl/scripts/build-test.sh`
mit vollständigem Linux-Mint-Build und
Staging: **162/162 Python-Tests, 41/41
native CTests**. Hauptgrid per Xvfb:
`ogl-viewer`-Fenster, NexVerse-Welcome,
HTTP-Sicherheitsdialog vor Anmeldung,
kein `STATE_LOGIN_AUTH_INIT`, Strg+Q mit
Exit-Code 0. Beta: synthetische Eingaben,
Beta-Sperrmeldung, kein Auth-Versuch und
Exit-Code 0. Bestehende `_v2`-/`script_copy`-
Dateien inventarisiert, ohne Nutzung-/Lizenzbeleg
nicht gelöscht.

**Echte P01-Blocker:** Erfolgreicher autorisierter
Hauptgrid-Live-Login, verifizierte Passwort-
persistenz nach Anmeldung, echter In-World-Shutdown
sowie vollständige Windows-/macOS-Viewer-Build-
und GUI-Nachweise. Die portable Grafikmodul-CI
ersetzt diese nicht. **Nächster Einstieg:
P01 Block 19/20.** Keine manuelle
Nutzerabnahme als Phasengate.

## P01 Block 19 – /goal-Durchlauf: HTTP-Zustimmung wirklich nur einmal

**Code-Commit `a7490d9f`**, Branch `ogl-develop`.
**20/20 unabhängige technische Arbeits-/Prüfpositionen
dieser Runde bestanden.** Die formale P01-Phase
bleibt **18/20 Phasenblöcke**: Weder echte
Hauptgrid-Anmeldedaten noch vollständige
Windows-/macOS-Viewer-Buildnachweise lagen vor.

**Umsetzung:**

- `oglnexversegrid.h`, `fspanellogin.cpp` und
  `llstartup.cpp`: Die alte globale Freigabe
  `gHTTPLoginApprovedThisSession` durch
  `gHTTPLoginApprovedForNextAttempt` ersetzt.
  Die Freigabe „Einmal fortfahren“ wird direkt
  **nach** `login->connect(gUserCredential)`
  verbraucht. Ein weiterer Versuch muss die
  HTTP-Risikoabfrage erneut bestätigen.
  Die separat gespeicherte Zustimmung wird nur
  bei expliziter Wahl „Zustimmung speichern“
  gesetzt und funktioniert weiterhin nur für
  die exakt festgelegte Hauptgrid-URI.
- Fehlende, falsch formatierte Credentials,
  abgewiesene Beta-/unbekannte Endpunkte sowie
  Loginformular-Änderungen (Passwort, Username,
  gespeichertes Konto, Grid, Startort,
  Speicheroptionen, Loginfenster-Neuaufbau)
  entziehen die nur einmalige Berechtigung.
  Der Nicht-OGL-Kompatibilitätspfad behält
  seine bisherige Funktion.
- In den vorhandenen DE-/EN-Benachrichtigungen
  `OGLHTTPLoginWarning` die bisher irreführende
  Angabe „Für diese Sitzung / This session“
  durch **„Einmal fortfahren / Continue once“**
  und die entsprechende Erklärung ersetzt.
  Die Standardaktion bleibt „Abbrechen“.
- Bestehende Python-/native Tests um die
  Einmal-Konsumierung und die Unabhängigkeit
  der bewusst gespeicherten Zustimmung
  ergänzt. **Keine neuen Projektdateien**.

**20/20 geprüfte Einzelpositionen:**
01 Branch; 02 keine neuen Projektdateien;
03 gepinnte Hauptgrid-URI; 04 Beta-Trennung;
05 exaktes URI-Pinning; 06 standardmäßig
keine Freigabe; 07 Konsumierungsfunktion;
08 getrennte gespeicherte Zustimmung;
09 einmalige Freigabe durch expliziten
Dialog-Callback; 10 dauerhafte Speicherung
nur über „Zustimmung speichern“;
11 Entzug bei Formularänderungen;
12 Credentialtyp-Prüfung; 13 Entzug bei
ungültigem Credential; 14 Beta-Rücksetzung;
15 Rücksetzung bei fremder URI;
16 Rücksetzung bei verweigerter Einwilligung;
17 Konsum nach Netzwerk-Login-Aufruf;
18 korrekte EN-/DE-Texte; 19 vollständiger
Linux-Build mit Python- und nativen
Regressionsprüfungen; 20 reale Hauptgrid-
und Beta-GUI-Negativtests.
Ein anfänglich auf den falschen Quelltextbereich
begrenzter Check 14 wurde am tatsächlichen
Beta-Startpfad nachgeprüft und bestanden.

**Messungen:** `bash ogl/scripts/build-test.sh`
komplett erfolgreich, einschließlich
**159/159 Python-Tests und 41/41 native CTests**
und vollständigem Linux-Viewer-Staging.
Hauptgrid unter Xvfb 1280×720: Loginfenster
bereit, HTTP-Warnung mit künstlichen Eingaben,
keine Zustimmung/Netzwerkanmeldung,
regulärer Exit **0** und WM-Klasse `ogl-viewer`.
Beta unter eigenem Testprofil: Anmeldeversuch
mit künstlichen Eingaben abgelehnt, kein
`STATE_LOGIN_AUTH_INIT`, sauberer Exit **0**.
Der einmalige **erfolgreiche** Anmeldeversuch
wurde mangels autorisierten Hauptgrid-Testkontos
nicht Ende-zu-Ende ausgeführt. Die native
Policy-Regression prüft den Wechsel von
„einmal erlaubt“ zu „erneut gesperrt“;
gespeicherte Zustimmung bleibt unabhängig.

**Weiterer Abschlussblocker:** autorisierter
Hauptgrid-Live-Login, Passwortwiederherstellung
nach erfolgreichem Login, In-World-Shutdown
und vollständige Windows-/macOS-Viewer-
Build-/Laufzeitnachweise.
Portable GitHub-Grafik-CI ist dafür kein Ersatz.
**Nächster Einstieg P01 Block 19/20**;
unabhängige Arbeiten können fortgesetzt werden,
P02 formell erst nach nachgewiesener P01-Abnahme
ohne manuelle Nutzerzwischenprüfung.

## P01 Block 19 – /goal-Durchlauf: sichere Startort-Links

**Code-Commit `e4f33c5a`, Branch `ogl-develop`.**
**20/20 überprüfte technische Arbeits- und Prüfpositionen bestanden.**
Dies sind separate Code-/Sicherheits-/GUI-Prüfungen
innerhalb von P01 Block 19, **kein Nachweis für 20/20
abgeschlossene Phasenblöcke**. Der formale Stand bleibt
**P01 18/20**.

**Umsetzung nur in bestehenden Dateien:**

- `indra/newview/fspanellogin.cpp`: `autologinToLocation`
  bewahrt bei OGL die erlaubte räumliche Zielregion,
  stößt aber keine automatische Authentifizierung mehr an.
  Nur die beiden festgelegten NexVerse-Grids werden als
  Startziel akzeptiert. Der Anwender muss die Anmeldung
  bewusst über die Loginmaske auslösen; die bisherige
  Ende-zu-Ende-Prüfung für gepinnte URI/HTTP-Einwilligung
  bleibt damit anwendbar. Eine Zieländerung invalidiert
  ältere HTTP-Consent-Dialoge. Der Nicht-OGL-Upstream-
  Ablauf bleibt im Originalpfad.
- Der bestehende interne `location_login`-Handler
  lehnt fehlerhafte x/y/z-Koordinaten ab: definierte
  Initialwerte, `LLStringUtil::convertToF32`,
  Prüfung auf `std::isfinite`, kein Auswerten
  uninitialisierter `F32` mehr.
- `onClickHelp` verlinkt für OGL auf das vorhandene
  offizielle NexVerse-Portal statt auf den alten
  Second-Life-Prelogin-Hilfedienst.
- Vorhandene Tests
  `ogl/tests/test_nexverse_grid.py` und
  `ogl/tests/test_branding_contract.py`
  erweitert; **keine neuen Projektdateien**, keine
  Nutzerprofile, gespeicherten Credentials oder Secrets
  überschrieben.

**20 Positionen der geprüften Runde:**
01 Branch; 02 Upstream-Synchronisation;
03 keine neuen Dateien; 04 beide festen Grids;
05 exakte HTTP-Einwilligung; 06 Beta-Sperre;
07 interner `location_login` blockiert untrusted
Aufrufe; 08 Tokenanzahl-Validierung; 09 Varianten
mit nur Regionsnamen bzw. Regionsname/x behalten;
10 x/y-Validierung; 11 z-Validierung;
12 NaN-/Inf-Abweisung; 13 uninitialisierte Parser
entfernt; 14 nur räumliche Startorte; 15 fremde
Grids blockiert; 16 Ziel erhalten und OGL-Login
manuell; 17 Nicht-OGL-Kompatibilität; 18 Hilfe
auf Stadt NexVerse; 19 echte Hauptgrid- und
Beta-GUI-Sicherheitstests; 20 vollständiger Linux-
Viewer-Build und alle Testgate-Ergebnisse.
**20/20 bestanden.**

**Testbelege:** `bash ogl/scripts/build-test.sh`
kompiliert und staged den kompletten Linux-Mint-
Viewer. **157/157 Python-Tests, 41/41 CTests**.
Hauptgrid: isolierter 1280×720-Xvfb-Start mit
künstlichen Eingaben, HTTP-Warnung,
`STATE_LOGIN_WAIT`, keine Einwilligung oder
`STATE_LOGIN_AUTH_INIT`, regulärer Quit **Exit 0**.
Beta: synthetische Eingaben, `Blocked beta authentication:
protocol and TLS not verified.`, keine Authentifizierung,
Quit **Exit 0**. Quelldiff und Altbestand geprüft;
bestehende `_v2`-/`script_copy`-Assets nicht
ohne Nutzung-/Lizenzbeweis gelöscht.

**P01-Abschlussblocker unverändert:** Erfolgreicher
autorisierter Hauptgrid-Live-Login, tatsächliche
gespeicherte Passwortwiederherstellung nach
diesem Login, In-World-Shutdown und komplette
Windows-/macOS-Viewer-Build-/GUI-Tests.
Die GitHub-Portable-Grafik-Matrix ist
kein Ersatz. **Nächster Einstieg bleibt
P01 Block 19/20**; danach ohne manuelle
Zwischenabnahme Block 20/P02, sobald
die Abschluss-Gates tatsächlich bestanden sind.

## P01 Block 19 – /goal-Durchlauf mit 20 überprüften Arbeitspositionen

**Code-Commit:** `a35f6caa` auf `ogl-develop`.
**20/20 Entwicklungs- und Prüfpositionen dieser Runde
bestanden.** Dieser Durchlauf ist keine künstliche
Fertigmeldung für P01: Formal **18/20 Phasenblöcke**;
End-to-End-Login und vollständige Plattform-Builds offen.

**Implementierte Korrekturen (nur bestehende Pfade):**
- `fspanellogin.cpp::updateServer` füllt in OGL
  nach einem Gridwechsel **nicht mehr automatisch
  Credentials** einer anderen Welt ein.
  Die bewusste Kontowahl und die Nicht-OGL-
  Kompatibilität bleiben erhalten.
- Unbekannte gespeicherte Gridkonten löschen
  das bisherige Loginformular. Ein unbekannter
  selbst eingegebener Nutzer übernimmt nie ein
  zuvor eingegebenes oder maskiertes Passwort.
  Bei Änderung des Benutzernamens wird das
  vorige Passwort **sofort beim Editieren**
  zurückgesetzt, nicht erst beim Fokuswechsel.
  Transient-Secret, Passwortlänge, Modified-Flag
  und Konto-Löschen-Schaltfläche werden mitgeprüft.
- `setFields` lässt OGL nur Credentials des
  aktuell ausgewählten fest angepinnten Grids
  anzeigen. Ein zurückgewiesenes Credential
  setzt **nicht** irrtümlich `sCredentialSet=true`.
- Die Revision eines ausstehenden HTTP-Warnungsdialogs
  wird zusätzlich bei Änderungen der
  „Benutzername speichern“- und „Passwort speichern“-
  Einstellungen sowie des Start-/Teleportziels
  ungültig. Keine Passwörter im Dialog gespeichert.
- Vorhandene `ogl/tests/test_nexverse_grid.py`
  um vier zusätzliche Regressionstests erweitert.
  Keine neuen Quelldateien erstellt oder
  echte Nutzer-Credentials verwendet.

**20/20 dokumentierte Positionen:**
01 Branch/Remote-Sync; 02 Dateiregel;
03 feste Grids; 04 kein Auto-Credential
nach Gridwechsel; 05 Welcome/Gridliste bleibt;
06 Kontoauswahl mit Gridwechsel vor Secret-Laden;
07 unbekannte gespeicherte Konten geleert;
08 keine Übernahme eines fremden Passworts bei
Freitext-Nutzer; 09 sofortiges Passwortleeren
bei Namensedit; 10 Transient-Secret gelöscht;
11 Konto-Löschen-Button deaktiviert;
12 Passwortflags zurückgesetzt; 13 Restore
prüft exakt das aktuelle Grid; 14 kein
`sCredentialSet` für abgelehnte Credentials;
15 Consent-Revision bleibt aktiv;
16 beide Speicher-Checkboxen invalidieren
alte Einwilligungen; 17 Teleportzieländerung
invalidiert Einwilligungen; 18 Hauptgrid-Xvfb-
HTTP-Warnung ohne Authentifizierung;
19 Beta-Negativtest und finaler Viewer-GUI-
Start jeweils mit regulärem Quit;
20 vollständiger Linux-Build und alle Tests.
**20/20 bestanden.**

**Technische Nachweise:** `bash ogl/scripts/build-test.sh`:
vollständiger Linux-Mint-Viewer inklusive Staging,
**154/154 Python-Tests und 41/41 native CTests**
erfolgreich. Separater echter Hauptgrid-GUI-Test
mit rein synthetischen Eingaben: HTTP-Warnung,
`STATE_LOGIN_WAIT`, kein `STATE_LOGIN_AUTH_INIT`,
Exit-Code 0. Beta: `Blocked beta authentication:
protocol and TLS not verified`, kein Auth-Versuch,
Exit-Code 0. Nach der letzten Änderung
zusätzlich finaler 1280×720-Xvfb-GUI-Start mit
`ogl-viewer`-Fensterklasse, Login bereit,
kein Auth-Versuch und sauberer Exit 0.
Duplikat-/Altbestandkontrolle: vorhandene
`_v2`-/`script_copy`-Dateien ohne
Nutzungsnachweis nicht gelöscht.

**Offene Abschluss-Gates:** Autorisierter
Hauptgrid-Live-Login, Passwortpersistenz nach
diesem Login, Logout/Shutdown **aus der Welt**
sowie volle Windows-/macOS-Viewer-Build-/GUI-
Tests. Die grüne GitHub-6er-Matrix prüft
portable Grafikmodule und ersetzt diese
End-to-End-Tests **nicht**. Nächster
`/goal`: P01 Block 19/20; danach P01
Block 20 und P02, sobald die echten
Gates erfüllt sind.

## P01 – /goal-Sammelrunde: Speicher-Opt-out und OGL-Menüs

**Code-Commit:** `d364f3e1` auf `ogl-develop`.
**20/20 getrennte technische Prüfpositionen bestanden; dies
sind nicht die 20 formalen Phasenblöcke. P01 weiterhin 18/20.**
Block 19 kann erst nach tatsächlichen Live-/Plattform-
Abschlussnachweisen geschlossen werden.

**Konkrete Implementierung, nur an vorhandenen Pfaden:**

- `indra/newview/fspanellogin.cpp`: OGL wertet das
  Abwählen von „Benutzername speichern“ nun
  vollständig aus: `UserLoginInfo` wird gelöscht,
  `RememberPassword` und `AutoLogin` werden
  deaktiviert, und die Passwortspeicher-Checkbox
  wird sichtbar abgewählt. Auch beim Öffnen eines
  migrierten Profils mit bereits deaktivierter
  Benutzernamenspeicherung werden alte
  Passwort-/AutoLogin-Einstellungen korrigiert.
  **Keine** bestehenden, bewusst gespeicherten
  Konten pauschal aus dem geschützten Speicher
  gelöscht; Löschung bleibt eine individuelle
  Nutzeraktion.
- `indra/newview/llstartup.cpp`: Derselbe Opt-out
  wird **vor** der automatischen Loginentscheidung
  durchgesetzt. Ein früheres AutoLogin-Flag kann
  die Datenschutzpräferenz nicht still umgehen.
  Ein ausdrücklicher Login über das geöffnete
  Formular bleibt möglich.
- Die vorhandenen Menü-XUI-Dateien für Anmelde-
  und Weltmenüs aller betroffenen Sprachen sind
  bereinigt. Firestorm-Blog-/Social-/Support-
  Promotions entfernt, alte Wiki-Hilfe ersetzt
  durch **NexVerse Portal / NexVerse Help** mit
  bekanntem Projektlink `https://stadt-nexverse.de/`.
  Internes Quellcode-/OpenSim-Verhalten sowie
  das „Über [APP_NAME]“-Fenster für rechtlich
  nötige Lizenzhinweise bleiben erhalten.
  Die Menüs wurden als XML und durch
  Quellcode-Regressionstests geprüft; ein
  manueller Klick auf jeden Navigationspunkt
  wurde nicht als bestanden behauptet.
- Regressionstest in der bestehenden
  `ogl/tests/test_branding_contract.py` erweitert.
  Keine neuen Quellprojektdateien, keine .bak-
  Kopien oder Änderungen an echten Nutzerprofilen.

**20 geprüfte Positionen dieser Runde:**
01 Branch; 02 keine neuen Projektdateien;
03 XML-Validität aller 26 Menu-Overlays;
04 keine sichtbaren Firestorm-Menütexte;
05 alte Social-/Supportmenüs entfernt;
06 NexVerse-Portal im Login-Menü;
07 NexVerse-Portal im Weltmenü;
08 korrekte Portal-URLs; 09 keine alten
Firestorm-/Phoenix-URLs in EN-Menüs;
10 Viewer-/Lizenz-About-Eintrag erhalten;
11 Grid-Hilfe erhalten; 12 Benutzerkennung
beim Opt-out entfernt; 13 Passwortspeichern
deaktiviert; 14 AutoLogin deaktiviert;
15 Startup-Entscheidung respektiert Opt-out;
16 vorhandene gesicherte Konten nicht
ungefragt gelöscht; 17 vollständiger
Linux-Mint-Build samt **150/150 Python-
Tests** und **41/41 nativen CTests**;
18 GUI-Start mit künstlichem Widerspruch
`FSRememberUsername=false`,
`RememberPassword=true`, `AutoLogin=true`:
Loginfenster statt Netzwerkanmeldung,
`RememberPassword=0`, sauberer Quit;
19 echter GUI-Klick auf „Benutzername
speichern“ mit `FSRememberUsername=0`
und `RememberPassword=0` als gespeicherte
Werte; 20 erneuter Viewerstart mit
demselben isolierten Profil, Werte
weiterhin 0, keine Authentifizierung
und Exit-Code 0. **Alle 20 bestanden.**

**Unveränderte externe P01-Blocker:** Kein
autorisierter erfolgreicher Hauptgrid-Login
und daher kein vollständiger Nachweis
einer gespeicherten Passwortanmeldung
oder eines In-World-Shutdowns;
keine kompletten Windows-/macOS-
Viewer-Build-/GUI-Tests. Portable
GitHub-CI allein ersetzt sie nicht.
Die alte upstream-Dateiliste `_v2`/
`script_copy` wurde erneut inventarisiert;
keine Löschung ohne Abhängigkeitsbeleg.

**Weiter:** `/goal P01 Block 19/20`;
unabhängige sichtbare P02-Navigations-
Vorarbeiten können bei echten Blockern
fortgesetzt werden, der formale Start
von P02 bleibt jedoch an P01 gebunden.

## P01 – /goal-Sammelrunde: Kontotrennung und Eingabeprüfung

**Code-Commit:** `15162927` auf `ogl-develop`.
**20/20 überprüfte technische Positionen bestanden.**
**Phasenstand bleibt P01 18/20** – Block 19 ist noch nicht
abgenommen, weil authentifizierter Login/Passwortpersistenz
und vollständige Windows-/macOS-Viewer-Builds fehlen.

**Implementierung:**
- `fspanellogin.cpp` lädt bei Auswahl eines gespeicherten
  Kontos einer **anderen** fest angepinnten Welt nicht mehr
  vor dem Gridwechsel dessen Credential. Stattdessen wird
  das Zielgrid gewechselt, der gesamte bisherige Login-
  Zustand zurückgesetzt, die Weltanzeige aktualisiert
  und eine verständliche DE-/EN-Nachricht angezeigt.
  Eine erneute bewusste Kontoauswahl auf dem Zielgrid
  ist erforderlich. Der alte Upstream-Kompatibilitäts-
  pfad ist nur für Nicht-OGL-Builds erhalten.
- Leerer beziehungsweise nur aus Leerzeichen bestehender
  Benutzername und leeres Passwort werden auch bei
  Enter-Tastenkürzel **vor** der HTTP-Einwilligungsabfrage
  abgefangen. Misslingt die Erzeugung eines Credential-
  Objekts, erfolgt eine Fehlermeldung statt Dereferenzierung
  eines Nullzeigers.
- Bestehende Python-Vertragstests erweitert; keine neuen
  Quelldateien, keine Änderungen an echten Nutzerprofilen.

**20 dokumentierte Einzelpositionen:**
01 Branch; 02 keine neuen Quelldateien; 03 zwei feste Grids;
04 feste Gridwahl; 05 gespeicherte Gridzuordnung geprüft;
06 Zielgrid vor Laden fremder Credentials erkannt;
07 altes Passwort vor Laden des Zielkontos gelöscht;
08 wiederholte Kontoauswahl erforderlich; 09 Nicht-OGL-
Kompatibilität; 10 sicherer Direkt-Connect-Gridwechsel;
11 exakter gepinnter HTTP-Loginserver; 12 leerer Benutzername;
13 leeres Passwort; 14 Null-Credential-Schutz;
15 Beta-Anmeldesperre; 16 HTTP-Abbrechen Standard in DE/EN;
17 realer Login-GUI-Test mit leerem Benutzernamen/Enter;
18 realer Hauptgrid-HTTP-Sicherheitsdialog-Test;
19 realer Beta-Login-Negativtest; 20 vollständiger Linux-
Viewer-Build und Testgate. **Alle bestanden.**

**Messungen:** `bash ogl/scripts/build-test.sh` baute
den vollständigen Linux-Mint-Viewer einschließlich Staging;
**147/147 Python-Tests, 41/41 native CTests** erfolgreich.
Drei getrennte Xvfb-Starts mit isolierten Testprofilen;
jeweils `STATE_LOGIN_WAIT`, **kein**
`STATE_LOGIN_AUTH_INIT`, reguläres Strg+Q mit
**Exit-Code 0**. Leerer Benutzername mit synthetischem
Passwort erzeugte **keinen** HTTP-Zustimmungsdialog.
Hauptgrid zeigte den HTTP-Dialog, Beta blockierte
synthetische Zugangsdaten. Kein erfolgreicher Login
wurde behauptet oder ausgeführt. Die bestehenden
`_v2`- und `script_copy`-Ressourcen wurden
inventarisiert, ohne belegte Abhängigkeiten zu löschen.

**Blocker:** Autorisierter Hauptgrid-Live-Login,
erneuter Login über gespeichertes Passwort,
Shutdown **nach** 3D-Welt-Eintritt und komplette
Windows-/macOS-Viewer-Build- und Laufzeittests.
Die GitHub-Matrix prüft portable Module, keine
vollständigen plattformübergreifenden Viewer-Builds.
**Weiter:** `/goal P01 Block 19/20`; danach
Block 20 und P02 ohne zusätzliche Nutzerabnahme.

## P01 – /goal-Sammelrunde: 20 Formularzustands- und Welcome-Prüfungen

**Code-Commit:** `3dba5f51` auf `ogl-develop`.
**20/20 technische Prüfpositionen in dieser Runde bestanden.**
Es handelt sich um zusätzliche Entwicklungs- und Regressionstests
in **P01 Block 19**, nicht um das fehlende Abschluss-Gate
der Phase. **P01 bleibt bei 18/20 Phasenblöcken.**

**Umsetzung in vorhandenen Projektdateien:**

- `indra/newview/fspanellogin.cpp`: Der HTTP-Warnungsdialog
  speichert nun einen **numerischen, monoton erhöhten
  Formular-Revisionsstand**, zusätzlich zu Grid und
  Kontonamen. Bei Eingaben im Passwortfeld, Änderungen
  des Benutzernamens, Wechsel eines gespeicherten Kontos,
  gridbedingtem Leeren oder dem Neuaufbau der Loginmaske
  wird dieser Stand invalidiert. Ein verspätet angeklickter
  Zustimmungsdialog mit veralteter Revision kann weder
  `OGLRememberPinnedHTTPLoginURI` setzen noch
  `gHTTPLoginApprovedThisSession` freigeben.
  Der Callback behält **kein Passwort und keinen Hash**
  als Zustimmungsreferenz zurück. Die vollständige,
  strikte Prüfung des gepinnten Hauptgrid-Endpunkts
  bleibt unverändert aktiv.
- **Welcome-Privatsphäre und Rebranding:** Der OGL-
  Welcome-Aufruf übermittelt weiterhin
  `lang`, `version`, `channel`, `grid` sowie
  bei Erststart den vorgesehenen `firstlogin`-Wert.
  Legacy-Firestorm-Parameter wie `skin`,
  `sourceid`, `os`, `login_content_version`,
  `noversionpopup` und alte Splash-UI-Flags
  werden im OGL-Pfad nicht mehr an die
  NexVerse-Welcome-Seite angehängt. Im Nicht-OGL-
  Kompatibilitätszweig bleibt das bisherige
  Verhalten erhalten.
- In den vorhandenen Dateien
  `ogl/tests/test_nexverse_grid.py` und
  `ogl/tests/test_branding_contract.py` wurden
  Quellen-/Regressionstests für den neuen
  Formular-Revisionsstand und die URL-Parameter
  ergänzt. **Keine neue Projektdatei erstellt.**

**20/20 überprüfte technische Einzelpositionen:**
01 Branch; 02 keine neuen Projektdateien;
03 zwei gepinnte Grids; 04 exakter Loginendpunkt;
05 Beta-Sperre; 06 HTTP-Abbruch als Standard;
07 Formular-Revision `U64`; 08 Neue-Panel-
Invalidierung; 09 Passwortänderung invalidiert;
10 Username-Änderung invalidiert;
11 gespeicherte Kontoauswahl invalidiert;
12 Gridwechsel invalidiert; 13 keine Passwortkopie
im HTTP-Warnungs-Callback; 14 Revisionsprüfung
vor Zustimmung; 15 NexVerse-Welcome fest gepinnt;
16 Legacy-Telemetrie aus OGL-Welcome entfernt;
17 Sprache/Version/Grid bleiben vorhanden;
18 realer Hauptgrid-Xvfb-GUI-Test mit HTTP-
Warnung und ohne Login; 19 realer Beta-GUI-
Negativtest; 20 vollständiger Linux-Viewer-
Build und Python-/native Tests.

**Tatsächliche Testbeweise:**
`bash ogl/scripts/build-test.sh`: vollständig
gebaut und gestaged; **145/145 Python-Tests**
und **41/41 native CTests** grün.
Hauptgrid und Beta wurden separat auf 1280×720
(Xvfb, Software-OpenGL, isolierte XDG-Profile)
gestartet. Beide erreichten `STATE_LOGIN_WAIT`
und die NexVerse-Welcome-Seite; bei Hauptgrid
erschien vor Authentifizierung der HTTP-
Zustimmungsdialog, während Beta explizit
`Blocked beta authentication: protocol and TLS
not verified` protokollierte. Bei beiden
GUI-Tests **keine `STATE_LOGIN_AUTH_INIT`**,
keine echten Zugangsdaten, Strg+Q führte
jeweils zu **Exit-Code 0**.
`git diff --check` erfolgreich.
Vorhandene `_v2`- und `script_copy`-Assets
wurden geprüft, mangels nachgewiesener
Unbenutztheit nicht gelöscht.

**Weiterhin nicht nachgewiesen:** Erfolgreicher
autorisierter Hauptgrid-End-to-End-Login,
Passwortpersistenz nach diesem Login,
In-World-Shutdown sowie vollständige
Windows-/macOS-Viewer-Build-/GUI-Tests.
Die portable GitHub-Matrix ist kein Ersatz.
**Nächster operativer /goal-Schritt bleibt
P01 Block 19/20**, ohne manuelle Abnahme.

## P01 – /goal-Sammelrunde: 20 Prüfblöcke zur Endpunkt-Pinning-Sicherheit

**Code-Commit:** `69aac477` (`ogl-develop`).
**20/20 zusätzliche Sicherheits-/Testblöcke erfolgreich; P01
bleibt ausdrücklich 18/20 Phasenblöcke, Block 19 aktiv.**

**Ursache und Fix:**
In `fspanellogin.cpp` und `llstartup.cpp` erlaubte
die historische Ausnahme `all_https` / `secure_login`
einen beliebigen HTTPS-Loginendpunkt, selbst wenn die
vermeintlich feste NexVerse-Grid-Konfiguration aus einer
geänderten Profilquelle einen anderen Server nannte.
Damit konnte ein fremder HTTPS-Server die
Credenzialeingaben entgegennehmen. Beide Prüfgrenzen
verlangen jetzt **`isExactPinnedHTTPLogin` für das
exakte Hauptgrid und die exakte öffentliche OpenSim-
Login-URI**, unabhängig vom URL-Schema.
Ein alternatives HTTPS-Ziel wird **nicht** durch
ein gültiges Zertifikat allein vertrauenswürdig.
Die Beta-Anmeldung bleibt bis geprüften TLS- und
OpenSim-Protokollbedingungen gesperrt.
Eine spätere freigegebene HTTPS-Migration muss
bewusst zentral umgepinnt und neu getestet werden.

Das HTTP-Einwilligungsfenster bindet außerdem
den Zustand von Grid und Konto zum Zeitpunkt
der Abfrage. Eine verzögert beantwortete
Einwilligung darf nach Grid-/Kontowechsel
oder Änderung der Loginadresse weder eine
dauerhafte Einwilligung speichern noch
eine Anmeldung starten. Fehlermeldungen
für unzulässige Server liegen in den
**bereits bestehenden DE-/EN-Strings**.
Keine vorhandenen Nutzer-Credentials berührt.

**20/20 nachgewiesene Prüfpositionen der Runde:**

01 Branchprüfung; 02 keine neuen Quelldateien;
03 zwei feste Grids; 04 erlaubter exakter HTTP-Endpunkt;
05 leere URI abgewehrt; 06 fremde HTTP-URI abgewehrt;
07 fremde HTTPS-URI abgewehrt;
08 unfreigegebene HTTPS-Hauptgridvariante abgewehrt;
09 mehrere Login-URIs abgewiesen;
10 URI-Pfadveränderung abgewiesen;
11 URL-Userinfo und Großschreibungsvariante abgewiesen;
12 Beta erhält keine Hauptgrid-Freigabe;
13 GUI prüft exakten URI vor Passwortzugriff;
14 Netzwerkanmeldung prüft exakten URI nochmals;
15 AutoLogin bei manipuliertem URI deaktiviert;
16 alte Zustimmungsdialoge können nicht später
Einwilligung erteilen; 17 Prüfung von Konto,
Grid und URI beim HTTP-Warnungs-Callback;
18 DE-/EN-Fehlermeldung und „Abbrechen“ als Default;
19 echter GUI-Test des HTTP-Zustimmungsdialogs
ohne Einwilligung/Authentifizierung, sauberer Quit;
20 echter Beta-Negativtest ohne Authentifizierung,
sauberer Quit, ergänzt durch vollständigen Viewer-Build.

**Reale Build- und Laufzeitprüfung:**
`bash ogl/scripts/build-test.sh` vollständig
erfolgreich: **143/143 Python-Unittests,
41/41 native CTests** und vollständiger
Linux-Viewer-Build/Staging.
Vorhandenen nativen `ogl_grid_http_policy`-Test
um fremdes HTTPS, mehrere Server, falsche
Ports, URI-Userinfo, veränderte Pfade und
Beta-Fremdgrid erweitert.
Zwei isolierte Xvfb-GUI-Testprofile:
NexVerse-Hauptgrid erreicht Loginfenster und
HTTP-Warnung, **kein `STATE_LOGIN_AUTH_INIT`**
und Exit 0 nach Strg+Q. Beta meldet
`Blocked beta authentication: protocol and TLS
not verified`, ebenfalls keine Authentifizierung,
Exit 0. Echte Konten, Secrets oder Tokens
wurden nicht eingegeben oder versendet.
Git-Diff/Altbestand-/Duplikat-Audit sauber;
die bestehenden `_v2`-/`script_copy`-
Dateien sind nicht ohne Nutzungsnachweis
als löschbar freigegeben.

**Echte P01-Abschlussblocker weiterhin offen:**
erfolgreicher autorisierter Hauptgrid-Login,
Passwortpersistenz nach erfolgreichem Login,
In-World-Shutdown sowie vollständige Windows-
und macOS-Viewer-Build-/GUI-Prüfungen.
Die portable GitHub-Matrix ist dafür kein
vollständiger Ersatz. Kein automatischer
Fortschritt zu P02, bevor die Blocker
tatsächlich nachgewiesen sind.

**Weiter:** `/goal P01 Block 19/20`,
ohne manuelles Nutzer-Abnahmegate.

## P01 – /goal-Sammelrunde: 20 Prüfpositionen zur Credential-Wiederherstellung

**Code-Commit `ad005fc9`**, Branch `ogl-develop`;
**20/20 zusätzliche Prüfpositionen bestanden.**
Die 20 Prüfpositionen bilden **keine abgeschlossene P01-Phase**.
Der maßgebliche Phasenstand bleibt **18/20, Block 19 aktiv**;
es fehlt weiterhin ein autorisierter Hauptgrid-End-to-End-Login.

**Korrekturen an ausschließlich bestehenden Dateien:**

- `fspanellogin.cpp`: Bei `setFields(..., from_startup=true)`
  wird vor dem Zugriff auf `stored_credential->getAuthenticator()`
  ein Null-Credential abgefangen. Nach einem abgebrochenen
  oder fehlgeschlagenen Login ohne jemals gespeichertes
  Konto wird dadurch nicht mehr auf einen Nullzeiger
  zugegriffen; die vorhandene Transient-Passwort-Wiederherstellung
  bleibt erhalten.
- Wenn der Nutzer `FSRememberUsername` deaktiviert,
  bereinigt der OGL-Login den bisherigen `UserLoginInfo`-Wert
  sofort beim Klick auf das Kontrollkästchen. Das gilt auch
  für Altprofile beim nächsten Loginfensterstart, selbst
  wenn der Nutzer nicht auf „Welt betreten“ klickt.
  Der Connect-Callback persistiert den Benutzernamen
  nur noch, wenn diese Einstellung aktiv ist. Bestehende
  verschlüsselte gespeicherte Credentials und die
  Nicht-OGL-Kompatibilität bleiben unangetastet.
- `fspanellogin.cpp` und `llstartup.cpp`: Im OGL-Pfad
  werden Benutzer-/Accountnamen nicht mehr in
  den bisherigen Authentifizierungs- und
  Favoriten-Debugmeldungen oder in der
  `Attempting login as`-Meldung ausgegeben.
  Der vorgeschriebene Upstream-Kompatibilitätspfad
  bleibt erhalten.
- In der bestehenden
  `ogl/tests/test_branding_contract.py` entsprechende
  Regressionstests für Nullprüfung, Namensspeicherung,
  Opt-out und reduzierte Loginlogs ergänzt.

**20 überprüfte Positionen:**

01 kanonischer Branch; 02 keine neuen Quelldateien;
03 zwei festgelegte Grids; 04 exakte HTTP-Ausnahme;
05 Beta-Login-Sperre vor Passwortlesen im Connect-Callback;
06 Beta-Auto-Login-Sperre; 07 HTTP-Abbruch als Default
in DE/EN; 08 Gridwechsel löscht vorherige Login-Eingaben;
09 Direkt-Connect unbekannter Grids gesperrt;
10 Fremdgrid-Credential-Löschung gesperrt;
11 verschwundene gespeicherte Konten abgefangen;
12 Transient-Credential Nullprüfung;
13 keine vollständigen Credential-Objekte im Loginlog;
14 Merken-Checkbox bei OGL-Login berücksichtigt;
15 Opt-out bereinigt letzten Benutzernamen sofort;
16 Altprofilbereinigung bei bereits deaktiviertem Merken;
17 Benutzernamen in OGL-Auth-Logs minimiert;
18 echte Checkbox-Persistenz nach Viewer-Neustart
unter separatem XDG-Testprofil;
19 reales Loginfenster und regulärer Voranmelde-Quit;
20 vollständiger Linux-C++-Build samt Tests.
Bei Position 05 wurde ein anfänglich falsch
abgegrenzter Quelltext-Suchbereich im externen
Prüfskript korrigiert; die auf den tatsächlichen
Connect-Callback begrenzte Nachprüfung bestand.

**Verifikationen:**

- `bash ogl/scripts/build-test.sh`: kompletter
  Linux-Mint-Viewer-Build und Staging erfolgreich,
  **141/141 Python-Tests, 41/41 native CTests**.
  `git diff --check` erfolgreich.
- Isolierter GUI-Test mit rein synthetischem
  `UserLoginInfo`: „Benutzername speichern“
  deaktiviert, Viewer beendet, anschließend mit
  demselben temporären Profil neu gestartet;
  `FSRememberUsername=0` geblieben und
  `UserLoginInfo` nicht mehr als gespeicherte
  Benutzerkennung vorhanden. Ein erster zeitlich
  zu früh gesendeter Ctrl+Q-Test endete mit
  Timeout 124; nach erneutem Start und Erreichen
  von `STATE_LOGIN_WAIT` gelang Ctrl+Q
  **regulär mit Exit-Code 0**.
- Die ältere GitHub-Portable-Grafik-CI auf
  `1ebb8ee0` bestand alle sechs Matrixjobs;
  diese CI ist **kein** vollständiger
  Windows-/macOS-Viewer-Build.
- Altbestandprüfung: `client_list_v2.xml`,
  `cycle_object_animations_v2.lsl` und
  `script_copy.png` sind bestehende Upstream-Ressourcen;
  ohne Nutzungs-/Lizenznachweis nicht löschen.
  **Keine neue Projektdatei angelegt.**

**Blocker unverändert:** Es gibt keinen belegten
erfolgreichen Hauptgrid-Login mit einem autorisierten
Testkonto, keine bestätigte Passwortpersistenz nach
erfolgreicher Authentifizierung, keinen Shutdown
nach Eintritt in die 3D-Welt und keine vollständigen
Windows-/macOS-Viewer-Laufzeittests.
Die separate portable CI ersetzt diese
Abschlussbedingungen nicht. Daher P01 weiterhin
**18/20**; der nächste `/goal`-Einstieg
ist **P01 Block 19/20**, ohne manuelle
Zwischenabnahme.

## P01 – /goal-Sammelrunde mit 20 Anmelde-/Sicherheitsprüfungen

**Code-Commit:** `d9fca485`, Branch `ogl-develop`.
**Ergebnis: 20/20 überprüfbare Teilaufgaben bestanden.**
Dies ist ein gebündelter Test-/Entwicklungsdurchlauf
**innerhalb von P01 Block 19**, nicht die fehlende
Live-Anmeldung oder ein Abschluss der ganzen Phase.
**Offizieller Phasenstand weiterhin 18/20.**

**Umgesetzte Änderungen in bestehenden Dateien:**

- `indra/newview/fspanellogin.cpp`: Der Connect-Button
  lehnt nicht definierte/unbekannte Gridwerte jetzt
  unmittelbar ab. Wird eine andere der beiden festen
  Welten erst durch den Connect-Button übernommen,
  werden alte Eingaben zurückgesetzt und die Anmeldung
  **nicht** ausgeführt. Der Nutzer erhält stattdessen
  eine freundliche Meldung und kann sich auf dem
  korrekten Grid neu anmelden.
- Gespeicherte Konten: Das Löschbestätigungs-Callback
  prüft den Gridwert erneut (auch bei veralteter
  Dialog-Aktion). Ein zwischenzeitlich gelöschtes
  Credential wird vor `userID()`, Passwortnutzung
  oder `deleteCredential()` auf Null geprüft.
  Ein ungültiger/staler Löschauftrag darf auch
  `UserLoginInfo` nicht verändern. Ebenso wird
  ein verschwundenes Konto bei Auswahl im Login
  abgefangen, statt es als nutzbaren Account
  darzustellen. Die bestehende verschlüsselte
  Credential-Verwaltung bleibt erhalten.
- `skins/default/xui/en/strings.xml` und
  `.../de/strings.xml`: verständliche Meldung
  „Grid gewechselt, Zugangsdaten erneut eingeben“
  ergänzt, ohne zusätzliche Dateien oder fremde
  Supportlinks.
- `ogl/tests/test_nexverse_grid.py`: drei neue
  Regressionstests für Direkt-Connect, sichere
  Kontolöschung und verschwundene gespeicherte
  Konten. Insgesamt **139/139 Python-Tests**,
  **41/41 native CTests** sowie vollständiger
  Linux-Mint-Viewer-Build erfolgreich.

**20 Einzelprüfungen der aktuellen Runde:**
01 richtiger Branch; 02 OpenSim-Hauptgrid live
vorhanden; 03 fest hinterlegte Login-URI stimmt
mit öffentlicher `/get_grid_info` überein;
04 öffentliche Welcome-URI stimmt überein;
05 Beta-TLS-Hostname stimmt weiterhin **nicht**;
06 genau zwei Grids; 07 keine freie Gridadresse;
08 Dropdown-Gridvalidierung; 09 direkte Connect-
Gridvalidierung; 10 Wechsel des Grids vor Passwort-
auslesen abgefangen; 11 Beta GUI-Sperre vor
Passwortauslesen; 12 Beta Auto-Login gesperrt;
13 Fremdgrid-Credential-Löschung abgewehrt;
14 Null-Prüfung vor Credential-Dereferenzierung;
15 keine Änderung gespeicherter Login-ID
bei fehlendem Credential; 16 fehlendes Konto
bei Auswahl abgefangen; 17 DE-/EN-Hinweis und
HTTP-Warnung mit „Abbrechen“ als Standard;
18 echter Hauptgrid-Xvfb-Loginbildschirm,
NexVerse-Welcome, künstliche Eingaben und
HTTP-Warnung ohne Einwilligung, Quit Exit 0;
19 echter Beta-Xvfb-Test mit synthetischen
Eingaben, gesperrter Anmeldung, Quit Exit 0;
20 vollständiger C++-Viewer-Build und Testgate.
**Alle 20 Prüfpositionen bestanden.**

**Live-Endpunktprüfung ohne Zugangsdaten:**
`http://hg.stadt-nexverse.de:80/get_grid_info`
liefert mit GET Status HTTP 200 und dokumentiert
`<platform>OpenSim</platform>`. Der Beta-Server
liefert ein Zertifikat für
`s359142.srv.nexvortex.de`, nicht für
`beta.stadt-nexverse.de` (Hostname mismatch,
openssl verify error 62); **Beta-Login bleibt gesperrt**.
Keine echten Passwörter, Sessions oder Logintokens
an die Netzdienste übertragen.

**Noch nicht nachgewiesen:** Erfolgreicher Login
mit autorisiertem Testkonto ins Hauptgrid,
gespeicherte Passwortwiederherstellung nach
Neustart, regulärer Shutdown nach
3D-Welt-Eintritt und vollständige
Windows-/macOS-Viewer-Build-/GUI-Tests.
Die bestehende GitHub-CI ist eine portable
Modul-/Regressionstestmatrix, kein Ersatz
für diese vollständigen Prüfungen.
Die Altbestandskontrolle identifizierte
bereits vorhandene `_v2`-LSL/XML- und
`script_copy`-Assets; sie wurden mangels
Abhängigkeitsbeweis nicht blind entfernt.
Keine neuen Quelldateien/Projektdateien erstellt.

**Nächster Schritt:** `/goal P01 Block 19/20`
mit tatsächlichem End-to-End-Login- und
Mehrplattform-Nachweis, danach Block 20 und
direkter Start von P02 ohne manuelles
Zwischengate.

## P01 – technische Arbeitsrunde vom 01.11.2026

**Verifizierte Code-Commits:** `9c29aba6` und `fed30c3e`
auf `ogl-develop`.
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
Release-Staging erzeugt, **121/121 Python-Tests** und
**41/41 native CTest-Tests** erfolgreich; `git diff --check` ohne
Fehler. Die Beta-Login-Sperre, feste Grid-Auswahl und
explizite HTTP-Einwilligung bleiben in den Sicherheitstests grün.
Keine neue Projektdatei angelegt. Duplikat-/Altbestand-Audit:
`git ls-files` auf *_neu, *_v2, *.bak und Kopie-Namen geprüft;
gefundenes upstream-notwendiges `client_list_v2.xml` sowie
Funktionsressourcen mit „copy“ nicht blind gelöscht.

**Ergänzung zur laufenden Arbeit an Block 7 (noch nicht
vollständig abgeschlossen):** In einem isolierten Xvfb-Test mit
1280×720 wurde die bisherige Standardfenstergröße 1024×738 bei
Y=-9 als zu groß ermittelt. Die vorhandene `settings.xml` wurde auf
1024×680 (neue Standardinstallation, bestehende Nutzereinstellung
bleibt erhalten) angepasst. Der wiederholte virtuelle Start zeigte
ein vollständiges OGL-Fenster mit 1024×680 bei Y=+20; die originale
NexVerse-Welcome-URL wurde im Log geladen. Ein 16-Sekunden-
Smoke-Test endete bewusst per Timeout (Exit 124), **kein
normaler Viewer-Shutdown-Test**. Windows/macOS und echte HiDPI-
Sichtprüfung bleiben offen.

Ebenfalls überarbeitet: der beim Login angezeigte DE-/EN-
Virenschutz-Hinweis und der vorhandene Whitelist-Dialog,
die vorher zur Firestorm-Wiki verlinkten und unkritische
Ausnahmen nahelegten. Neue Hinweise verlangen eine
bestätigte Fehlmeldung und raten ausdrücklich zum
aktivierten Virenschutz. Bestehende andere Sprachdateien
sind noch im vollständigen Branding-Audit zu prüfen.
Verifizierte XML-/Branding-Tests plus vollständiger Linux-
Build: **121 Python, 41 native Tests**, alles bestanden.

**Noch offen:** Blöcke 19–20, insbesondere bestätigter Live-Login auf
720p–4K/HiDPI, Speichern und Wiederherstellen der Zugangsdaten,
sichtbarer Gridwechsel, bestätigtes öffentliches Grid-Login,
vollständiger sichtbarer OGL-Rebranding-Audit, UI-/Sicherheitsregression
und relevante Windows-/macOS-CI. Ein lokaler Modul-/Linux-Build
ist kein Beleg für plattformübergreifende Installer. **P01 bleibt
aktiv**, kein Phasenabschluss und noch kein Übergang nach P02.

**Nächster Arbeitsschritt:** `/goal P01 Block 19/20` –
mit einem autorisierten Hauptgrid-Testkonto tatsächliche Anmeldung
und Wiederherstellung gespeicherter Zugangsdaten nach erneutem
Programmstart prüfen; danach regulären Shutdown **nach Welt-Login**
messen. Die Voranmelde-Schließprüfung besteht bereits. Windows-/
macOS-FULL-Viewer-Builds und GUI-Regressionsnachweise bleiben
ausstehend (ein Modulmatrix-CI ersetzt diese nicht).
Beta-Login bleibt bis bestätigtem TLS/Protokoll gesperrt.
Block 20 und P02 erst bei tatsächlich erfülltem Abschluss-Gate.

## P01 – weitere /goal-Runde: Blöcke 7–8/20

**Code-Commit:** `459ba13b` (`ogl-develop`). Fortschritt nun
**8/20, Phase weiterhin aktiv**.

- **Block 7 abgeschlossen:** Passwort-Sichtbarkeitsschalter aus dem
  Checkbox-Bereich in das Passwortfeld verschoben; Text rechts
  gepolstert, Schalter innerhalb des Eingabebereichs und per
  Tastatur erreichbar. Bestehende Username-/Passwort-Verarbeitung
  nicht ausgetauscht. 1280×720 OGL-Login sichtbar geprüft; 4K-
  Virtual-Display startete mit zentriertem 1024×680-Fenster
  (+1408/+740). Diese Geometrie ist **kein** abgeschlossener
  HiDPI-Text- und Mehrplattformtest (Block 16).
- **Block 8 abgeschlossen:** OGL-Gridfeld auf feste Dropdown-Auswahl
  (kein Texteingabe-URI) umgebaut; zwei gepinnte Systemgrids
  werden explizit und einmalig eingetragen. Deutscher/englischer
  Tooltip erläutert die noch gesperrte Beta-Anmeldung. Die
  Beschriftung 'WELT:' wurde nach einem echten 720p-Screenshot
  von 35 auf 55 UI-Einheiten erweitert. Im virtuellen X11-Test
  das Dropdown per XTest angeklickt und beide Einträge sichtbar
  verifiziert, ohne Login oder Passwortversand. Codeprüfung
  bestätigt: unbekannte Gridwerte bleiben zurückgewiesen.
- **Vorarbeiten für spätere P01-Blöcke:** Kontospeichern,
  Passwortspeichern, Passwortsichtbarkeit und Kontoauswahl in
  vorhandenen C++-Callbacks/XUI mit Testverträgen geprüft (9,
  noch kein echter gespeicherter Zugangsdaten-Livetest).
  Der alte Firestorm-Grid-Hilfe-Klick ist bei OGL gesperrt
  (10/14). Die bisher beim ersten Login störende Firestorm-
  Whitelist-Meldung wird für OGL nicht mehr eingeblendet;
  Sicherheitssoftware wird nicht deaktiviert (10).
  Der bereits entfernte RTX-Diagnosestatus und DE/EN-Texte
  bleiben geprüft (11–15).
- **Altbestand (17–18 noch nicht vollständig abgenommen):**
  Die alten StarLight- und StarLightCUI-Login-Overlays enthielten
  Firestorm-Logo/alte Mode-Auswahl. Ein zunächst ausprobierter
  reiner XUI-Wurzelknoten verursachte **leere Loginwidgets**;
  diese Variante wurde verworfen. Da die Skin-Laufzeit vollständige
  Panels erwartet, wurden die **vorhandenen beiden Dateipfade**
  mit dem kanonischen OGL-Logininhalt überschrieben – keine
  neue Projektdatei. Der StarLight-Viewer wurde 35 Sekunden
  isoliert auf Xvfb getestet: moderne OGL-Loginoberfläche
  sichtbar, `STATE_LOGIN_WAIT`, **0 Dummy-Controls**.
  Eine künftige echte zentralisierte XUI-Vererbung bleibt
  P03-Aufgabe; aktuell müssen die zwei bestehenden Skin-
  Layoutdateien noch separat gepflegt werden. Keine ungenutzten
  Upstream-Skin-/Lizenz-/Copy-Assets blind gelöscht.
- **Verifikation:** Linux-Mint-Build/Packaging erfolgreich,
  **126/126 Python-Tests**, **41/41 native CTests** bestanden.
  Xvfb: 1280×720 Default/Login mit realem Dropdown und
  StarLight/Login, sowie 3840×2160 Startfenstergeometrie.
  Die Smoke-Tests endeten absichtlich nach Timeout (Exit 124);
  das ist **kein** sauberes Schließen oder Login in die 3D-Welt.
  Live-Grid-Login, echte Kontopersistenz, 4K-HiDPI-
  Schriftqualität, Windows/macOS Viewer-Builds bleiben offen.

**Weiter:** `/goal P01 Block 9/20`. Ohne
manuelles Phasengate, aber fehlende echte Funktionen
oder externe Abhängigkeiten dürfen nicht als bestanden gelten.

## P01 – /goal-Runde: Blöcke 9–15/20

**Code-Commit:** `07015f23` (Branch `ogl-develop`).
**Sequenzieller Fortschritt: 15/20, P01 weiterhin aktiv.**

- **Block 9 – Konto-/Passwortspeicherung:** Die vorhandene
  Speicherung über `gSecAPIHandler->saveCredential` nach erfolgreichem
  Login und die Kontoauswahl über `listCredentials/loadCredential`
  unverändert erhalten. OGL zeigt keine gespeicherten Konten fremder
  Grids mehr an und akzeptiert keine unbekannten Gridwerte aus dem
  Account-Picker. Beim Wechsel zwischen den beiden gepinnten Grids
  werden Benutzername, eingegebener Passworttext, Passwortstatus und
  Anmeldebutton zurückgesetzt. Der gespeicherte Zielgrid-Account
  bleibt separat auswählbar. Gespeicherte Konten werden nur über
  die tatsächlich ausgewählte Identität mit Bestätigung gelöscht;
  ein gelöschtes Konto hinterlässt kein aktives Passwortfeld.
  Fehler bei Benutzernamen unmittelbar vor `@` korrigiert.
  **GUI-Test nur mit künstlichen Eingaben**: beide Felder zunächst
  gefüllt, dann Beta ausgewählt, beide Felder wieder leer, Login
  deaktiviert. **Keine** echte Kontopersistenz über einen
  erfolgreichen Netzlogin verifiziert; bleibt Teil von Block 19.
- **Block 10 – Einwilligung/Fehler:** HTTP ohne TLS weiterhin nur
  für exakt gepinnte Stadt-NexVerse-URI nach bewusster Zustimmung;
  Beta bleibt gesperrt. Bei der HTTP-Warnung ist die Standardaktion
  jetzt **Abbrechen**, nicht mehr Einwilligen. Die Option
  „Für diese Sitzung“ bezeichnet korrekt eine Sitzungseinwilligung,
  die zweite Option speichert eine an die genaue Adresse gebundene
  Zustimmung. DE-/EN-Dialoge, Bestätigung zum Kontolöschen und
  Beta-Meldung verständlicher formuliert. Im isolierten X11-Viewer
  erschien der echte HTTP-Warndialog vor einer Netzwerkverbindung;
  keine Einwilligung erteilt, keine echten Zugangsdaten benutzt.
- **Blöcke 11–13 – GPU-/RT-Loginbereinigung:** Die früher
  entfernten Statusanzeigen „Vulkan RT bereit“ und „Effekt aus“
  sowie die Login-RT-Hardwareabfrage bleiben laut Tests
  vollständig aus Login-XUI/C++ entfernt. Welt-RT-Prototyp
  weiterhin ausdrücklich **kein** vollwertiger Vulkan-Renderer;
  Grafik-/Logdiagnosen sind nicht Loginstatus.
- **Block 14 – Buttons/Tastatur:** Passwort-Auge, beide
  Speichern-Checkboxen und das feste Grid-Dropdown bestehen
  Fokus-/Positionsverträge; der echte Viewer ließ synthetische
  Username-/Passwort-Eingaben zu. Kritische modale Aktionen
  Einwilligung und Kontolöschen haben „Abbrechen“ als Default.
- **Block 15 – DE-/EN-Lokalisierung:** Die initial im echten
  DE-Viewer noch **englischen HTTP-Schaltflächen** wurden
  entdeckt. Ursache war ein nicht benanntes `form` in den
  vorhandenen XUI-Dialogen; nach Änderung auf `form name="form"`
  zeigte ein erneuter Screenshot **alle drei Schaltflächen auf
  Deutsch**. Englische und deutsche Texte/Indexreihenfolge per
  Regressionstests geprüft.
- **Block 16 begonnen, nicht abgeschlossen:** Xvfb 1280×720
  und 3840×2160 mit Standard-UI waren zuvor geometrisch
  geprüft. Der zusätzliche Test `1920×1080 -dpi 192`
  mit `--set UIScaleFactor 1.5` startete, zeigte aber eine
  **rechts abgeschnittene Loginleiste**: darunter fallen
  Startort und Login-CTA. Das vorhandene Loginlayout benötigt
  noch echte Breiten-/DPI-Anpassung. Kein HiDPI-Gate bestanden.
- **Verifikation:** Vollständiger lokaler Linux-Viewer-Build
  und Staging erfolgreich, **128/128 Python-Tests** und
  **41/41 native CTest** bestanden; `git diff --check`
  erfolgreich. Xvfb-Interaktion mit ausschließlich synthetischen
  Daten, HTTP-Modal- und DE-Button-Screenshot überprüft.
  Die GUI-Läufe wurden per Timeout beendet: **kein**
  bestätigter sauberer Shutdown, keine Grid-Live-Anmeldung,
  keine Windows-/macOS-Installer-Abnahme.
- **Altbestands-Audit:** Keine neuen Projektdateien angelegt.
  Tracked `client_list_v2.xml` und LSL-Testressourcen
  mit `_v2` als bestehender Upstream-Bestand erneut
  gefunden und **nicht ohne Nutzungs- und Lizenzbeweis
  entfernt**. Zwei bestehende StarLight-Overlays aus der
  vorigen Runde bleiben als OGL-Inhalt aktiv.

**Weiter:** `/goal P01 Block 16/20`; P02 noch nicht
freigegeben. Phasengates sind technische Prüfungen,
keine manuelle Nutzerabnahme.

## P01 – /goal-Runde: Blöcke 16–17/20

**Verifizierter Code-Commit:** `af1ff08b` auf `ogl-develop`.
**Sequenzieller Fortschritt: 17/20; P01 weiterhin aktiv.**

- **Block 16 abgeschlossen – adaptive Bildschirm-/HiDPI-Skalierung:**
  Alle **bestehenden** Fensterklassen (SDL/Linux, Win32 und
  macOS) besitzen jetzt eine Abfrage der nutzbaren Arbeitsfläche
  des aktuellen Monitors. Bei OGL-Start wird das Fenster
  entsprechend der gewünschten UI-Skalierung bei ausreichend
  Platz vergrößert und auf dem Monitor zentriert. Die
  `UIScaleFactor`-Vorgabe wird nicht direkt überschrieben;
  reicht die Fensterfläche nicht, begrenzt die Laufzeit
  den effektiven UI-Maßstab auf die notwendigen 1024×680
  logischen Einheiten. Das Linux-SDL-Backend positioniert
  Fenster jetzt tatsächlich über `SDL_SetWindowPosition`.
  Nach der initialen SDL-Größenmeldung erfolgt höchstens
  ein erneuter Resize-Abgleich, da das Fenster sonst
  auf die alten 1024×680 zurückfallen konnte. Die
  normale Fenstergrößenspeicherung der Viewer-Engine
  bleibt aktiv.
- **Reale GUI-Regressionen unter Linux Mint/Xvfb:**
  1280×720 bei gewünschtem 150 %: 1024×680, effektive
  Skalierung begrenzt, Login inklusive aller Bedienelemente
  vollständig im Fenster. 1920×1080 bei 150 %:
  **1536×1020 bei (+192,+30)**, alle Login-Felder und
  Links sichtbar. 3840×2160 bei 200 %:
  **2048×1360 bei (+896,+400)**, alle Login-Felder und
  Links sichtbar. Welcome-/Nachrichten-Webseite blieb
  unverändert. Alle drei Screenshots visuell geprüft und
  `STATE_LOGIN_WAIT` geloggt. Smoke-Prozesse bewusst
  per Timeout beendet (Exit 124); kein sauberer
  Logout-/Shutdown-Nachweis.
- **Block 17 abgeschlossen – Altbestand inventarisiert:**
  `panel_fs_login.xml` wird von `fspanellogin.cpp`
  weiterhin für einen nicht-OGL-Kompatibilitätszweig
  referenziert. `mode_combo` und `grid_builder_text`
  existieren in den aktuellen XUIs noch als im OGL
  ausgeblendete Kompatibilitäts-Controls und besitzen
  C++-Callbacks; die beiden vorhandenen StarLight-XUI-
  Dateien wurden bereits auf OGL-Login umgestellt.
  Vorhandenes Upstream-`client_list_v2.xml` und
  `cycle_object_animations_v2.lsl` nicht ungeprüft
  gelöscht. Vollständige sichere Entfernung der
  versteckten Altteile ist **Block 18**, noch nicht
  als abgeschlossen verbucht.
- **Build und Regression:** `bash ogl/scripts/build-test.sh`
  mit **129/129 Python-Tests**, **41/41 native CTests**
  und erfolgreichem realem Linux-Viewer-Build/Staging.
  `git diff --check` bestanden. Windows/macOS-
  Implementierungen wurden strukturell ergänzt,
  **aber nicht auf diesen Betriebssystemen kompiliert
  oder visuell getestet**; entsprechende Nachweise
  bleiben für Block 19 beziehungsweise spätere
  Release-Phasen offen.
- **Nutzerdaten und Sicherheit:** Keine neuen
  Quellprojektdateien angelegt, vorhandene
  Benutzerprofile/Secrets nicht verändert,
  die exakt zwei Grids und die Beta-Sperre
  unverändert belassen.

**Weiter:** `/goal P01 Block 18/20`. Kein manuelles
Nutzer-Abnahmegate. Block 19 darf ohne realen
Login-/Plattformnachweis nicht als vollständig
bestanden gelten.

## P01 – /goal-Runde: Block 18/20

**Code-Commit:** `450163aa` (`ogl-develop`).
**Sequenzieller Stand: 18/20, P01 weiter aktiv.**

- **Altes OGL-Login entfernt, nicht nur versteckt:**
  Aus **12 vorhandenen** `panel_fs_nui_login.xml`-
  Ressourcen (Standard-EN plus 9 Sprach-Overrides sowie
  StarLight und StarLightCUI) wurden die unnötige
  Firestorm/Phoenix-Modusauswahl `mode_selection`,
  `mode_combo` und der alte Grid-Builder-Link
  `grid_builder_text` direkt entfernt.
  Das behält die moderne OGL-Anmeldemaske, Welcome-Seite,
  zwei feste Grid-Werte, Kontofelder und Passwortschalter.
  Im C++-Login werden alte Mode-/Grid-Builder-Callbacks
  nur noch für den **Nicht-OGL-Kompatibilitätszweig**
  registriert; OGL erzeugt keine Dummy-Widgets.
- **Abhängigkeit/Lizenz:** Die bestehenden
  `panel_fs_login.xml`-Altmasken bleiben erhalten,
  weil `FSPanelLogin` sie im anderen Produktpfad
  weiterhin lädt. Die alten `onModeChange`- und
  Grid-Builder-Callbacks bleiben für genau diesen Pfad.
  Rechtlich erforderliche Originalquellen/Copyrights
  und belegte Theme-Assets werden nicht blind gelöscht.
  Die bereits vorhandenen `client_list_v2.xml` und
  `cycle_object_animations_v2.lsl` sowie
  `script_copy.png` sind keine neuen Ersatzdateien.
- **Verifikation:** Realer vollständiger Linux-Build
  und Staging erfolgreich, **130/130 Python-Tests**,
  **41/41 native CTest**, `git diff --check` grün.
  Xvfb 1280×720: **Standard, StarLight und StarLightCUI**
  sichtbar gestartet (je 1024×680 bei +128,+20),
  jeweils `STATE_LOGIN_WAIT` und **0 Dummy-Widgets**.
  Screenshots geprüft; keine alte Modusauswahl sichtbar.
  Die Smoke-Läufe liefen absichtlich in Timeout (124).
- **Block-19-Negativprüfung (vorbereitend):** Im
  echten OGL-Viewer **Stadt NexVerse Beta**
  ausgewählt, synthetischen Benutzernamen und
  Kunstpasswort eingegeben und Login ausgelöst.
  Eine deutschsprachige Sperrbenachrichtigung erschien;
  das Log bestätigt exakt
  `Blocked beta authentication: protocol and TLS not verified.`
  Keine echten Zugangsdaten verwendet. Diese Prüfung
  belegt **nicht** Hauptgrid-Anmeldung oder gespeicherte
  Credentials nach einem erfolgreichen Login.
- **Noch offen:** Testkonto-/Live-Login und tatsächliche
  Konto-/Passwortpersistenz am Hauptgrid, reguläres
  Schließen nach Login, Windows- und macOS-Build-/GUI-
  Nachweise; `Block 19` ist daher **nicht**
  vollständig abgeschlossen. P02 und P01 Block 20
  bleiben ausstehend.

**Weiter:** `/goal P01 Block 19/20`, ohne manuelle
Zwischenabnahme, aber mit echten technischen Prüfgates.

## P01 – /goal-Runde: Block 19/20 (technische Teilprüfungen)

**Code-Commit:** `6ac3c63f` (`ogl-develop`).
**Fortschritt unverändert: 18/20; Block 19 weiterhin in Arbeit.**

- **Korrektur 1 – über Startort-URL ausgelöster Gridwechsel:**
  Die Loginmaske konnte zuvor über einen Grid-tragenden SLURL
  auf ein anderes Grid wechseln, ohne alte Benutzernamen/
  Passwort-Eingaben zu verwerfen. Der bereits bestehende
  Reset von `onSelectServer()` und der SLURL-Pfad rufen
  jetzt dieselbe neue, in bestehenden C++-Dateien definierte
  Methode `clearLoginFieldsForGridChange()` auf.
  Ein unbekanntes SLURL-Grid wird explizit abgewiesen.
  Die gespeicherten Konto-Objekte werden nicht gelöscht.
- **Korrektur 2 – Credential-Log/Welcome:**
  Das volle Credential-Objekt wird nicht mehr in der
  Login-Logzeile ausgegeben. Null-Credentials werden
  vor Dereferenzierung abgewiesen. OGL ignoriert
  alte `ForceLoginURL`-Overrides im Benutzerprofil
  für die Welcome-Anzeige, ohne die Einstellung des
  Nicht-OGL-Kompatibilitätszweigs zu ändern.
- **Korrektur 3 – Voranmelde-Beenden:** Bei
  `STATE_LOGIN_WAIT` löst Strg+Q bei OGL
  `requestQuit()` direkt aus, statt die alte
  Abfragemeldung "Wirklich beenden?" zu öffnen.
  Die vorhandene Bestätigungs-/Logoutlogik nach
  Eintritt in die Welt wurde nicht verändert.
- **Build und Tests:** Vollständiger Linux-Mint-C++-
  Viewer-Build und Staging erfolgreich;
  **133/133 Python-Unittests** und
  **41/41 native CTests** erfolgreich. `git diff --check`
  bestanden. Die letzte vorherige GitHub-Matrix-CI
  für `bce9aeb7` war erfolgreich, prüft aber
  portable Module und Python-Quelltests, **keine**
  kompletten Windows-/macOS-Viewer-Installer.
- **Isolierte echte GUI-Regression:** 1280×720 Xvfb
  mit gesetzter fremder `ForceLoginURL` zeigte
  die korrekte NexVerse-Welcome-URI in Logs/Browser,
  keine fremde Navigation. Strg+Q wurde erkannt
  und der Viewer beendete sich im erneuten Test
  **regulär mit Exit-Code 0**, ohne Quit-Modal.
  Mit einem ausschließlich unter `/tmp` angelegten
  synthetischen Testprofil wurde "Benutzername
  speichern" deaktiviert. `FSRememberUsername`
  wurde als 0 gespeichert und nach echtem Neustart
  als deaktivierte Checkbox visualisiert. Auch
  der zweite Viewerstart endete mit Exit-Code 0.
  **Es wurde kein echtes Konto angemeldet und
  kein Passwort-Secret gespeichert oder übertragen.**
- **Negativprüfung/Bestandsprüfung:** Die bereits
  geprüfte Beta-Sperre, feste Zwei-Grid-Auswahl,
  HTTP-Zustimmungsregeln und OpenGL-Fallback
  bleiben durch Regressionstests abgedeckt.
  Wiederholte Duplikatprüfung: alte `_v2`-
  und `script_copy`-Ressourcen sind vorhandener
  und nicht ohne Nutzungsnachweis zu löschender
  Upstream-Bestand. Keine neuen Projektdateien.
- **Blocker für Block 19/20 und Phasenabschluss:**
  Kein autorisierter Live-Hauptgrid-Test mit
  erfolgreichem Login und Wiederherstellung
  gespeicherter Passwörter; kein nach-Welt-Login-
  Shutdown-Test; keine vollständigen Windows-/
  macOS-Viewer-Build-/Lauftests. Diese Nachweise
  ausdrücklich **nicht** als bestanden markieren.

**Weiter:** `/goal P01 Block 19/20`, mit Schwerpunkt
auf den fehlenden externen Ende-zu-Ende-Nachweisen.
Block 20/P02 bleiben bis zum P01-Abschluss-Gate gesperrt.

## P01 – /goal-Sammeldurchlauf: 20 technische Prüfblöcke auf einmal

**Code-Commit:** `1e4a6d4a` (`ogl-develop`).
**Ergebnis:** **20/20 zusätzliche, unabhängig bezeichnete
Prüfblöcke bestanden**. Dies sind **QA-/Entwicklungsprüfungen
innerhalb von P01 Block 19**, nicht die 20 Phasenblöcke
P01-1–20. **Operativer Phasenstand bleibt ehrlich 18/20.**

- **Bestehenden Quellbestand aktualisiert, ohne neue Projektdateien:**
  `indra/newview/fsdata.cpp` lädt für OGL keine veralteten
  Firestorm-/Phoenix-XML-Daten, Support-Agentenlisten oder
  Script-/Clienttag-Feeds über unverschlüsseltes
  `phoenixviewer.com` mehr. Bei gesperrten Downloads setzt
  OGL die vom Startautomaten erwarteten Flags
  `mFSDataDone` und `mAgentsDone`, damit kein
  Wartezustand entsteht. Nicht-OGL-Kompatibilitätscode
  und urheberrechtliche Hinweise bleiben erhalten.
  `indra/newview/llviewermedia.cpp` meldet als
  eingebetteten Browser-User-Agent jetzt `OGL skin`
  statt `firestorm skin`; der `SecondLife/`-Präfix
  bleibt für bestehende Web-/Media-Protokolle erhalten.
  In vorhandener `ogl/tests/test_branding_contract.py`
  Regressionstests dafür ergänzt.
- **Laufzeitbeweis:** Neues isoliertes Linux-Xvfb-Profil
  mit absichtlich gesetztem `ForceLoginURL` zu
  `example.invalid` und `FSdataQAtest=true`.
  Viewer-Fenster 1024×680 auf 1280×720 sichtbar,
  NexVerse-Welcome-URL geladen, `STATE_LOGIN_WAIT`
  erreicht, User-Agent
  `SecondLife/7.2.5.95 (OGL Viewer Development; OGL skin)`
  geloggt. `Skipped legacy Firestorm configuration downloads`
  protokolliert. Im Log **keine Phoenix-Downloadversuche**
  und **keine fremde Welcome-Navigation**. Strg+Q
  führte direkt zu `requestQuit`, Viewer beendete
  sich **regulär mit Exit-Code 0**.
- **20 tatsächlich bestandene Prüfblöcke dieser Runde:**
  01 korrekter Branch; 02 zwei feste Grids; 03 genau zwei
  Grid-Dropdown-Zeilen; 04 keine freie Grid-Adresse;
  05 Beta-GUI-Login-Sperre; 06 Beta-Autologin-Sperre;
  07 nur exakt gepinnte HTTP-Ausnahme nach Einwilligung;
  08 Abbrechen als Default; 09 Passwort-Reset beim
  Gridwechsel; 10 kein vollständiges Credential-Log;
  11 feste Welcome-URL; 12 keine Firestorm-Konfiguration;
  13 keine fremde Agentenliste; 14 keine Phoenix-Netzabfrage;
  15 OGL-Browser-Agent live; 16 echte NexVerse-Welcome-
  Navigation; 17 kein fremder URL-Override; 18 Fenster
  und Login-Wartezustand; 19 sauberer Voranmelde-Quit;
  20 vollständiger lokaler Test-/Build-Gate.
  Alle 20 wurden automatisiert anhand konkreter
  C++-Quellen, XUI, Laufzeitlogs und Testergebnisse
  geprüft; **keine** echte Passwortanmeldung vorgenommen.
- **Ergebnisse:** Reales Linux-Mint-Binary und Staging
  erfolgreich, **135/135 Python-Tests**, **41/41 native CTests**,
  `git diff --check` erfolgreich. Die vorherige
  OGL-Portable-Grafik-CI auf `754880b9` bestand
  auf sechs Runnern (Linux x64/ARM, Windows x64/ARM,
  macOS Intel/Apple Silicon), prüft dort jedoch
  **nur portable Module**, nicht den ganzen Viewer.
  Kein vollständiger Windows-/macOS-Installer nachgewiesen.
- **Sichere Altbestandkontrolle:** Nur bestehende
  Quelldateipfade verändert. Vorhandene
  `client_list_v2.xml`-, LSL-`_v2`- und
  `script_copy.png`-Ressourcen sind durch bloßen
  Namen nicht als unbenutzt nachgewiesen und
  wurden deshalb nicht gelöscht. Vollständige
  Release-/Signing-Workflows mit vorhandener
  Firestorm-Deploy-Stufe wurden **nicht**
  unbeaufsichtigt gestartet oder veröffentlicht.
- **Externe Blocker unverändert:** Erfolgreicher
  autorisierter Hauptgrid-Login, Speicherung und
  Wiederherstellung des Login-Secrets, Logout
  nach Eintritt in die 3D-Welt sowie vollständige
  Windows-/macOS-Viewer-Builds und Laufzeittests
  fehlen. Diese notwendigen P01-Abschlussnachweise
  können die 20 grünen Teilprüfungen **nicht**
  ersetzen.

**Nächster operativer Einstieg:** `/goal P01 Block 19/20`
mit den fehlenden Ende-zu-Ende-Nachweisen;
erst danach Block 20/P02 nach Abschluss-Gate.
Alle 20 Prüfungen in einer Sitzung abgearbeitet,
keine manuelle Zwischenabnahme eingeführt.

## P01 – /goal-Sammelrunde: 20 technische Arbeits- und Prüfblöcke

**Code-Commit:** `65e4b62e` auf `ogl-develop`.
**20/20 Prüfpositionen bestanden; der operative P01-Stand bleibt 18/20.**
Die geprüften Positionen sind echte Änderungen und Regressionen in
**Block 19**, nicht ein Ersatz für die P01-Phasenblöcke 19 und 20.

- **Linux-/Desktop-Rebranding:** In den bestehenden
  `llwindowsdl2.cpp`- und Desktop-Installerskripten
  der installierte App-Titel auf `OGL Viewer` geändert.
  `StartupWMClass=ogl-viewer`; SDL-SDL-Hints für
  X11/Wayland ergänzt. Der erste reale X11-Test zeigte,
  dass SDL-Hints allein **nicht** ausreichen. Deshalb
  am tatsächlichen X11-Fenster `XSetClassHint`
  ergänzt. **Zweiter realer Test:** X11-Klasse
  `("ogl-viewer" "ogl-viewer")` statt
  `do-not-directly-run-firestorm-bin`. Für Wayland
  wurde bislang nur ein Hint implementiert, **kein
  echter Wayland-GUI-Test** durchgeführt.
- **Bestehender Linux-Desktop-Installer:** Erzeugt
  `ogl-viewer.desktop` statt eines Firestorm-Menüeintrags
  und verwendet das bestehende ausführbare Programm
  (keine ungetestete Umbenennung von Binärdateien).
  Korrigierte Ausgabe/Fehlerbehandlung. Ein alter
  `firestorm-viewer.desktop`-Eintrag wird **nur**
  entfernt, wenn sein Exec-Pfad nachweislich zur
  exakt gleichen Installation gehört. Temporärer
  HOME-Test: alter eigener Eintrag migriert, korrekter
  Name/WMClass/Pfad, fremder Eintrag geschützt,
  wiederholte Installation idempotent. Reale
  Systeminstallation/Wayland-Verhalten noch offen.
- **Erkennbare Fehlermeldungen:** Noch sichtbare
  Firestorm-Download-/Supportlinks bei beschädigter
  Konfiguration und Netzwerkfehlern verwenden in
  OGL jetzt die bekannte NexVerse-Projektseite
  `https://stadt-nexverse.de/` und verweisen auf
  offizielle OGL-Builds statt fremde Downloads.
  Die Linux-Meldung zu nicht unterstützter
  32-Bit-True-Color-Anzeige nennt ebenfalls OGL.
  Der Nicht-OGL-Kompatibilitätszweig und
  Lizenzhinweise bleiben erhalten.
- **20/20 Prüfpunkte im selben Durchlauf erfolgreich:**
  01 Branch, 02 keine neuen Quelldateien, 03 feste
  Grids, 04 Beta-Sperre, 05 DE/EN-HTTP-Einwilligung,
  06 sicherer Gridwechsel, 07 Credential-Logging,
  08 12 Login-Overrides, 09 Desktop-Titel,
  10 Desktop-Klasse, 11 X11-Klassenimplementierung,
  12 Wayland-Hint (nur Codevertrag), 13 abgesicherte
  Desktop-Migration, 14 Installer-Fehlerbehandlung,
  15 OGL-Fehler-/Supporttexte, 16 OGL-True-Color-Text,
  17 720p GUI, 18 1080p/150-%-GUI,
  19 4K/200-%-GUI, 20 vollständiger
  Linux-Build samt Python/native Tests.
  Die sechs Installer-/Desktop-Simulationen
  liefen ausschließlich unter einem temporären HOME,
  ohne Nutzerprofile zu verändern.
- **Reale Viewer-Regressionsläufe:** 1280×720 bei
  150-%-Wunsch: Fenster **1024×680**.
  1920×1080 bei 150 %: **1536×1020**.
  3840×2160 bei 200 %: **2048×1360**.
  Bei allen drei Starts NexVerse-Welcome geladen,
  `STATE_LOGIN_WAIT`, **0 Dummy-Widgets** und
  Ctrl+Q mit Exit-Code **0**. Der X11-Klassenwechsel
  wurde am tatsächlich laufenden Viewer festgestellt.
- **Build:** `bash ogl/scripts/build-test.sh`
  vollständiges Linux-Mint-Binary und Staging erfolgreich,
  **136/136 Python-Unittests** und
  **41/41 native CTests** bestanden;
  `git diff --check` und Bash-Syntaxprüfung erfolgreich.
  Die vorherige portable Grafik-CI für
  `2ad45c91` war auf 6 Plattform-Runnern grün,
  deckt aber **keine kompletten Windows-/macOS-
  Viewer-Builds** ab.
- **Offene echte Gates:** Es fehlt ein ausdrücklich
  autorisierter realer Hauptgrid-Login,
  gespeicherte Passwortwiederherstellung nach Login,
  Shutdown nach Eintritt in die 3D-Welt und
  vollständige Windows-/macOS-Viewer-Builds/GUI-Tests.
  Nach diesen Nachweisen erst P01 Block 19/20
  abschließen und Block 20 sowie P02 freigeben.
  Paket-Signierung/GitHub-Release noch nicht
  implementiert. In der Entwicklung wurden
  keine neuen Projektdateien angelegt, bestehende
  Lizenzen und nachweislich benötigte Alt-Ressourcen
  nicht blind entfernt.

**Weiter:** `/goal P01 Block 19/20`; erneute
20er-Sammelprüfungen können unabhängige Risiken
abarbeiten, fehlende Live-/Plattform-Gates jedoch
nicht als erfolgreich fingieren.

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

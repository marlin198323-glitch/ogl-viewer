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

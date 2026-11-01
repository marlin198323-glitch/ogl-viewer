# Stadt NexVerse – zwei fest eingebaute Grids im OGL Viewer

Stand: 01.11.2026 | Branch: `ogl-develop`

## Feste Auswahl im Login

Die Grid-Profile sind zentral in `indra/newview/oglnexversegrid.h` definiert.
Der OGL-Viewer zeigt genau diese beiden Einträge an:

| Auswahl | Interner Schlüssel | Adresse | Anmeldung |
|---|---|---|---|
| Stadt NexVerse | `hg.stadt-nexverse.de:80` | `http://hg.stadt-nexverse.de:80/` | Bisheriger OpenSim-Login mit ausdrücklicher HTTP-Zustimmung |
| Stadt NexVerse Beta | `beta.stadt-nexverse.de` | `https://beta.stadt-nexverse.de/` | **Noch gesperrt – Protokoll und TLS nicht verifiziert** |

Die bestehende Welcome-Seite `https://stadt-nexverse.de/service/welcome`
bleibt erhalten. Der zuletzt gewählte Beta-Eintrag kann beim nächsten Start
wieder ausgewählt sein; unbekannte oder fremde Grids werden verworfen. Bei einer
frischen Installation bleibt **Stadt NexVerse** der Standard.

## Sicherheit beim Beta-Login

Die Beta-Adresse wurde am 01.11.2026 von einem separaten Rechner über HTTPS
geprüft. Die TLS-Prüfung wurde abgelehnt, weil der Zertifikatsname nicht zum
Hostnamen `beta.stadt-nexverse.de` passt. Ein OpenSim-kompatibler Login-Endpunkt
bzw. das geplante separate Beta-Protokoll ist ebenfalls noch nicht bestätigt.

Deshalb gilt ausdrücklich:

- Beta ist **auswählbar**, aber der Viewer überträgt dort **keine** Zugangsdaten.
- Bei einem Login-Versuch erscheint ein lokalisierter Hinweis (DE/EN).
- Zusätzlich blockiert `LLStartUp` unmittelbar vor
  `LLLoginInstance::connect()`, auch beim automatischen Login.
- Ein HTTPS-URL-Schema allein ist **kein** Beleg für ein gültiges
  Zertifikat oder ein passendes OpenSim-Authentifizierungsprotokoll.
- Erst nach Zertifikatskorrektur, dokumentiertem Login-Vertrag und
  eigenem Protokolladapter wird die Beta-Anmeldung gesondert freigegeben.
- Die Beta-URL darf niemals als unbestätigter XML-RPC-Endpunkt verwendet werden.

## Bestehender öffentlicher OpenSim-Login

Der Eintrag **Stadt NexVerse** funktioniert weiterhin nach der bisherigen
festen HTTP-Ausnahmeregel:

- Einzige zugelassene HTTP-Adresse: `http://hg.stadt-nexverse.de:80/`.
- Vor dem Login ausdrücklicher Hinweis mit „Einmal anmelden“,
  „Für dieses Grid merken“ und „Abbrechen“.
- Nur die explizit bestätigte exakte Adresse darf gespeichert werden;
  eine Änderung der Login-URI übernimmt niemals die HTTP-Freigabe.
- Diese HTTP-Verbindung ist weiterhin unverschlüsselt.
  Für eine spätere HTTPS-Umstellung sind getrennte Arbeiten notwendig.

OpenSim-Zugangsdaten und gespeicherte Benutzer sind über den jeweiligen
Grid-Schlüssel voneinander getrennt.

## Technische Umsetzung

- `oglnexversegrid.h`: beide festen Identitäten und URL-Konstanten.
- `fsgridhandler.cpp`: Registrierung nur der beiden Systemgrids,
  fremde und nachträglich überschreibende Grid-Definitionen werden verworfen.
- `fspanellogin.cpp`: Grid-Dropdown aktiviert und immer sichtbar;
  der Beta-Verbindungsversuch endet vor dem Lesen der Login-Felder.
- `llstartup.cpp`: zweite Sicherung unmittelbar vor der Anmeldung.
- `skins/default/xui/{de,en}/strings.xml`: lokalisierte Erklärung.
- `ogl/tests/test_nexverse_grid.py`: Regressionstests ohne Zugangsdaten.

Alle Änderungen liegen in gemeinsamem C++- und XUI-Code für Linux, Windows und
macOS; es gibt keine CPU- oder GPU-spezifische Sonderbehandlung.

## Lokaler Test auf Linux Mint

Nach neuer Build- und Paketierung:

```bash
cd "/home/jam/Schreibtisch/OGL-Viewer/ogl-firestorm"
bash ogl/scripts/build-test.sh
bash ogl/scripts/run-viewer-test.sh
```

Im Login-Dropdown sollten **Stadt NexVerse** und **Stadt NexVerse Beta**
erscheinen. Die Auswahl soll den zuletzt gewählten Eintrag speichern.
Die Beta-Auswahl darf **nicht** bis zum eigentlichen Netzwerk-Login gelangen.

**Abnahmestand:** Die Änderung wurde auf GitHub umgesetzt.
Der Linux-Mint-PC war beim Bearbeiten nicht mit Desktop Commander verbunden.
Deshalb sind lokale Kompilierung, Viewer-Paketierung und visuelle Abnahme
noch nicht bestätigt. Die GitHub-CI prüft die plattformübergreifenden
Modul-/Python-Verträge getrennt von einem vollständigen Viewer-Build.

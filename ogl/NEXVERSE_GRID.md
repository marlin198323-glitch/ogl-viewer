# Stadt NexVerse – festes Grid-Profil im OGL Viewer

Stand: 31.10.2026 | Branch: `ogl-develop`

## Feste Werte

Die Werte sind zentral in `indra/newview/oglnexversegrid.h` hinterlegt.

| Eigenschaft | Wert |
|---|---|
| Name | Stadt NexVerse |
| Grid-Schlüssel | hg.stadt-nexverse.de:80 |
| OpenSim-Login | http://hg.stadt-nexverse.de:80/ |
| Welcome | https://stadt-nexverse.de/service/welcome |
| Stadtportal | https://stadt-nexverse.de/ |
| Hypergrid-Basis | hop://hg.stadt-nexverse.de:80/ |

Die HTTP-Login-URI entspricht der öffentlichen GridInfo-Ausgabe des
Stadt-NexVerse-OpenSim-Servers. Port 443 ist derzeit nicht erreichbar.
Die vorhandenen Services auf dem Server wurden nicht verändert.

## HTTP-Login – ausdrücklich vom Betreiber gewünscht

- Einzige HTTP-Ausnahme: der **exakte** hinterlegte Grid-Schlüssel und
  die **einzige** URI `http://hg.stadt-nexverse.de:80/`.
- Beim ersten manuellen Login zeigt OGL einen deutschen oder englischen
  Hinweis zur unverschlüsselten Übertragung mit drei Möglichkeiten:
  „Einmal anmelden“, „Für dieses Grid merken“ und „Abbrechen“.
- Nur „Für dieses Grid merken“ speichert die ausdrückliche Zustimmung
  als exakte, öffentliche URI im normalen Viewer-Einstellungsprofil.
  Der Hinweis wird bei folgenden Starts für diese unveränderte URI
  nicht erneut angezeigt. „Einmal anmelden“ erlaubt nur den laufenden Prozess.
- Die Speicherung lässt sich jederzeit über die Debug-Einstellung
  OGLRememberPinnedHTTPLoginURI auf einen leeren Wert zurücksetzen.
  Wenn sich die Login-Adresse ändert, gilt die Zustimmung nicht mehr.
  Das Abschalten der wiederholten Frage verschlüsselt keine Verbindung.
- Das Authentifizierungsverfahren überprüft unmittelbar vor
  `LLLoginInstance::connect()` erneut die Zieladresse und Zustimmung.
  Ohne zuvor ausdrücklich gespeicherte Zustimmung bleiben automatische
  HTTP-Logins blockiert.
- Unbekannte oder zusätzliche HTTP-Adressen werden nicht erlaubt.
  Andere Grids bleiben in diesem OGL-Fork nicht auswählbar.
- Auch beim eigentlichen Verbindungsaufbau wird dieselbe feste URI
  erneut geprüft; andere HTTP-Grids bleiben gesperrt.
- Die bisherigen Funktionen zum Speichern von Benutzername/Passwort
  sind unverändert; HTTP schützt die Übertragung jedoch **nicht**.
  Zugangsdaten oder wiederverwendbare Authentifizierungswerte können
  auf dem Netzwerkweg mitgelesen oder manipuliert werden.
  Deshalb möglichst mit einem separaten Testkonto prüfen.

Das ist eine bewusste Kompatibilitätsausnahme für Stadt NexVerse.
Sie macht HTTP nicht sicher und ersetzt kein HTTPS. Optional sind
unter `ogl/server/` weiterhin geprüfte Vorlagen für eine spätere
HTTPS-Umstellung vorhanden.

## Testen auf Linux Mint

```bash
cd "/home/jam/Schreibtisch/OGL-Viewer/ogl-firestorm"
bash ogl/scripts/build-test.sh
bash ogl/scripts/run-viewer-test.sh
```

Oder auf dem Schreibtisch `OGL-Viewer-Test.desktop` starten.
Den Login selbst muss der Benutzer im Viewer bestätigen und testen;
keine Passwörter in Chatnachrichten oder Debugausgaben übertragen.

## Abnahmestand

- OGL-Fenster, feste Grid-Vorauswahl, Welcome-Seite und RTX-Erkennung:
  lokal bereits geprüft.
- HTTP-Freigabe und Bestätigungsdialog: implementiert; Build- und
  automatisierte Tests bestanden.
- Manueller Login über HTTP und tatsächliche Welt-/Avatar-Darstellung:
  noch durch den Benutzer abzunehmen.
- Raytracing-Schatten, Reflexionen und GI: noch nicht implementiert.

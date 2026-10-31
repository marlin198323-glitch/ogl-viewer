# Stadt NexVerse – festes Grid-Profil im OGL Viewer

Stand: 31.10.2026 | Branch: `ogl-develop`

## Zentrale Konfiguration

Die festen Werte stehen in `indra/newview/oglnexversegrid.h`.
OGL übernimmt sie beim Viewer-Start, ohne dass Benutzer eine Grid-Adresse
eingeben oder ein weiteres Grid im Manager hinzufügen müssen.

| Eigenschaft | Eingestellter Wert |
|---|---|
| Name | Stadt NexVerse |
| OpenSim-Grid-Schlüssel | hg.stadt-nexverse.de:80 |
| Veröffentlichte Login-URI | http://hg.stadt-nexverse.de:80/ |
| Welcome-Seite | https://stadt-nexverse.de/service/welcome |
| Webseite | https://stadt-nexverse.de/ |
| Hypergrid-SLURL-Basis | hop://hg.stadt-nexverse.de:80/ |

Quelle: offizielle, öffentlich erreichbare
`http://hg.stadt-nexverse.de/get_grid_info` (direkt geprüft).

Die öffentliche Grid-Info veröffentlicht für Registrierung, Passwort,
Hilfe und Über-uns derzeit lediglich die HTTP-Startseite. OGL bietet
dafür ersatzweise die existierende HTTPS-Startseite an. Eigene, nicht
verifizierte Unterseiten wurden nicht erfunden.

## Verhalten

- Das Grid ist beim Programmstart automatisch ausgewählt.
- Zusätzliche Grids aus Firestorms Online-, App- oder Benutzerliste
  werden im OGL-Fork nicht geladen. Die vorhandenen Konfigurationsdateien
  anderer Viewer werden nicht gelöscht.
- Gridwechsel und neue Gridregistrierungen werden für OGL abgelehnt.
- Der Grid-Auswahlschalter ist im Loginbereich deaktiviert.
- Benutzername und Passwort bleiben normale Benutzerfelder.
- Die vorhandene Firestorm-Anmeldespeicherung bleibt grundsätzlich
  erhalten, aber eine Anmeldung wird erst über HTTPS erlaubt.

## Sicherheitsblockade – noch offen

`hg.stadt-nexverse.de` antwortet auf Port 80 mit einer HTTP-Login-URI,
aber derzeit nicht auf TCP 443/HTTPS. Deshalb verhindert OGL sowohl
Anmeldungen über den Login-Button als auch den Aufruf des eigentlichen
Authentifizierungsdienstes aus automatischen Anmeldewegen.

Dieser Zustand schützt Passwörter, verhindert aber ausdrücklich einen
realen Login oder Test der 3D-Welt. Das ist **kein Viewerfehler**.
Für eine Freigabe muss Stadt NexVerse eine gültige HTTPS-Login-URI
veröffentlichen und einen TLS-terminierten Login-Endpunkt bereitstellen.
Danach `kLoginURI` in der zentralen OGL-Konfiguration auf die tatsächlich
geprüfte HTTPS-URI ändern und neu bauen.

## Linux Mint testen

```bash
cd "/home/jam/Schreibtisch/OGL-Viewer/ogl-firestorm"
bash ogl/scripts/build-test.sh
bash ogl/scripts/run-viewer-test.sh
```

Alternativ doppelklicken:
`~/Schreibtisch/OGL-Viewer-Test.desktop`.

Diagnose:
```bash
python3 ogl/scripts/ogl-test-report.py
```

Abnahme:
- C++-Build + Resource-Staging
- Viewer-Start, Loginmaske, NexVerse-Welcome
- Log `OGLGrid: Using fixed Stadt NexVerse OpenSim grid`
- Keine anderen auswählbaren Grids
- Sicherheitsregel HTTPS vor dem eigentlichen Authentifizierungsaufruf
- 9 Quellcode-Kontrakttests sowie 3 Modul-Tests
- Erst nach Server-TLS: echter Grid-Login, Avatar, Mesh, Texturen, Himmel

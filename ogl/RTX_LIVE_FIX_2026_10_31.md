# OGL RTX – Korrektur nach Live-Test vom 31.10.2026

**Ausgangslage:** Nutzer hat die 3D-Welt erfolgreich betreten. Der Log meldete Vulkan-Ray-Query-Hardware sowie gültige 3440×1390→8×8-Deferred-Depth-Readbacks, aber **keinen erfolgreichen GPU-Shadow-Probe** und keine Debug-Vorschau. Der Entwickler-Test war in den Prozessumgebungsvariablen korrekt aktiviert.

## Bestätigte Fehlerursachen

1. Die Geometriesuche traversiert jeweils nur einen rotierenden Teil der OpenSim-Objekte. Wenn kein verwendbares Objekt in diesem Teil gefunden wird, wurde fälschlich `ogl_rtx_submit_world_triangles(nullptr,0)` aufgerufen – das Modul behandelte dies als Regionswechsel. Tatsächlich war dieselbe Welt weiterhin geladen. Im Log wechselten 1.024 Dreiecke und scheinbar leere Szenen mehrfach.
2. Die Depth-Guided-RTX-Probe arbeitet asynchron auf einem *unveränderlichen Frame-Snapshot*. Nach dem Start konnten die normale laufende Kamerabewegung und neue Mesh-Teil-Samples den GPU-Job invalidieren, bevor sein Ergebnis auf dem Hauptthread eingelesen wurde.
3. Das Python-Diagnoseskript war veraltet: Es behauptete, Depth-Werte seien noch nicht mit Vulkan verbunden, obwohl die GPU-Diagnose-Schnittstelle bereits implementiert war. Außerdem war „no probe“ stets als fehlendes Opt-in formuliert, auch wenn Opt-in tatsächlich gesetzt war.

## Korrekturen

- Im Firestorm-Szenenscanner den **echten OpenSim-`getRegionID()`-Wechsel** verfolgen. Nur dann das bisherige RTX-Szenenbild freigeben. Ein leerer periodischer Objekt-Scan behält den letzten vorhandenen Mesh-Snapshot, damit in bevölkerten Regionen keine falschen Teleports ausgelöst werden.
- Depth-Guided-UI ist ausdrücklich eine mit `NOT WORLD SHADOWS` gekennzeichnete, auf acht Sekunden begrenzte Diagnose eines eingefrorenen GPU-Frames. Kamera- und Mesh-Teilscan-Änderungen dürfen den noch laufenden diagnostischen Snapshot nicht aufheben. **Echte Regionswechsel sowie ein neuer Tiefen-Snapshot invalidieren dagegen weiterhin alle alten GPU-Aufträge.**
- Der GPU-Arbeiter protokolliert bei Opt-in die numerischen Konfigurationsflags (worker, camera, depth_guided) und beim Einreihen eines Tiefen-Snapshots eine klare, anonyme Zeile.
- Der Diagnosebericht unterscheidet nun: GPU-Abschluss, gestarteter aber noch nicht abgeschlossener Auftrag, oder kein abgeschlossener Probelauf im Log. Ein Viewer ohne `Goodbye!`-Marker kann lediglich noch laufen; das ist nicht automatisch ein Absturz.
- Der echte GPU-Regressionstest simuliert Kameraänderung und einen geänderten Mesh-Teilscan nach Einreihen eines Depth-Snapshots. Die resultierende Maske wird trotzdem als Debug-Snapshot veröffentlicht; beim echten Szenen-Clear wird sie wieder entfernt.

## Grenzen

**Noch keine sichtbaren Raytracing-Schatten in der 3D-Welt**: Die Ausgabe bleibt eine optionale 8×8-UI-Diagnose. Der vollständige OpenSim-Mesh-Collector ist unvollständig und auf 1.024 Dreiecke beschränkt. Die GPU-Diagnose bleibt auf einmal pro Minute gedrosselt. Eine tatsächliche Live-Visuelle Abnahme der Korrektur ist noch erforderlich. Die neuen Tests ersetzen keine vollständige Nvidia-/AMD-/Intel-Validierung und keine volle Windows-Viewer-Abnahme.

## Erneut testen

Erst den alten OGL Viewer sauber schließen, dann im Linux-Mint-Terminal:

```bash
cd "$HOME/Schreibtisch/OGL-Viewer/ogl-firestorm"
bash ogl/scripts/run-rtx-preview-test.sh
```

In der 3D-Welt nach einer gültigen GPU-Probe die kleine Debug-Anzeige rechts oben prüfen. Anschließend den Viewer schließen:

```bash
python3 ogl/scripts/ogl-test-report.py
```

Der Bericht muss zwischen `worker=1`, „snapshot queued“ und der tatsächlichen abgeschlossenen GPU-Probe unterscheiden. Wenn trotz der Korrektur kein GPU-Ergebnis folgt, kann der Entwickler anhand dieser drei Stufen den nächsten Fehler präzise eingrenzen.

**Weitere separate Beobachtung:** In den alten Live-Logs gab es zahlreiche Texture- und CoreHttp-Warnungen. Sie betreffen primär Texturen/Asset-HTTP und dürfen nicht mit GPU-Ray-Query-Fehlern gleichgesetzt werden.

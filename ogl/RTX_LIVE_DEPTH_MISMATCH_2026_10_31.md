# OGL RTX – Tiefenabweichung nach echtem NexVerse-Live-Test

**Datum:** 31.10.2026 · **Branch:** `ogl-develop`

## Reales Fehlerbild

Der eingeloggte Linux-Mint-Viewer mit RTX 4070 SUPER meldete:

- OpenSim-GPU-Snapshot: 6.362 Dreiecke (später bis 8.192)
- OpenGL-Depth-Gitter: 47/64 sichtbare Oberflächen, 17/64 Hintergrund
- Vulkan-Kamera-Primärstrahlen: 37 von 47 Oberflächen treffen eine
  (nur teilweise erfasste) GPU-Beschleunigungsstruktur
- **0 von 47 Oberflächen** stimmen innerhalb 2 % oder 0,25 m mit
  der aus OpenGL rekonstruierten Entfernung überein
- Vulkan-Sonnenstrahlen: 0/64 Schattenhits, kein GPU-Absturz

Wichtig: `primary_hits>0` beweist nicht, dass der Vulkan-Primärtreffer
dieselbe Oberfläche sieht wie Firestorm. Die GPU-Probe erfasst nicht
alle Prims, Avatare, Alpha-/Skinned-Meshes und Terrain-Patches der Region.

Die originale Firestorm-Deferred-Utility verwendet in
`indra/newview/app_settings/shaders/class1/deferred/deferredUtil.glsl`
die Standard-OpenGL-Depth-Formel `linearDepth` und
`inv_proj`. Damit ist ein willkürlicher Wechsel auf Reverse-Z oder
lineare Z-Werte **nicht belegt**. Die reale Viewer-Projektion und
der zeitliche Zusammenhang zwischen Teilscan und Depth-Sample müssen
für die endgültige Korrektur separat validiert werden.

## Implementierung

1. Vulkan-Shader `depth_guided_sun.comp` ermittelt für gültige
   OpenGL-Depth-Pixel den nächsten GPU-Primärtreffer.
2. Zusätzlich zum unveränderten `depthMatch` gibt die GPU eine
   vierte 64-Zellen-Maske `primaryDepthRelation` zurück:
   `0` kein Treffer, `1` übereinstimmend,
   `2` Vulkan-Treffer näher, `3` Vulkan-Treffer weiter weg.
3. Vulkan-SSBO-Readback validiert vier statt drei 64-U32-Abschnitte.
   Pro GPU-Auftrag gilt unveränderlich
   `primary_hits == depth_matches + primary_closer + primary_farther`.
   Ungültige Kodierungen werden zurückgewiesen.
4. Versionierte Diagnoseschnittstelle `OGL_GPU_PROBE_VERSION 7`
   veröffentlicht `primary_closer` und `primary_farther`.
   Vulkan-BLAS/TLAS-ABI sowie das öffentliche OpenGL-Rendering
   bleiben unverändert.
5. Der asynchrone GPU-Worker übernimmt beide Kennzahlen mit
   dem zugehörigen, eingefrorenen Kamera-/Geometrie-/Tiefen-Snapshot.
   Die Resultate werden zusammen mit derselben GPU-Revision geloggt.
6. `ogl-test-report.py` zeigt nach dem nächsten Live-Test getrennt,
   ob die Vulkan-BVH vor oder hinter der sichtbaren GL-Oberfläche
   liegt. Bestehende Logs bleiben weiterhin lesbar.
7. GPU-Regression mit **identischer** perspektivischer GL- und
   Vulkan-Fläche: 64/64 passende Depth-Werte.
8. GPU-Regression mit absichtlich **fehlender Vordergrundfläche**
   in der Vulkan-BVH: 64/64 Vulkan-Treffer *weiter entfernt*
   als die GL-Oberflächen.
9. GPU-Regression mit absichtlich **zusätzlichem GPU-Hindernis**:
   16/64 Vulkan-Treffer liegen *näher* als die GL-Oberfläche.
   Sky-/Background-Pixel bleiben ohne Primärtreffer.
10. Vollständige Python-/Native-/OpenGL-Fallback- und Linux-Viewer-
    Abnahme inklusive Windows-/Ubuntu-CI.

## Auswertung des nächsten echten Logs

Bei `primary_farther > primary_closer`: Die GPU sieht überwiegend
eine zu weit entfernte Fläche. Kandidaten sind **fehlende vordere
Meshes** in der begrenzten BLAS, eine veraltete Mesh-/Depth-Revision,
oder eine Projektions-/Koordinatentransformation.

Bei `primary_closer > primary_farther`: Die GPU sieht überwiegend
ein Objekt vor der aus GL rekonstruierten Oberfläche. Kandidaten:
falsche Transformationsmatrizen, eine nur in der GPU vorhandene
Vordergrundfläche, LOD/Alpha-Transparenz oder fehlende Synchronisation.

Bei gemischten Resultaten: Teilweise nicht übereinstimmende
Szenenabdeckung und/oder mehrere unabhängige Geometriefehler.

**Keiner dieser Fälle beweist automatisch eine einzelne Ursache.**
Eine Änderung der Tiefenformel ohne Messung wäre eine Verschlechterung
der Diagnostik.

## Nächste Live-Abnahme

```bash
cd "$HOME/Schreibtisch/OGL-Viewer/ogl-firestorm"
bash ogl/scripts/run-rtx-preview-test.sh
```

Nach dem Betreten der OpenSim-Welt und einer abgeschlossenen
Vulkan-Probe:

```bash
python3 ogl/scripts/ogl-test-report.py
```

Neu im Bericht: `RTX depth disagreement: X Vulkan surfaces closer
than GL, Y farther`. Das Verhältnis der beiden Zahlen bestimmt,
ob als Nächstes der fehlende Vordergrund-Mesh-Collector oder
die GPU-/Firestorm-Projektion gezielt korrigiert werden muss.

**Noch kein sichtbares Raytracing der Welt:** Die Preview
ist weiterhin `NOT WORLD SHADOWS`. Es fehlen vollständige
BLAS/TLAS, hochaufgelöste GPU-Maske und direkte
Sonnenlicht-Integration. Keine OpenSim-Serveränderungen.

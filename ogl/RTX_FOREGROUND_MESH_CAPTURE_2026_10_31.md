# OGL RTX – 16. Entwicklungsschritt: Vordergrund-Meshes statt Objektlisten-Reihenfolge

**Datum:** 31.10.2026 · Branch `ogl-develop` · Firestorm/OpenSim

## Befund aus echter NexVerse-Sitzung

Der Nutzer meldete 6.645 RTX-Szenendreiecke und einen gültigen
Vulkan-GPU-Test mit **50/64** sichtbaren OpenGL-Oberflächen und
**37/50** Vulkan-Kamerastrahl-Treffern. Aber nur **2/50** Tiefen
stimmten überein. Bei allen **35** verbleibenden GPU-Treffern lag
die Vulkan-Oberfläche *hinter* der sichtbaren GL-Oberfläche;
kein Vulkan-Treffer war näher. Die Sonnen-Schattenprobe ergab 0/64.

Das passt zu **fehlender Vordergrundgeometrie** im noch
unvollständigen GPU-Snapshot. Es beweist nicht abschließend, dass
keine zusätzliche Projektions-/Materialabweichung vorliegt.

## Wesentlicher Scannerfehler

Der bisherige Firestorm-Scanner verarbeitet 4.096 Objektlisteneinträge
alle 180 Renderframes in ihrer internen Reihenfolge. Pro Volume
werden bis zu 16 Faces und bis zu 32 **erste** Indexdreiecke
je Face genommen. Ein einziges hochpolygonales Mesh kann deshalb
bis zu 512 von 8.192 Dreiecksplätzen beanspruchen, bevor
andere sichtbare Objekte geprüft werden. Mehrere große Objekte
können den 8.192er-Budgetbereich komplett ausfüllen,
während die tatsächlich im Bild sichtbaren Vordergrund-Meshes
fehlen. Der rotierende Scan-Cursor allein löst diese
Szenenlücke nicht.

## Änderungen

1. Neue portable `ogl_scene_sampling.h`: numerisch begrenzte,
   deterministische Verteilung von Dreiecksindices über **das
   ganze Mesh-Face**, ohne Integer-Overflow.
2. GPU-Entwicklermodus sammelt zuerst nahe (max. 90m) Kandidaten,
   statt die 8.192 Slots während der Objektlistentraversierung
   bereits aufzubrauchen.
3. Kandidaten werden nach Kameranähe, ungefährer Sichtrichtung
   und Objektgröße priorisiert; hinter der Kamera liegende
   Sonnenoccluder bleiben berücksichtigbar.
4. Im Entwicklungsmodus maximal **128 statt 512 Dreiecke pro
   Objekt**, damit mehr unabhängige Vordergrundobjekte
   bei gleichem GPU-Speicherbudget einen Platz erhalten.
5. Die bisherigen **32 Dreiecke pro Face** bleiben als
   Obergrenze erhalten, werden aber über die gesamte
   Indexliste verteilt statt nur am Anfang entnommen.
6. Nicht-rigged Attachments dürfen ebenfalls als
   Oberflächenkandidaten dienen; rigged Skins sind mangels
   deformierter aktueller Vertexdaten weiterhin ausgeschlossen.
7. Gesamtbudget von **8.192 Dreiecken** einschließlich
   begrenztem Terrain-Snapshot bleibt unverändert.
8. Objektlokale UUID-Registrierung und per-Frame-Poseupdates
   bleiben bestehen. Der erste normale OpenGL-Modus führt
   keinen erweiterten RTX-Scan durch.
9. Neuer anonymer Live-Diagnoseeintrag
   `OGL RTX scene selection: candidates=N sampled_objects=M
   mesh_triangles=K terrain_triangles=T cap=8192`,
   nur periodisch im Entwicklerbetrieb.
10. Native Tests für Scanpriorität, Randfälle, gleichmäßige
    Mesh-Samples und maximale Belegung; vollständiger
    Linux-Viewer-Build, Regressionen, Fallback und GitHub-CI.

## Einschränkungen

**Keine vollständige Welt-BVH**: Es werden weiterhin nur
Teilgeometrien in einer aggregierten Vulkan-BLAS/TLAS benutzt.
Viele Material-, Avatar-, LOD-, bewegte Rigged- und entfernte
Terrain-Meshes fehlen. Das neue Verfahren ist eine
*repräsentativere Teilaufnahme*, kein vollständiger Renderer.

Außerdem ist noch kein beweisender neuer NexVerse-Login-Test
mit den modifizierten Scans erfolgt. Die 35 zu weit hinten
liegenden GPU-Treffer können erst nach einem erneuten
Live-Test erneut bewertet werden. Der aktuelle Vulkan-GPU-Pfad
rendert weiterhin **keine echten Weltschatten**.

## Linux-Mint-Abnahme

Alten Viewer schließen und neuen Build starten:

```bash
cd "$HOME/Schreibtisch/OGL-Viewer/ogl-firestorm"
bash ogl/scripts/run-rtx-preview-test.sh
```

Danach bei eingeloggter Welt und einer abgeschlossenen
Vulkan-Probe:

```bash
python3 ogl/scripts/ogl-test-report.py
```

Wichtige Vergleichswerte:
- `RTX scene selection: ...` – wie viele der nahen Objekte
  passen in das begrenzte Dreiecksbudget?
- `RTX primary-ray geometry coverage` – wie viele der
  8×8-GL-Oberflächen sind auch in Vulkan vertreten?
- `RTX OpenGL/Vulkan depth agreement` – wird die
  Vordergrundoberfläche korrekt erfasst?
- `RTX depth disagreement` – werden die **GPU-farther**-
  Treffer gegenüber zuvor 35 weniger?

Der Snapshot wird weiterhin im Entwicklerbetrieb gedrosselt.
Der optische Hinweis `NOT WORLD SHADOWS` bleibt korrekt.

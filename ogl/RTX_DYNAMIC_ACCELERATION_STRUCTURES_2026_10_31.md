# OGL Viewer – Dynamische BVH/Acceleration Structures

**Stand:** 31.10.2026 · **Entwicklung:** `ogl-develop`

## Ergebnis dieser Runde

Die Vulkan-Ray-Query-Komponente führt echte GPU-Beschleunigungs-
strukturen aus. Vor dieser Runde wurde bei jeder Änderung eines
Dreiecks der BLAS-Bestand vollständig ersetzt und eine neue
einzelne TLAS aufgebaut. Das ist für animierte Geometrie und
bewegte OpenSim-Objekte langfristig zu teuer.

**Neu implementiert und auf der RTX 4070 SUPER getestet:**

1. **Vulkan-BLAS-Refit:** Der Erstaufbau setzt
   `VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR`.
   Bei unveränderter Dreiecksanzahl und geänderten Vertexpositionen
   kann die vorhandene BLAS mit
   `VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR` in-place
   aktualisiert werden. Die BLAS-GPU-Adresse und ihre TLAS-
   Referenz bleiben bestehen. Änderungen der Dreiecksanzahl
   führen weiterhin zum vollständigen Neuaufbau.
2. **Vulkan-TLAS-Refit (eine Instanz):** Die TLAS wird ebenfalls mit
   `ALLOW_UPDATE` gebaut. Bei reinen Änderungen der
   Instanz-Translation wird die vorhandene TLAS in-place
   aktualisiert, ohne die BLAS neu aufzubauen. Der GPU-Test
   bewegt die Instanz aus dem Strahlenbereich und zurück;
   die erwarteten Vulkan-Ray-Hits ändern sich entsprechend.
   Dieser GPU-Prototyp hat **erst eine TLAS-Instanz**, nicht
   eine Instanz pro Prim.
3. **GPU-Speicherverwaltung:** Build- und Update-Scratchgrößen
   werden vor der ersten Vulkan-Beschleunigungsstruktur-
   Erstellung berücksichtigt. Adressmasken verwenden 64-Bit-
   `VkDeviceSize` und vermeiden abgeschnittene GPU-Adressen.
   Die GPU-Befehle werden ausschließlich auf dem asynchronen
   Entwicklungsworker ausgeführt; dieser Prototyp wartet intern
   weiterhin auf die GPU-Queue und ist **nicht** frame-rate-ready.
4. **UUID-basierte Szenenregistrierung:** Neue portable
   `ogl::DynamicScene` verwaltet Objektgeometrie anhand der
   vollständigen 16 Bytes der OpenSim-UUID. Einträge haben
   lokale Dreiecke, 3×4-Transform und voneinander getrennte
   Geometrie- und Transformationsgenerationen.
   `NewInstance`, `GeometryChanged`, `TransformChanged`,
   `Removed` und `Unchanged` werden unterschieden.
   Der Speicher ist auf 1.024 Einträge und zusammen
   8.192 lokale Dreiecke begrenzt.
5. **C-Modul-ABI:** `ogl_rtx_track_object`,
   `ogl_rtx_track_transform`, `ogl_rtx_forget_object` und
   Statusfunktionen für Eintragszahl/Revisionen sind vorhanden,
   validieren Eingaben und verändern im Normalbetrieb keine
   OpenGL- oder Vulkan-Pipeline.
6. **Firestorm-Objektscanner:** Im ausschließlich aktivierten
   `OGL_RTX_SCENE_PROBE=1`-Entwicklermodus werden bereits
   gescannte Prim-/Volume-Faces zusätzlich in **lokalen**
   Objektkoordinaten gespeichert und mit ihrer UUID und
   dem lokalen-zu-Agent-Transform registriert.
   Ein begrenzter Satz bereits erfasster Objekt-UUIDs wird
   pro Renderframe nachgeführt: Position, Drehung und Skalierung
   werden ohne erneuten Mesh-Face-Walk aktualisiert.
   Gelöschte Objekte und echte Regionswechsel werden entfernt.
7. **Portierbare Tests:** Stabile UUID-Unterscheidung, reine
   Transform-Updates ohne Geometriegeneration, Vertexänderungen,
   Löschungen, begrenztes Budget, NaN/ungültige Transforms.
8. **GPU-Tests:** BLAS-Refit statt BLAS-Neuaufbau, TLAS-Refit
   bei transform-only, Cache-Hits und vollständiger BLAS-
   Neubau bei anderer Primitivezahl.
9. **Hardware-Fallback:** Native Tests ohne Vulkan-Treiber
   dürfen nicht benötigte GPU-Hardwaretests überspringen,
   während die dynamische UUID-Verwaltung weiterhin getestet
   werden kann. Der öffentliche Viewer läuft weiterhin mit OpenGL.
10. **Viewer-Build und portable CI:** Vollständiger Linux-Build,
    Shader-Assets, Native- und Python-Tests sowie Ubuntu-/Windows-
    Modultests bilden die Freigabegrenzen.

## Was noch NICHT implementiert ist

- **Noch keine TLAS mit einem BLAS pro eigenständigem Prim/Mesh/
  Avatar/Gelände-Patch.** Die aktuelle Live-GPU-Probe nutzt
  weiterhin *eine aggregierte BLAS mit einer TLAS-Instanz*.
  Die UUID-basierte CPU-Szenenregistrierung ist vorbereitet;
  sie steuert diese aggregierte Vulkan-TLAS noch nicht.
- **Keine vollständige OpenSim-Szenenabdeckung.**
  Der Viewer scannt nur eine begrenzte Auswahl von
  Volume-Faces und vereinfacht Terrain (100×100-m-Ausschnitt).
  Rigged-/Skinned-Avatar-Meshes, Attachment-Geometrie,
  Material-Alpha sowie weit entfernte Prims fehlen.
- **Kein garantiertes GPU-Refit in jedem Frame.** Zwar werden
  bereits erfasste Objekt-Transforms pro Frame auf der CPU
  aktualisiert, aber das asynchrone Vulkan-GPU-Probe-System
  bleibt für Diagnosezwecke auf ungefähr einen Job pro
  Minute begrenzt, nutzt `vkQueueWaitIdle` und rendert
  keine vollaufgelösten Weltschatten.
- **Keine fertige World-Raytracing-Beleuchtung.** Die
  rot/türkise/graue Anzeige bleibt ausschließlich
  `NOT WORLD SHADOWS`. Die physikalisch sinnvolle Integration
  muss direkt in Firestorms Deferred-Sonnenlichtpass erfolgen,
  mit Tiefe, Normalen, Material-/Alpha-Regeln, synchronisierten
  Frame-Ressourcen, GPU-Budget und Qualitäts-/Fallback-Stufen.

## Nächste GPU-Integrationsschritte

Objektlokale BLAS-Caches nach Mesh-/LOD-Identität aufbauen;
eine TLAS mit mehreren `VkAccelerationStructureInstanceKHR`
erstellen und beim Objekttransform ohne BLAS-Änderung
aktualisieren; Avatar-Skinned-Meshes anhand ihrer
aktuellen GPU-Vertexdaten refitten; robuste Änderungs-
und Löschereignisse sowie asynchrone Vulkan-Timeline/
Fence-Synchronisation einführen. Anschließend profilen
(NVIDIA, AMD, Intel) und erst danach Schattenwerte in
die Sonnenlichtstufe übertragen.

**Keine Änderungen am OpenSim-Server.**

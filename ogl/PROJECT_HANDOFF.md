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

**P05 Block 2 (04.11.2026):** Mit dreifachem Dev-Opt-in
bleiben VkSurfaceKHR und VkSwapchainKHR am echten SDL/X11-
Fenster bestehen. SDL-Resize-Hook baut die Swapchain
bei 1230x820 und 1050x680 nachweislich neu auf;
Destroy-Hook gibt die WSI vor SDL-Abbau frei. Beide
GUI-Resize-Smokes liefen bei OpenGL bis Login-Wait
und Exit 0. Linux-Build, 324 Python-/41 native Tests
grün. P05 jetzt 2/20; Block 3 Vulkan-GPU-Frame-
Synchronisation und echtes Present offen.

**P05/3 Vorarbeit (04.11.2026):** Im bestehenden
Vulkan-Modul developer-only GPU-Queue-Test mit
2 VkCommandBuffers, 2 VkFences und echten
vkQueueSubmit/vkWaitForFences. Das neue C-ABI-
Symbol meldet bei funktionierender GPU 2,
sonst 0. Native Tests und zwei GUI-Loginstarts
bei 2/2 und 0/2 ohne Vulkan-ICD zeigen
OpenGL-Fallback und Exit 0.
Linux-Vollbuild, 325 Python-/41 CTests grün.
P05 weiter 2/20: echter Image-Acquire/
Present und Vulkan-3D-Welt fehlen.

**P05/4 GPU-Geometriestaging (04.11.2026,
noch keine Vulkan-Welt):** Native Vertex-/Index-
VkBuffer samt HOST_VISIBLE|COHERENT VkDeviceMemory
für bestehende OpenSim-XYZ-Dreiecke, bis 8192
Dreiecke/24576 Vertices, RT-unabhängig.
Transaktionales Update, invalides NaN/Overlimit
abgewiesen, Region-Clear und Modul-Shutdown
geben Ressourcen frei. Entwickler-Opt-in
OGL_VULKAN_WORLD_MESH_UPLOAD=1 bei laufendem
OGL_VULKAN_WORLD_BOOTSTRAP=1. Die bestehende
Szenenerfassung kann dadurch auch Vulkan-Geräte
ohne RTX bedienen, nur im Entwicklerbetrieb.
Real-GPU-Test, Linux-Vollbuild, 326 Python-/
41 native Tests und isolierter GUI-Smoke bestanden.
In-World-Szene und Vulkan-Draws nicht geprüft.
P05 weiter 2/20, Block 3 Present offen.

**P05/3 Weiterarbeit 05.11.2026:** GPU-Renderpass
erzeugt nun echte Vulkan-Offscreen-Pixel (32x32
RGBA8) mit Image-/Host-Speicherbarrieren,
VkBuffer-Rücklesung und Farbverifikation an drei
Pixelpositionen, nur unter Opt-in
OGL_VULKAN_OFFSCREEN_FRAME_PROBE=1.
Zweimaliger GPU-Test grün; Linux-Vollviewer
gebaut, 327 Python-/41 native Tests bestanden,
drei echte Xvfb-GUI-Smokes: GPU 1/1, fehlender
ICD 0/1, Standard kein Probe, stets GL-Login/
Exit 0. Noch KEIN Swapchain-Acquire/Present
oder NexVerse-Vulkan-Weltframe. P05 bleibt
2/20, Block 3 offen. Neue optionale C-ABI-
Funktion ogl_vulkan_world_render_offscreen_probe.

**P05/3 Vulkan-WSI-Present-QA (05.11.2026, Commit e492f8be):**
20 reale Entwicklungs-/Prüfschritte im bestehenden Projekt, aber
kein P05-Phasenabschluss. Ein ausdrücklich eingeschalteter nativer
X11-Test erzeugt eine eigene reine Vulkan-Window-Swapchain,
erwirbt und rendert das erste und ein nach Resize neu erzeugtes
zweites Swapchain-Bild und präsentiert beide tatsächlich.
Owner-/Opt-in-/Doppelpresentschutz und fehlender Vulkan-ICD
negativ geprüft; 328 Python-/41 CTests, Linux-Vollviewer-Build
7.2.5.229 und isolierter GL-Viewer-Login/Exit 0 erfolgreich.
**Der echte Viewer hat weiterhin keine Vulkan-World-Präsentation.**
P05 bleibt 2/20, Block 3 offen; nächster Schritt exklusives
Fenster-/Present-Ownership und Multi-Frame-Synchronisation im SDL-Viewer.
Windows-/macOS-Full-Viewer-Test noch nicht freigegeben.

**P05/3 Vulkan-Multiframe 20er-Runde vom 05.11.2026 (Code 1428a4d9):**
Auf einem isolierten nativen X11-Fenster 16 reale Vulkan-
Swapchain-Presents am Stück mit Pixelprüfung 16/16,
RGB-Wechsel, 17. Frame abgelehnt, Resize/Reattach
und Rückkehr zum Einmal-Present geprüft. Vollständiger
Linux-Build, 329 Python-/41 native Tests sowie zwei
isolierte GL-GUI-Smokes mit Exit 0 bestanden.
Das ist weiterhin KEIN Vulkan-World-Frame im echten
SDL-Viewer; GL bleibt der einzige aktive Welt-Renderer.
**P05 formal 2/20, Block 3 offen.** Nächster Schwerpunkt:
exklusives OpenGL->Vulkan-Fenster-Ownership im Viewer,
danach P05/4 Terrain/Mesh. Keine neuen Projektdateien.

**P05/3 echter SDL-Pre-GL-Present (05.11.2026, Code 7c533cc6):**
Eine weitere 20er-Runde weist erstmals echtes Vulkan-
GPU-Present im tatsächlichen SDL/X11-Viewerfenster
vor Erstellung des GL-Kontexts nach: 1/1,
pixel=verified. Danach GL-Login/Exit 0. Vier
negative GUI-Fälle erfolgreich, Linux-Vollbuild,
330 Python-/41 native Tests, zusätzlich nativer
16-Frame-Present-Test. Kein Vulkan-Welt-Renderer,
kein dauerhafter Live-Engine-Wechsel. P05 2/20,
Block 3 offen: SDL-Fenster-Ownership mit dauerhaftem
Frame-/Resize-Lifecycle und GL-Fallback als nächstes.

**P05/3 Fortsetzung 05.11.2026 (Code aa71c8dd):**
20 technische Arbeitsschritte mit einer echten
20-Frame-Swapchain-Probe auf dem tatsächlichen
SDL/X11-Viewerfenster. Drei Fenstergrößen-
Lebenszyklen (8+8+4 Frames), je Frame sichtbare
RGB-Pixelprüfung, Present-Zähler und sicherer
Detach vor OpenGL. Positiv-Xvfb-GUI 20/20, Pixel-
und Resize-Test bestanden, GL-Login und Exit 0.
Einmal-Present, fehlender Vulkan-ICD und fehlende
Hauptfreigabe mit korrekt erhaltenem OpenGL geprüft.
Linux-Vollbuild und 331 Python-/41 native Tests
bestanden. Die Vulkan-Testbilder werden ausschließlich
VOR GL gerendert: KEIN Vulkan-World-Renderer.
P05 formal 2/20, Block 3 offen. Nächster Schritt:
exklusives Live-Renderer-Ownership, GPU-Frame-/
Resize-Lifecycle und zuverlässiger GL-Fallback.

**P05/3 Fortsetzung 05.11.2026 (Code 25787548):**
Weitere 20 technische Teilaufgaben umgesetzt/geprüft:
100 tatsächliche X11-Pixel aus 20 Vulkan-Presents
auf der echten SDL-Viewer-Window-ID, über drei
Window-/Swapchaingrößen. XGetGeometry gegen SDL-
Fenstergrößen; Fehlereinleitung nach echten
GPU-Frames in Phase initial/resized/restored.
Sieben echte GUI-Tests erfolgreich, darunter
20/20 positive Frames sowie 3/20, 11/20, 19/20
abgebrochene Frames mit normalem GL-Login und
Exit 0. Invalides ICD, Einmalmodus und
komplett deaktivierte Probe ebenso okay.
Linux-Vollbuild, 332 Python-/41 native Tests.
Vulkan weiterhin nur Entwicklerprobe VOR
OpenGL, keine echte Vulkan-OpenSim-Welt.
**P05 2/20, Block 3 offen.** Nächster
Schritt echte Vulkan-Raster/Draw-Pipeline
und anschließende Live-Engine-Ownership.

**P05/3-4 echter Vulkan-Indexed-Draw (05.11.2026, Code 410f7d9a):**
20 technische Schritte. Echte VkGraphicsPipeline mit
Khronos-valider, eingebetteter SPIR-V-Vertex-/Fragment-
Pipeline; VkVertex/VkIndexBuffer und VkCmdDrawIndexed
zeichnen ein rotes Testdreieck mit GPU-X11-
Pixelnachweis: Mitte 237/48/38, Hintergrund 20/54/115.
Auch vor GL-Kontexterzeugung in der tatsächlichen
SDL-Viewer-Window-ID nachgewiesen. Raster-Opt-in,
ohne-Mesh-/ungültiger-ICD-Fallback und vorheriger
Clear-Test: 5 echte Viewer-GUI-Fälle bis GL-Login/Exit 0.
Linux-Vollbuild, 334 Python-/41 native Tests.
Keine neuen Projektdateien, kein zusätzliches
Programm für den Betrieb. Wichtig: nur CLIP-SPACE-
Entwicklerdreieck VOR OpenGL; KEIN Vulkan-World-
Renderer, keine echte OpenSim-Kamera-Matrix/Materialien.
P05 formal 2/20; Block 3/4 nicht abgeschlossen.
Nächstes Gate: Projektion echter Weltgeometrie,
dauerhaft exklusives Vulkan-SDL-Fenster-Ownership,
produktive Frame-/Swapchain-Synchronisation.

**P05/3–4 Kamera-/World-XYZ-Projektion (05.11.2026, Code 439b8a27):**
Wieder 20 nachweisbare technische Aufgaben. Der Vulkan-Vertexshader
verarbeitet echte World-XYZ per neuer v3-Kamera-ABI und
64-Byte-Push-Constant-Matrix. CPU-Validierung: Kamera-FOV,
Clips, Achsen, Aspect, Händigkeit und Stale-Kamera-Invaliderung.
Nativer GPU-Test: dieselben XYZ werden aus einer Testkamera
sichtbar gerastert; geänderte Kamera ohne neuen Vertexupload
entfernt das Dreieck aus der Bildmitte (visible=1 moved=1
invalid=1). Tatsächliches SDL/X11-Viewerfenster vor GL:
1/1 presented, Pixel und Kamera verifiziert, dann
OpenGL-Login/Exit 0. 8 neue und 7 vorherige GUI-Fälle
bestanden; Linux-Vollbuild, 335 Python-/41 native Tests,
portabler SPIR-V-Binärvergleich grün. Synthetische World-
Daten, **keine echte OpenSim-In-World-Darstellung**, keine
Vulkan-Live-Session. P05 bleibt 2/20, Block 3/4 noch offen.
Nächstes Gate: echte aktuelle Kameraframe/Region-Mesh-ABI
in Vulkan-eigener Render-Session, nicht vor GL-Boot.

**P05 Live-Kamera-und-Mesh-Eingangsbridge (05.11.2026, Code ae00535c):**
20 technische Teilaufgaben im bestehenden Viewer-/RTX-Modul-
Quellcode. Der echte Firestorm-Deferred-World-Pass übergibt
die bisher RTX-gated Kamera nun zusätzlich an Vulkan
OHNE Raytracing-Hardwarevoraussetzung, nur unter drei
Dev-Flags für Bootstrap, GPU-Meshupload und Kamera.
Vorhandener echter OpenSim-Regionsgeometrie-Capture
wird als VkVertex-/VkIndexbuffer gestaged; neuer Mesh-
Snapshot und Region-Clear invalidieren die alte Kamera.
Neue read-only C-ABI meldet erst ein validiertes Kamera-
nach-Mesh-Eingangspaar, zählt nur angenommene Inputs.
Native Vulkan-GPU-Abnahme einschl. ungültiger Kamera,
Meshwechsel, Region-Clear/Shutdown erfolgreich.
Linux-Vollbuild, 336 Python, 41 native Tests,
12/12 isolierte GUI-Login-/GL-Fallback-Tests grün.
WICHTIG: Keine authentifizierte OpenSim-Region im
Test betreten, deshalb kein tatsächliches In-World-
Camera+Mesh-Paar im GUI-Log bestätigt; noch keine
Vulkan-Weltbilder, Vulkan-Weltengine bleibt gesperrt.
P05 operativ 2/20. Nächstes Gate echte In-World-
Eingänge nachweisen, dann alleiniger Vulkan-
Window-/Frame-/Present-Besitz mit Fallback.

**P05/3–4 Vulkan-Depth-Buffer, 05.11.2026 (Code cf2cc17f):**
20 technische Teilaufgaben umgesetzt und geprüft.
Realer Vulkan-GPU-Z-Test per eigenem VkDepthImage,
D32_SFLOAT/D16_UNORM, VkRenderPass-Depth-Attachment,
Framebuffer-/Pipeline-Depth-State, LESS-Depth-
Vergleich und Depth-Clear 1.0. Native GPU-Prüfung
mit identisch überlappenden Dreiecken: nah rot
Z=.25 zuerst, fern grün Z=.75 zuletzt; Vordergrund
bleibt rot bei 512x384 und 640x400 mit Resize.
Auch im echten SDL/X11-Viewer-Window nur VOR
SDL_GL_CreateContext mit separatem Depth-Opt-in
geprüft: 1/1 Present, Depth/Pixel verified,
GL-Login/Exit 0. Insgesamt 12/12 isolierte
GUI-Starts, Linux-Vollbuild, 337 Python-Tests
und 41 native CTests bestanden.
Keine authentifizierte OpenSim-Live-Welt abgenommen,
kein vollständiger Vulkan-Renderer oder produktiver
Live-Frame-Loop. P05 formal 2/20, Block 3/4 offen.
Nächster Gate: echte Welt/Depth-/Kameraframe-
Integration und dauerhafter SDL-Vulkan-Window-
Owner mit sicherem GL-Fallback.

**P05/3–4 – gemeinsame World-Z-Kamera/Depth-GPU-Prüfung (05.11.2026, c34a0d44):**
20 technische Aufgaben umgesetzt/überprüft.
Vulkan-World-to-Clip-Projektion + GPU-Depth-Buffer
zeichnen nun zwei synthetische Welt-XYZ-Dreiecke mit
identischer Projektion und unterschiedlichen Tiefen:
nahes rotes Dreieck vorn (GPU-Z=.25), entferntes
grünes hinten (Z=.75, zuletzt gerendert).
Negativkontrolle mit demselben GPU-Shader aber
OHNE Depth-Image zeigt korrekt die grüne Rückfläche.
GPU-Gates: nahe Fläche, Negativkontrolle,
Kameraverschiebung ohne neue Meshbuffer, Resize
512x384 -> 640x480, ungültige Kamera und Region-
Clear alle bestanden. Echte SDL-Viewer-XID vor GL
camera/depth/pixel=verified; 14/14 isolierte
GUI-/Fallback-Starts inkl. fehlender Freigaben
und ungültigem Vulkan-Treiber mit GL-Login/Exit 0.
Linux-Vollbuild, 338 Python-/41 native Tests
sowie validierte SPIR-V-Shader bestanden.
Das sind ENTWICKLER-GPU-Testwerte, keine echte
OpenSim-Region oder Laufzeit-World-Engine.
P05 operativ 2/20, Block 3/4 offen. Nächstes
Gate: echte eingeloggte In-World-Snapshots und
exklusive Vulkan-Live-Present-Frame-Synchronisation.

**P05/3 Zwei-GPU-Submits und WSI-Kamera-Aspect (05.11.2026, 49b28a90):**
20 technische Teilaufgaben. Die Vulkan-Sync-Diagnose
führt zwei VkQueueSubmits vor EINEM begrenzten
wait-all-Fence-Wait aus. Drei read-only-ABI-
Zähler zeigen 2 eingereihte und 2 abgeschlossene
GPU-Fences bei maximal 2 ungeprüften Submissions.
Keine simultane GPU-Ausführung oder Weltpräsentation
wird damit behauptet. Bei 4:3->16:9 X11-Resize
wird der alte Welt-Kameraframe ungültig; nach
gültiger 16:9-Kamera wieder korrekte GPU-Pixel.
5 neue SDL-Dual-Sync-GUI-Fälle und 14 ältere
GUI-Regressionen bestanden, alle GL-Login/Exit 0.
Linux-Vollbuild, 339 Python- und 41 native
Tests bestanden. Achtung: Der separate
VkQueuePresentKHR-Entwicklerpfad verwendet
weiterhin vkQueueWaitIdle. Es existiert KEINE
produktive Vulkan-Live-Welt und keine
authentifizierte OpenSim-In-World-Abnahme.
P05 weiterhin 2/20, Block 3/4 offen.
Nächster Gate: persistent verwaltete Frame-
Fences/Present-Semaphores/Swapchain-Images
und echte In-World-Frames.

**P05/3 persistent X11 Vulkan WSI (05.11.2026, Code 8f33196e):**
20 technische Arbeitsschritte mit echtem GPU-Nachweis.
Neuer opt-in Vulkan-Clear-Pfad hält VkRenderPass,
Framebuffers pro VkSwapchain-Image, VkCommandPool,
zwei Command-Buffer/Fences/Acquire-Semaphores und
Image-indexierte Present-Semaphores über 16 Bilder.
KEIN vkQueueWaitIdle pro Frame; genau EIN
Queue-Idle pro Detach vor dem Freigeben der
Present-Objekte. Native GPU: 16 Frames, sichtbare
abwechselnde Pixel, Resize/Neuaufbau, builds=2,
releases=2, teardown_idle=2 bestanden.
Echtes SDL/X11-Viewerfenster VOR OpenGL:
20 Frames, 100 X11-Pixel, drei Größen und
anschließend GL-Login/Exit 0. 7 neue + 19
alte isolierte GUI-Fälle erfolgreich; Linux-
Vollbuild, 340 Python-/41 native Tests grün.
Windows-Vorläufer-CI war ROT: MSVC C2589/C2059
durch max-Makro bei std::max. Portable Schreibweise
im Code angepasst, Windows-GitHub-CI ausstehend.
Nur synthetischer Clear-Renderer, KEINE echte
OpenSim-In-World-Vulkan-Welt, keine produktive
Vulkan-Engine. P05 formal 2/20, Block 3 offen.
Nächster Fokus: bestehende Welt-Raster- und
Depth-Pipeline in die persistente WSI übernehmen;
reale OpenSim-Kamera-/Regionsmesh-Frames prüfen.

**P05 persistentes Vulkan-World-Depth-Raster (05.11.2026, Code 6734cdef):**
20 konkrete technische Schritte. Der persistente Vulkan-Swapchain-
Pfad rendert echte indexierte GPU-Dreiecke aus synthetischen
World-XYZ-Daten mit Kamera-PushConstant und Tiefentest, nicht
mehr nur Farb-Clear. Eine VkGraphicsPipeline und DepthImage/
Memory/View pro Swapchain-Image bleiben über viele Frames
bestehen. Zwei Frame-Slots, Fences, Image-Present-Semaphores;
kein vkQueueWaitIdle pro Frame. Mesh-Upload/Region-Clear während
in-flight GPU-Buffer-Referenzen gesperrt bis Detach.
Native GPU: 16 World-/Depth-Frames, Kamera-Move, Mesh-Gate,
Resize und RGB-Pixel bestanden. Im echten SDL-X11-Fenster
vor GL: 20 World-Frames, 100 Pixel, drei Größen und dann
OpenGL-Login/Exit 0. 5 neue +26 ältere GUI-Fälle bestanden;
Linux-Vollbuild, 341 Python- und 41 native Tests grün.
Vorige plattformübergreifende GitHub-CI grün; aktuelle
Commit-CI noch prüfen. Echte eingeloggte OpenSim-Region,
Terrain, Texturen, Avatare, UI und produktiver Vulkan-Switch
weiterhin NICHT implementiert/abgenommen.
P05 formal 2/20, Blöcke 3/4 offen; nächstes Gate echte
Live-Region-Frames und generationssichere Mesh-Updates.

**P05/3–4 GPU-Fence-Meshgeneration und Region-Clear (05.11.2026, Code fb53a9e9):**
20 technische Aufgaben; der persistenten Vulkan-
World-Depth-Entwicklerpipeline wurde dynamischer
GPU-Meshupload ohne Swapchain-Neustart ergänzt.
Separate Modul-/SDL-Entwicklerfreigaben;
alte VkBuffers nur nach maximal 2 s bounded
waitAll über wirklich ausstehende Submit-Fences
der vorherigen Scene-Generation freigeben.
Kein vkQueueWaitIdle pro Frame oder Meshwechsel.
Bei Timeout sperren und GPU-Buffers belassen;
ungültige NaN-Meshdaten ohne Fence-Wait verwerfen.
Region-Clear und neuer Meshsnapshot invalidieren
die alte Kamera bis zur nächsten gültigen Pose.
Native GPU: zwei vorherige Frame-Slot-Submissions,
sichtbar Rot/Grün/Leer/Rot/Grün, drei Fence-Waits,
0 Retirement-Fehler, kein zusätzlicher QueueIdle.
Echter SDL-X11-Viewer vor GL: 20 World-Frames,
100 GPU-RGB-Samples, drei Extents, geordnete
Meshuploads/Region-Clear, danach GL-Login/Exit 0.
7 neue plus 31 alte GUI-Fälle bestanden; Linux-
Vollbuild, 342 Python- und 41 CTests grün.
VORHERIGE GitHub-CI vollständig grün, NEUE nach
Code fb53a9e9 noch abzunehmen.
Synthetische Testmeshes, keine authentifizierte
OpenSim-Welt. Keine fertige Vulkan-Engine mit
Terrain, Texturen, Avataren, UI oder Live-Window-
Ownership. P05 weiter 2/20, Block 3/4 offen.

**P05/3–4 echter Firestorm-Kamera-/Mesh-Handoff (06.11.2026, bf85bff5):**
20 technische Teilaufgaben. Der real erfasste Deferred-Kameraframe
steht bisher VOR dem späteren OpenSim-Regionsmesh-Upload und
wird dadurch invalidiert. Nur unter separatem Entwicklerflag
OGL_VULKAN_LIVE_CAMERA_REBIND=1 wird die im Modul zuvor
validierte Kamera nach erfolgreichem Mesh-Upload erneut
gebunden. Ein echter Regionswechsel invalidiert den Cache
vor GPU-Clear. Kamera- und Mesh-Generationen müssen
für GPU-World-Present und live_inputs_ready übereinstimmen.
Read-only-ABI zur Kamerageneration ergänzt.
OGL_VULKAN_WORLD_MESH_DEDUP=1 dedupliziert exakt
gleiche Dreiecke ohne GPU-Wait, neue Buffer oder
Kamerainvalidierung. CPU-Snapshot atomar publiziert.
Native echte GPU mit dedup=1, Rotation und Depth grün;
Linux-Vollbuild, 343 Python-/41 native Tests bestanden.
7 isolierte GUI-Tests GL-Login/Exit 0; zwei wichtigste
20-Frame-/100-Pixel-GPU-Fälle nach letztem Fix wiederholt.
NOCH KEIN authentifizierter Live-OpenSim-Vulkan-Present:
die sichtbaren GPU-GUI-Tests sind synthetische pre-GL-
Testmeshes. Keine fertige Vulkan-Engine mit Terrain,
Avataren, Texturen oder UI. P05 2/20; Block 3/4 offen.

**P05/3–4 GPU-Fence-Regionsbarriere (06.11.2026, Code 4520ce33):**
20 konkrete technische Teilaufgaben.
Bei echtem Firestorm-Region-ID-Wechsel kann
eine separate Opt-in-Operation mit Flag
OGL_VULKAN_LIVE_REGION_BARRIER=1
die alte GPU-Kamera SOFORT sperren,
danach bis maximal 2s die ausstehenden
Submit-Fences beider GPU-Frame-Slots
für die alte Meshgeneration abwarten
und erst dann alte Vulkan-Vertex/
Indexbuffer freigeben. Kein QueueIdle
pro Frame/Region. Bei GPU-Wait-Fehlern
bleiben Buffer erhalten und weitere
Vulkan-Presents sind gesperrt.
Firestorm-Regionwechsel-Callback gibt
booleschen Barriere-Erfolg zurück:
nur bei Erfolg generischen GPU-Clear
überspringen, GL bleibt aktiv.
Erstes NEW-region Mesh darf nach
Barriere ohne allgemeines Live-Mesh-
Rotationsflag hochgeladen werden.
Native echte X11-Vulkan-GPU:
2 Regionsbarrieren, 0 Fehler,
2 weitere GPU-Fence-Waits,
stale-camera reject, neue grüne
Depth-Weltfläche korrekt präsentiert.
SDL-Vor-GL-Probe unter zwei separaten
Opt-ins: 20 Frames/100 RGB-Pixel,
3 Extents, danach GL-Login/Exit 0.
5 neue +7 vorhandene isolierte
GUI-Smokes = 12/12 grün.
Linux-Vollbuild, 344 Python/41 native
CTests und 53 gezielte Tests grün.
VORIGE GitHub-CI grün, neue prüfen.
KEINE echte eingeloggte OpenSim-
Live-Vulkan-Welt gerendert, nur
synthetische Entwicklergeometrie
vor dem OpenGL-Kontext. Keine
Vulkan-Terrain-/Avatar-/Textur-/UI-
Engine, kein produktiver Switch.
P05 formal 2/20, Block 3/4 offen.

**P05/3–4 private Vulkan-GPU-Weltbilder während GL-Fensterownership (06.11.2026, 525c0945):**
20 technische Teilaufgaben. Echter Firestorm-
Kamera-/Meshsnapshot ist im Host an eine
Vulkan-GPU-World-Offscreen-ABI angebunden,
nur mit OGL_VULKAN_LIVE_WORLD_OFFSCREEN=1
und zusätzlichen vier Modul-Dev-Freigaben.
Die bestehenden World-XYZ-SPIR-V-Shader,
Depth-Test, 64-Byte-Kamera und VkVertex-/
Index-Buffer rendern 32×32 echte GPU-Welt-
Pixel in ein privates VkImage, das per
VkCopyImageToBuffer nach max. 2s Fence-
Wait ausgelesen wird. KEIN VkSurfaceKHR,
keine VkSwapchainKHR, kein Vulkan-Present,
keine GL-/SDL-Fensterübernahme. Host ruft
nur nach exakt gepaarter Camera-/Mesh-
Generation auf, max. erste 3 und dann
alle 64 neue Meshgenerationen, ohne
Spieler-/Regions-/Bilddaten im Log.
Read-only ABI: completed GPU draws,
0..1024 nicht-Hintergrund-Pixel.
Native Vulkan: 3 echte World/Depth-
GPU-Offscreen-Frames, erst sichtbare
Dreiecke, danach off-view=0 Pixel,
nach Region erneut 192 nichtleere
Pixel, ohne X11-Swapchain. Native
WSI-, Mesh-, Camera-, Fence- und
Depth-Regressionen weiterhin grün.
Linux-Vollviewer, 345 Python/41
native Tests erfolgreich. 15/15
reale isolierte Viewer-GUI-Starts,
normaler GL-Login-Wait und Exit 0.
**NICHT abgenommen:** tatsächlich
eingeloggtes OpenSim-Live-Region-
Bild oder produktiver Vulkan-
Window-/World-Renderer. Die
sichtbaren Probe-GPU-Szenen sind
synthetisch; echte Datenanbindung
ist nur kompiliert, Live-Login-Test
steht aus. P05 formal 2/20,
Block 3/4 weiter offen.

**P05/3–4 wiederverwendbarer World-GPU-Offscreen-Cache (06.11.2026, Code cb364549):**
20 technische Aufgaben. Der 32×32-
Vulkan-World-/Depth-Developerpfad
legt GPU-Farb-/Depth-VkImages,
GPU-Memory/Views, Framebuffer,
Shader/Pipeline, Readback-Buffer,
CommandPool/Buffer und Fence jetzt
EINMAL pro VkDevice-Cachelebensdauer
an und nutzt sie für neu eingereichte
Kamera-/Mesh-Generationen erneut.
CommandBuffer und Fence erst NACH
abgeschlossener GPU-Submission
resetten; alte Buffer bei ausstehenden
Fences niemals vorzeitig freigeben.
Bei Modulende vor GPU-Mesh und
VkDevice kontrolliert freigeben,
kein QueueIdle pro Renderframe.
Alter Offscreen-Clear-Test bleibt
temporär, Weltcache nur unter
bestehenden Developer-Flags.
Additive ABI/Host-Logs: Builds,
Reuses, Releases (ohne Positions-/
Regions-/Bilddaten).
Native Vulkan: vier World-GPU-Frames
inkl. Kamera-off-view, Regionswechsel,
2 Cache-Builds, 2 Reuses, 2 Releases
und überprüfte 192/1024 Weltpixel.
Voll-Linux-Build 346 Python/
41 CTests erfolgreich, 55 gezielte
Grafiktests. 15/15 isolierte reale
GUI-Starts bis GL-Login und Exit 0;
Vulkan-WSI-Regressionen grün.
KEIN authentifiziertes Live-OpenSim-
Vulkan-Weltbild abgenommen, sondern
synthetische Testgeometrie.
GL-Fenster weiterhin allein
produktiver Renderer. P05 formal
2/20, Blöcke 3/4 weiterhin offen.
Code cb364549; nächste Arbeit:
echte Login-Region-Capture-Probe
und erst dann Vulkan-Window-Handoff.

**P05/3–4 Kamera-only-GPU-Tick nach echter Regionsprüfung (06.11.2026, Code d527892b):**
20 technische Arbeitsschritte. Echt erfasste
Firestorm-Deferred-Kamera kann nun ohne neue
Regionsmesh-Uploads private Vulkan-World-/Depth-
GPU-Bilder erzeugen, sobald die Real-Region-UUID
geprüft und die Kamera-Mesh-Generation gültig ist.
Separates Developer-Flag
OGL_VULKAN_LIVE_CAMERA_TICK=1 plus die
bisherigen GPU-/Mesh-/Offscreen-Freigaben.
Maximal 1/120 geeignete Deferred-Frames,
unveränderte Kamera und reine Sonnenbewegung
lösen KEINE neue GPU-Probe aus.
Regionwechsel invalidiert Tick und Pose,
ungültige Vulkan-Kamera löscht Ready-Status.
Mesh-Rebind und Camera-Tick nutzen
denselben Host-GPU-Helper und Cache,
ohne OpenGL/SDL-Fensterübernahme.
Native Vulkan-GPU 6 Offscreen-Frames,
2 Camera-only mit gleichbleibender
Meshgeneration/Uploads/Fence-Waits,
0 und 192/1024 sichtbare Pixel,
Cache 2 Builds/4 Reuses/2 Releases.
Linux-Vollbuild 347 Python, 41 CTests,
56 gezielte Grafiktests bestanden.
18 isolierte GUI-Fälle bis normalem
GL-Login/Exit 0 bestanden.
Vorige GitHub-CI erfolgreich,
neue Commit-CI separat prüfen.
KEIN echtes authentifiziertes
OpenSim-Live-Vulkan-World-Bild,
GPU-Pixeltests sind synthetisch.
P05 formal 2/20, Blöcke 3/4 offen.

**P05/3–4 Quelle echter Firestorm-Deferred-Snapshots und GPU-Quittungen (06.11.2026, Code 68f7ac39):**
20 konkrete Arbeitsschritte. Echte OpenSim-
Meshsnapshots aus LLPipeline verwenden
jetzt einen eigenen Deferred-Host-
Aufruf (mit gültiger Region/UUID).
Nur dessen erfolgreiches GPU-Meshupload
darf nummerierte Source-Receipts erzeugen.
Generisches Developer-Triangle-ABI
erzeugt NIE gültige Real-Capture-
Herkunft. Vulkan-Offscreen-GPU-
Receipt nur bei current Session,
Region-Epoche, Capture-Nummer,
passender GPU-Meshgeneration und
bestätigtem Fence/Framezähler.
Der Weg mesh-rebind/camera-tick
wird ausschließlich numerisch
geloggt. Keine OpenSim-Region-IDs,
Nutzer-/Grid-Daten, Kamera-XYZ,
GPU-Bilder oder Meshdaten im Log.
Neuer lokaler Report in vorhandener
ogl/scripts/ogl-test-report.py:
keine Session/Region/Quelle,
GPU pending oder source-
qualified GPU complete; strikt
jüngste Session/Region/Snapshot,
alte GPU-Ergebnisse invalidieren.
13 weitere Eventparser-Unit-Tests,
4 CLI-Fixture-End-to-End-Szenarien;
361 Python, 41 native CTests,
57 Grafik-Vertragstests erfolgreich.
21/21 isolierte Viewer-GUI-Smokes
und native Vulkan-GPU-Tests grün,
drei zusätzliche Vorlogin-Prüfungen
erzeugen KEINE Fake-Live-Quittungen.
KEIN echter authentifizierter
OpenSim-Login/Region-GPU-Frame
abgenommen. OpenGL weiterhin
sichtbarer Renderer, Vulkan-World
nur Entwickler-Offscreen. P05
formal 2/20, Block 3/4 offen.
Nächstes Gate echte LIVE-Region-
Source-/GPU-Receipts nach Login
korrelieren, dann sichere Vulkan-
In-World-Fensterhoheit.

**P05/3–4 private GPU-Weltrendergrößen (06.11.2026, Code 0e86d6fd):**
Weitere 20 technische Arbeitsschritte: Der private Vulkan-
World-/Depth-Framebuffer kann jetzt vier allowlistierte
Auflösungen 32x32 default, 128x96, 256x144, 512x288
verwenden. Ungültige 8192x8192-Auflösung wird ohne
GPU-Ressourcenänderung abgewiesen. VkColor-/Depth-
Images, Framebuffer, RenderArea, Viewport, Scissor,
Readback und Kamera-PushConstant-Seitenverhältnis
werden für das private Zielformat skaliert.
Sichere GPU-Fence-freigegebene Reallokation nur
bei Größenwechsel; sonst bestehender Cache.
Additive Breite-/Höhe-Vulkan-C-ABI und
anonymisierte Host-Deferred-GPU-Größenlogs.
Der Report prüft pixel coverage gegen Bildmaße
und akzeptiert alte 32x32-GPU-Logs.
Hardware-GPU 12 World-/Depth-Frames,
6 Builds/6 Reuses/6 Releases, korrigierte
32x32-Kamera 254/1024 Testpixel.
Vollviewer 366 Python, 41 CTests, 58 Grafik-
und 41 Reporter-Tests grün. Isolierte
GUI-Starts/GL-Fallbacks mit neuen Flags geprüft.
KEIN Live-OpenSim-GPU-Weltbild nach Login
abgenommen, keine Vulkan-Fenster-Ownership.
P05 2/20, Block 3/4 weiterhin offen.

**P05/3–4 GPU-Weltflächennormalen und Lambert-Entwicklerlicht (06.11.2026, Code 5938a395):**
Weitere 20 tatsächliche technische
Teilaufgaben. Separater Developer-Flag
OGL_VULKAN_WORLD_LAMBERT_PROBE=1:
2 neue GLSL-450-Shader (Vulkan 1.0
SPIR-V mit glslang/spirv-val geprüft)
sind direkt in bestehende Vulkan-C++
Moduldatei eingebettet. GPU-Vertex-
Weltposition, Fragment-dFdx/dFdy-
Normalen, gerichtete Lambert- und
Ambient-Beleuchtung mit Behandlung
degenerierter Normale. Kein PBR,
keine Texturen oder NVIDIA-
Raytracing-Voraussetzung.
Unbekannte Flagwerte fail-closed,
rot/grüner Tiefen-Debugshader bleibt
Standard. Cache differenziert die
Shader-Modi und behandelt
Fenced-Freigabe und wiederverwendete
Shader-/Pipeline-Ressourcen sicher.
Read-only-Vulkan-ABI für anonyme
beleuchtete Pixel und mittlere
RGB-Helligkeit je fertig gerendertem
privaten Weltframe. Host-Deferred-
GPU-Receipts und vorhandener
lokaler Report ergänzt; alte Logs
bleiben lesbar und ungültige
Statistiken werden verworfen.
Hardware-GPU: 18 Welt-/Depth-
Frames, 10 Builds/8 Reuses/
10 Releases; zwei echte
orientierungsverschiedene
GPU-Testdreiecke 274 vs 327
mittlere RGB-Summe.
Voll-Linux-Build 371 Python,
41 CTests, 59 Grafik-, 45
Reporter-Tests grün. **29/29 isolierte
SDL/X11-GUI-Starts** einschließlich
Lambert-Opt-in-/Fallback-Fällen.
KEIN authentifizierter
OpenSim-Live-Vulkan-GPU-Frame;
Testmeshes synthetisch, sichtbare
Welt ausschließlich OpenGL.
P05 formal 2/20, Block 3/4 offen.
Code 5938a395.

**P05/3–4 echte Firestorm-Kamera-Sonnenrichtung als Vulkan-Entwicklerlicht (07.11.2026, Code 043efe22):**
20 konkrete technische Arbeitsschritte, siehe ogl/PROJECT_STATUS.md.
Vulkan-1.0-SPIR-V-Fragmentshader für dynamisch normalisierte
Firestorm-v3-Sonnenrichtung (Kamera 64B + Sonne 16B PushConstant).
Dynamik nur mit SUN_LIGHT_PROBE=1 und LAMBERT_PROBE=1 sowie
bestehenden Vulkan-GPU-/Mesh-/Camera-/Offscreen-Gates; bei
Sun-only oder fehlerhaften Flags kein GPU-World-Draw.
Normierte Lichtdaten werden bei Region-/Mesh-/WSI-/Camera-
Invalidierung verworfen. Drei unabhängige Shadercache-Modi,
keine Pipeline-/Mesh-Rebuilds bei Lichtänderungen.
GPU: 22 World-/Depth-Frames, 12 Builds/10 Reuses/12 Releases,
Sun-X/Sun-Z mittlere RGB-Summen 70/349 bei gleichem Mesh.
Linux: 372 Python, 41 CTests, 60 Grafik-, 45 Reporter-Tests grün.
33/33 isolierte echte SDL-/X11-Viewerstarts bestanden.
Kein authentifizierter OpenSim-Live-Welt-GPU-Nachweis;
sichtbar ausschließlich OpenGL; P05 2/20, Block3/4 offen.

**P05/3–4 Face-RGB aus echten Texture-Entries zu Vulkan GPU (07.11.2026, Code 11529400):**
20 technische Arbeitsschritte, siehe ogl/PROJECT_STATUS.md.
Die tatsächliche Firestorm-Deferred-Szenen-VolumeFace-Quelle
kann unter FACE_TINT_PROBE=1 und allen bisherigen Vulkan-
World-Lambert-Sun-Mesh-Camera-Offscreen-Gates je opakem
VolumeFace ein RGB-Tripel an ein separates Vulkan-Vertex-
Binding 1 übergeben; Binding 0 mit XYZ/RTX unverändert.
Terrain/Prepass/Transparenz nutzen neutrale Ersatzfarbe,
keine echten Bildtexturen oder PBR-Materialien.
Neue geprüfte Vulkan1.0-SPIR-V-Shader verwenden RGB
mal GPU-Lambert/Sun. Farbsidecar ist streng auf
aktuelle GPU-Meshgeneration und identische Triangle-
Anzahl beschränkt; bei fehlerhaften RGB-Werten, Mesh-
oder Regionswechsel keine Verwendung alter Farben.
Native echte GPU: 26 private Weltframes, 15 Builds,
11 Reuses, 15 Releases; rot R51/B0, blau R0/B51
auf identischem Mesh und gleichem Sonnenvektor.
38/38 isolierte SDL/X11-Viewerstarts und fünf neue Farb-Opt-in-/Fallback-Fälle grün.
Linux-Vollbuild: 373 Python/41 CTests, 61
Grafik-/45 Reportchecks grün.
Kein authentifizierter OpenSim-Live-Vulkan-
Farbframe; sichtbare Welt weiter OpenGL.
P05 formal 2/20, Block3/4 offen.

**P05/3–4 echte OpenSim-UV-Quellen an Vulkan-GPU-Checker (07.11.2026, Code ee2d7544):**
20 echte technische Aufgaben, siehe ogl/PROJECT_STATUS.md.
Der reale Firestorm-Deferred-LLVolumeFace-Scanner kann
optional Face-UVs (drei float2 pro XYZ-Dreieck) mit
üblicher TE-Rotation/S/T-Skalierung/Offset an den
quellqualifizierten GPU-Host übergeben.
Planar/animierte Texturen und PBR bleiben offen,
Terrain/Anker/ungültige UVs neutral.
Getrennte UV-vs-FaceRGB-vs-XYZ-GPU-Vertexbuffer,
Vulkan1.0-geprüfte ShaderMode4-Entwicklerpipeline
mit echtem GPU-UV-4x4-Schachbrettmuster.
Nur bei UV_CHECKER_PROBE=1 plus allen
FaceTint/Lambert/Sun/World/Mesh/Camera/Offscreen-
Freigaben. 1:1-Zuordnung, finite UVs,
Meshgeneration, GPU-Speicher-Rollback.
Kein echter Texturasset-/VkImage-Sampler.
Native Vulkan-GPU 30 World-/Depth-Frames,
16 Builds/14 Reuses/16 Releases, UV-Rot R48/G6,
UV-Grün R6/G47, Wrapping/Invalid-Checks grün.
Voll-Linux-Build 374 Python/41 CTests,
62 Grafik-/45 Reporter-Tests und 43/43
isolierte echte SDL/X11-Viewerstarts grün.
Noch kein authentifizierter Live-OpenSim-
Vulkan-Textur-/Weltframe, Welt bleibt OpenGL.
P05 formal 2/20, Block3/4 offen.

**P05/3–4 echter privater Vulkan RGBA8 VkImage-Sampler (09.11.2026, Code 0fa365e4):**
20 echte Quellcode-/Testschritte, Details ogl/PROJECT_STATUS.md.
Neuer additive C-ABI für EINE vorab decodierte, vollständig
deckende 1..64px-RGBA8-Textur pro aktueller GPU-Meshrevision.
Vulkan-1.0-validierter Fragmentshader nutzt tatsächliches
VkImage, VkImageView, Nearest-Repeat-VkSampler, kombiniert
ImageSamplerDescriptor und zuvor gekoppelte XYZ/FaceRGB/
UV/Sun. Linear-Tiling-Format nur bei SAMPLED_IMAGE-Support,
Host-coherent GPU-Image-RowPitch und explizite Vulkan-
Imagebarriere. Nur RGBA_IMAGE_PROBE=1 plus bestehende
UV_CHECKER/FACE_TINT/LAMBERT/SUN/Mesh/Camera/Offscreen-
Flags; bei fehlender Berechtigung keine UV/Texture-
GPU-Bilder. Bildrevision und Region-Clear sicher.
GPU: 35 private Frames, 19 Builds/16 Reuses/19 Releases,
realer Image-Sampler rot R51/G0, grün R0/G51,
64x64-Maximalbild, 2x2-Wrapping/Replacement geprüft.
Linux: 375 Python, 41 CTests, 63 Grafik-, 45 Report-
Checks und 5 isolierte echte SDL/OpenGL-GUI-Starts grün.
Keine echte OpenSim-TextureAsset-Decodierung,
kein authentifizierter Live-Welt-GPU-Frame, kein
sichtbarer Vulkan-Welt-Renderer. P05 formal 2/20.

**P05/3–4 reale Firestorm-Decoded-Raw-Quelle an Vulkan (09.11.2026, Code edd33d9b):**
20 konkrete Entwicklungs-/Testschritte, Details ogl/PROJECT_STATUS.md.
Zusätzlicher synchroner Firestorm-Deferred-Source-Pfad aus
LLViewerFetchedTexture::getRawImage (bereits decodiertes
LLImageRaw) für EIN einheitliches deckendes Legacy-Diffuse-Bild
pro vollständigem World-Mesh: Source- und Raw-Lebensdauer
über LLPointer, SharedLock auf Pixelpuffer, keine
Neudecodierung/GL-Readback oder Asset-ID-Logs.
Nur bei OGL_VULKAN_LIVE_RGBA_SOURCE=1 plus
OGL_VULKAN_WORLD_RGBA_IMAGE_PROBE=1 und vorherigen
GPU-/Mesh-/RGB-/UV-/Sun-Dev-Gates.
Mixed/Default/Missing/Transparent/Animated/PBR/GLTF,
Terrain und Prepass-Snapshots sind nicht geeignet.
RGB3-/RGBA4->RGBA8, Originalalpha-Validierung,
1..1024px Quellseiten, maximal 64px aspektrichtige
CPU-Kopie. Vulkan-Image-ABI bleibt unverändert,
source-qualified und generationstreu.
Voll-Linux 376 Python-/41 CTests, 64 Grafik- und
45 Reporttests grün. Native Bilddaten-/Aspect-
Negativtests grün; Vulkan-X11 weiter 35 GPU-Frames,
19 Builds/16 Reuses/19 Releases, Sample R51/G0
vs R0/G51. 10 isolierte GL-/Vulkan-Login-Fälle.
Wichtig: NOCH KEIN authentifizierter Live-Region-
Vulkan-Texturframe, kein Multi-Asset-/PBR-Renderer;
sichtbare Welt bleibt OpenGL, P05 formal 2/20.
Letzter GitHub-Workflow 42673502520 FAIL mit
null Jobs, Ursache noch ungeklärt; neue CI
nach neuem Push prüfen.

**P05/3–4 Mehrmaterial-Atlas aus echten Firestorm-TextureEntries (09.11.2026, Code cde4fb88):**
20 nachprüfbare technische Arbeiten, Details ogl/PROJECT_STATUS.md.
Die bisherige EINE dekodierte RGBA8-Textur ist optional
durch vier unterschiedliche dekodierte opaque Legacy-
Diffuse-VolumeFace-Bilder ergänzbar: 4x32x32 Atlas-
Quadranten in einer echten 64x64 VkImage, eigene
pro-Dreieck Material-Slot-ABI (0..3) über GPU-Binding3,
neue Vulkan1.0-validierte SPIR-V-Vertex-/Fragmentshader,
flat Materialslot/frac(UV) und kein Tile-Bleeding.
Host/source-qualified handoff über LLViewerFetchedTexture/
LLImageRaw mit LLPointer+SharedLock und separaten
LIVE_MATERIAL_ATLAS_SOURCE/MATERIAL_ATLAS_PROBE-Flags.
Alle alten UV-/RGB-/Mesh-/Sun-/Camera-/GPU-Gates weiter.
Mixed >4, fehlende/Default-/transparente/PBR/GLTF/
animierte Materialquellen, Terrain/Anker fail-closed.
Native echte Vulkan GPU 42 Welt-/Depth-Frames, Cache
22 Builds/20 Reuses/21 Releases vor Modulstop,
vier Materialfarben je R/G/B 51, Gelb R51/G51.
2 unterschiedlich belegte Dreiecke IM SELBEN GPU-Frame
R25/G25/B0, UV-Wrap/Region-Clear validiert.
Voll Linux377 Python/41 CTests, 65 Grafik-/45 Reporttests,
16 isolierte SDL/OpenGL-Login-/Fallback-Starts.
Letzte sechsplattformige GitHub-CI 42676970109 SUCCESS;
nächster Commit-CI separat prüfen.
Kein authentifizierter OpenSim-Live-Vulkan-Materialframe
und keine sichtbare Vulkan-Welt. P05 formal 2/20.

## Automatischer /goal-Phasenlauf

**Maßgeblich sind P01–P13 im bestehenden Hauptprojektplan.**
Nicht mehr frei zwischen den alten Fachkapiteln A–I
wechseln. Neuer Einstieg: P05 Block 3/20.
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

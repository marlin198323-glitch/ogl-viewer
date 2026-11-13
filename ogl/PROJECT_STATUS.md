# OGL Firestorm – aktueller Projektstand

Stand: 13.11.2026
Repository: https://github.com/marlin198323-glitch/ogl-viewer
Branch: ogl-develop
Verbindlicher Auftrag: GOAL_DUAL_GRID_AND_VIEWER_ROADMAP_2026_11_01.md
Übernahme in neuen Chats: PROJECT_HANDOFF.md und AGENTS.md (Repo-Wurzel)

## Server-Repository-Verknüpfung und Änderungsbaseline – 13.11.2026

Externes Quellrepo `opengenesislink/opengenesislinkserver` als
**dauerhafte, schreibgeschützte Kompatibilitätsreferenz** im
OGL-Hauptprojektplan und `AGENTS.md` verankert. Git-Baum
(3.931 Einträge), relevante Server-Dokumente, Releasehistorie,
Prüfung der `main`-/Stable-Refs sowie Viewer-Allowlist,
Login/Discovery, CAPS/EventQueue, Mesh/PBR, AssetService,
Voice und Sicherheits-/Featureflags ausgewertet.

Zuletzt geprüft: `main` = Stable `v0.9.4.0` =
`ee15af8e728ac024c4ab6beb8bd195f3662850eb`;
Release vom 12.11.2026, .NET 10, Linux x64. Separater
`gen1-updates`-Kanal ist als Workflow vorbereitet, derzeit
kein veröffentlichtes Update-Release gefunden. Externes
Issue #435 bleibt offen (Profile Feed standardmäßig aus);
`/CE/<uuid>` kann legitime EventQueue-502-Long-Polls liefern.

**Nächste /goal-Runde:** Vor Coding `main`/Release/Update-
SHA und relevante Issues erneut vergleichen. Nur
nachgewiesene Viewer-Schnittstellenänderungen als neue
Implementierungs-/Abnahmeblöcke planen. Server- und
OGL-Versionen strikt getrennt halten. Dies war eine
Dokumentations-/Quelleninventur, **keine** neue erfolgreich
abgenommene Vulkan-Funktion; **P05 unverändert 2/20**.

## P05/3 – /goal 20 Teilblöcke: erste valide Vulkan-GPU-Probe sofort nach echtem Welt-Swap (13.11.2026)

**Funktionale Live-Integrationsverbesserung:** Der bereits opt-in an
einen echten, sichtbaren OpenSim/OpenGL-Welt-Swap gekoppelte
Vulkan-Offscreen-GPU-Pfad wartete bislang auf den **120.**
qualifizierten sichtbaren Weltframe, bevor er die erste
private Vulkan-Probe ausführte. Selbst wenn ein echter
Firestorm-Deferred-Mesh-Receipt und die passende GPU-
Kamera schon bereit waren, konnte der erste Source-/Pixel-
Nachweis bei langsam ladenden oder niedrig-FPS-OpenSim-
Regionen lange ausbleiben.

Der bestehende plattformneutrale
`ogl_world_shadow_policy.h` enthält jetzt die neue reine
C++-Regel `ogl::present_coupled_sample_due(...)`: **Beim
ersten tatsächlich source-, mesh-, kamera- und region-
qualifizierten OpenGL-Welt-Swap darf sofort genau ein
privater Vulkan-Offscreen-GPU-Versuch gestartet werden.**
Alle **späteren** synchronen GPU-Versuche, unabhängig davon,
ob die vorige Probe erfolgreich war oder scheiterte,
bleiben mindestens **120 tatsächlich sichtbare Welt-Swaps**
voneinander entfernt. Überlauf-/Rückwärtszählung und
Nullframes sind ausgeschlossen. Aufeinanderfolgende
GPU-Fehlversuche müssen **zusätzlich** die bestehende
Backoff-Regel (120/240/480/maximal 960 Frames) erfüllen.
Die letzte *tatsächliche* Attempt-Frame-Nummer wird erst
nach sämtlichen bestehenden Entwickler-Opt-ins, realen
Deferred-Quellprüfungen, GPU-Pose-/Mesh-/Kamera-Gates und
Fehler-Backoff **vor dem privaten GPU-Aufruf** gespeichert.
Beim Modul-Init/-Shutdown bzw. echten Regionswechsel wird
auch sie über den vorhandenen zentralen Reset zurückgesetzt.

**Abgrenzung:** Die neue Möglichkeit gehört ausschließlich
zur ausdrücklichen privaten Vulkan-Entwicklerdiagnose.
Die normale sichtbare OpenGL-Grafik, deren Frame-Pacing,
Login und SDL-Swapchain bleiben unangetastet; Vulkan
wird weiterhin **nicht** im Viewerfenster präsentiert.
Der erste GPU-Versuch könnte im opt-in Entwicklermodus
eine einmalige synchrone GPU-Last verursachen; es ist
keine gemessene FPS-Verbesserung und kein fertiger Vulkan-
Weltrenderer. Ein erfolgreicher authentifizierter positiver
`present-coupled`-Vulkan-Frame wurde nicht beobachtet.

**20 tatsächlich ausgeführte Implementierungs- und Prüfschritte:**

01. `ogl-develop` auf `c7293ef6`, sauberen Git-Working-Tree und Remote-Synchronität geprüft.
02. Verbindliche `AGENTS.md`, `PROJECT_HANDOFF.md`, `PROJECT_STATUS.md` und Hauptroadmap P05 erneut gelesen.
03. GitHub-CI `43508213219` des Ausgangscommits als vollständig `success` verifiziert.
04. Echten `llviewerdisplay.cpp::swap()`-gekoppelten GPU-Pfad und OpenGL-Window-Owner abgeglichen.
05. Bisherige starre `ogl_vulkan_camera_tick_due(live_visible_world_swaps)`-Wartezeit von 120 qualifizierten Frames ermittelt.
06. Bestehende echte Firestorm-Deferred-Capture-/Region-/Mesh-/Kamera-Quellgates nachvollzogen.
07. Aufeinanderfolgende Fehlschläge und 120/240/480/960-Frame-Retry-Regel auf Wechselwirkung geprüft.
08. Neue CPU-only-Policy `present_coupled_sample_due` im bestehenden Header für die sofortige erste Probe eingeführt.
09. Null-Frame- sowie rückläufige sichtbare Welt-Swap-Zähler im neuen Guard abgewiesen.
10. Weitere GPU-Versuche nach dem ersten auf mindestens 120 sichtbare Welt-Swaps Abstand begrenzt.
11. `live_present_coupled_last_attempt_swap` als expliziten letzten tatsächlich versuchten GPU-Welt-Swap eingeführt.
12. Initialisierungs-, Shutdown- und Regionswechsel-Reset auf diesen neuen Attempt-Zähler erweitert.
13. Die bisherige starre `%120`-ähnliche Frame-Grenze durch den neuen ersten-Swap-/Folgeabstand-Guard ersetzt.
14. Letzten GPU-Attempt erst nach positivem Quellen-/Redraw-/Backoff-Gating und unmittelbar vor realem GPU-Aufruf eingetragen.
15. Bestehenden Quellen-/Pixelnachweis, Failure-Backoff, individuellen GPU-Receipt-Log und OpenGL-Fallback unverändert erhalten.
16. Neue native C++-Validierungsfälle für Erstversuch, spätere 120-Frame-Abstände, rückläufige Zähler, uint64-Grenzen und Failure-Backoff ergänzt.
17. Neuen Python-Grafik-Vertragstest für echten Host-Pfad, genaue Reihenfolge und unveränderte Entwicklerfreigaben ergänzt: 97/97 Grafiktests grün.
18. Native Vulkan-Modulkompilierung und 41/41 CTests bestanden; `git diff --check` erfolgreich.
19. Vollständigen Linux-Viewer-Build mit 419/419 Python- und 41/41 nativen CTests bestanden; X11-Vulkan-Clear-/World-/Depth-GPU-Läufe mit jeweils 16/32 Frames, Mesh-Rotation und Region-Barrier bestanden.
20. Frisch gebauten SDL/Xvfb-Viewer mit isoliertem Profil und echten Opt-in-Flags bis `STATE_LOGIN_WAIT` gestartet: 0 falsche Weltframe-Logs, 0 private Vulkan-GPU-Receipts vor Login, 0 ERROR/FATAL; Report `world_entry_not_observed`.

**Testgrenzen:** Native 16/32-Frame-Vulkan-Tests verwenden
synthetische GPU-Testgeometrie. Der tatsächliche positive
OpenSim-In-World-GPU-Versuch kann nur bei einer autorisierten
angemeldeten Sitzung getestet werden; **kein** Benutzerkonto
oder bestehender Nutzerprozess wurde angefasst. Der
isolierte Viewer wurde per Timeout 124 beendet (kein
regulärer Shutdown-Nachweis). Windows-/macOS-Vollviewer
lokal nicht gebaut, keine Server-/Credential-/Profil-
Änderungen, keine neue Repo-Datei. **P05 weiterhin 2/20;
P05/3 und vollständige Vulkan-SDL-World-Present-/UI-
Pipeline bleiben offen.**

## P05/3 – /goal 20 Teilblöcke: Live-World-Swap-GPU-Quittungen vollständig protokollieren (13.11.2026)

**Funktionaler Fehler zwischen echtem Viewer und Live-Diagnose behoben:**
Der bereits vorhandene Host
`indra/newview/oglrtxhost.cpp::ogl_rtx_after_visible_world_swap()`
veröffentlicht positive `source=present-coupled`-GPU-Receipts
für private Vulkan-Bilder nur nach erfolgreich qualifiziertem
OpenGL-Welt-Swap. Dessen zusätzliche numerische Welt-Swap-
Bestätigung wurde bisher jedoch nur für die **ersten drei**
erfolgreichen GPU-Proben und danach **alle 64** protokolliert.
Der im vorherigen Commit implementierte `ogl-test-report.py`
braucht hingegen für **jedes** GPU-Receipt die zugehörige
Welt-Swap-Bestätigung. Dadurch konnte er reale spätere
Erfolge trotz gültiger Quell- und Pixel-Evidenz nicht
als bestätigt zählen.

Der echte Viewer schreibt jetzt **für jede erfolgreich
abgeschlossene** `present-coupled`-GPU-Probe genau eine
anonymisierte Abschlussmeldung mit sichtbarem Welt-Swap-
Zähler, privatem Framezähler, Suppressionsstatistik,
Mesh-Generation, Quell-Receipt und Region-Epoch. Die Meldung
folgt erst nach positivem GPU-Readback, Quell-/Pixelevidenz,
Fehlerrückstellung und Aktualisierung des bestätigten
Live-Caches. Der Pfad ist weiterhin nur mit den expliziten
Entwicklerfreigaben aktiv und liegt mindestens 120
qualifizierte Welt-Swaps zwischen regulären GPU-Versuchen;
zusätzliche Fehler-Backoff-Gates gelten weiterhin. Es werden
keine URLs, Namen, Account-/Regions-UUIDs, Kamera-Koordinaten,
Bildinhalte oder Texturdaten geloggt. **Die Vulkan-Ausgabe
bleibt ein privates Offscreen-Bild; OpenGL behält die SDL-
Fenster-Ownership.**

Der bestehende numerische Report validiert außerdem jetzt,
dass `completed_private_frames` niemals größer sein darf
als der aus derselben Deferred-Quelle nachgewiesene globale
GPU-Framezähler. Die bisherige Quell-/Region-/Mesh-
Provenienz, monotone Welt-Swap-/Framezähler und die
Bestätigungspflicht bleiben erhalten.

**20 tatsächlich ausgeführte Entwicklungs- und Prüfschritte:**

01. Git-Branch `ogl-develop`, sauberen Arbeitsbaum und Ausgangscommit `07d5ca1e` verifiziert.
02. `AGENTS.md`, `ogl/PROJECT_HANDOFF.md`, `ogl/PROJECT_STATUS.md` und P05-Hauptroadmap gelesen.
03. GitHub-CI des vorherigen Commits `07d5ca1e` mit vollständigem `success` geprüft.
04. Echten `ogl_rtx_after_visible_world_swap()` auf Erfolgs- und Fehlerrückstellungsreihenfolge untersucht.
05. Numerischen Live-GPU-Receipt-Parser und seine Welt-Swap-Quittungslogik geprüft.
06. Inkonsistenz zwischen jedem positiven GPU-Receipt und nur ersten drei/jeder 64. Host-Meldung identifiziert.
07. Bestehende Entwickler-Opt-in- und 120-World-Swap-Grundtakt-Gates kontrolliert.
08. Unvollständige Bestätigungs-Taktung im echten Host entfernt.
09. Numerische Welt-Swap-Abschlussmeldung für jeden erfolgreich abgeschlossenen privaten GPU-Frame aktiviert.
10. Geloggte Felder auf numerische Swap-, Frame-, Capture-, Mesh- und Epoch-Zähler beschränkt.
11. Sicherung der Meldungsreihenfolge nach tatsächlichem positivem GPU-Sample und Cache-Aktualisierung beibehalten.
12. Report um einen Plausibilitäts-Guard gegen private Framezähler oberhalb der globalen GPU-Frames ergänzt.
13. Python-Grafik-Regressionstest für bedingungslose Quittungen und Abfolge nach GPU-Erfolg ergänzt.
14. Report-Langzeittest für neun einzeln bestätigte, aufeinanderfolgende private GPU-Frames ergänzt.
15. Negativtest für überhöhte private GPU-Framezähler implementiert.
16. Bestehende Daten- und Stale-Capture-Parserregressionen geprüft: 418/418 Python-Tests einschließlich 55/55 Reporttests bestanden.
17. Nativen Vulkan-Modulbuild und 41/41 native CTests erfolgreich durchgeführt.
18. Linux-Vollviewer erfolgreich gebaut und sämtliche 418 Python-/41 CTests erneut bestanden.
19. Reale native X11-Vulkan-GPU-Clear-/World-/Depth-Läufe für jeweils 16/32 Frames sowie Mesh-Rotation/Region-Barrier bestanden; vorhandenen Nutzerlog geprüft: `no_deferred_evidence_session` statt fälschlichem Vulkan-Live-Erfolg.
20. Frisch gebauten Viewer unter isoliertem SDL/Xvfb-Testprofil mit Live-Vulkan-Opt-ins bis `STATE_LOGIN_WAIT` geprüft: 0 falsche Weltframes, 0 Pre-Login-Vulkan-Live-GPU-Receipts, 0 ERROR/FATAL, Report `world_entry_not_observed`; `git diff --check` erfolgreich.

**Testgrenzen:** Der Report-Langzeittest verwendet ausschließlich
anonymisierte synthetische numerische Receipt-Folgen. Die
native Vulkan-X11-GPU-Szene ist ebenfalls synthetisch.
Ein echter erfolgreicher `present-coupled`-GPU-Frame nach
authentifiziertem OpenSim-Login wurde nicht durchgeführt.
Der isolierte SDL-Viewer wurde nach 18 Sekunden per Timeout
(Exit 124) beendet und hat keinen regulären Quit-Nachweis.
Die produktive Welt bleibt OpenGL; Windows/macOS-Vollviewer
lokal nicht gebaut. Kein Nutzerprofil, laufender Viewer,
Secret oder Server verändert, keine neue Repository-Datei.
**P05 formal weiterhin 2/20; P05/3 nicht abgenommen.**

## P05/3 – /goal 20 Teilblöcke: echten World-Swap-Vulkan-Receipt im lokalen Logreport qualifizieren (13.11.2026)

**Funktionslücke behoben:** Der bestehende, lokale
`ogl/scripts/ogl-test-report.py` wertete bisher echte Vulkan-
Deferred-GPU-Receipts nur für `source=mesh-rebind` und
`source=camera-tick` aus. Der bereits implementierte,
**an den sichtbaren GL-Welt-Swap gekoppelte** `source=present-coupled`
war im Report nicht erkennbar. Damit konnten tatsächlich
qualifizierte private Vulkan-Frames aus angemeldeten Weltsitzungen
bei der Projektabnahme nicht eindeutig von der normalen
Kamera-/Mesh-Probe getrennt werden.

Jetzt unterscheidet der bestehende Report die dritte Quelle und
prüft zusätzlich die nach einem erfolgreichen Welt-Swap
protokollierte Meldung
`P05 REAL GL-world-swap -> Vulkan offscreen GPU`.
Ein `source=present-coupled`-Receipt allein wird **nicht** als
bestätigter Swap-gebundener Erfolg gezählt. Dafür müssen eine
aktuelle Deferred-Capture, nichtleere GPU-Pixel, Region, Mesh-
und Capture-Identität, monoton steigende private Frame- und
sichtbare Swap-Zähler sowie die passende spätere
Swap-Bestätigung vorhanden sein. Ohne `STATE_STARTED`, nach
einem Modul-/Sitzungsneustart, einem Regionswechsel, einer
neueren Capture, bei Duplikaten oder inkonsistenten Zahlen
wird die Bestätigung zurückgewiesen. Frühere Meldungen ohne
`suppressed_retries`-Statistik werden weiterhin akzeptiert.
Der Report gibt nur numerische Kennzahlen und feste Statuswerte
aus; keine URLs, Konto-/Regionsnamen, IDs oder Inhalte des
Viewerbildes. **Auch ein bestätigtes privates GPU-Bild ist
kein sichtbarer Vulkan-Window-Present und kein unabhängiger
Beweis einer authentifizierten Anmeldung.**

**20 tatsächlich ausgeführte Entwicklungs-/Prüfschritte:**

01. Sauberen Branch `ogl-develop` mit Ausgangscommit `0cfb8a93` überprüft.
02. `AGENTS.md`, `PROJECT_HANDOFF.md`, `PROJECT_STATUS.md` und P05-Roadmap gelesen.
03. GitHub-CI für `0cfb8a93` als vollständig erfolgreich verifiziert.
04. Den echten 13.11.-OpenSim-Viewerlog auf erfolgreiche OpenGL-Welt, fehlendes Vulkan-Live-Receipt und Simulator-HTTP-502-Warnungen untersucht.
05. Vorhandenen Report-Code und 45 historische Report-Vertragstests analysiert.
06. Fehler identifiziert: `P05_GPU` akzeptierte bisher nur Mesh- und Kameraquellen.
07. Dritten festen, erlaubten Source-Typ `present-coupled` im bestehenden Report aufgenommen.
08. Separate numerische `P05_WORLD_SWAP`-Bestätigung aus realem Host-Log in den Parser eingebaut.
09. Private GPU-Quellbestätigung an dieselbe Region und neueste Deferred-Capture gebunden.
10. Mesh-Generation der Bestätigung mit der letzten gültigen Capture abgeglichen.
11. GPU-Receipts vor tatsächlichem `STATE_STARTED` zurückgewiesen.
12. Für `present-coupled` mindestens ein nichtleeres Offscreen-GPU-Pixel gefordert.
13. Monoton ansteigende private GPU- und sichtbare Swap-Zähler validiert; doppelte Bestätigungen verworfen.
14. Ausstehende Bestätigungen bei neuer Sitzung, Regionswechsel oder neuer Capture verfallen lassen.
15. Alten Logmodus ohne `suppressed_retries`-Statistik kompatibel erhalten.
16. Zwei neue rein numerische Report-Zähler sowie einen von normalem Vulkan-Window-Present getrennten Status ausgegeben.
17. Acht konkrete positive/negative Python-Regressionstests einschließlich alter Logmodi, gefälschter Meldungen, Pre-Login, Sessions/Regionen und leerer Bilder ergänzt: 53/53 Reporttests grün.
18. Gesamte Python-Testserie 415/415, natives Vulkan-Modul und 41/41 CTests bestanden; `git diff --check` bestanden.
19. Linux-Vollviewer-Build und native X11-Clear-/World-/Depth-GPU-Regressionen je 16/32 Frames sowie Mesh-Rotation und Region-Barrier erfolgreich geprüft.
20. Echten vorhandenen Nutzerlog mit Report ausgewertet: `no_deferred_evidence_session`, 0 echte private World-Swap-Receipts; zusätzlich neuen isolierten SDL/Xvfb-Viewer mit Dev-Gates bis `STATE_LOGIN_WAIT` gestartet: `world_entry_not_observed`, 0 falsche World-Frames, 0 vorzeitige GPU-Receipts, 0 ERROR/FATAL.

**Abgrenzung:** Es wurde kein Nutzerkonto für einen Test
angemeldet. Die Positivfälle des neuen Report-Parsers sind
anonymisierte numerische Test-Fixtures, **kein** tatsächlich
gemessener positiver OpenSim-`present-coupled`-Frame.
Die tatsächlichen 16/32-Frame-Vulkan-Regressionen verwenden
synthetische native GPU-Szenen. Der isolierte Viewer wurde per
Timeout beendet (Exit 124, kein regulärer Quit-Nachweis).
Keine Nutzerprofile oder laufenden Nutzerprozesse verändert;
keine Serveränderungen, Secrets oder neuen Repository-Dateien.
**P05 bleibt 2/20, P05/3 offen: keine dauerhafte Vulkan-SDL-
Window-Ownership, keine vollständige Vulkan-Welt/GUI.**

## P05/3 – /goal 20 Teilblöcke: private Vulkan-GPU-Fehler mit begrenztem Wiederanlauf absichern (13.11.2026)

**Implementiert:** Der streng opt-in an den echten sichtbaren OpenGL-
Welt-Swap gebundene Vulkan-GPU-Offscreen-Prototyp hatte bisher
bei einer gescheiterten privaten GPU-Probe keine besondere
Fehlerrückstellung. Der nächste Versuch konnte regelmäßig alle
120 qualifizierten World-Swaps erneut die synchronen Vulkan-
Readbacks auslösen. Das ist insbesondere bei instabilen Vulkan-
Treibern oder unter Speicherdruck unerwünscht.

Nun hält der bestehende Host den Zähler aufeinanderfolgender
Fehler sowie den Welt-Swap des letzten fehlgeschlagenen Versuchs
vor. Ein neuer plattformneutraler C++-Guard in der bestehenden
`ogl_world_shadow_policy.h` staffelt den **zusätzlichen** Abstand
bis zum nächsten privaten GPU-Versuch auf 120, 240, 480 und
maximal 960 sichtbare OpenGL-Weltframes. Der normale
120-Frame-Grundtakt bleibt bestehen; Wiederholungen erfolgen
nur, wenn der Abstand wirklich abgelaufen ist. Der Fehlerschritt
sperrt nie das GL-`swapBuffers()` und deaktiviert Vulkan nicht
endgültig. Ein erfolgreicher privater GPU-Durchlauf setzt
Fehlerzähler und Zeitpunkt zurück. Modul-Init, Modul-Shutdown
und Regionswechsel resetten sie ebenso über die bestehende
Lifecycle-Resetfunktion. Das vorhandene, validierte Deferred-
Quell-/Kamera-/Mesh-Gating, die überprüften positiven GPU-
Receipts und die exklusive OpenGL-Fenster-Ownership bleiben
unverändert. Fehlversuche erzeugen höchstens drei zusätzliche
Diagnosewarnungen je Fehlerfolge, keine wiederkehrende Warnflut.

**20 tatsächlich ausgeführte Entwicklungs- und Prüfschritte:**

01. Sauberen `ogl-develop`-Arbeitsbaum und Ausgangscommit `5e1834b1` geprüft.
02. `AGENTS.md`, Projektübergabe, aktuellen Status und Hauptroadmap erneut gelesen.
03. GitHub-CI des vorigen Commits `5e1834b1` mit `success` abgeglichen.
04. Echten GL-Welt-Swap-Gate und nachgeschalteten Vulkan-Opt-in-Host analysiert.
05. Bestehende 120-Swap-Taktung und den synchronen GPU-Readback-Pfad geprüft.
06. Fehlende Begrenzung wiederholter GPU-Fehlschläge identifiziert.
07. Eigenständigen, plattformneutralen `present_coupled_retry_delay`-Guard eingeführt.
08. Fehlerfreie erste GPU-Probe ohne Wartezeit erlaubt.
09. Ersten Fehlschlag auf 120-Welt-Swaps Rückstellung begrenzt.
10. Zweiten Fehlschlag auf 240-Welt-Swaps Rückstellung erweitert.
11. Dritten Fehlschlag auf 480-Welt-Swaps Rückstellung erweitert.
12. Weitere Fehler auf maximal 960-Welt-Swaps gedeckelt (keine permanente Deaktivierung).
13. Swap-Differenz zur Verhinderung vorzeitiger Wiederholungen und Rückwärts-Zählerstände geprüft.
14. Laufzeitfelder für Fehlerfolge, letzten Fehlversuch und unterdrückte Wiederholungen ergänzt.
15. Vor dem echten `ogl_firestorm_offscreen_gpu_sample` die neue Wiederholungsprüfung eingebaut.
16. Nach fehlgeschlagenem GPU-Sample den Versuchspunkt gesetzt und Warnungen auf die ersten drei Fehler beschränkt.
17. Nach erfolgreichem GPU-Sample Fehlerfolge und letzten Fehlerzeitpunkt zurückgesetzt; erfolgreichen Quellen-Cache unverändert beibehalten.
18. Reset bei Regionwechsel und Modul-Init/-Shutdown gesichert; 17 native positive/negative C++-Grenzprüfungen sowie einen Python-Quellcode-Regressionsfall ergänzt, 95/95 Grafiktests bestanden.
19. Vollständigen Linux-Viewer neu gebaut: 407/407 Python- und 41/41 native CTests; echte X11-Vulkan-Clear-/World-/Depth-Regressionen mit jeweils 16/32 Frames, Mesh-Rotation und Region-Barrier bestanden.
20. Frisch gebauten SDL/Xvfb-Viewer mit separatem temporären Testprofil und Vulkan-Live-Opt-ins bis `STATE_LOGIN_WAIT` gestartet: 0 falsche GL-Weltframe-Logs, 0 unerlaubte private Vulkan-Live-Frames, 0 Backoff-Warnungen vor Login und 0 ERROR/FATAL; `git diff --check` bestanden.

**Testgrenzen:** Die native Backoff-Policy wurde ausführbar mit
Erfolgs-/Fehler-/Überlauf-/Rückwärts- und Sättigungsfällen
geprüft, eine gezielte Fehlerserie **innerhalb einer
angemeldeten OpenSim-Welt** wurde nicht provoziert. Der
isolierte Viewer endete per `timeout` (Exit 124), nicht
über regulären Quit. Windows/macOS-Vollviewer lokal nicht
gebaut. Die neuen Schutzmechanismen sind nur bei expliziter
Vulkan-Entwicklerfreigabe aktiv; normale OpenGL-Anwender
bleiben unberührt. Keine Userprofile, Credentials, Server
oder Nutzerprozesse verändert; keine neuen Repo-Dateien.
**P05 bleibt 2/20, P05/3 weiter offen: kein vollständiger
Vulkan-Window-World-Present.**

## P05/3 – /goal 20 Teilblöcke: echte Welt-Swaps von Login/Teleport/Logout unterscheiden (13.11.2026)

**Funktionalen Real-Viewer-Fehler korrigiert:** `llviewerdisplay.cpp::swap()`
rief bislang `ogl_record_world_frame_presented()` bei JEDEM
OpenGL-Buffer-Swap auf, einschließlich des Loginbildschirms.
Dadurch wurde `OGLWorldLastWorkingEngine=opengl` ohne bestätigten
Weltstart gesetzt und das Log meldete fälschlich einen
„OpenGL world frame presented“. Der nachgeschaltete, streng
opt-in Vulkan-Offscreen-Hook kontrollierte nur `STATE_STARTED`,
nicht aber Teleport, Logout, GL-Wiederherstellung, Sichtbarkeit,
Minimierung oder Regionsverfügbarkeit.

Beide Aufrufe sind jetzt hinter **einem gemeinsamen, nach
`swapBuffers()` ausgewerteten** `real_world_visible`-Guard
zusammengefasst. Dieser verlangt tatsächlich gestartete Welt,
vorhandene Region, keinen Teleport-Bildschirm, keinen angefragten
Logout, keine GL-Wiederherstellung, keinen Programm-Exit sowie
ein aktives, sichtbares, nicht minimiertes Viewerfenster.
Die Bedingungen sind in der vorhandenen plattformneutralen
`ogl_world_shadow_policy.h` als pure C++-Funktion
`ogl::visible_world_swap_eligible(...)` implementiert und
mit ausführbaren nativen Negativfällen geprüft. OpenGL bleibt
Fensterinhaber, Vulkan bleibt nur separate GPU-Diagnose; kein
neues Feature im Grafikmenü freigeschaltet.

**20 tatsächlich durchgeführte Entwicklungs- und Prüfschritte:**

01. Sauberen Git-Branch `ogl-develop` und Ausgangscommit `5a31dd38` verifiziert.
02. `AGENTS.md`, `PROJECT_HANDOFF.md`, `PROJECT_STATUS.md` und P05-Roadmap erneut gelesen.
03. GitHub-CI des letzten Commits `5a31dd38` mit `success` geprüft.
04. Echten `llviewerdisplay.cpp::swap()` und `ogl_record_world_frame_presented()` untersucht.
05. Fälschliche Weltframe-/Einstellungsschreibfolge bereits bei GL-Login-Swaps identifiziert.
06. Bisherige Vulkan-Post-Swap-Prüfung auf alleinige `STATE_STARTED`-Abhängigkeit analysiert.
07. Teleport-/Logout-/Restore- und Anzeigezustände des realen `display()`-Pfades abgeglichen.
08. Plattformneutrale C++-Weltframe-Qualifizierungsfunktion im bestehenden Policy-Header ergänzt.
09. Ausdrückliche `STATE_STARTED`- und aktive-Region-Gates in die Qualifizierung übernommen.
10. Teleport-Progress- und Logout-Progress-Bilder ausgeschlossen.
11. GL-Restore- und Programm-Exit-Frames ausgeschlossen.
12. Nicht aktive, unsichtbare oder minimierte Viewerfenster ausgeschlossen.
13. Vorhandenen OpenGL `swapBuffers()` unverändert und an erster Stelle belassen.
14. Weltframe-Erfolgsreport erst innerhalb `if (real_world_visible)` ausgeführt.
15. Neuen Post-Swap-Vulkan-Diagnosehook hinter exakt denselben Weltframe-Guard gesetzt.
16. Neun positive/negative Fallprüfungen in vorhandenen nativen C++-Policy-Test eingebaut.
17. Python-Grafikvertragstest für Login-/Teleport-/Logout-/Restore-/Exit-Gates und korrekte Aufrufreihenfolge ergänzt: 94/94 Grafiktests bestanden.
18. Nativen Modulbuild und alle 41/41 native CTests bestanden.
19. Linux-Vollviewer kompiliert und 406/406 Python-Tests bestanden; native GPU-X11-Clear-/World-/Depth-16-/32-Frame- sowie Mesh-Rotation-/Regionswechsel-Regressionsläufe bestanden.
20. Frisch gebauten Viewer unter isoliertem Xvfb-Testprofil mit allen vorhandenen Live-Vulkan-Opt-ins bis `STATE_LOGIN_WAIT` gestartet: 0 falsche Weltframe-Meldungen, 0 unerlaubte `present-coupled`-GPU-Ausgaben, 0 ERROR/FATAL; `git diff --check` bestanden.

**Grenzen:** Der isolierte Viewer wurde per Timeout beendet
(Exit 124, kein regulärer Shutdown). Die Negativfälle der
Weltzustände wurden nativ im CPU-Guard geprüft; ein echter
angemeldeter Teleport-/Logout- oder OpenSim-Vulkan-Weltbetrieb
wurde nicht durchgeführt. Der erfolgreiche OpenGL-Login in der
Smoke-Prüfung ist keine Vulkan-In-World-Abnahme. Windows/macOS-
Vollviewer lokal nicht getestet. Keine Userprozesse, Profile,
Zugangsdaten oder Server geändert. **P05 weiterhin 2/20;
dauerhafte Vulkan-SDL-Window-Ownership und Vollwelt-Present
bleiben offen.**

## P05/3 – /goal 20 Teilblöcke: echte GPU-Pixelevidenz und in-flight Quellenbindung (13.11.2026)

**Implementiert:** Der an den echten Firestorm-Welt-Swap gebundene
Vulkan-Offscreen-Prototyp meldet ein privates GPU-Bild nicht mehr
als erfolgreiches `source=present-coupled`-Live-Ergebnis, nur weil
die Vulkan-Funktion erfolgreich zurückkehrt und ihr Framezähler
steigt. Ein neuer **plattformneutraler, nativer CPU-Guard**
`ogl::present_coupled_gpu_evidence_current(...)` validiert,
dass **dieselbe echte Deferred-Aufnahme** mit derselben Region-
Epoch, Capture-Receipt- und GPU-Mesh-/Kamera-Generation **vor
und nach** der GPU-Ausgabe gültig ist. Darüber hinaus verlangt
er genau ein zusätzlich abgeschlossenes Offscreen-Frame und
mindestens ein verifiziertes, nicht gelöschtes Pixel innerhalb
eines plausiblen, begrenzten privaten Bildes (8–4096 Pixel je
Seite; `nonclear_pixels <= width*height`).

Der vorhandene Host `ogl_firestorm_offscreen_gpu_sample(...)`
zieht die Quell-Snapshots **vor** `world_render_offscreen_scene()`
und prüft beim `present-coupled`-Modus die tatsächlichen GPU-
Bildkennzahlen **danach**. Leere, ungültige, in-flight veraltete
oder inkonsistente Ergebnisse erhöhen den vorhandenen
Fehlerzähler, generieren **kein** qualifiziertes GPU-Receipt und
aktualisieren den bestätigten erfolgreichen Welt-Swap-Cache
nicht. Andere unabhängige Offscreen-Diagnosequellen sowie die
produktive OpenGL-Welt bleiben funktional unverändert. Das
Vulkan-Fenster bleibt ausdrücklich **nicht** produktiv übernommen.

**20 tatsächlich bearbeitete Entwicklungs-/Prüfschritte:**

01. `ogl-develop`-Branch und sauberen Arbeitsbaum auf `384cbfbb` kontrolliert.
02. `AGENTS.md`, `PROJECT_HANDOFF.md`, `PROJECT_STATUS.md` und verbindliche P05-Roadmap erneut gelesen.
03. Letzte GitHub-Workflow-Ergebnisse einschließlich des bestandenen Windows-UTF-8-Fixes ausgewertet.
04. Realen GL-World-Swap-Hook und dessen separat freigegebene GPU-Diagnose analysiert.
05. Host-Provenienz-Gates für echte Deferred-Quelle, Kamerabindung und Mesh-Revision überprüft.
06. Bisherigen GPU-Erfolgsnachweis auf leere Pixel und während der Berechnung wechselnde Quellen geprüft.
07. Vor-GPU-Snapshot von Region-Epoch, Deferred-Receipt und Mesh-Revision eingeführt.
08. Nach-GPU-Validierung gegen unveränderte Region-Epoch und aktuelle Capture-Epoch umgesetzt.
09. Quell-Receipt-Veränderung während der GPU-Arbeit zur Ablehnung geführt.
10. Mesh- und GPU-Kamera-Generation nach der Arbeit erneut gegen die Aufnahme verglichen.
11. Sicherung gegen Framezähler-Überlauf (`UINT64_MAX`) und fehlendes Einzel-Frame-Inkrement ergänzt.
12. Minimale und maximale private Bildbreite/-höhe mit 8–4096 Pixeln abgesichert.
13. Positive Nicht-Hintergrundpixelzahl und `nonclear_pixels <= width*height` gefordert.
14. Plattformneutralen Guard in der bestehenden `ogl_world_shadow_policy.h` implementiert, ohne neue Datei.
15. Guard an den echten `ogl_firestorm_offscreen_gpu_sample("present-coupled")`-Aufruf vor Receipt-Inkrement gebunden.
16. Fehlerhafte oder leere Ergebnisse auf bestehenden Fehlerpfad geleitet; keine unberechtigte positive Erfolgsmeldung.
17. Ausführbaren nativen C++-Regressionstest um 14 positive/negative GPU-Provenienz- und Pixel-Grenzfälle ergänzt.
18. Python-Quellcode-Regression für Post-GPU-Validierung, Quellenmodus und Reihenfolge ergänzt: 93/93 Grafiktests; 41/41 native CTests grün.
19. Linux-Vollviewer neu gebaut: 405/405 Python-Tests, 41/41 native CTests und X11-Vulkan-Clear-/World-/Depth-GPU-Regressionen mit 16/32 Frames sowie Mesh-Rotation und Region-Barrier erfolgreich.
20. Frisch gebauten SDL/Xvfb-Viewer mit temporärem Profil und Opt-in-Flags gestartet: OpenGL-Login `STATE_LOGIN_WAIT`, kein unberechtigter Pre-Login-GPU-Receipt, 0 ERROR/FATAL; `git diff --check` bestanden.

**Offene Abnahme:** Der Testviewer wurde kontrolliert per
`timeout` beendet (Exit 124, kein regulärer Quit-Nachweis).
Echte positive `source=present-coupled`-GPU-Pixel im
**angemeldeten OpenSim-Weltbetrieb** wurden nicht geprüft;
die negativen Guard-Fälle sind durch den ausgeführten C++-
Test, die bisherigen normalen GPU-Pfade durch X11-Lavapipe-
Regressionen nachgewiesen. Kein produktiver Vulkan-Present,
keine SDL-Vulkan-Exklusiv-Window-Ownership und keine
vollständige Welt-/UI-Pipeline. Windows/macOS-Vollviewer
nicht lokal gebaut. Keine Benutzerprofile, Nutzerprozesse,
Secrets oder Server geändert. **P05 bleibt 2/20, P05/3 offen.**

## P05/3 – /goal 20 Teilblöcke: Live-Swap-Aufnahmen bei Modul-Reload und Materialwechsel korrekt binden (13.11.2026)

**Funktionaler Fehler behoben:** Im neuen, opt-in an den tatsächlichen
`llviewerdisplay.cpp::swap()` gebundenen Vulkan-Offscreen-Pfad
blieben die nach Erfolg zwischengespeicherten Kamera- und
Framezustände beim vollständigen Vulkan-Modul-Reload erhalten.
Der Host löscht jetzt Quelle, Pose, Region-, Mesh- und Aufnahme-
Revision sowie den 120-Frame-Zähler **bei Modul-Initialisierung,
Modul-Shutdown und jedem echten Regionswechsel**. Die kumulative
Erfolgszählung bleibt nur über Regionswechsel derselben Sitzung
erhalten und wird bei Neuinitialisierung zurückgesetzt.

**Neue echte Quellenabsicherung:** Der erfolgreiche Vulkan-GPU-Frame
speichert nun auch die laufende Quellaufnahmenummer des realen
OpenSim-Deferred-Scanners (`live_deferred_receipts`). Identische
Kamerapose und GPU-Mesh-Revision rechtfertigen **kein** Überspringen,
wenn die Deferred-Quelle seither erneut erfasst wurde (z. B.
andere Material-/Quelldaten). Zwei CPU-only, plattformneutrale
Prüfregeln in der vorhandenen `ogl_world_shadow_policy.h` lehnen
Null-Receipts, abweichende Region-Epochen, alte Mesh-Daten oder
falsch gebundene Kamera-Generationen ab. Ihre Entscheidungen
werden von der realen Host-Funktion verwendet und zusätzlich
in einem ausführbaren C++-Test mit positiven/negativen Fällen
getestet. OpenGL bleibt der exklusive SDL-Fenster-Renderer.

**20 ausgeführte Entwicklungs- und Prüfschritte:**

01. Sauberen Git-Branch `ogl-develop`, Ausgangscommit `697f133c` und Remote-Synchronisierung geprüft.
02. Verbindliche `AGENTS.md`, Projektstatus, Handoff und P05-Roadmap erneut gelesen.
03. GitHub-CI-Status des vorherigen Windows-UTF-8-Testfixes abgefragt.
04. Den echten `swap()`-Hook auf In-World- und Opt-in-Gates kontrolliert.
05. Modulinitialisierung, Modulentladung und Regionswechsel auf zurückbleibende Live-Swap-Caches geprüft.
06. Unzurückgesetzte Kamera-/Framezustände bei erneutem Modulstart als Lifecycle-Fehler identifiziert.
07. Gemeinsame, modulinterne Resetfunktion für Pose, Source-Receipt, Mesh- und Region-Revision eingeführt.
08. Reset dieser Caches und der Zähler bei vollständiger Vulkan-Modulinitialisierung eingebaut.
09. Identischen Reset vor beziehungsweise bei vollständiger Vulkan-Modulfreigabe ergänzt.
10. Regionswechsel-Reset integriert; kumulative Erfolgszählung innerhalb einer Sitzung erhalten.
11. Eigene `live_present_coupled_capture_receipt` für die letzte erfolgreiche Deferred-Aufnahme eingeführt.
12. Neuen GPU-Frame an ein vorhandenes, von der Deferred-Pipeline stammendes Nonzero-Receipt gebunden.
13. Prüfung eingeführt, dass Region-Epoch der Capture-Epoch entspricht.
14. GPU-Mesh-Revision mit ursprünglicher Deferred-Mesh-Revision abgeglichen.
15. Vulkan-GPU-Kamera-Mesh-Revision auf dieselbe aktuelle Mesh-Generation festgelegt.
16. Den Redraw-Entscheid um den Source-Receipt-Wechsel trotz gleicher Pose/Mesh-Revision erweitert.
17. C++-Native-Test um 13 positive und negative Guard-/Redraw-Fälle ergänzt; 41/41 native CTests bestanden.
18. Zwei fokussierte Python-Vertragstests für Modul-Reload-Reset und Quellversionsidentität ergänzt; 92/92 Grafiktests bestanden.
19. Linux-Vollviewer neu gebaut: 404/404 Python-Tests und 41/41 native CTests; reale X11-Vulkan-Clear- und World-/Depth-GPU-Regressionen mit jeweils 16/32 Frames bestanden.
20. Isolierten SDL/Xvfb-Viewer ohne Nutzerkonto mit vorhandenen GPU-Entwickler-Opt-ins gestartet: OpenGL `STATE_LOGIN_WAIT`, 0 unerlaubte Pre-Login-Live-GPU-Receipts, 0 Fehler/Fatal-Meldungen; `git diff --check` bestanden.

**Abgrenzung:** Der Xvfb-Viewer wurde nach 18 Sekunden per Timeout
(Exit 124) beendet; kein regulärer Quit oder authentifizierter
OpenSim-Live-World-GPU-Frame ist damit nachgewiesen. Die nativen
Vulkan-X11-Tests verwenden synthetische Geometrie. Der neue
Quellversionsvergleich wird von echten Host-Pfaden verwendet,
aber die tatsächliche Live-Region benötigt gesonderte Abnahme.
Windows/macOS-Vollviewer lokal nicht gebaut. **Keine dauerhafte
Vulkan-Window-Ownership und kein Vollwelt-Renderer; P05 weiter
2/20, P05/3 noch nicht abgeschlossen.** Keine neuen Repo-Dateien,
Benutzerprofile oder laufenden Nutzerviewer verändert.

## P05/3 – In-World-Swap an echte Vulkan-Deferred-Quelle gekoppelt (13.11.2026; Teilfortschritt, keine Freigabe)

**Codeintegration statt weiterer Pre-GL-Tests:** Der echte Firestorm-
Renderer ruft im bestehenden `llviewerdisplay.cpp::swap()`
**nach** `swapBuffers()` und der OpenGL-World-Frame-Meldung,
**nur bei** `STATE_STARTED`, den neuen Host-
Einstiegspunkt `ogl_rtx_after_visible_world_swap()` auf.
Dieser ist ausschließlich mit `OGL_VULKAN_LIVE_PRESENT_COUPLED_OFFSCREEN=1`
und den bereits bestehenden separaten Offscreen-/Mesh-/Kamera-
Entwicklerfreigaben aktiv. Er akzeptiert nur Quellen, die der
authentifizierte OpenSim-Deferred-Pipeline-Scan tatsächlich
mit gültiger Region-Epoch, Mesh-Revision und gebundener Kamera
übertragen hat. Es gibt **keine** synthetischen Dreiecke im
neuen Live-Hook, keinen GL-Framebuffer-Readback und keine
Weitergabe von Nutzerbildern oder Regionskennungen.

Der Hook führt maximal beim ersten und jedem 120. qualifizierten
sichtbaren Swap eine private Vulkan-GPU-Offscreen-Probe durch;
bei gleicher Mesh-Revision und unveränderter Kamerapose wird
übersprungen. Ein Regionwechsel invalidiert die neue Pose-
und Mesh-Verknüpfung. Erst ein erfolgreicher privater GPU-Draw
liefert `source=present-coupled` und eine Bestätigung im Log.
**Die tatsächliche SDL-Swapchain bleibt weiterhin allein im
Eigentum von OpenGL**; die neue Vulkan-Ausgabe wird nicht in
das Fenster präsentiert. Das ist ein überprüfbarer
Integrationsschritt zu P05/3, **kein** erfüllter P05/3-Block.

**Abnahme dieser Codeeinheit:** 90/90 fokussierte Grafiktests;
402/402 Python-Regressionen und 41/41 native CTests mit
vollständigem Linux-Viewer-Build erfolgreich. Isolierter
SDL/Xvfb-Viewer ohne Benutzerkonto mit gesetztem neuen
Opt-in startet weiterhin bis `STATE_LOGIN_WAIT` und meldet
`UNAUTHORIZED_GPU=0`, `FATAL=0`. Der Test endet per Timeout
124; regulärer Logout nicht geprüft. **Kein realer OpenSim-
GPU-Receipt wurde ohne Anmeldung behauptet**; das benötigt
einen gesonderten autorisierten Live-Test mit echtem Deferred-
Material und Kamera. Vulkan-Menü bleibt deaktiviert.

**Noch für P05/3 erforderlich:** vollständige In-World-
Renderpass-Szene/Materialien, echte SDL-Vulkan-Window-
Exklusivübernahme nach Login, UI-Kompositing, Resize-/
Region-/Teleport-/Logout-Lebensdauer und Fallback ohne
OpenGL-/Vulkan-Simultan-Present; Windows/macOS-Backend und
Live-Abnahme. **P05 formal weiterhin 2/20.**

## 13.11.2026 – Live-Viewer-Logprüfung und Grafikmenü-Klarstellung (kein abgeschlossener P05-Block)

Beim laufenden OpenSim-Live-Viewer wurde eine erfolgreiche
Anmeldung und ein **echter OpenGL-World-Present** bestätigt.
`oglrtxhost.cpp` meldet `vulkan_world_pipeline_complete=false`,
`rendering_active=0`; die im Grafikmenü deaktivierte Vulkan-
Option ist daher korrekt und darf nicht blind freigeschaltet
werden. Linux-Vulkan-WSI-/GPU-Proben laufen bislang nur in
separaten, vor dem OpenGL-Start freigegebenen Entwicklerpfaden.
Die deutsche und englische Renderer-Beschreibung benennt nun
explizit die noch fehlende dauerhafte In-World-3D-Pipeline
anstelle der irreführenden Aussage, es gebe noch keine
Swapchain. Vulkan bleibt bis zum vollwertigen Renderer
**gesperrt**. `getFontPathSystem()` erkennt unter Linux jetzt
vorhandene System-Fontverzeichnisse über `LLFile::isdir` und
warnt nur einmal, wenn keines existiert.

**Prüfung:** 89/89 Python-Grafiktests, vollständiger Linux-
Viewer-Build und 401/401 Python- sowie 41/41 native CTests
bestanden. Isolierter Linux-Xvfb-Viewerstart: Login-Wartezustand
erreicht, `Could not determine system fonts path` **0 Mal**;
OpenGL mit Mesa-Fallback funktioniert. Testende per Timeout
124, kein normaler Exit nachgewiesen. Bestehende Nutzerprozesse
und Benutzerprofile nicht verändert.

**Offen (nicht als lokal behoben markieren):** Beim realen
Stadt-NexVerse-Weltstart wurden 502-Antworten eines Simulator-
Caps-Endpunkts, 404-Texturen, dreimal `CertExpired`,
CEF-`chrome-sandbox`-Fehlkonfiguration und Druck auf
physischen RAM mit mehreren parallelen Viewerprozessen
protokolliert. Zertifikate, HTTP-Routing und fehlende Assets
müssen auf ihren zuständigen Servern diagnostiziert werden.
CEF-Sandbox-Berechtigungen erfordern sicheren Installer-/
Paketierungsweg, **kein** unkontrolliertes `chmod u+s`.
Keine Benutzerprozesse beendet, keine Server geändert.
**P05 formal weiterhin 2/20; Live-Vulkan-World-Window-Ownership offen.**

## P05/3 – /goal 20 Teilblöcke: laufende Vulkan-Present-Queue/FIFO-Eignung absichern (13.11.2026)

**Implementiert:** Vor jedem persistenten Entwickler-GPU-Frame
werden jetzt `vkGetPhysicalDeviceSurfaceSupportKHR` für die
bereits verwendete Queue-Familie sowie die aktuelle Liste
`vkGetPhysicalDeviceSurfacePresentModesKHR` geprüft.
Ein Verlust der Queue-Present-Eignung oder des ursprünglich
verwendeten `VK_PRESENT_MODE_FIFO_KHR`, eine unplausible
Anzahl Modi und eine instabile zweite Enumeration führen
fail-closed zu einem gesperrten WSI-Attach **vor**
`vkAcquireNextImageKHR` und GPU-Submit. Ein separater,
zweifach freigegebener nativer X11-GPU-Test simuliert die
verlorene Present-Freigabe **nach einem wirklich präsentierten
Frame** und verlangt vor dem nächsten Submit Detach und
Re-Attach. Die normale SDL-/OpenGL-Startfolge bleibt
unverändert.

**20 tatsächlich ausgeführte Entwicklungs- und Prüfaktionen:**

01. Sauberen Branch `ogl-develop` und Ausgangscommit `bd6104ea` geprüft.
02. `AGENTS.md`, `PROJECT_HANDOFF.md`, `PROJECT_STATUS.md` und P05-Roadmap gelesen.
03. GitHub-CI-Status der letzten vier Branch-Commits kontrolliert.
04. Vulkan-Xlib-WSI-Attach auf initiale Queue- und FIFO-Prüfung untersucht.
05. Persistenten Present-Pfad auf noch fehlende Live-Queue-/FIFO-Prüfung analysiert.
06. Sicherung vor dem bestehenden Fence-/Acquire-/Submit-Pfad festgelegt.
07. Live-Funktionsauflösung für `vkGetPhysicalDeviceSurfaceSupportKHR` hinzugefügt.
08. Queue-Familie und bestehende Surface vor jedem neuen Frame erneut geprüft.
09. Live-Funktionsauflösung für `vkGetPhysicalDeviceSurfacePresentModesKHR` hinzugefügt.
10. Anzahl der Live-Present-Modi auf 1–256 begrenzt und Abfragefehler fail-closed behandelt.
11. Zweite Present-Mode-Enumeration mit stabiler Anzahl validiert.
12. Weiterhin verfügbaren `VK_PRESENT_MODE_FIFO_KHR` vor Image-Acquire verlangt.
13. Native QA-Injektion mit zwei Freigaben nur ab dem zweiten Frame hinzugefügt, ohne produktive UI-Freigabe.
14. Echten X11-Present mit anschließend simuliertem Freigabeverlust und blockiertem weiteren Submit ergänzt.
15. GPU-Detach, Wiederherstellung per Re-Attach und sichtbare Pixelverifikation in bestehendem Native-Test ergänzt.
16. Acht vollständige native Swapchain-Ressourcenaufbau-/Freigabe-/Queue-Idle-Zyklen nachgewiesen.
17. Neuen Python-Grafikregressionstest für Queue-, FIFO-, QA-Gate- und Prüfungsreihenfolge ergänzt: 87/87 Grafiktests bestanden.
18. Vulkan-Modul gebaut; 41/41 CTests und native X11-Clear/World-/Depth-Proben mit je 16/32 GPU-Frames bestanden.
19. Native Mesh-Rotation und Region-Barrier geprüft; vollständigen Linux-Viewer gebaut: 399/399 Python-Tests und 41/41 native CTests bestanden.
20. Frisch gebauten Viewer unter isoliertem Xvfb-Profil geprüft: 20/20 Vulkan-World-/Depth-Pre-GL-Presents, 100/100 Pixel bei drei Extents, danach OpenGL `STATE_LOGIN_WAIT`; `git diff --check` bestanden.

**Grenzen:** Das negative Verlustszenario wurde gezielt nach
einem echten GPU-Present im nativen X11-Test simuliert;
eine tatsächlich vom Treiber verweigerte Present-Queue oder
FIFO-Liste wurde nicht provoziert. Der SDL-Testviewer wurde
per Timeout beendet (Exit 124, kein regulärer Quit-Beleg).
Keine authentifizierte OpenSim-Welt, kein dauerhafter
Vulkan-In-World-Renderer. Kein lokaler Windows-/macOS-
Vollviewer-Test. Keine Änderungen an Benutzerdaten,
Credentials, laufenden Nutzerprozessen und keine neuen
Repository-Dateien. **P05 bleibt 2/20; P05/3 weiter offen.**

## P05/3 – /goal 20 Teilblöcke: aktive Swapchain auf Live-Farbraum und Composite-Alpha prüfen (13.11.2026)

**Umgesetzt:** Der persistente Vulkan-X11-QA-Present-Pfad
sichert das konkrete Swapchain-`VkFormat` nun zusammen mit
dem ursprünglichen `VkColorSpaceKHR` und der bei Attach
ausgewählten `VkCompositeAlphaFlagBitsKHR`. Vor jedem
weiteren GPU-Image-Acquire wird anhand der erneut abgefragten
`vkGetPhysicalDeviceSurfaceFormatsKHR`-Liste geprüft, ob das
Format-Farbraum-Paar noch für dieselbe Surface gültig ist.
Eine abweichende ColorSpace trotz gleichem VkFormat ist kein
Ersatz. Der von Vulkan erlaubte Sonderfall, dass die Surface
**genau ein** `VK_FORMAT_UNDEFINED` meldet, bleibt erlaubt,
sofern der Farbraum identisch ist. Auch die Kompatibilität des
ausgewählten Composite-Alpha-Bits mit den aktuellen
Surface-Capabilities wird geprüft. Fehlerhafte, fehlende,
überlange oder instabile Formate-Enumerationen sperren den
Attach **vor** GPU-Acquire und Queue-Submit. Der gespeicherte
Farbraum/Alpha-Zustand wird beim Detach zurückgesetzt.

**20 tatsächlich ausgeführte Entwicklungs-/QA-Teilblöcke:**

01. Sauberen `ogl-develop`-Arbeitsbaum und Ausgangscommit `acf1cfdf` verifiziert.
02. Verbindliche `AGENTS.md`, `PROJECT_HANDOFF.md`, `PROJECT_STATUS.md` und P05-Roadmap erneut gelesen.
03. GitHub-CI-Stand einschließlich der noch laufenden vorherigen Commits geprüft.
04. Bestehende `VkSurfaceFormatKHR`- und `VkCompositeAlpha`-Auswahl beim Attach analysiert.
05. Aktuellen persistenten Surface-Capability-Check und Frame-Acquire-Ablauf überprüft.
06. Fehlende erneute Prüfung der Farbformat-/Farbraum-Kombination zur Laufzeit identifiziert.
07. Originalen Swapchain-Farbraum als separates Feld im bestehenden Vulkan-Modul ergänzt.
08. Originalen Swapchain-Composite-Alpha-Modus als separates Feld ergänzt.
09. Bei erfolgreichem WSI-Attach beide tatsächlichen Konfigurationswerte gesichert.
10. Beim Window-Detach beide Werte auf gültige Ausgangswerte zurückgesetzt.
11. Live-`supportedCompositeAlpha` gegen das bereits gewählte Bit geprüft.
12. `vkGetPhysicalDeviceSurfaceFormatsKHR` vor GPU-Acquire dynamisch abgefragt.
13. Fehlende Formate, unplausible Anzahl über 256 und fehlgeschlagene Queries fail-closed behandelt.
14. Bei veränderter zweiter Format-Enumeration einen weiteren Present verweigert.
15. Das konkrete ursprüngliche `VkFormat`-`VkColorSpaceKHR`-Paar mit allen aktuellen Einträgen verglichen.
16. Vulkan-Sonderfall einer einzigen `VK_FORMAT_UNDEFINED`-Antwort bei gleichem Farbraum erhalten.
17. Python-Grafikregression für neue gespeicherte Felder, Paarkompatibilität und Prüfung vor Acquire ergänzt: 86/86 Grafiktests bestanden.
18. Vulkan-C++-Modul erfolgreich gebaut, 41/41 native CTests und reale X11-Clear-/World-/Depth-16-/32-Frame-GPU-Regressionsläufe bestanden.
19. Native Mesh-Rotation und Region-Barrier erneut geprüft; vollständigen Linux-Vollviewer erfolgreich gebaut: 398/398 Python- und 41/41 native Tests.
20. Frisch gebautes SDL/X11-Viewerprogramm in isoliertem temporärem Profil gestartet: 20/20 World-/Depth-Pre-GL-Presents, 100/100 Pixelproben bei drei Größen, danach OpenGL `STATE_LOGIN_WAIT`; `git diff --check` bestanden.

**Grenzen:** Ein nachträglicher Treiberwechsel des Format-
Farbraum-Paares wurde über den Python-Quellcodevertrag,
aber nicht durch manipulierte echte ICD-Antworten geprüft;
die echten GPU-Tests belegen die weiterhin funktionierenden
zulässigen Formate und bekannten Resize-/Unmap-Fehlerfälle.
Der SDL-Test wurde per Timeout nach Login-Wait beendet
(Exit 124, kein normaler Shutdown-Nachweis). Es gibt weiterhin
keinen authentifizierten Vulkan-In-World-Frame; GL rendert die
Welt. Windows/macOS-Vollviewer nicht lokal getestet.
Keine Nutzerprofile, Secrets oder fremde Prozesse verändert;
keine neuen Repository-Dateien. **P05 formal 2/20;
Block 3 dauerhafte Vulkan-World-Window-Ownership offen.**

## P05/3 – /goal 20 Teilblöcke: VK_SUBOPTIMAL_KHR erzwingt neuen GPU-Swapchain-Attach (13.11.2026)

**Implementiert:** Die persistente X11-Vulkan-Entwickler-WSI
behandelt `VK_SUBOPTIMAL_KHR` nun als Signal, den laufenden
Swapchain-Attach nicht für weitere Frames zu verwenden. Ein
beim Acquire suboptimal gemeldetes Image wird dennoch über die
gewöhnlichen GPU-Fence-/Semaphore-/Queue-Submit-Schritte
vollständig präsentiert; anschließend bleibt `p.blocked=true`.
Dasselbe gilt für ein suboptimales `vkQueuePresentKHR`.
Andernfalls würde die bereits signalisierte Acquire-Semaphore
unerlaubt ungenutzt bleiben oder auf einer suboptimalen
Swapchain unbegrenzt weitergerendert. Ein kontrollierter
Detach und Re-Attach sind erforderlich. Der neue native
X11-Faultmodus kann nach einem **wirklich erfolgreichen**
`vkQueuePresentKHR` eine suboptimale Rückgabe simulieren.
Er ist ausschließlich mit zwei unabhängigen Entwickler-
Freigaben aktiv und kann die normale SDL/OpenGL-Anmeldung
nicht beeinflussen.

**20 konkret bearbeitete Entwicklungs-/QA-Teilschritte:**

01. Sauberen Git-Arbeitsbaum `ogl-develop`, Commit `71eef75e` kontrolliert.
02. `AGENTS.md`, Handoff, Status und verbindliche P05-Roadmap erneut gelesen.
03. GitHub-CI für `71eef75e` geprüft (bei Abfrage noch `queued`).
04. Persistent-Swapchain-Fence-/Semaphore-Lifetime im vorhandenen Modul analysiert.
05. `VK_SUBOPTIMAL_KHR`-Verhalten für Image-Acquire und Queue-Present verglichen.
06. Ungeschützte Wiederverwendung nach suboptimalem GPU-Present identifiziert.
07. Suboptimalen Image-Acquire per `acquire_suboptimal` festgehalten.
08. Bereits erworbenes Image weiter korrekt bis Submit und Present geführt.
09. Nach erfolgreichem Present die normale Framezählung beibehalten.
10. Bei suboptimaler Acquire- oder Present-Rückgabe den Attach für Folgeframes gesperrt.
11. Native X11-Faultinjektion nur mit `OGL_VULKAN_PRESENT_TEST_X11=1` und `OGL_VULKAN_PERSISTENT_SUBOPTIMAL_TEST_X11=1` ergänzt.
12. Faultinjektion erst nach einem tatsächlich erfolgreichen GPU-Present ausgewertet.
13. Im bestehenden nativen X11-Clear-Test ein neues WSI-Attach für Suboptimal-/Blocked-Prüfung ergänzt.
14. Nach suboptimalem Frame die Unterdrückung des zweiten GPU-Submits anhand des Framezählers nachgewiesen.
15. Detach nach dem gesperrten Attach mit vollständiger GPU-Ressourcenfreigabe überprüft.
16. Einen weiteren WSI-Re-Attach mit erfolgreichem Real-GPU-Present und Pixelverifikation ergänzt; sechs Build-/Release-/Idle-Zyklen nachgewiesen.
17. Python-Grafikregression für Eventreihenfolge, getrennte QA-Freigaben und Neu-Attach ergänzt: 85/85 Grafiktests bestanden.
18. C++-Module gebaut und 41/41 native CTests bestanden; echte X11-Clear-/World-/Depth-GPU-Regressionsläufe mit 16/32 Frames erfolgreich.
19. Native Mesh-Rotation und Region-Barrier erneut getestet; vollständigen Linux-Viewer erfolgreich gebaut und 397/397 Python- sowie 41/41 native Tests bestanden.
20. Frisch gebauten SDL/X11-Viewer in isoliertem temporärem Profil geprüft: 20/20 Pre-GL-World-/Depth-Presents, 100/100 Pixelproben über drei Fenstergrößen, OpenGL `STATE_LOGIN_WAIT`; `git diff --check` bestanden.

**Grenzen:** Eine echte Treiberrückgabe `VK_SUBOPTIMAL_KHR`
wurde nicht ausgelöst, sondern das Ereignis erst nach einem
realen erfolgreichen GPU-Present im separaten nativen QA-Modus
simuliert. Der echte SDL-Start blieb synthetisch und wurde
nach dem Login-Wartezustand per Timeout beendet (Exit 124,
kein regulärer Shutdown-Nachweis). Keine Anmeldung in eine
OpenSim-Welt; kein produktiver Vulkan-In-World-Renderer.
Windows und macOS nicht lokal vollständig getestet. Keine
Änderungen an Benutzerprofilen, Secrets oder fremden Prozessen;
keine neuen Repository-Dateien. **P05 formal 2/20;
Block 3 echte Live-Welt-Window-Ownership weiter offen.**

## P05/3 – /goal 20 Teilblöcke: laufende Vulkan-Surface-Capabilities vor jedem Present prüfen (13.11.2026)

**Implementiert:** Der persistente Entwickler-WSI-Pfad überprüft
zusätzlich zum bestehenden X11-Sichtbarkeits- und Extent-Schutz
vor jedem GPU-Acquire die aktuellen Vulkan-Surface-Capabilities
gegen die beim Attach festgelegte VkSwapchain: tatsächliche
Swapchain-Bildanzahl gegen neu gemeldete min/maxImageCount,
maxImageArrayLayers, zulässige min/maxImageExtent, den
unveränderten Surface-Transform und dessen weiter bestehende
Unterstützung. Ein neuer, bei Attach eingefrorener
`swapchain_transform` wird beim Detach zurückgesetzt. Eine
geänderte/ungültige Surface wird fail-closed vor Acquire/Submit
blockiert, nicht im laufenden Frame mit einem neuen Transform
weitergerendert. Vulkan ist weiterhin ausschließlich QA vor GL.

**20 tatsächlich bearbeitete Implementierungs- und Prüfschritte:**

01. Sauberen Arbeitsbaum `ogl-develop` und Ausgangscommit `fce7538a` kontrolliert.
02. `AGENTS.md`, `PROJECT_HANDOFF.md`, `PROJECT_STATUS.md` und P05-Hauptroadmap erneut gelesen.
03. CI `43481846258` für `fce7538a` als erfolgreich abgeschlossen geprüft.
04. Bestehenden Vulkan-X11-Attach und seine anfänglichen VkSurfaceCapabilities untersucht.
05. Persistenten GPU-Frame-Lifecycle mit Fence-, Acquire- und Queue-Submit analysiert.
06. Unterschied zwischen validierten anfänglichen und zur Laufzeit möglicherweise geänderten Surface-Grenzen identifiziert.
07. `swapchain_transform` als während eines Window-Attach eingefrorenen VkSurfaceTransformFlagBitsKHR-Zustand eingeführt.
08. Nach erfolgreichem WSI-Attach den ursprünglichen `caps.currentTransform` gesichert.
09. Auf dem vorhandenen Detach-Pfad den gespeicherten Transform zurückgesetzt.
10. Live-`minImageCount` und optionales `maxImageCount` gegen die tatsächliche persistent gebundene Bildanzahl abgesichert.
11. Live-`maxImageArrayLayers` und gültige min/max Surface-Extents vor GPU-Acquire geprüft.
12. Alte Swapchain-Extent auf weiterhin gültige minimale und maximale Surface-Bildgrößen geprüft.
13. Geänderte Surface-Transformation bzw. verlorene Unterstützung der ursprünglichen Transformation vor dem GPU-Submit blockiert.
14. Bisherige X11-Drawable-Sichtbarkeits-, Größen- und Fixed-Extent-Prüfungen erhalten.
15. Python-Grafikregression für eingefrorenen Transform, Detach-Reset und die Reihenfolge der Live-Capability-Grenzen ergänzt: 84/84 Grafiktests bestanden.
16. Bestehendes Vulkan-Modul erfolgreich kompiliert, native 41/41 CTests bestanden.
17. Echte X11-Clear-GPU-Tests mit 16 und 32 Frames, Unmap/Remap, Resize und vier Ressourcen-Auf-/Abbauzyklen bestanden.
18. Echte X11-World-/Depth-GPU-Tests mit 16 und 32 Frames, Kameraänderung, Resize und GPU-Fence-Regressionskontrollen bestanden.
19. Native Mesh-Rotation-/Region-Barrier-Proben sowie Linux-Vollviewer-Build bestanden: 396/396 Python- und 41/41 native Tests.
20. Frisch gebauten SDL/X11-Viewer mit isoliertem temporärem Profil geprüft: 20/20 Pre-GL-World-/Depth-Frames, 100/100 Pixelproben, drei Fenstergrößen, danach OpenGL `STATE_LOGIN_WAIT`; `git diff --check` erfolgreich.

**Grenzen:** Die negativen geänderten Treiber-Capability-
Antworten wurden als Quellcode-Grenzprüfungen, nicht durch
manipulierte Treiber-/ICD-Antworten getestet. Die echten GPU-
Proben bestätigen den unverändert gültigen Normalpfad und die
bestehenden Resize-/Unmap-Regressionsszenarien. Der SDL-Viewer
wurde nach Login-Wait mit Timeout 124 beendet (kein regulärer
Quit-Nachweis). Keine authentifizierte OpenSim-Region und kein
dauerhaftes Vulkan-In-World-Rendering. Windows/macOS nicht
lokal voll getestet. Keine Nutzerprofile, Secrets oder
laufende Userprozesse geändert, keine neuen Projektdateien.
**P05 formal weiterhin 2/20; Live-Welt-Window-Ownership offen.**

## P05/3 – /goal 20 Teilblöcke: X11-Window-Unmap und GPU-Present-Rückkehr (13.11.2026)

**Umgesetzt:** Die persistente Vulkan-Xlib-Entwickler-WSI
prüft jetzt zusätzlich, ob das von SDL geliehene X11-Fenster
mit `XGetWindowAttributes().map_state==IsViewable` tatsächlich
sichtbar/gemappt ist. Bei `IsUnmapped` oder einer sonstigen
nicht sichtbaren Window-State wird VOR Acquire und Submit
`p.blocked=true` gesetzt und der Present verweigert. Das
bestehende fail-closed-Protokoll verlangt danach Detach und
neuen Attach; die Vulkan-Ressourcen bleiben nicht an ein
vorübergehend ausgehängtes SDL-Fenster gebunden. Der normale
OpenGL-Login bleibt erhalten.

**20 konkret ausgeführte Schritte:**

01. `ogl-develop`-Arbeitsbaum und letzten Commit `8f15a9f1` geprüft.
02. `AGENTS.md`, `PROJECT_HANDOFF.md`, `PROJECT_STATUS.md` und aktive P05-Roadmap erneut gelesen.
03. GitHub-CI für `8f15a9f1` auf `success` kontrolliert.
04. SDL-Pre-GL-Fensterlebensdauer und bestehende Xlib-Drawable-Größenvalidierung analysiert.
05. Vulkan-Persistent-Frame-Clear-/World-/Depth-Acquire-, Submit- und Present-Reihenfolge geprüft.
06. `XGetWindowAttributes`-Abfrage als vorhandenen sicheren Checkpunkt vor GPU-Acquire identifiziert.
07. Das bisher nicht geprüfte X11-Mapping (`map_state`) als Window-Lifecycle-Lücke abgegrenzt.
08. Sichtbarkeitsprüfung mit `IsViewable` im vorhandenen Vulkan-Modul implementiert.
09. Sperre durch den bestehenden `p.blocked=true`-Pfad bei ausgehängtem Fenster beibehalten.
10. Kontrolliert, dass die zusätzliche Prüfung vor `vkAcquireNextImageKHR` und `vkQueueSubmit` stattfindet.
11. Bereits vorhandenen nativen Persistent-Clear-X11-Test um ein drittes WSI-Attach nach Resize/Stale-Reject erweitert.
12. Im echten X11-Testfenster `XUnmapWindow` und `XGetWindowAttributes(...).map_state==IsUnmapped` verifiziert.
13. GPU-Present im nicht gemappten Zustand abgewiesen, unveränderten Framezähler nachgewiesen.
14. Vulkan-Window-Detach und anschließendes `XMapWindow` in den Regressionstest integriert.
15. Viertes WSI-Attach mit erneuter GPU-Pixelverifikation sowie vier Build-/Release-/Idle-Lebenszyklen nachgewiesen.
16. Python-Grafikquellcodetest für Unmap, Remap, Sichtbarkeitscheck und GPU-Checkreihenfolge ergänzt: 83/83 Grafiktests bestanden.
17. Vulkan-Modul neu gebaut, alle 41/41 nativen CTests bestanden.
18. Echte X11-16-/32-Frame-Clear- und World-/Depth-GPU-Regressionsläufe bestanden, inklusive Unmap/Remap im Clear-Test.
19. Echte X11-Mesh-Rotation-/Fence- und Region-Barrier-Tests sowie vollständigen Linux-Viewer-Build bestanden: 395/395 Python-Tests und 41/41 native CTests.
20. Frisch gebauten SDL-Viewer mit isoliertem temporärem Xvfb-Profil geprüft: 20/20 World-/Depth-Pre-GL-Frames, 100/100 Pixelproben, drei Fenstergrößen und anschließend GL `STATE_LOGIN_WAIT`; `git diff --check` bestanden.

**Grenzen:** Die neue Window-Unmap-Regressionsprüfung nutzt
ein echtes X11-Fenster und die persistenten GPU-Testpresents,
nicht eine authentifizierte OpenSim-Welt. Der SDL-Test wurde
per Timeout nach dem Login-Wartezustand beendet (Exit 124;
kein regulärer Shutdown-Nachweis). Windows/macOS-Vollviewer
nicht lokal ausgeführt. Keine Benutzerprofile, Credentials
oder bestehenden Nutzerviewer-Prozesse verändert und keine
neuen Repository-Dateien angelegt. **P05 weiterhin 2/20;
dauerhafte Vulkan-In-World-Window-Ownership nicht fertig.**

## P05/3 – /goal 20 Schritte: X11-Drawable-Größe auch bei variablen Vulkan-Extents validieren (13.11.2026)

**Implementiert:** Die persistente Entwickler-WSI rendert jetzt nur,
wenn die tatsächlichen Abmessungen des geliehenen SDL/X11-
Clientfensters weiterhin mit `swapchain_extent` übereinstimmen.
Das ergänzt die vorhandene Vulkan-`VkSurfaceCapabilitiesKHR`-
Prüfung, deren `currentExtent` bei manchen Treibern beide
Dimensionen als `UINT32_MAX` (variable Größe) melden kann.
`XGetWindowAttributes` wird VOR Image-Acquire und GPU-Submit
verwendet; bei abweichender, fehlender oder ungültiger
Fenstergeometrie wird die bestehende Swapchain bis zum
Detach fail-closed gesperrt. X11-Display und Window-XID bleiben
fremdes SDL-Eigentum, ihre geliehenen Identitäten werden erst
nach erfolgreichem WSI-Attach gespeichert und beim Detach
verworfen. Die neue Abhängigkeit `libX11` wird auf Linux nur
bei gefundenem X11-Entwicklungspaket eingebunden und das
optionale Xlib-Modul damit verknüpft; ohne X11 bleibt
OpenGL beziehungsweise der plattformneutrale Modulpfad.

**20 tatsächlich ausgeführte Entwicklungs- und Prüfschritte:**

01. Sauberes `ogl-develop`, Ausgangscommit `04926a5d` und jüngste Git-Historie geprüft.
02. Verbindliche `AGENTS.md`, Handoff, Status und aktive P05-Roadmap erneut gelesen.
03. CI des Ausgangscommits `04926a5d` mit Ergebnis `success` verifiziert.
04. Vulkan-Xlib-Swapchain-Attach sowie ihre bisherigen Fixed-Extent-Grenzen analysiert.
05. Variablen `currentExtent=UINT32_MAX` als verbleibenden Resize-Blindfleck identifiziert.
06. Vorhandenes SDL-Window-Borrowing und Lifecycle beim Detach geprüft.
07. Zwei Xlib-Window-Identitätsfelder nur für den optionalen Linux-Xlib-Build ergänzt.
08. Geliehenes X11-Display und XID erst nach erfolgreich validiertem Swapchain-Attach gespeichert.
09. Beide geliehenen Identitäten beim Window-Detach wieder auf null gesetzt.
10. Vor persistenten Presents `XGetWindowAttributes` auf dem echten SDL/X11-Fenster eingeführt.
11. Fehlende/ungültige Window-Attribute und inkonsistente native Fenstergrößen fail-closed abgewiesen.
12. X11-Check vor `vkAcquireNextImageKHR` und `vkQueueSubmit` angeordnet, bestehende Vulkan-Caps-Prüfung beibehalten.
13. Xlib-Abhängigkeit im vorhandenen Modul-CMake Linux-spezifisch ergänzt; keine Xlib-Zwangsabhängigkeit für Windows/macOS.
14. Plattform-Compile-Gate an `OGL_XLIB_LINKED` gekoppelt, damit fehlende X11-Entwicklungsbibliotheken keinen unaufgelösten Xlib-Symbolverweis erzeugen.
15. Zwei neue Python-Grafikregressionen für native Drawable-Größe, Lifetime, CMake-Linking und Reihenfolge ergänzt; 82/82 Grafiktests bestanden.
16. Anfängliche Linkregression entdeckt (`XGetWindowAttributes` nicht aufgelöst), durch explizites `libX11`-Linking behoben; `ldd -r` ohne undefinierte Symbole verifiziert.
17. Alle 41/41 nativen CTests nach Linkkorrektur bestanden.
18. Echte X11-GPU-Regressionen: 16/16 und 32/32 persistente Clear- und World/Depth-Frames sowie Resize ohne Detach bestanden.
19. Echte GPU-Regressionsläufe für Mesh-Rotation und Region-Barrier sowie vollständigen Linux-Viewer-Neubuild bestanden: 394/394 Python- und 41/41 native Tests.
20. Frisch gebauten SDL/X11-Viewer in separatem temporärem Profil getestet: 20/20 Pre-GL-World-/Depth-GPU-Presents, 100/100 Pixelproben, drei Fenstergrößen, Vulkan vollständig vor GL-Kontext freigegeben, danach OpenGL `STATE_LOGIN_WAIT`; `git diff --check` erfolgreich.

**Abgrenzung:** Die 16-/32-Frame-GPU-Szenarien und der
SDL-Pre-GL-Test verwenden synthetische Szenen, keine
authentifizierte OpenSim-Welt. Der SDL-Test wurde nach dem
Login-Wartezustand per `timeout` beendet (Exit 124, kein
regulärer Quit-Nachweis). Unter Windows und macOS ist
kein lokaler Vollviewer-Test erfolgt. Keine neuen
Projektdateien und keine Änderungen an Benutzerprofilen
oder Secrets. **P05 operativ unverändert 2/20;
P05/3 echte dauerhafte Vulkan-World-/Window-Ownership
offen.**

## P05/3 – /goal 20 Teilblöcke: veraltete Vulkan-Surface nach Resize verweigern (13.11.2026)

**Implementiert:** Der persistente Entwickler-WSI-Renderer fragt vor
jedem GPU-Frame die aktuellen `VkSurfaceCapabilitiesKHR` ab.
Bei fehlgeschlagener Abfrage, verlorener Color-Attachment-Eignung,
inkonsistentem variable/fixed `currentExtent` oder einer
**festen Surface-Größe, die nicht mehr mit der gebundenen
Swapchain übereinstimmt**, wird der Present-Pfad dauerhaft
für diesen Attach blockiert. Das passiert vor
`vkAcquireNextImageKHR` und `vkQueueSubmit`, damit die
alten Render-/Depth-Ressourcen nicht auf einer inzwischen
vergrößerten SDL/X11-Surface benutzt werden. Ein neuer
Detach und WSI-Attach sind erforderlich. Vulkan bleibt
weiterhin Entwickler-QA vor OpenGL; der produktive
OpenSim-Viewer verwendet unverändert OpenGL.

**20 tatsächlich durchgeführte technische Entwicklungs- und Prüfschritte:**

01. Aktuellen Branch `ogl-develop` und sauberen Arbeitsbaum bei `45105d9a` geprüft.
02. `AGENTS.md`, Übergabe, Projektstatus und aktive P05-Roadmap erneut gelesen.
03. GitHub-CI-Stand der jüngsten Commits kontrolliert; `45105d9a` war `success`.
04. Persistent-Clear-/World-GPU-Present und Xlib-WSI-Surface-Capabilities analysiert.
05. Bisher fehlende Vorabprüfung auf Surface-Resize während einer bestehenden VkSwapchain identifiziert.
06. Vor jedem persistierenden GPU-Present `vkGetPhysicalDeviceSurfaceCapabilitiesKHR` ergänzt.
07. Nicht verfügbare bzw. fehlgeschlagene Surface-Abfragen fail-closed behandelt.
08. Fortbestehende COLOR_ATTACHMENT-Unterstützung vor Acquire geprüft.
09. Widersprüchliche feste/variable Surface-Extent-Werte abgewiesen.
10. Abweichende feste Surface-Größe gegen ursprüngliche Swapchain-Extent abgewiesen.
11. Den Fehlerpfad bis zum Detach durch `p.blocked=true` gesperrt, ohne GPU-Arbeit anzustoßen.
12. Ein fokussierter Python-Grafiktest prüft die Abbruchkriterien und die Reihenfolge vor Acquire/Submit.
13. Bestehenden nativen X11-Clear-Test um einen absichtlichen Resize ohne Detach erweitert.
14. Nativen World-/Depth-Test um dieselbe stale-Swapchain-Verweigerung ergänzt.
15. `80/80` fokussierte Python-Grafiktests bestanden.
16. C++-Module gebaut und `41/41` native CTests erfolgreich ausgeführt.
17. Reale X11-Clear- und World-/Depth-Tests mit je 16 Frames und erzwungenem ungültigem Resize bestanden.
18. Reale X11-Clear- und World-/Depth-Langläufe mit je 32 Frames und demselben Resize-Fehlerfall bestanden.
19. Native Mesh-Rotation-/GPU-Fence- und Region-Barrier-/Stale-Camera-Regressionsläufe unter Xvfb bestanden.
20. Vollständigen Linux-Vollviewer nach vollständigem Neuaufbau erfolgreich kompiliert (392/392 Python-Tests, 41/41 native CTests); danach frisch gelinkten SDL-X11-Viewer in getrenntem Testprofil geprüft: 20/20 GPU-World-/Depth-Presents, 100/100 Pixelproben, drei Größen und `STATE_LOGIN_WAIT` unter OpenGL.

**Abgrenzung:** Die neuen Negativtests prüfen reales Xlib-Resize
mit gültigem Vulkan-ICD, keine simulierte Treiberfehlermeldung.
Die Native-GPU-World ist weiterhin eine synthetische
Sicherheitsprobe und keine angemeldete OpenSim-Region.
Windows/macOS-Vollviewer nicht lokal ausgeführt.
Der SDL-Testprozess wurde nach dem Login-Wartezustand per
Timeout 124 beendet; damit kein regulärer Shutdown-Nachweis.
Keine Benutzerprofile oder Credentials geändert, keine neuen
Quellprojektdateien angelegt. **P05 operativ bleibt 2/20;
Block 3 mit dauerhafter exklusiver Live-Welt-Window-Ownership
offen.**

## P05/3 – /goal 20 Teilblöcke: unveränderliches GPU-Frame-Budget je VkSwapchain (10.11.2026)

**Umgesetzt:** Der bestehende Long-Run-Entwickler-Modus
für persistente Vulkan-GPU-Presents las seine 16/32-
Frame-Grenze bisher bei jedem Frame erneut aus der
Prozessumgebung. Damit war die Grenze während einer
laufenden Swapchain nachträglich veränderbar. Ab jetzt
wird `max_frames` beim erfolgreichen ersten GPU-Frame
für die gesamte Swapchain-Anbindung einmal festgelegt;
der Wert übersteht Änderungen von Entwicklerflags und
wird erst beim vollständigen Detach zurückgesetzt.
Das höhere 32er-Budget erfordert gleichzeitig
`OGL_VULKAN_PERSISTENT_LONG_RUN_PROBE=1` und den
nativen X11-Akzeptanztest `OGL_VULKAN_PRESENT_TEST_X11=1`.
Der normale SDL-Pre-GL-Viewer kann dadurch nicht allein
durch ein Long-Run-Flag die 16er-Grenze verlängern.

**20 tatsächlich ausgeführte Teilblöcke:**

01. Sauberen `ogl-develop`-Git-Stand und Ausgangscommit `d45ee366` geprüft.
02. `AGENTS.md`, `PROJECT_HANDOFF.md`, `PROJECT_STATUS.md` und aktive P05-Roadmap erneut gelesen.
03. GitHub-CI-Status geprüft; vorheriger Commit `d45ee366` auf SUCCESS bestätigt.
04. Vulkan-Persistent-Present-Swapchain und zwei Submit-Slots untersucht.
05. Wiederholtes Lesen der Long-Run-Freigabe bei jedem Present als unerwünschten variablen Zustand identifiziert.
06. Das zur Swapchain gehörende `OglPersistentClearFrames` um ein dauerhaftes `max_frames`-Feld mit Default 16 erweitert.
07. Framegrenze vor weiterer GPU-Arbeit anhand des eingefrorenen Budgets geprüft.
08. Initialisierung fixiert das Framebudget beim ersten erfolgreichen Aufbau der persistenten GPU-Ressourcen.
09. 32er-Budget nur bei beiden nativen Entwickler-/X11-Testfreigaben zugelassen.
10. Automatisches Zurücksetzen des Budgets über die vorhandene vollständige `persistent={}`-Freigabe beibehalten.
11. Bestehenden Grafikvertrag auf per-Swapchain-Budget statt Live-Umgebungsabfrage aktualisiert.
12. Fokussierten Python-Test für initialisierte Grenzen, Native-Gate und Reihenfolge ergänzt.
13. Native X11-Clear-QA so erweitert, dass sie nach dem achten Frame das Long-Run-Flag absichtlich umschaltet.
14. Native X11-World-/Depth-QA ebenso um Flagwechsel beim achten Frame ergänzt, gleichzeitig mit Kamerabewegung.
15. Die Testumgebung nach jeder Anbindung wieder in den ursprünglichen Zustand versetzt; eigener Python-Regressionstest ergänzt.
16. 78/78 Python-Grafiktests und 41/41 native CTests bestanden, C++-Module erfolgreich gebaut.
17. Native Echt-GPU-Clear-Presents ohne und mit Long-Run bei wechselndem Opt-in bestanden: 16/16 und 32/32 Frames.
18. Native Echt-GPU-World-/Depth-Presents mit Kamerabewegung, Tiefentest und Flagwechsel bestanden: 16/16 und 32/32 Frames.
19. Vollständigen Linux-Viewer erfolgreich gebaut: 390/390 Python-Tests und 41/41 native CTests bestanden; `git diff --check` sauber.
20. Isolierten echten SDL/X11-Viewer mit Long-Run-Flag, aber ohne native Testfreigabe geprüft: 20/20 Pre-GL-Frames über drei Fenstergrößen, 100/100 sichtbare Pixel, danach OpenGL `STATE_LOGIN_WAIT`; die separate Long-Run-Grenze bleibt nicht für den SDL-Viewer freigeschaltet.

**Grenzen:** Native Vulkan-X11-Proben sind isolierte
synthetische GPU-Szenarien, keine authentifizierte
OpenSim-In-World-Darstellung. Der SDL-Pre-GL-Test ist
kein dauerhafter Vulkan-Renderer; er wurde nach dem
Login-Wartezustand per Test-Timeout beendet (Exit 124,
kein regulärer Quit). Keine manipulierten Treiberantworten
oder Windows-/macOS-Vollviewer-Starts. Keine neuen
Projektdateien, Userprofile, Secrets oder gefährlichen
Altdateilöschungen. **P05 operativ bleibt 2/20;
Block 3 Live-Vulkan-Weltfensterübernahme offen.**

## P05/3 – /goal 20 Entwicklungs-/QA-Teilblöcke: 32 persistente Vulkan-GPU-Frames (10.11.2026)

**Umgesetzt:** Der persistente Vulkan-X11-GPU-Pfad bleibt ohne
zusätzliche Entwicklerfreigabe strikt auf 16 GPU-Presents pro
Swapchain-Anbindung begrenzt. Mit
`OGL_VULKAN_PERSISTENT_LONG_RUN_PROBE=1` wird die Grenze
für einen separat aktivierbaren nativen QA-Stresstest auf
**32 Frames** erweitert. Der neue QA-Modus testet die
Wiederverwendung der zwei GPU-Submit-Fences, der
Image-indexierten Render-Finished-Semaphoren, des
VkCommandPool und der Renderpass-/Depth-Ressourcen über
16 Frames hinaus. Er baut die Ressourcen pro Anbindung
nur einmal auf und gibt sie beim Detach wieder frei.
Keine Freigabe als produktiver In-World-Renderer.

**20 tatsächlich ausgeführte Schritte:**

01. Sauberen `ogl-develop` und Ausgangscommit `73822e58` geprüft.
02. `AGENTS.md`, Handoff, Projektstatus und P05-Roadmap erneut gelesen.
03. Aktuelle CI der letzten Pushes geprüft; frühere Gesamtläufe nach tatsächlichem Ergebnis unterschieden.
04. Persistente Swapchain-/GPU-Fence-Lebenszyklen im Vulkan-Modul analysiert.
05. Die bisherige harte 16-Frame-Grenze auf ihre Eignung für längere QA-Szenarien geprüft.
06. Verwendete Image-Fences, Bildindex-Semaphore und zwei Submit-Slots verifiziert.
07. Ein zusätzliches, ausdrücklich freizugebendes 32-Frame-QA-Limit im bestehenden Modul umgesetzt.
08. Ohne Long-Run-Opt-in die bisherige 16-Frame-Grenze erhalten.
09. Native X11-Clear-QA in der bestehenden Testdatei auf wahlweise 16 oder 32 Frames erweitert.
10. Eindeutige native Long-Run-Testfreigabe und abschließendes Zurücksetzen des Modul-Opt-ins ergänzt.
11. Image-/Fence-/Cache-Lifecycle bei 32 wiederholten GPU-Submits mitgezählt.
12. Native X11-World-/Depth-QA auf die längere 32-Frame-Sequenz erweitert.
13. Bestehende Kameraänderung, unveränderte Meshbuffers und Depth-Pixelprüfung erhalten.
14. Testmeldungen um tatsächliche Frame-Zielzahl erweitert; kein pauschaler 16-Frame-Erfolg bei 32 angegeben.
15. Bestehenden Python-Quellcodevertrag auf variable, ausdrücklich begrenzte Framezahl angepasst.
16. Neuen Python-Grafikregressionstest für getrennte Freigabe, Limit und native Testabdeckung ergänzt: 76/76 Grafiktests bestanden.
17. Native C++-Module erfolgreich neu gebaut und 41/41 native CTests bestanden.
18. Echte X11-GPU-Long-Run-QA: 32/32 persistente CLEAR-Presents und 32/32 World-/Depth-Presents auf jeweils einer Swapchain, Resize und Detach bestanden.
19. Normale X11-QA ohne Long-Run-Opt-in erneut bestanden: 16/16 CLEAR und 16/16 World-/Depth-Presents; anschließend vollständigen Linux-Viewer gebaut und 388/388 Python- sowie 41/41 native Tests bestanden.
20. Isolierten echten SDL/X11-Viewerstart nach Vollbuild geprüft: 20/20 Pre-GL-Clear-Presents, 100 Pixelproben, drei Größen, anschließender OpenGL-Login-Wartezustand; `git diff --check` bestanden.

**Grenzen:** Long-Run-Freigabe ist nur für Entwickler-QA; die
32 GPU-Frames finden in separaten nativen X11-Testfenstern
und nicht innerhalb einer authentifizierten OpenSim-Welt
statt. SDL-Pre-GL bleibt bei seiner bisherigen 20-Frame-
Probe. Kein Windows-/macOS-Vollviewer-Test; keine gezielte
GPU-Fault-Injektion. SDL-Viewer nach Login-Wartezustand per
Test-Zeitlimit beendet (Exit 124), kein regulärer Quit.
Keine User-Profile oder Secrets verändert und keine neuen
Repository-Dateien angelegt. **P05 formal weiterhin 2/20;
P05/3 mit dauerhafter Vulkan-In-World-Fensterübernahme offen.**

## P05/3 – /goal 20 Teilblöcke: SDL-Vulkan-Preflight aller Probe-Berechtigungen (10.11.2026)

**Umgesetzt:** Der bestehende SDL/X11-Startpfad validiert jetzt auch
die Entwickler-Berechtigungspaare für Depth, Persistent-Clear,
Persistent-World, Mesh-Rotation und Region-Barrier sowie das
zusätzliche SDL-Dual-Sync-Opt-in **vor** `SDL_GetWindowWMInfo()`
und `SDL_LoadObject()` des Vulkan-Moduls. Bisher wurden solche
Mismatch-Konfigurationen erst nach dem Laden der Bibliothek
abgewiesen. Der Fail-Closed-Abbruch verändert weder OpenGL noch
Benutzerprofile. Bestehende modulseitige Prüfungen bleiben erhalten.

**20 tatsächlich bearbeitete Entwicklungs- und Prüfschritte:**

01. Git-Status `ogl-develop`, sauberer Branch und Ausgangscommit `f52e9435` kontrolliert.
02. `AGENTS.md`, `PROJECT_HANDOFF.md`, `PROJECT_STATUS.md` und Hauptroadmap P05 gelesen.
03. Aktuelle GitHub-CI-Läufe abgefragt; laufenden jüngsten Lauf nicht als bestanden ausgegeben.
04. SDL-X11-Pre-GL-Fensterübernahme und deren zeitliche Reihenfolge untersucht.
05. Bestehende Freigaben für Depth und Raster/World analysiert.
06. Freigaben für Persistent-Clear und Persistent-World analysiert.
07. Separate Freigaben für GPU-Mesh-Rotation und Region-Barrier analysiert.
08. Zusätzliche duale GPU-Fence-Synchronisationsfreigabe geprüft.
09. Identifiziert, dass die genannten Freigabemismatches erst nach SDL-Modulladen abgefangen wurden.
10. Vorzeitige Freigabeprüfung für Depth in der bestehenden SDL-Quelldatei implementiert.
11. Vorzeitige Freigabeprüfung für Persistent-Clear und Persistent-World implementiert.
12. Vorzeitige Freigabeprüfung für Mesh-Rotation und Region-Barrier implementiert.
13. Vorzeitige Abweisung fehlender Dual-Sync-Modulberechtigung implementiert.
14. Bestehende normale OpenGL-Initialisierung und späte Defense-in-Depth-Prüfung erhalten.
15. Fokussierten Python-Grafikregressionstest mit Prüfungen der Flagpaare und Reihenfolge ergänzt: 75/75 Grafiktests bestanden.
16. Vulkan-Modultest gebaut; native CTests 41/41 bestanden.
17. Vollständigen Linux-Viewer gebaut: 387/387 Python- und 41/41 native Tests bestanden.
18. Zwei unabhängige SDL/X11-Starts mit einseitigen Depth- bzw. Persistent-Clear-Freigaben getestet: beide ohne Vulkan-Present, OpenGL `STATE_LOGIN_WAIT` erreicht.
19. Zwei weitere SDL/X11-Starts mit einseitigem Region-Barrier- bzw. Dual-Sync-Flag getestet: beide ohne Vulkan-Present, OpenGL `STATE_LOGIN_WAIT` erreicht.
20. Positiven SDL/X11-Start mit vollständig freigegebener Persistent-Clear-20-Frame-Sequenz getestet: 20/20 Frames, Pixelverifikation und OpenGL `STATE_LOGIN_WAIT`; `git diff --check` bestanden.

**Abgrenzung:** Die fünf SDL-Probeläufe wurden mit eigenen
Xvfb-Anzeigen und temporären Profilen nach der Login-Prüfung
per `timeout` (Exit 124) beendet, nicht per regulärem Quit.
Die positive Probe benutzt synthetische Clear-Frames vor der
GL-Kontexterzeugung; sie zeigt keine authentifizierte
OpenSim-Region und keine dauerhafte Vulkan-Weltengine.
Windows/macOS-Vollviewer nicht abgenommen, keine neuen
Projektdateien oder unsicheren Löschungen. **P05 operativ
2/20; P05/3 Live-In-World-Window-Ownership offen.**

## P05/3 – /goal 20 technische Schritte: SDL-20-Frame-Freigabe muss vollständig sein (10.11.2026)

**Konkret umgesetzt:** Die echte SDL/X11-Viewer-Startprobe hat
zwei getrennte Freigaben für die 20-Frame-Vulkan-Sequenz.
Bislang konnte eine fehlende Freigabe unbemerkt in einen
einzelnen Vulkan-Testframe zurückfallen. Jetzt bewirkt
`sequence_requested!=sequence_permitted` einen ausdrücklichen,
frühen Abbruch **vor Vulkan-Modulladen und Window-Attach**;
OpenGL wird normal initialisiert. Nur beide Freigaben zusammen
erlauben die 20 Frames. Die bestehende Vulkan-Welt ist weiter
nur ein isolierter Entwickler-Pre-GL-Test.

**20 tatsächlich ausgeführte Arbeits-/Prüfschritte:**

01. `ogl-develop`, Git-Status und Ausgangscommit `28dfa2cd` geprüft.
02. Verbindliche `AGENTS.md`, `PROJECT_HANDOFF.md`, `PROJECT_STATUS.md` und P05-Roadmap gelesen.
03. GitHub-CI-Status des vorherigen Pushes kontrolliert, keine unbestätigte neue CI-Freigabe angegeben.
04. Vulkan-Persistent-Present-Fence-, Acquire- und Queue-Lifecycle auf gefährliche automatische Umschaltungen untersucht.
05. Echten SDL-X11-Pre-GL-Present-Code samt bestehenden Fault-Gates geprüft.
06. Die beiden separaten SDL-/Modul-Freigaben für die 20er-Sequenz identifiziert.
07. Fehlverhalten bei nur einer Freigabe (impliziter Single-Frame-Fallback) festgestellt.
08. Beide Flagwerte im vorhandenen SDL-Fensterpfad explizit als boolesche Freigaben erfasst.
09. Einseitige Entwicklerfreigabe vor `SDL_GetWindowWMInfo` und Vulkan-Modulladen fail-closed abgewiesen.
10. Technische Warnmeldung für die unvollständige Freigabe implementiert, ohne in den normalen Login einzugreifen.
11. Bestehende 20-Frame-Bedingung auf die beiden validierten Werte umgestellt.
12. In der bestehenden Python-Grafiktestdatei eine Guard-/Reihenfolge-Regression implementiert.
13. Bereits vorhandenen 20-Frame-Quellcodetest auf die neue Freigabevariable angepasst.
14. 74/74 Grafiktests und 41/41 native CTests bestanden; Vulkan-Modul neu kompiliert.
15. Vollständigen Linux-Viewer mit `bash ogl/scripts/build-test.sh` gebaut: 386/386 Python- und 41/41 native Tests bestanden.
16. Isolierten Xvfb-SDL-Start nur mit SDL-Sequenzflag durchgeführt: Vulkan-Test vollständig übersprungen, OpenGL `STATE_LOGIN_WAIT` erreicht.
17. Unabhängigen Xvfb-SDL-Start nur mit Modul-Frame-Loop-Flag durchgeführt: ebenfalls kein Vulkan-Present, OpenGL `STATE_LOGIN_WAIT` erreicht.
18. Dritten isolierten SDL-Start mit beiden Sequenzflags und Persistent-Clear-Freigabe geprüft: 20/20 Vulkan-Frames, verifizierte Pixel und danach OpenGL `STATE_LOGIN_WAIT`.
19. Native X11-Persistent-Clear-GPU-QA mit 16 Frames, Recreate und Teardown bestanden.
20. Native X11-World-/Depth-GPU-QA mit 16 Frames, Kamera und Resize bestanden; `git diff --check` ohne Beanstandung.

**Abgrenzung:** Die drei SDL-Tests liefen in separaten temporären
Profilen und wurden nach dem Pre-Login-Test kontrolliert mit
Zeitlimit beendet (`timeout` 124 ist kein regulärer Shutdown).
Es wurde keine authentifizierte OpenSim-Welt betreten und kein
in-world Vulkan-Renderer aktiviert. Keine Nutzerprofile oder
Secrets geändert, keine neuen Quellprojektdateien angelegt und
keine ungesicherten Altbestände gelöscht. Windows/macOS-
Vollviewer nicht ausgeführt. **P05 operativ unverändert 2/20;
Block 3 bleibt bis zur dauerhaften sicheren SDL-Vulkan-
In-World-Fensterübernahme offen.**

## P05/3 – /goal 20 Teilblöcke: kohärente Vulkan-Surface-Extents und Swapchain-Bildgrenzen (10.11.2026)

**Änderung:** `attach_world_xlib_window()` verweigert
inkonsistente `VkSurfaceCapabilitiesKHR` vor dem
Swapchain-Aufbau. Vulkan erlaubt bei `currentExtent`
beide Dimensionen variabel (`UINT32_MAX`) oder beide
fest; ein gemischtes Paar wird ausgeschlossen.
Ebenfalls früh abgewiesen: keine Mindestbildanzahl,
mehr als 16 erforderliche Swapchain-Bilder (Limit des
bestehenden GPU-Framecaches) und null zulässige
Image-Array-Layers. Ansonsten blieben Auswahl von
FIFO, Present-Queue, GL-Fallback und opt-in-Schutz
unverändert.

**20 tatsächlich durchgeführte technische Schritte/Prüfungen:**

01. Sauberen Git-Stand `ogl-develop` und Ausgangscommit `de8877ce` geprüft.
02. Projektanweisungen `AGENTS.md`, Handoff, Status und `/goal`-Roadmap erneut gelesen.
03. Aktuelle GitHub-CI-Läufe nach Zustand und Plattformjobs geprüft.
04. Beim älteren CI-Fehler die fünf erfolgreichen Jobs und den abgebrochenen macOS-15-Job abgegrenzt; kein pauschales Plattform-Fail behauptet.
05. Vulkan-Surface-Fähigkeiten im vorhandenen Xlib-Attach untersucht.
06. Vulkan-Sentinel-Regel für variable/feste `currentExtent`-Dimensionen kontrolliert.
07. Gemischte feste/variable Extents vor `vkCreateSwapchainKHR` ausgeschlossen.
08. Ungültiges `minImageCount==0` vor Swapchain-Erstellung ausgeschlossen.
09. `minImageCount>16` passend zum begrenzten persistenten Framecache ausgeschlossen.
10. `maxImageArrayLayers==0` vor Swapchain-Erstellung ausgeschlossen.
11. Bestehende Prüfungen für Größen-Min/Max, Transform, FIFO und Image-Enumeration beibehalten.
12. In der bestehenden Python-Grafiktestdatei einen fokussierten Guard-/Reihenfolgetest ergänzt.
13. 73/73 Grafiktests bestanden.
14. Vorhandenes Vulkan-Modul als native C++-Shared-Library neu gebaut.
15. Alle 41/41 native CTests einschließlich GPU-Regressionssuite bestanden.
16. Echtes isoliertes X11-Persistent-Clear-Present mit 16 Frames und Resize/Recreate bestanden.
17. Echtes isoliertes X11-World-/Depth-Present mit 16 Frames, Kamera und Resize bestanden.
18. Vollständigen Linux-Firestorm-Viewer mit `bash ogl/scripts/build-test.sh` kompiliert.
19. Gesamttest des Linux-Viewers: 385/385 Python-Tests und 41/41 native CTests bestanden.
20. Tatsächlichen SDL-Viewer in separatem Xvfb/Testprofil getestet: 20/20 Vulkan-World-/Depth-Pre-GL-Presents, 100/100 Pixelproben, 3 Größen, anschließend GL-Kontext und `STATE_LOGIN_WAIT`; `git diff --check` erfolgreich.

**Abgrenzung:** Der SDL-Test rendert eine synthetische
Testwelt vor OpenGL, nicht die angemeldete OpenSim-Welt.
Kein regulärer Shutdown-Test (Testprozess per Timeout).
Keine injizierten fehlerhaften ICD-Rückgaben, keine
Windows-/macOS-Vollviewer-Abnahme. Keine neuen
Repository-Dateien und keine Änderungen an echten
Benutzerprofilen oder Secrets. Keine verlässlich
identifizierten Altdateien gelöscht. **P05 operativ
weiterhin 2/20, Block 3 (dauerhafte exklusive
Vulkan-In-World-Window-/Present-Ownership) offen.**

## P05/3 – /goal 20 Teilblöcke: Surface-Format-Validierung, SDL-World-Pre-GL und fehlender ICD (10.11.2026)

**Ergebnis:** Die bisherige Vulkan-X11-Swapchain akzeptierte
`VK_FORMAT_UNDEFINED` in einer Liste mit mehreren Formaten,
obwohl der Vulkan-Sonderfall nur bei genau einem gemeldeten
Surface-Format zulässig ist. Der vorhandene Attach-Pfad
verwirft nun einen solchen widersprüchlichen Formatdatensatz
vor `vkCreateSwapchainKHR`. Der legale Single-Format-Fall
bleibt mit einer expliziten B8G8R8A8/SRGB-Auswahl erhalten.
Der produktive Renderer bleibt OpenGL.

**20 konkret ausgeführte Entwicklungs- und Prüfschritte:**

01. Sauberen `ogl-develop` und Ausgangscommit `257dabf2` geprüft.
02. `AGENTS.md`, `PROJECT_HANDOFF.md`, `PROJECT_STATUS.md` und P05-Roadmap gelesen.
03. GitHub-CI-Status des letzten Branch-Stands geprüft; laufender Workflow noch nicht als erfolgreich bezeichnet.
04. Vulkan-Device-/Swapchain- und persistenten Frame-Lifecycle untersucht.
05. Den vollständigen SDL-X11-Pre-GL-World-/Depth-Present-Codepfad und die Entwicklerfreigaben geprüft.
06. Ein echtes isoliertes SDL-X11-20-Frame-World-/Depth-Szenario mit allen erforderlichen Opt-ins ausgeführt.
07. Dabei 20/20 tatsächlich präsentierte GPU-Frames mit Indexed-Geometry/Camera/Depth verifiziert.
08. Die 100/100 sichtbaren RGB-Pixelproben und drei unterschiedlichen Fenstergrößen bestätigt.
09. Sicheren Vulkan-Abbau vor GL-Kontext und danach den OpenGL-Login-Wartezustand belegt.
10. Format-Auswahllogik auf die Vulkan-Spezialregel `VK_FORMAT_UNDEFINED` hin untersucht.
11. Fehlerfall `VK_FORMAT_UNDEFINED` bei mehr als einem Surface-Format erkannt.
12. Im vorhandenen Vulkan-C++-Modul Multi-Format-Undefined-Einträge fail-closed ausgeschlossen.
13. Legale einmalige Undefined-Format-Abfrage mit B8G8R8A8_UNORM/SRGB-Auswahl erhalten.
14. Regressionstest in bestehender `ogl/tests/test_public_graphics.py` für beide Regeln ergänzt.
15. Modul erfolgreich neu gebaut; 72/72 Python-Grafiktests und 41/41 native CTests bestanden.
16. Reale X11-Clear-/Persistent-WSI-GPU-Tests mit 16 Frames, Resize/Recreate und Teardown bestanden.
17. Reale X11-World-/Depth-GPU-Tests mit 16 Frames, Kameraänderung und Resize bestanden.
18. Vollständigen Linux-Viewer mit `bash ogl/scripts/build-test.sh` gebaut; 384/384 Python- und 41/41 native Tests bestanden.
19. Aktualisierten SDL-Viewer erneut vor GL mit 20/20 GPU-World-/Depth-Presents, 100 Pixelproben, 3 Fenstergrößen und anschließendem OpenGL-Login-Wartezustand verifiziert.
20. Unabhängigen SDL-X11-Test mit nicht vorhandenem Vulkan-ICD ausgeführt: Vulkan 0/1 und `surface=unavailable`, danach dennoch `STATE_LOGIN_WAIT` unter OpenGL; `git diff --check` bestanden.

**Grenzen:** Die 20 sichtbaren GPU-World-/Depth-Frames benutzen
synthetische Szenengeometrie vor GL-Kontexterzeugung. Sie sind
kein authentifizierter Live-OpenSim-Renderer und keine dauerhafte
Vulkan-In-World-Fensterübernahme. Die negativen Formatlisten
wurden über Quellcode-Regressionen, nicht über manipulierte
Treiberantworten geprüft. Test-Viewer sind isolierte temporäre
Prozesse und wurden per Zeitlimit beendet; kein regulärer
Shutdown-Nachweis. Keine Nutzerprofile/Secrets geändert, keine
neuen Quellprojektdateien angelegt, keine ungesicherten
Altdateien gelöscht; Windows/macOS ungetestet.
**Operativer P05-Zähler bleibt 2/20: Block 3 ist offen.**

## P05/3 – /goal 20 Entwicklungs- und QA-Teilblöcke: Vulkan-WSI-Capabilities und echtes SDL-Pre-GL-Present (10.11.2026)

**Ergebnis:** Die bestehende Linux-X11-Vulkan-Swapchain-Erstellung
verweigert ab jetzt inkonsistente Surface-Capabilities schon vor
`vkCreateSwapchainKHR`: ungültige minimale Extents, min>max,
unsupported `currentTransform`, `maxImageCount<minImageCount`
sowie feste/extents nach Clamping außerhalb der gemeldeten Grenzen.
Die Prüfung ist auf Entwickler-WSI begrenzt; OpenGL bleibt produktiv.

**20 tatsächlich durchgeführte Teilblöcke und Nachweise:**

01. Sauberen `ogl-develop` gegen `origin/ogl-develop` geprüft; Ausgangscommit `2bb257fc`.
02. `AGENTS.md`, Übergabe, Projektstatus und aktive `/goal`-Roadmap erneut eingelesen.
03. Ausstehende CI-Läufe des vorherigen Branch-Stands abgefragt; keine unbelegten Plattformfreigaben.
04. Den tatsächlichen SDL-Pre-GL-Fensterpfad und Vulkan-Xlib-Attach-Aufrufkette analysiert.
05. Vulkan-Surface-Capability-Prüfpfad einschließlich Resize/Extent-Klemmung untersucht.
06. Die bisher fehlende Validierung von minimalen, maximalen und festen Extents identifiziert.
07. Leere minImageExtent-Werte vor Swapchain-Erstellung abgewiesen.
08. Umgekehrte min/max-Grenzen für Breite und Höhe abgewiesen.
09. Nicht unterstützte `currentTransform`-Bitangaben abgewiesen.
10. Widersprüchliche min/max-Swapchain-Bildanzahl abgewiesen.
11. Tatsächlich gewählte Breite/Höhe nach Clamp gegen beide Grenzen validiert.
12. Fehlerpfade weiterhin transaktional mit `detach_window()` und unverändertem GL-Fallback ausgeführt.
13. Eine neue fokussierte Python-Regression in `ogl/tests/test_public_graphics.py` ergänzt.
14. Guards vor dem realen `VkSwapchainCreateInfoKHR` durch Regression geprüft (71/71 Grafiktests grün).
15. Vulkan-C++-Modul gebaut und 41/41 native CTests bestanden.
16. Native echte X11-Clear-/Persistent-WSI-Frames: 16 Bilder, Resizes/Recreate/Teardown bestanden.
17. Native echte X11-World-/Depth-Frames: 16 Bilder, Kamerabewegung, Resize und Depth bestanden.
18. Ersten isolierten echten SDL/X11-Vollviewer-Pre-GL-Start mit 20/20 Vulkan-Presents, 100/100 Pixelproben und 3 Extents validiert; danach OpenGL `STATE_LOGIN_WAIT`.
19. Vollständigen Linux-Vollviewer mit `bash ogl/scripts/build-test.sh` gebaut: 383/383 Python-Tests und 41/41 CTests bestanden.
20. Zweiten isolierten SDL/X11-Pre-GL-Start mit neuem Build erneut geprüft: 20/20 Vulkan-Presents, 100/100 Pixelproben, 3 Extents, erfolgreicher Vulkan-Detach und OpenGL-Login-Wartezustand. Keine Userprofile verändert. Der Viewer wurde nach dem Prelogin-Smoke gezielt mit Timeout 124 beendet, nicht regulär heruntergefahren; `git diff --check` sauber.

**Präzisierung:** Die neuen ungültigen Treiberantworten sind
quellcodebasiert abgesichert, nicht durch einen gezielt gefälschten
Vulkan-ICD zur Laufzeit injiziert. Die SDL-Vulkan-Presents zeigen
synthetische Clear-Pixel ausdrücklich **vor** dem OpenGL-Kontext,
keine angemeldete OpenSim-3D-Welt und keinen dauerhaften
Vulkan-Renderer. Keine Windows-/macOS-Vollviewer-Läufe.
Keine neuen Dateien im Repository oder blind entfernte Altdateien.
**Operativer P05-Stand: 2/20; Block 3 bleibt bis zur echten
exklusiven In-World-Vulkan-Window-/Frame-Übergabe offen.**

## P05/3 – /goal 20 technische Teilblöcke: stabile Xlib-Surface-Fähigkeiten (10.11.2026)

**Ergebnis:** Die bestehende passive Linux-X11-WSI-Erstellung
verlangt nun vollständig konsistente Vulkan-Enumerationen auch
für `VkSurfaceFormatKHR` und `VkPresentModeKHR`. In der
zweistufigen Vulkan-Abfrage konnte sich die Anzahl zwischen
Count- und Datenaufruf ändern; dann drohten die Verarbeitung
unvollständiger Array-Einträge oder ungültige Formatwahl.
Der Attach-Pfad bricht nun bei `VK_INCOMPLETE`, veränderter
Anzahl oder unzulässiger Undefined-Format-/Farbraum-Kombination
vor dem Erzeugen der Swapchain fail-closed ab. Keine
User-Profile oder Zugangsdaten angefasst.

**20 tatsächlich bearbeitete Entwicklungs- und Prüfschritte:**

01. Sauberen lokalen Branch `ogl-develop` und Ausgangscommit `85c75836` geprüft.
02. Repository-Anweisungen, Handoff, Status und P05-Hauptroadmap gelesen.
03. GitHub-CI-Status des vorherigen Commits abgefragt (noch queued zum Prüfzeitpunkt).
04. Xlib-WSI-Oberflächen-/Geräte- und Queue-Vertrag geprüft.
05. Surface-Format-Count- und Datenabfrage mit ihren Abbruchbedingungen geprüft.
06. Fehlende Gleichheit des zweiten Format-Counts erkannt.
07. Zweiten Format-Count gegenüber dem ersten unveränderlich abgesichert.
08. Undefined-Format-/Farbraum-Kombinationen vor der Formatwahl überprüft.
09. FIFO-Present-Mode-Count-/Datenabfrage geprüft.
10. Fehlende Gleichheit des zweiten Present-Mode-Counts erkannt.
11. Zweiten Present-Mode-Count gegenüber dem ersten abgesichert.
12. Abbruch und `detach_window()` bei inkonsistenten Fähigkeiten beibehalten.
13. Readiness-Flag erst nach verifizierter Format-/Present-/Imageabfrage erhalten.
14. Eine neue Python-Grafikregression in der bestehenden Testdatei ergänzt.
15. Regression prüft Count-Guards vor Formatnutzung und WSI-Ready-Markierung.
16. Vulkan-C++-Modul erfolgreich neu kompiliert; 70/70 Grafiktests bestanden.
17. Native Gesamtsuite separat geprüft: 41/41 CTests bestanden.
18. Echte Vulkan-X11-Clear-/Persistent-GPU-QA: 16 Frames, Resize/Recreate, Teardown bestanden.
19. Echte Vulkan-X11-World-/Depth-GPU-QA: 16 Frames, Kameraänderung und Resize bestanden.
20. Linux-Vollviewer mit `bash ogl/scripts/build-test.sh` gebaut: 382/382 Python- und 41/41 native Tests grün; `git diff --check` ohne Beanstandung.

**Grenzen:** Die neuen Fehlerfälle sind durch Quellcode- und
Regressionstests, nicht durch künstlich injizierte Treiberantworten
verifiziert. Die GPU-Nachweise nutzen isolierte X11-Testfenster,
keine authentifizierte OpenSim-Welt. Windows-/macOS-Vollviewer
nicht geprüft. Keine nachweislich redundanten Quellressourcen
gefunden; keine ungesicherten Löschungen vorgenommen.
**Operativer P05-Stand bleibt 2/20; Block 3 mit echter
SDL-Viewer-Fensterübernahme und Live-Vulkan-Present offen.**

## P05/3 – /goal 20 technische Entwicklungs-/QA-Teilblöcke: persistente WSI-Imagevalidierung (10.11.2026)

**Ziel und Grenze:** Das bereits vorhandene passive Xlib-WSI-Attach
prüft Swapchain-Image-Handles; der separate, tatsächlich Frames
präsentierende persistente X11-Vulkan-Pfad musste denselben
fail-closed-Vertrag erhalten. Diese 20 Teilblöcke betreffen
**P05/3 als technische Vorarbeit**, nicht 20 erledigte operative
Phasenblöcke. Reales SDL-/OpenSim-Vulkan-Window-Ownership bleibt offen.

**20 ausgeführte Entwicklungs- und Prüfschritte:**

01. Arbeitsbranch `ogl-develop`, sauberen Arbeitsbaum und Ausgangscommit `5766501f` geprüft.
02. `AGENTS.md`, `PROJECT_HANDOFF.md`, `PROJECT_STATUS.md` und aktive `/goal`-Roadmap erneut gelesen.
03. Die vorhandene Xlib-WSI-Attach-Validierung auf unveränderte Bildanzahl und gültige Handles untersucht.
04. Vulkan-Persistent-Present-Initialisierung bis zur Image-Enumeration analysiert.
05. Fehlenden Schutz gegen zwischen beiden Abfragen geänderte Swapchain-Bildanzahl lokalisiert.
06. Fehlenden Schutz gegen nullwertige `VkImage`-Handles im persistenten Bildarray lokalisiert.
07. Im bestehenden Vulkan-Modul die erwartete Bildanzahl nach dem ersten API-Aufruf eingefroren.
08. Zweite Image-Abfrage mit explizit nullinitialisiertem `VkImage`-Array verbunden.
09. Jede Abweichung der zurückgegebenen tatsächlichen Bildanzahl fail-closed abgewiesen.
10. Null-Image-Handles vor ImageView-/Framebuffer-Erzeugung fail-closed abgewiesen.
11. `p.image_count` ausschließlich nach vollständiger Image-Enumeration aktualisiert.
12. Bestehenden transaktionalen Fehlschlag- und GPU-Ressourcenfreigabepfad beibehalten.
13. Keine WSI-/Present-Aktivierung im produktiven OpenGL-Viewer vorgenommen.
14. In bestehender `test_public_graphics.py` Regressionen für Guards und deren Reihenfolge ergänzt.
15. Vulkan-Modul als native C++-Shared-Library erfolgreich neu kompiliert.
16. 69/69 gezielte Python-Grafiktests und 41/41 native CTests bestanden.
17. Echtes isoliertes X11-Vulkan-Clear-Present: 16 GPU-Frames, Frame-Limit und zwei Resizes bestanden.
18. Persistentes Clear-Present: ein Cache-Aufbau, Wiederverwendung, Resize/Recreate und Teardown-Queue-Idle bestanden.
19. Echtes isoliertes X11-Vulkan-World-/Depth-Present: 16 GPU-Frames, Kamerabewegung, unverändertes Mesh, Resize/Recreate bestanden.
20. Vollständigen Linux-Mint-Viewer erfolgreich gebaut und 381/381 Python- sowie 41/41 native Tests bestanden; `git diff --check` geprüft.

**Dateiregel:** Nur bestehende Quell-/Test- und Dokumentationsdateien
aktualisiert; keine neuen Repository-Dateien, kein Zugriff auf
Benutzerprofile oder Anmeldedaten. Keine nachweislich unbenutzten
Altdateien im betroffenen Pfad gefunden und daher nichts gelöscht.

**Nicht nachgewiesen:** Injektion eines inkonsistenten realen
Vulkan-Treiberrückgabewerts, Live-Viewer-Fensterübernahme unter
SDL, authentifizierte OpenSim-Vulkan-Welt, Windows/macOS-Vollviewer.
Die vorhandenen GPU-Präsentationen laufen in separaten
X11-QA-Testfenstern. **P05 bleibt operativ 2/20,
Block 3 offen.**

## P05/3 – WSI-Swapchain-Imagevalidierung (10.11.2026)

**Umgesetzt:** Die bestehende Vulkan-Xlib-WSI-Attach-Funktion
`attach_world_xlib_window()` in
`ogl/modules/rtx/ogl_rtx_module.cpp` meldet die
Swapchain erst nach vollständiger zweiter
`vkGetSwapchainImagesKHR`-Abfrage als bereit.
Ein Fehler/VK_INCOMPLETE, eine geänderte Bildanzahl
oder ein Null-Imagehandle verwirft das WSI-Attach
transaktional über `detach_window()`; OpenGL erhält
weiter allein die Fenster-/Present-Verantwortung.
Die neue Regression in `ogl/tests/test_public_graphics.py`
prüft die Guard-Reihenfolge und verhindert versehentliches
Acquire/Present innerhalb der passiven Attach-Funktion.
Keine neuen Projektdateien erzeugt.

**Prüfung:** Realer Linux-Mint-Vollviewer mit
`bash ogl/scripts/build-test.sh` gebaut;
**380/380 Python-Tests**, **41/41 native CTests**,
separat **68/68 Grafiktests** erfolgreich,
`git diff --check` sauber.
Die Tests decken den Quellcodevertrag und vorhandene
GPU-Testfälle ab; ein künstlich injizierter
Treiberfehler in der zweiten Enumerationsabfrage,
produktiv authentifizierte OpenSim-Welt und
Windows-/macOS-Vollviewer wurden nicht abgenommen.
**P05 operativ weiterhin 2/20.** Block 3:
exklusives SDL-Vulkan-Window-Ownership und sichtbares
Live-Present bleiben offen.

## P05/3–4 – HQ-Atlas-Grenzfallregression (09.11.2026)

**Zusätzliche QA-Vorarbeit; kein operativer Phasenabschluss.**
Die vorhandene native Datei `ogl/modules/rtx/frame_bridge_test.cpp`
prüft nun auch den 4×4-HQ-Atlaspacker mit extrem schmalen
1×2- und breiten 2×1-Bildquellen: deterministische
Nearest-Neighbour-Skalierung bis 64×64, exakte RGBA-Pixel
an beiden Halbgrenzen, weiße unbenutzte Atlasbereiche und
fail-closed Ablehnung von 65×1-, 1×65-, 0×2- und
abgeschnittenen Quellbilddaten. Tests benutzen ausschließlich
synthetische Pixel und schreiben keine Nutzerprofile.

**Verifiziert:** vollständiger Linux-Mint-Viewer-Build
`bash ogl/scripts/build-test.sh`, 379/379 Python-Tests,
41/41 native CTests und separat 67/67 Grafiktests erfolgreich;
`git diff --check` ohne Fehler. Keine authentifizierte
OpenSim-Welt, kein Live-Vulkan-Window-Present und kein
vollständiger Windows-/macOS-Viewer-Test durchgeführt.
P05 bleibt formal **2/20**, Hauptblock 3 (exklusives
SDL-Vulkan-Window-Ownership/echtes Live-Present) offen.

## P05/3–4 – 20 technische Schritte: 16 Vulkan-Materialien mit höherer Texturauflösung (09.11.2026)

**Erreicht:** Der private, NICHT im normalen Viewerfenster
sichtbare Vulkan-World-Renderer unterstützt optional ein
HQ-Bildlayout mit **16 unabhängigen 64×64-Pixel-Texturen**.
Das ist die **16-fache Pixelmenge je Material** gegenüber
dem vorigen 4×4-Atlas mit nur 16×16 Pixeln pro Textur.
Dafür wurde ein eigenes privates **256×256-RGBA8-VkImage**
und ein separater Vulkan-1.0-SPIR-V-Fragmentshader
(ShaderMode8) implementiert. Die bisherige 16er-
Variante (ShaderMode7, 64×64-VkImage mit 16×16
pro Material), der Vierer-Atlas (ShaderMode6) und
sämtliche übrigen ShaderModes0–5 bleiben getrennt.
Die vorhandenen VkSampler/VkImageView-/Descriptor-,
4-Vertexbuffer-/FaceMaterialSlot-, UV-/FaceRGB-,
Kamera-/Sonnen-/Depth- und GPU-Fence-Pfade
werden weiterverwendet.

**Firestorm-Quellpfad und Isolation:** Bereits im Speicher
von Firestorm decodierte LLViewerFetchedTexture/
LLImageRaw-Bilder werden nur über LLPointer und
LLImageDataSharedLock synchron kopiert. Neuer,
seitenverhältnistreuer Nearest-Atlaspacker mit bis zu
16 Quellbildern 1..64×64, alle Bilder vollständig
deckend und alle Eingabegrößen/Bytezahlen geprüft.
Eine sichtbare bzw. authentifizierte OpenSim-Welt
wurde NICHT umgestellt oder abgenommen.
Keine Asset-ID-Übertragung, kein erzwungener Fetch,
kein OpenGL-Texture-Readback, keine PBR-/GLTF-
oder Alpha-/Terrain-Unterstützung.
>16 verschiedene Texturen, mehrdeutige Material-
Zuordnung sowie unzulässige Quellen werden
geschlossen abgewiesen.

**Freigabekette:** Für GPU-HQ
OGL_VULKAN_WORLD_MATERIAL_ATLAS_HQ_PROBE=1,
zusätzlich zur vorhandenen 16er-Atlas- und allen
alten RGBA-/UV-/RGB-/Sun-/Camera-/Mesh-/Offscreen-
Entwicklerfreigaben. Für echte Firestorm-Quellen
muss außerdem
OGL_VULKAN_LIVE_MATERIAL_ATLAS_HQ_SOURCE=1
aktiv sein. Falls eine Freigabe fehlt, dürfen
weder die neue Bildgröße noch die bereits
vorhandene Material-/Bildgeneration fälschlich
als erfolgreicher Live-HQ-Frame gelten.
Die GPU-Größenprüfung akzeptiert exakt
256×256 nur im HQ-Modus und 64×64 in
den bisherigen Atlas-Modi; ein Quellenwechsel
wird nach bestehender begrenzter Vulkan-Fence
freigegeben.

**20 tatsächlich ausgeführte Entwicklungsschritte:**

01. Git-Ausgangslage und sauberen Branch ogl-develop sowie die erfolgreich abgeschlossene sechsfache CI 42685116191 geprüft.
02. Bestehende Vulkan-Offscreen-World-Pipeline (ShaderModes0–7), 2×2-/4×4-Atlanten, Face-Slot-ABI, Meshgenerationen und Vulkan-Image-Upload analysiert.
03. Neuen Bildqualitätsmodus ohne Austausch der 16 Material-Slots festgelegt: 16×64×64 statt 16×16×16 Texel, weiterhin ein privates VkImage.
04. Zusätzlichen GLSL-450-Fragmentshader mit 4×4-Atlas, 64-Texel-Kacheln und unabhängig wiederholten Face-UVs erstellt.
05. Neuen Shader durch glslangValidator gegen Vulkan 1.0 kompiliert und mit Khronos spirv-val erfolgreich validiert.
06. Shader-SPIR-V ausschließlich in die vorhandene Vulkan-C++-Quelldatei eingebettet; keine neuen Repository-Dateien oder Duplikate erzeugt.
07. ShaderMode8 für die optionale 256×256-Textur implementiert; Legacy-ShaderModes0–7 und ihre Layouts nicht umgewidmet.
08. Den vorhandenen Vier-Vertexbuffer-Pfad (XYZ, FaceRGB, FaceUV, Materialslot) inklusive Sonnenlicht und VkSampler auch für ShaderMode8 verwendet.
09. Opt-in OGL_VULKAN_WORLD_MATERIAL_ATLAS_HQ_PROBE=1 eingeführt und streng an die vorherige Basis-Atlas- und 16-Material-Entwicklerfreigabe gekoppelt.
10. Die private Vulkan-Texturgrößenprüfung an den ausgewählten Modus gebunden (64×64 für Legacy, exakt 256×256 für HQ).
11. Host-kohärenten VkImage-Upload auf maximal 262.144 RGBA-Bytes erweitert; Zeilenabstand wird weiter mit VkGetImageSubresourceLayout geprüft.
12. Eigenen begrenzten 4×4-HQ-CPU-Atlaspacker implementiert: 16 unabhängige 64×64-Quellkacheln in einem 256×256-Pixelbild.
13. Original-Alpha, Quellgröße, Datenlänge, Bildanzahl 1–16 und leere/ungültige Bilddaten bei der neuen CPU-Packung fail-closed behandelt.
14. Firestorm-Deferred-Scanner um den optionalen HQ-Quellpfad ergänzt, weiterhin nur für vorhandene, valide, bereits decodierte LLImageRaw-VolumeFace-Daten.
15. Zusätzliche Live-Quellfreigabe OGL_VULKAN_LIVE_MATERIAL_ATLAS_HQ_SOURCE=1 eingeführt; fehlende GPU-/Source-Gates werden nicht durch andere Freigaben ersetzt.
16. Host-Handoff mit HQ-Bildabmessungen und vorhandener XYZ-/RGB-/UV-/Materialslot-Meshgeneration gekoppelt; keine Asset-IDs, erzwungenen Decoder oder OpenGL-Readbacks hinzugefügt.
17. Native CPU-Tests für 16 Atlaspositionen, Ecken/Mitten, weiße Leerfelder, Übergröße, ungültige Bytes/Alpha und exakt erhaltene 1-Pixel-Farbwechsel erstellt.
18. Native echte Vulkan-GPU-Prüfung für alle 16 unterscheidbaren Materialslots und zwei gleichzeitig sichtbare Texturen im selben Frame ausgeführt.
19. Auf der GPU 1-Pixel-Abstand im 64×64-Quellbild mit unterschiedlich rotem/grünem Ergebnis gemessen; Wechsel HQ→16er-Legacy ohne XYZ-Meshneuladen validiert.
20. Vollständigen Linux-Firestorm-Viewer erfolgreich gebaut; 379 Python-, 41 native CTests sowie 67 Grafik-/45 Reporttests bestanden und 32 isolierte SDL/X11-GUI-Szenarien angestoßen.

**GPU-Prüfung (echtes isoliertes Vulkan-X11, nicht nur Quellcode):**
**81 private Welt-/Depthframes; 27 GPU-Cache-Aufbauten,
54 Wiederverwendungen und 26 Freigaben** vor dem
Modul-Shutdown. Alle 16 HQ-Materialslots mit
erwarteten, eindeutig verschiedenen Rot/Grün-
Werten geprüft; zwei unterschiedliche Slots
gleichzeitig im selben GPU-Bild. Entscheidend:
ein bereits decodiertes 64×64-Rot-/Grün-
Streifenbild mit nur **einem Texel Abstand**
wird tatsächlich mit unterscheidbaren
Fragmentshader-Ergebnissen abgetastet.
Ebenfalls abgenommen: Ablehnung falscher
64×64-/257×256-Uploadgröße im HQ-Modus,
Rückwechsel zum 64×64-Vulkan-16er-Modus
ohne erneuten XYZ-Mesh-Upload, unveränderte
ältere Vulkan-WSI-/Depth-/Region-/Sampler-
Tests. Diese GPU-Bilder sind KONTROLLIERTE
Testdaten, keine authentifizierten Assets
einer laufenden OpenSim-Region.

**Vollständiger Linux-Build:** 379/379
Python-Tests, 41/41 native CTests,
67/67 gezielte Grafik- und 45/45
Report-/Herkunftstests.
**32/32 echte isolierte SDL/X11-Viewerstarts** mit unabhängigen temporären Profilen und ohne Nutzerkonten erfolgreich: normaler OpenGL-Login, HQ-16er-Atlas, ältere 16er-/4er-/Einzeltexturpfade, gültige/fehlende/ungültige Entwicklerfreigaben sowie fehlender Vulkan-ICD. Sämtliche von diesen Tests gestarteten Prozesse sind beendet. Keine authentifizierte OpenSim-Welt wurde angemeldet.
Die vorausgehende portable GitHub-CI
42685116191 ist mit ihren sechs
Linux-/Windows-/macOS-Grafikjobs
grün. Die neue Commit-CI ist separat
zu kontrollieren; eine portable CI
ist keine Vollviewer-/Inworld-
Abnahme auf Windows und macOS.

**Weiter offen:** Vulkan-Window-Present
mit kompletter OpenSim-Welt, GPU-
Residency-/Mip-/Texturstreaming,
Materialcache je Face über Frames,
Alpha/PBR/GLTF, Terrain, Wasser, Avatare,
UI und authentifizierte echte Live-
Region-Texturprüfung.
Die sichtbare Welt bleibt OpenGL.
P05 formal **2/20 operative Hauptblöcke**,
auch nach dieser 20er-Teilaufgabenrunde.
Code-Commit: a32e457ed04a534ddb1992702bc327b30e24f2de.

## P05/3–4 – 20 technische Schritte: optionaler 16-Material-Vulkan-Atlas (09.11.2026)

**Implementiert:** Der bisherige reale private Vulkan-4-Material-
Atlas (2×2 Kacheln mit je 32×32 Bildpunkten) kann optional auf
**16 voneinander unabhängige Materialien** erweitert werden.
Im neuen Vulkan-ShaderMode7 enthält dasselbe private
64×64-RGBA8-VkImage ein **4×4-Kachelraster mit 16×16 Pixeln
pro Textur**. Die Face-Materialslot-ABI, VkVertexBuffer-Binding3,
VkSampler, GPU-Mesh-/FaceRGB-/UV-Generation, Kameraprojektion
und Sonnenbeleuchtung bleiben kompatibel. ShaderModes0–6
und die alte 2×2-Datenanordnung bleiben unverändert.

**Separates Opt-in:** OGL_VULKAN_WORLD_MATERIAL_ATLAS_16_PROBE=1
zusätzlich zu sämtlichen vorherigen Vulkan-WORLD-/Mesh-/Camera-/
Offscreen-/Sun-/RGBA-/Tint-/UV-/Atlas-Entwicklergates.
Für eine tatsächliche Firestorm-Quelle ist ZUSÄTZLICH
OGL_VULKAN_LIVE_MATERIAL_ATLAS_16_SOURCE=1 erforderlich.
Ohne beide neuen Flags wird keine 4×4-Texturquelle angenommen
oder als alte 2×2-Textur ausgegeben. Die Validierung prüft
exakt dieselbe aktive Meshgeneration und vollständige
Materialslotdaten pro erfasstem Dreieck.

**Datenquelle:** Der bestehende Firestorm-Deferred-Scanner
ermittelt die bis zu sechzehn eindeutigen, bereits als
LLImageRaw decodierten LLViewerFetchedTexture-Objekte in
der Reihenfolge ihrer erfassten VolumeFace-Dreiecke.
Alle Pixel werden rein lesend unter LLPointer und
LLImageDataSharedLock synchron kopiert, als RGBA8-
Thumbnail vorbereitet und von einem begrenzten
4×4-CPU-Packer zu 16 unabhängigen Kacheln angeordnet.
Es finden KEIN JPEG2000-Netzwerkfetch, erzwungenes
Decoding, OpenGL-Pixelreadback oder Asset-ID-Logging
statt. Transparente/Default-/Missing-/animierte/
PBR-/GLTF-/Normal-/Specular- oder gemischte Terrain-
Materialien bleiben ausgeschlossen; >16 Quellen
werden fail-closed zurückgewiesen. Die 16px-Texturgröße
ist ausdrücklich ein Entwickler-Proof und NICHT
Produktionsqualität oder GPU-Asset-Streaming.

**20 ausgeführte technische Arbeiten:**

01. Branch ogl-develop, Git-Arbeitsverzeichnis, vier maßgebliche Projektdateien und den erfolgreich abgeschlossenen sechsplattformigen CI-Lauf 42681231907 geprüft.
02. Die existierende 2x2/32px Vier-Material-GPU-Pipeline inklusive VkImage/VkSampler/UV/FaceRGB/Meshgeneration und ihren nativen Testverträgen untersucht.
03. Das neue 4x4/16px Atlaslayout ausschließlich als optionalen separaten Modus definiert, ohne die alte 2x2-Vier-Texturen-Darstellung umzudeuten.
04. Reinen vorhandenen Host-Header um einen synchronen CPU-Konverter für bis zu sechzehn unabhängige und bereits decodierte RGBA8-Texturen erweitert.
05. Quelle strikt auf ein bis sechzehn Bilder, maximal 64x64 je Bild, ausreichend Bytebuffer und vollständig deckende Alphapixel geprüft.
06. Jede 16x16-Zielkachel seitenverhältnisstreu per Nearest-Sampling gefüllt; nicht verwendete Atlasflächen deckend weiß initialisiert.
07. Exakte Positionen aller sechzehn Kacheln per nativen C++-Tests geprüft; Null-Eingabe, 17 Quellen, fehlerhafte Bytes und transparente Bilddaten negativ getestet.
08. Eigenen Vulkan-1.0-Fragmentshader für 4x4 Atlas, flache Face-Materialslots 0..15 und UV-Wiederholung ohne Überlauf in Nachbarkacheln erstellt.
09. Den neuen Shader mit glslangValidator kompiliert und durch Khronos spirv-val auf Vulkan-1.0-Konformität kontrolliert.
10. SPIR-V-Binärdaten als zusätzliche ShaderMode7-Ressource in der vorhandenen Modul-Quelldatei integriert; frühere ShaderModes0..6 erhalten.
11. Rendererbindung3 für Face-Materialslot, vorhandene VkImage/VkImageView/VkSampler-Descriptor und Push-Constant-Beleuchtung im neuen Modus wiederverwendet.
12. Neue optionale Freigabe OGL_VULKAN_WORLD_MATERIAL_ATLAS_16_PROBE=1 mit exakter Stringvalidierung eingeführt; ohne bestehenden Atlas-Modus ist Mode7 gesperrt.
13. GPU-Slot-Upload nur mit korrekter aktueller XYZ/UV/Tint-Meshgeneration und 1..16 statt 1..4 Materialslots zugelassen, sonst fail-closed.
14. Firestorm-Deferred-Scanner um bis zu 16 Referenzen auf eindeutige, bereits decodierte LLViewerFetchedTexture-Bilder pro aktuellem Material-Mesh erweitert.
15. Live-Scanner nutzt vorhandene LLPointer-/LLImageRaw-/SharedLock-Lebensdauer sowie Thumbnail-Konvertierung, ohne Texture-Fetch oder OpenGL-Pixelreadback.
16. Die Atlasquelle über separate zusätzliche Freigabe OGL_VULKAN_LIVE_MATERIAL_ATLAS_16_SOURCE=1 streng an die GPU-16er-Entwicklerfreigabe gekoppelt.
17. Fehlende GPU- oder Live-Freigaben, >16 verschiedene Quellen, Default-/Alpha-/PBR-/animierte Texturen und gemischte Terrain-Dreiecke weiter sicher ausgeschlossen.
18. Native Vulkan-X11-GPU-Abnahme aller 16 eindeutigen Materialslots durchgeführt und tatsächliche GPU-Farbresultate mit erwarteten Kanalwerten abgeglichen.
19. Zwei entfernte Materialslots (#3 und #12) im selben GPU-Frame gleichzeitig gerendert; Mode7-zu-Mode6-Rückfall ohne erneuten Mesh-Upload geprüft.
20. Kompletten Linux-Firestorm-Viewer gebaut; 378 Python-/41 native CTests sowie 66 Grafik-/45 Reportregressionen bestanden; zusätzliche isolierte SDL/X11-GUI-Freigabeszenarien gestartet.

**Abnahme lokal (Vulkan-GPU):** 60 echte PRIVATE
GPU-offscreen Welt-/Depthframes,
24 Shader/Render-Cache-Aufbauten,
36 Wiederverwendungen und 23 Releases
vor Modul-Shutdown. Alle 16 Materialslots
lieferten 16 unterscheidbare Rot/Grün-Kanalpaare
mit geprüften erwarteten Einzelwerten
(Slot15: Rot51, Grün51).
Zwei räumlich getrennte Dreiecke mit
Materialslot #3 und #12 wurden in EINEM
GPU-Frame korrekt dargestellt; ein
Wechsel zurück zum alten 2×2-ShaderMode6
ohne Meshneuladen ebenfalls.
Mehr als 16 Slots, Bruchteilslots,
fehlende Ein-/Vier-/16er-Gates,
fehlerhafte RGBA-Daten und stale
Material-/Bildgenerationen sind
abzuweisen. Der native Vulkan-X11-
Kompletttest war erfolgreich.

**Abnahme Linux:** Der vollständige
Firestorm-Build besteht **378/378
Python-Tests**, **41/41 native CTests**,
**66/66 gezielte Grafik- und 45/45
Herkunfts-/Reportprüfungen**.
**24/24 isolierte reale SDL/X11-Viewerstarts** mit eigenständigen temporären Profilen (OpenGL-Login, 4er-/16er-Atlas, Berechtigungen, ungültige Flags und fehlender Vulkan-ICD) erfolgreich. Keine Anmeldung in eine authentifizierte OpenSim-Welt.
Letzte vorausgehende sechsplattformige
CI 42681231907 (Linux x64/ARM64,
Windows x64/ARM64, macOS ARM64/Intel)
vollständig erfolgreich; den neuen
Commit-CI separat prüfen.

**Unverändert offen:** Es gibt noch
keinen authentifizierten Live-OpenSim-
GPU-Frame, der die neue real decodierte
16-Material-Quelle testet. Die 60
Vulkan-GPU-Testframes verwenden
kontrollierte synthetische Bilddaten.
Echte Asset-Streams, >16 Texturen,
Mipmaps/Materialcache, Alpha-/PBR/
GLTF-Features, Terrain, Avatare, UI
und das eigentliche Vulkan-Window-
Present fehlen. Die sichtbare Welt
bleibt OpenGL. Phase P05 formal
**2/20 operative Hauptblöcke**;
Block3/4 noch nicht fertig.
Code-Commit: 55f9016d4c981450fb664179d27c11a6e32352a4.

## P05/3–4 – 20 technische Schritte: bis zu vier echte OpenSim-Face-Texturen über Vulkan-Atlas (09.11.2026)

**Erreicht:** Der bisherige Vulkan-Sampled-Image-Prototyp konnte
nur eine Textur pro gesamtem Szenenmesh darstellen. Jetzt können
bis zu **vier unterschiedliche, bereits decodierte opake Legacy-
Diffuse-Bilder** aus der tatsächlichen Firestorm-Deferred-Szene
pro akzeptiertem VolumeFace-Dreieck indiziert werden. Ein
64×64-RGBA8-Atlas enthält vier unabhängige 32×32-Kacheln;
jedes Dreieck erhält über eigene GPU-Vertexbindung3 seinen
Materialindex (0 bis 3), während XYZ, Face-RGB, UV-Koordinaten
und Sonnenrichtung getrennt und generationsgekoppelt bleiben.

**Quellpfad und Grenzen:** Der reale Scanner nutzt ausschließlich
LLViewerFetchedTexture::getRawImage() und die bereits decodierten
LLImageRaw-Bilder; Materialzuordnung nach ihrer stabilen Instanz
innerhalb EINES synchronen Scans. Alle Bilder werden über
LLPointer und LLImageDataSharedLock sicher gelesen, als
maximal 64px-RGBA8-Thumbnail kopiert und zu vier 32px-Tiles
zusammengefügt. Kein JPEG2000-Netzwerkfetch, erzwungener
Decoder, OpenGL-Readback oder Asset-/UUID-Logging.
Ein Snapshot ist NUR dann für Atlas-Texturen freigegeben,
wenn ALLE akzeptierten Dreiecke aus eindeutig texturierten
VolumeFaces mit unterstützter UV-Transformation stammen,
ohne Terrain-/Ray-Anker, fehlende/Default-/transparente,
animierte, normal-/specular- oder GLTF-/PBR-Materialien.
Über vier unterschiedliche Bildquellen werden nicht
teilweise als korrekt ausgegeben: die Materialabnahme
scheitert insgesamt (fail-closed).

**Vulkan-Pipeline:** Neue SPIR-V-Shader für Vulkan 1.0
(validiert mit spirv-val) legen den Face-Material-Slot
flach pro Dreieck fest und wenden frac() auf Original-
UV-Koordinaten pro Texturkachel an. Die Fragment-
abtastung quantisiert auf Texelzentren und verhindert
Materialüberläufe an Kachelgrenzen, auch bei negativen
Wiederholungskoordinaten. ShaderMode6 bindet vier
Vertexbuffers: Positionen (0), FaceRGB (1), FaceUV (2)
und Material-Slot (3). Der bereits vorhandene echte
VkImage/VkImageView/VkSampler/Descriptor und der
fence-sichere GPU-Cache bleiben erhalten. Die früheren
ShaderModes0..5 werden unverändert beibehalten.
Die GPU-Atlasdaten sind aus Platz- und Sicherheitsgründen
auf 64×64 Pixel ohne Filter-/Mipmapping-/PBR-Streaming
begrenzt: das ist noch KEIN universeller Texturrenderer.

**Separates Entwickler-Opt-in:** Der GPU-Atlas erfordert
OGL_VULKAN_WORLD_MATERIAL_ATLAS_PROBE=1. Für die
tatsächliche Firestorm-Quelle ist zusätzlich
OGL_VULKAN_LIVE_MATERIAL_ATLAS_SOURCE=1 und das
bestehende OGL_VULKAN_LIVE_RGBA_SOURCE=1 nötig.
Alle vorherigen unabhängigen World-/Mesh-/Camera-/
Offscreen-/RGB-/UV-/Sun-Freigaben gelten weiterhin.
Bei fehlender Quelle, nicht passenden Material-
Slot-/Bild-/Meshgenerationen oder ungültigem Bild
wird die vorherige Live-Quittung verworfen.
**Keiner dieser Entwicklerpfade übernimmt das
OpenGL-Viewerfenster oder dessen Renderziel.**

**20 tatsächlich ausgeführte technische Aufgaben:**

01. Sauberen Branch ogl-develop, bisherige P05-Anweisungen, Vulkan-/Materialquellen und den vollständig erfolgreichen Sechs-Plattformen-CI-Lauf 42676970109 geprüft.
02. Die bisherige Ein-Bild-VkImage-/Sampler-Pipeline und die tatsächliche Firestorm-Deferred-Quelle LLViewerFetchedTexture/LLImageRaw analysiert.
03. Die nächste Materialausgabe ausdrücklich auf vier einzigartige deckende Legacy-Diffuse-Texturen pro Mesh und 64×64-Pixel-RGBA-Atlas begrenzt.
04. Rein synchronen CPU-Atlas-Konverter aus ein bis vier bereits decodierten RGBA8-Bildern in bestehendem Host-Header implementiert.
05. Jede Quelle auf RGB-Datenlänge, Maximalmaße 64×64 und vollständig opake Pixel geprüft; ungültige Eingaben löschen das Kandidatenresultat.
06. Vier unabhängige 32×32-Atlasquadranten erzeugt, mit Nearest-Neighbor-Sampling und deckend weißen unbelegten Slots.
07. Additive Vulkan-C-ABI ogl_rtx_face_material_slot eingeführt: drei identische ganzzahlige Slotwerte je bereits akzeptiertem Dreieck.
08. Separaten hostkohärenten GPU-Materialslot-Vertexbuffer als Vulkan-Binding 3 eingebaut, ohne die bisherigen XYZ-/RGB-/UV-Sidecars zu verändern.
09. GPU-Slotdaten vor Allokation auf exakte Dreieckszahl, aktuell akzeptierte Mesh-/UV-/RGB-Generation sowie ganzzahlige Slots 0..3 geprüft.
10. Slotbuffer bei Region-/Mesh-Clear und Modul-Shutdown invalidiert und sicher freigegeben; ungültige Uploads ändern gültige Buffer nicht.
11. Eigenen GLSL-450-Vulkan-Vertexshader für XYZ, FaceRGB, FaceUV und flache Material-Slot-Interpolation entwickelt.
12. Eigenen GLSL-450-Vulkan-Fragmentshader erstellt, der UV-Wiederholung pro Fläche korrekt innerhalb eines 32×32-Atlasquadranten abtastet.
13. Beide Atlas-Shader gegen Vulkan 1.0 kompiliert und mit Khronos spirv-val validiert; SPIR-V direkt in bestehender Moduldatei eingebettet.
14. Private ShaderMode6-Renderpipeline mit VkVertexInput-Binding3 und den vorhandenen ImageView-/Sampler-/Descriptor-Ressourcen verbunden.
15. Auswahl des Atlasmodus streng an OGL_VULKAN_WORLD_MATERIAL_ATLAS_PROBE=1 und sämtliche vorhandenen RGBA-/UV-/Tint-/Sun-/World-/Camera-Gates gebunden.
16. Quellqualifizierten Host um optionalen Face-Slot-/Tile-Count-Handoff erweitert, vor Bild-Upload geprüft und an dieselbe GPU-Meshgeneration gekoppelt.
17. Realen Firestorm-Szenenscanner so erweitert, dass jedes akzeptierte VolumeFace-Dreieck einen Material-Slot nach erster Texturverwendung erhält; mehr als vier Texturen scheitern geschlossen.
18. Mehrere bereits im Speicher decodierte LLImageRaw-Bilder mit LLPointer-Lebensdauer und LLImageDataSharedLock gelesen, auf RGBA8 verkleinert und an den Atlaspacker übergeben.
19. Native echte Vulkan-GPU-Abnahme mit vier unabhängig gefärbten Slots, Materialwechsel ohne Meshneuladen sowie zwei verschieden texturierten Dreiecken IM GLEICHEN Frame bestanden; negative/positive UV-Wiederholung ebenfalls geprüft.
20. Kompletten Linux-Viewer und Regression mit 377 Python-Tests, 41 nativen CTests, 65 Grafik- und 45 Reporttests bestanden; 16 isolierte SDL-/X11-Viewerstart- und Treiber-/Berechtigungsfälle ausgeführt.

**Echte native Vulkan-GPU-Abnahme:** 42 private
World-/Depth-Frames; vor dem endgültigen
Modul-Shutdown 22 GPU-Cache-Aufbauten,
20 Wiederverwendungen und 21 Freigaben.
Die vier Atlas-Materialslots erzeugten
jeweils ihre RGB-Hauptfarbe (R51, G51,
B51 sowie Gelb R51/G51). Ein zweites
Mesh mit ZWEI verschieden texturierten
Dreiecken wurde in **einem GPU-Frame**
gerendert: mittlere gemessene Kanäle
**R25/G25/B0**. Auch unterschiedliche
negative und positive UV-Perioden auf
beiden Flächen blieben korrekt.
Ungültige Slotwerte (NaN/Bruchteile/
unterschiedliche Vertexslots), nicht
passende Dreieckszahl, zu viele Tiles,
ungültige Texturalpha-/Bytepuffer und
stale Mesh-/Bildgeneration wurden
abgewiesen. Der vollständige bisherige
native Vulkan-X11-/Depth-/Regiontest
blieb erfolgreich.

**Vollständige Linux-Abnahme:** 377/377
Python-Tests, 41/41 native CTests,
65/65 Grafik- und 45/45 Report-
/Herkunftsverträge. Zusätzlich 16
voneinander isolierte SDL/X11-Viewer-
starts mit normalem OpenGL-Login,
verschiedenen GPU-Atlas-/Live-Source-
Flags und fehlendem Vulkan-ICD.
Keine echten Loginkonten oder Nutzer-
Viewerprozesse angefasst.

**Unverändert offene P05-Abnahme:**
Die gekoppelten realen Firestorm-
Datenquellen wurden kompiliert und
quellcode-/plattformgeprüft, aber
noch NICHT über einen authentifizierten
OpenSim-Region-Frame mit realen Vulkan-
Materialpixels abgenommen. Die messbaren
GPU-Testbilder stammen aus kontrollierten
synthetischen RGB-Texturen, nicht aus
authentifizierten Region-Assets. Für
produktive Welten fehlen >4 Materialslots,
PBR/GLTF-Textures, Alpha-Blending, Terrain,
Wasser, Avatare, Streaming-/Residency-
Management, optimales Texturstaging und
volle Vulkan-Fensterhoheit/UI.
**P05 formal weiterhin 2/20 Hauptblöcke;
Block3/4 nicht vollständig abgeschlossen.**
Quellcode-Commit: cde4fb8813ff4a946818aa4f1744db15ffee7810.

## P05/3–4 – 20 technische Schritte: reale decodierte Firestorm-Texturquelle mit Vulkan verknüpfen (09.11.2026)

**Neuer tatsächlicher Datenpfad:** Der Firestorm-Viewer decodiert
OpenSim-Texturen bereits in LLViewerFetchedTexture/LLImageRaw.
Die optionale World-Source-Bridge kann jetzt diese vorhandenen,
decodierten Pixel einer akzeptierten Deferred-Szenengeometrie
zuordnen, in einem kleinen CPU-RGBA8-Bild zusammenfassen und
an die vorhandene private Vulkan-VkImage/VkSampler-Pipeline
weiterreichen. Es werden keine JPEG2000-Dateien neu decodiert,
keine Assets aus dem Netzwerk geladen, keine OpenGL-Textur
ausgelesen und keine Pixel protokolliert.

**Voraussetzungen und Quellenintegrität:** Der bisherige Vulkan-
Testshader unterstützt weiterhin nur **ein Bild pro vollständigem
World-Mesh**, nicht beliebige Materialien je Objekt oder Face.
Deshalb liefert der neue echte Source-Pfad bewusst nur dann
ein Bild, wenn ALLE erfassten Dreiecke ausschließlich aus
VolumeFaces stammen und dasselbe valide, bereits im RAM
decodierte Legacy-Diffuse-Bild besitzen. Terrain-/Ray-Anker-
Snapshots, gemischte Texturen, fehlende/Default-Assets,
transparente TextureEntries oder Bildpixel sowie animierte
und PBR-/GLTF-Materialien werden abgewiesen. Dies ist ein
enger Prototyp und für normale, komplexe OpenSim-Regionen
noch keine flächendeckende Materiallösung.

**Implementierung:** Nur mit OGL_VULKAN_LIVE_RGBA_SOURCE=1
UND OGL_VULKAN_WORLD_RGBA_IMAGE_PROBE=1, zusätzlich zu
vorhandenen UV-/FaceRGB-/Lambert-/Sun-/World-/Device-/
Mesh-/Camera-/Offscreen-Gates. Die Quelle wird durch
LLPointer-Referenzen auf Texture und Raw-Image gepinnt,
mit LLImageDataSharedLock kopiert und nicht über den
synchronen Aufruf hinaus gehalten. Ein rein funktionaler
RGB/RGBA->RGBA8-Konverter validiert jeden Original-Alpha-
Wert, Komponenten (3/4), Pixelmaße (1..1024), Bytezahl
und baut ein seitenverhältnistreues Nearest-Neighbor-Bild
(max. 64x64). Host und Vulkan-Modul erzwingen, dass
dieses Bild mit exakt der akzeptierten XYZ/FaceRGB/UV-
Meshgeneration übereinstimmt. Ein fehlgeschlagener
Source-Upload invalidiert alte Quittungen; es werden
keine falschen realen Welt-GPU-Frames behauptet.

**20 tatsächlich ausgeführte Implementierungs-/
Testaufgaben:**

01. Arbeitsbranch ogl-develop, sauberen Projektstand, AGENTS/Handoff/Status/Roadmap und den zuletzt fehlerhaften GitHub-Lauf mit null gestarteten Jobs geprüft.
02. Die bestehende Firestorm-Texture-Fetch-/JPEG2000-Verarbeitung und die Schnittstellen LLViewerObject::getTEImage, LLViewerFetchedTexture und LLImageRaw untersucht.
03. Eine bewusst eingeschränkte erste echte Textur-Quellübernahme gewählt: nur ein einheitliches, bereits decodiertes Legacy-Diffuse-Bild je vollständigem GPU-Szenensnapshot.
04. Ein zusätzliches eigenständiges Entwickler-Opt-in OGL_VULKAN_LIVE_RGBA_SOURCE=1 eingeführt; das bestehende RGBA_IMAGE_PROBE bleibt weiterhin erforderlich.
05. Live-Materialquelle nur aus dem quellqualifizierten Deferred-OpenSim-Regionenscanner und nicht aus Prelogin-/Synthetic-/Generic-Geometry akzeptiert.
06. Alle aufgenommenen Dreiecke anhand derselben LLViewerFetchedTexture-Instanz geprüft; gemischte Texturen werden für den Ein-Image-Prototyp vollständig zurückgewiesen.
07. Snapshots mit Terrain- oder Kameraanker-Dreiecken ohne eindeutige VolumeFace-Materialquelle für echte GPU-Bildpixel ausgeschlossen.
08. Fehlende Texturassets, Platzhalterbilder und ungültige/fehlende VolumeFace-UV-Transformationen geschlossen abgewiesen.
09. TextureEntry-Oberflächen mit Transparenz, animiertem Texture-Transform oder GLTF-/PBR-Material nicht als gültige Legacy-Diffuse-Bildquelle zugelassen.
10. Vorhandene RGBA-/UV-/FaceRGB-Quell-ABI beibehalten und nur optional um Bilddatenzeiger, Bildabmessungen und die bereits gekoppelte Dreieckszahl ergänzt.
11. LLViewerFetchedTexture::isRawImageValid/getRawImage nur lesend verwendet; keine erzwungene Neudecodierung und kein OpenGL-Readback eingebaut.
12. LLPointer<LLViewerFetchedTexture> sowie LLPointer<LLImageRaw> zur referenzsicheren Lebensdauer der bereits vorhandenen CPU-Bilddaten während des synchronen Scans eingesetzt.
13. LLImageDataSharedLock während der Quell-Pixelkopie ergänzt, damit weder Bildpuffer noch Quelldaten pointerbasiert nach dem Scan zurückbehalten werden.
14. Plattformneutrale RGB/RGBA-zu-RGBA8-Konvertierung in bestehendem Host-Header implementiert, ohne neues Repository-Dateiduplikat.
15. Quelldimensionen und Speichergröße auf 1..1024 Pixel je Achse beziehungsweise geprüfte Bytezahl beschränkt; unzulässige Formate und transparente Originalpixel abgewiesen.
16. Seitenverhältnistreue Nearest-Neighbor-Verkleinerung auf maximal 64x64 implementiert, einschließlich extremer Seitenverhältnisse ohne Null- oder Integer-Overflow.
17. Host-Symbolauflösung für die bisherige additive Vulkan-RGBA-Upload-ABI ergänzt und reine CPU-Quelldaten synchron nach erfolgreichem XYZ/FaceRGB/UV-Meshupload übergeben.
18. GPU-Bildrevision ausdrücklich an dieselbe aktuelle Vulkan-Meshgeneration gekoppelt; ohne Live-Opt-in, bei fehlerhaftem Raw-Image oder bei fehlgeschlagenem Meshupload wird der frühere Live-Quellnachweis invalidiert.
19. Native plattformneutrale C++-Negativ- und Downsample-Tests ergänzt und frühere echte Vulkan-GPU-Image-/Sampler-/Region-/Depth-Tests unter isoliertem X11 erneut erfolgreich ausgeführt.
20. Vollständigen Linux-Viewer gebaut, 376 Python-/41 CTests und 64 Grafik-/45 Reportprüfungen erfolgreich kontrolliert sowie 10 isolierte SDL-Login-/Vulkan-Fallback-Szenarien ohne Nutzerprofil ausgeführt.

**Lokale Abnahme:** Der Linux-Vollviewer-Build
bestand mit **376/376 Python-Tests** und
**41/41 nativen CTests**. Die gezielten
Quellcode-/Funktionsverträge umfassen
**64 Grafik- und 45 Herkunfts-/Reporttests**.
Der native C++-Test prüft echte rohe RGB-
und RGBA-Bildarray-Kopien, Transparenz,
korrupte Datenlänge, invalide Kanalzahl,
256x128->64x32- und 1x1024->1x64-
Verkleinerung sowie Besitz der Bilddaten.
Die unveränderte private Vulkan-X11-GPU-
Abnahme bestand mit **35 VkDrawIndexed-
Welt-/Depth-Frames, 19 Cache-Builds,
16 Reuses, 19 Releases**; echtes
VkImage-Sampling zeigte R51/G0 gegen
R0/G51. Die erneuten isolierten SDL/
OpenGL-Viewerstarts prüfen 10 Varianten
mit/ohne Live-Flag und Vulkan-Rückfall,
jeweils nur bis zum Login-Screen.
Alle dabei genutzten Testdaten und
Testprozesse sind vom Benutzerprofil
isoliert.

**Wesentliche offene Abnahme:** Es wurde
KEIN authentifizierter Live-OpenSim-Region-
Snapshot mit gleichzeitig gültiger
einheitlicher decodierter Textur als
Vulkan-Worldframe nachgewiesen.
Der GPU-Sampler wurde bislang mit
synthetischen 2x2-/64x64-Testbildern
abgenommen; das neue Firestorm-Quell-
Sampling ist compiliert und lokal
vertraglich getestet, nicht live
authentifiziert. Produktionsmaterialien
brauchen per-Face-Texturzuordnung,
Sampler-/Residency-/Streaming-Management,
Opaque- und Alpha-Blending, PBR/GLTF,
Terrain, Avatare und Fenster-Present.
Die tatsächliche Welt rendert weiter
OpenGL. P05 formal **2/20 operative
Hauptblöcke**, Block3 und vollständiger
Block4 weiter offen.

**GitHub-CI:** Der vorangegangene
GitHub-Lauf 42673502520 wurde von
GitHub als Workflow-File-Fehler
ohne gestartete Jobs markiert;
lokale YAML-Analyse allein erklärt
das nicht. Neue Commit-CI
separat kontrollieren, nicht als
bestanden markieren.
Code-Commit: edd33d9b52f6c95d79b01b633db7cf4549dc838f.

## P05/3–4 – 20 technische Schritte: Vulkan-VkImage-Sampler für decodierte RGBA8-Texturpixel (09.11.2026)

**Erreicht:** Die private, NICHT sichtbare Vulkan-World-GPU-Pipeline
kann zum ersten Mal tatsächliche RGBA8-Bildpixel in einem
eigenen VkImage speichern und über einen VkSampler/Descriptor
anhand vorhandener interpolierter OpenSim-kompatibler UV-
Koordinaten im Fragmentshader abtasten. Die tatsächliche
OpenGL-Weltdarstellung und die bisherige UV-/FaceRGB-/
XYZ-Quell-ABI bleiben unverändert.

**Quellen und Grenzen:** Aktuell besitzt das Vulkan-Modul
nur eine additive Schnittstelle für **bereits decodierte,
deckende RGBA8-Bildpixel** (1 bis 64 Bildpunkte pro Achse,
ein Bild pro aktuellem GPU-World-Mesh). Das Modul holt
keine JPEG2000-/GLTF-/PBR-Assets aus Firestorm, kein
Asset-UUID oder Texturbild einer authentifizierten Region
ist automatisch verbunden. Die nachgewiesenen GPU-
Testtexturen sind kontrollierte 2x2- und 64x64-
Entwicklerbilder, keine OpenSim-Weltassets.

**Implementierung:** Neuer Vulkan-1.0-SPIR-V-Fragment-
shader verbindet texture(sampler,uv) mit der bereits
umgesetzten Opacity-/FaceRGB-/Sonnen-Lambert-
Diagnose. Ein echter VkImage/VkImageView/VkSampler
und kombinierter VkImageSamplerDescriptor werden
ausschließlich für den neuen ShaderMode5 erzeugt.
Linear-tiling R8G8B8A8 wird nur bei nachgewiesener
SAMPLED_IMAGE-Formatunterstützung zugelassen.
VkGetImageSubresourceLayout.rowPitch/offset beugt
falsch ausgerichteten Zeilen vor; HOST_WRITE-zu-
Fragmentshader-Pipelinebarriere und Layoutwechsel
gehen jedem ersten GPU-Draw voraus. Diese portable
Laufzeitprüfung ist ein erster 1.0-kompatibler
Entwicklerpfad, noch kein universelles optimales
GPU-Staging für beliebige Texturassets.

**Isolation und Fehlerbehandlung:** Nur
OGL_VULKAN_WORLD_RGBA_IMAGE_PROBE=1 ZUSÄTZLICH
zu UV_CHECKER/FACE_TINT/LAMBERT/SUN und den
vorherigen Vulkan-Device/Mesh/Camera/Offscreen-
Gates. Format und Größe, Deckkraft, GPU-Mesh-
generation sowie gültige UV-/RGB-Buffer werden
geprüft; Fehler verändern das vorherige Bild
nicht. Eine neue Textur verwirft nur den privaten
Sampler-/Offscreen-Cache nach einem begrenzten
GPU-Fence, NICHT die Weltmeshes. Regions-/Mesh-
Clear entfernt den CPU-Texturstatus; der Vulkan-
Cache übernimmt seine Image- und Descriptor-
Lebensdauer bis nach Abschluss des GPU-Fence.
Keine VkQueuePresentKHR-Nutzung auf dem
OpenGL-Viewerfenster.

**20 nachweisbar ausgeführte Teilaufgaben:**

01. Git-Stand ogl-develop, sauberen Ausgangs-Checkout und letzte vollständig grüne plattformübergreifende GitHub-CI kontrolliert.
02. Den bestehenden Firestorm-Deferred-XYZ-/FaceRGB-/UV-Pfad und den getrennten privaten Vulkan-Offscreen-Renderer untersucht.
03. Eine eng begrenzte v1-Bildschnittstelle für bereits decodierte RGBA8-Pixel definiert, ohne OpenSim-JPEG2000-Bilddecodierung vorzutäuschen.
04. Additive C-ABI für Pixel-Upload, aktive Textur-Meshgeneration und Textur-Uploadzähler implementiert.
05. Neues Entwicklerflag OGL_VULKAN_WORLD_RGBA_IMAGE_PROBE eingeführt, zusätzlich zu allen vorherigen GPU-/UV-/RGB-/Sonnen-/Kamera-Freigaben.
06. Upload-Grenzen 1 bis 64 Bildpixel je Achse validiert, ohne dynamische GPU-Bildspeicher-Allokation bei unzulässigen Maßen.
07. Nur vollständig deckende RGBA8-Texel zugelassen; transparente Alpha-Werte als nicht unterstützte Materialfunktion abgewiesen.
08. Textur-Upload vor Zustandsänderung in einen begrenzten separaten CPU-Buffer kopiert und Mesh-/UV-/FaceRGB-Generation auf Gleichheit geprüft.
09. Neue Bildrevision so gekoppelt, dass nach Region-/Meshwechsel nie ein alter Textur-Asset-Buffer für eine neue Meshgeneration autorisiert wird.
10. Alten GPU-Bildcache vor gültigem Texturwechsel oder explizitem Clear mit bereits vorhandener begrenzter Vulkan-Fence-Warteprüfung freigegeben.
11. Zwei aktive Texture-Sidecar-Zustände getrennt: im Modul gehaltene sichere RGBA-Quelldaten und ausschließlich vom privaten GPU-Cache gehaltenes VkImage.
12. Neuen GLSL-450-Fragmentshader mit kombinierter UV-Texturabtastung, Face-RGB-Tint und richtungsabhängiger Sonnendiffusbeleuchtung erstellt.
13. Fragmentshader zu Vulkan-1.0-SPIR-V kompiliert und mit Khronos spirv-val geprüft; Binärdaten in bestehende Modulquelldatei eingebettet.
14. Fünften privaten Vulkan-Shadermodus für das tatsächliche Abtasten eines Sampled-Image-Descriptors ergänzt; die bisherigen vier Shaderpfade bleiben separat.
15. Vor VkImage-Erzeugung die Vulkan-R8G8B8A8_UNORM-SAMPLED_IMAGE-Fähigkeit bei linearem Tiling abgefragt und bei Nichtunterstützung geschlossen abgelehnt.
16. Ein reales VkImage mit begrenztem, host-kohärentem Bildspeicher implementiert; RowPitch/Offset aus VkGetImageSubresourceLayout für den CPU->GPU-Upload beachtet.
17. Vulkan-ImageView, VkSampler mit Nearest-/Repeat-Adressierung, DescriptorSetLayout/Pool/Set und Fragmentshader-Binding 0 eingerichtet.
18. Explizite Vulkan-Pipelinebarriere von PREINITIALIZED zu SHADER_READ_ONLY_OPTIMAL und VkCmdBindDescriptorSets vor VkDrawIndexed ergänzt.
19. Echte Vulkan-X11-GPU-Abnahmen für 2x2-Rot/Grün-Texel, UV-Wrapping, Bildersatz, Maximum 64x64, Alpha-/Dimensions-/Flag-Ablehnung und Mesh-Upload-Unabhängigkeit bestanden.
20. Linux-Vollviewer gebaut, 375 Python-Tests, 41 CTests, 63 Grafikverträge, 45 Reporttests und fünf isolierte SDL/OpenGL-Login-Szenarien abgenommen; kanonische Projektdateien aktualisiert.

**GPU-Hardwareabnahme:** Echte, isolierte Vulkan-X11-
GPU-Prüfung mit **35 privaten Welt-/Depth-Frames**,
**19 GPU-Cache-Builds, 16 Reuses und 19 Releases**.
Ein hochgeladenes 2x2-RGBA8-Bild liefert über die
UV-Koordinaten **Rot R51/G0** gegenüber
**Grün R0/G51**; negativer UV-Wrap liefert
dieselben Texel. Bildinhaltswechsel ohne
erneuten GPU-XYZ-Meshupload erfolgreich.
Ein vollständig deckendes 64x64-Bild wurde
über den tatsächlichen Vulkan-Subresource-
RowPitch gesampelt. Transluzenz, fehlende
RGBA-Daten, ungültige Maße/Flags wurden
abgewiesen; voriger valider Texturstand
bleibt bei ungültigem Ersatz erhalten.
Alte Vulkan-X11-/Depth-/Region-/UV-Tests
bleiben erfolgreich.

**Volltest:** Linux-Viewer-Build mit
**375/375 Python-Tests**, **41/41 nativen
CTests**, davon 63 gezielte Grafik-
und 45 Herkunfts-/Report-Tests.
Zusätzlich **5/5 eigenständig isolierte
echte SDL/X11-Viewerstarts** geprüft:
normaler OpenGL-Login, gültiges Sampled-
Image-Opt-in, ohne UV-Freigabe, ungültiges
Bildflag und fehlender Vulkan-ICD.
Kein Test hat einen authentifizierten
OpenSim-3D-Weltframe erzeugt.
Testläufe mit temporären Profilen und
Xvfb ohne Nutzerkonten/Anmeldung.

**Noch offen:** Die eigentliche OpenSim-
Texturasset-Decode-/Upload-/Residency-
Pipeline, realer Texturwechsel je
VolumeFace, SRGB-/PBR-/Alpha-/Material-
handling, GPU-optimaler Stagingupload,
Terrain, Wasser, Avatare, UI und
sichere exklusive Vulkan-Window-
World-Presentation. Ein
authentifizierter Live-OpenSim-Vulkan-
Textur-GPU-Frame wurde NICHT abgenommen.
P05 bleibt formal 2/20 operative
Hauptblöcke; Block3/4 noch offen.
Code-Commit: 0fa365e4bbe03873d2a15be71cab0939a04738cb.

## P05/3–4 – 20 technische Schritte: echte OpenSim-UV-Daten im privaten Vulkan-Testshader (07.11.2026)

**Umsetzung:** Als Folgeschritt auf echte OpenSim-Face-RGB-Farbwerte
kann der Firestorm-Deferred-Szenenscanner jetzt auch die originalen
LLVolumeFace-UV-Koordinaten der akzeptierten Volume-Dreiecke übergeben.
Die TextureEntry-Transformation wird für unterstützte normale
Flächen wie in Firestorm LLFace angewendet: Rotation um das
UV-Zentrum (0,5/0,5), getrennte Skalierung und danach Offset.
Nicht sicher zuordenbare Terrain-/Ray-Anker-, Planar- und ungültige
Daten erhalten neutrale UVs. Die dynamische Texture-Animation
und komplette PBR-Textur-/Planarprojektion sind nicht implementiert.

**GPU-Pfad:** Der Vulkan-World-Meshbuffer (XYZ, Binding0) und der
bestehende opake Face-RGB-Buffer (Binding1) bleiben unabhängig.
Ein eigener Face-UV-Buffer (Binding2) enthält für jedes indexierte
Dreieck drei UV-Paare. Alle Quellen müssen zur gleichen aktuell
gültigen GPU-Meshgeneration und exakt gleicher Dreieckszahl gehören.
Der Upload beider Materialsidecars verwendet jetzt einen gemeinsamen
hostkohärenten GPU-Allokationspfad mit Rollback bei Fehlern.
Änderungen der UV-Koordinaten erzeugen kein neues XYZ-Mesh und
keinen Shader-Neuaufbau. Region-/Meshwechsel verwerfen alte UVs.

**Shader:** Zwei GLSL-450-Shader wurden in Vulkan-1.0-SPIR-V
übersetzt und mittels spirv-val validiert. Der neue ShaderMode4
interpoliert die UV-Werte auf der GPU, wiederholt Koordinaten und
zeigt ein kontrolliertes 4x4-rotes/grünes Raster, mit echter
aktueller Sonnenrichtung und Face-RGB multipliziert.
Dies ist ausdrücklich eine UV-Diagnose: Weder eine
VkImage-Texture noch ein VkSampler mit realen JPEG2000-/
OpenSim-Assetpixeln ist vorhanden. Der private Vulkan-Framebuffer
wird nicht im echten Benutzerfenster angezeigt.

**Sicherheitsgates:** OGL_VULKAN_WORLD_UV_CHECKER_PROBE=1
erfordert die getrennten FACE_TINT-/LAMBERT-/SUN-Developerflags
und vorhandene Vulkan-Device-/Mesh-/Camera-/Offscreen-
Freigaben. Null-/NaN-/Infinity-/Außerbereich-UVs, fehlende
RGB-/UV-Sidecars, nicht passende Generationen und ungültige
Opt-in-Werte scheitern geschlossen. Normale OpenGL-Darstellung,
ShaderModes0 bis 3, Alpha-/Sky-/Avatar-/UI-Pfade unverändert.

**20 tatsächlich ausgeführte Entwicklungs-/Testaufgaben:**

01. Git-Branch ogl-develop, sauberen vorherigen Stand, kanonische Roadmap und vollständig grüne vorherige Linux-/Windows-/macOS-CI geprüft.
02. Firestorm-Deferred-Weltabtastung, reale LLVolumeFace.mTexCoords sowie OpenSim-TextureEntry-Daten als Quelle für gemeshte UV-Koordinaten verifiziert.
03. Die originale Firestorm-LLFace-Texturtransformation geprüft: Drehung um 0,5, anschließend getrennte S/T-Skalierung und UV-Offset.
04. Unveränderte bestehende XYZ-Dreiecks-ABI und getrennte RGB-Sidecar-ABI als Kompatibilitätsgrenze festgelegt.
05. Neue additive C-ABI ogl_rtx_face_uv mit genau drei UV-Paaren pro akzeptiertem Dreieck definiert.
06. Eigenen GPU-UV-Vertexbuffer an Vulkan-Binding 2 mit strikt zur XYZ-Meshgeneration passender Revision eingerichtet.
07. UV-Werte vor jeder GPU-Speicheränderung auf endliche Koordinaten im erlaubten Bereich -4096 bis +4096 und exakte Mesh-/Face-Zuordnung geprüft.
08. Bestehende RGB- und neue UV-Buffer-Allokationen in gemeinsamen sicheren, hostkohärenten VkBuffer-/VkDeviceMemory-Uploadpfad zusammengeführt.
09. GPU-UV-Lebenszyklus abgesichert: eigenständige Freigabe, Rücksetzen bei Region-/Mesh-Clear sowie fehlgeschlagenem oder ungültigem UV-Upload.
10. GLSL-450-Vulkan-Vertexshader für drei unabhängige Vertexdatenströme (XYZ, Face-RGB, Face-UV) und die bisherige Kameraprojektion entwickelt.
11. GLSL-450-Vulkan-Fragmentshader mit echten interpolierten UV-Daten und einem viermal vier wiederholten roten/grünen Entwicklermuster entwickelt.
12. Beide neuen Shader für Vulkan 1.0 kompiliert, mit Khronos spirv-val validiert und direkt in die bestehende Vulkan-Modul-Quelldatei eingebettet.
13. Vulkan-ShaderMode 4 und drei GPU-Vertexbindings eingerichtet, ohne Shader-/Pipeline-Eingriffe in Depth-Debug, feste Lambert-, Sun- und Face-RGB-Modi.
14. Neue unabhängige Entwicklerfreigabe OGL_VULKAN_WORLD_UV_CHECKER_PROBE=1 plus zwingende FaceTint-/Lambert-/Sun-/World-/Mesh-/Camera-/Offscreen-Freigaben eingeführt.
15. GPU-Leseprobe um den gemessenen mittleren Grünkanal erweitert, um echte UV-gesteuerte Farbwechsel überprüfbar zu machen.
16. Realen Firestorm-Deferred-Szenenscanner um indexgenaues Einsammeln von UVs jeder gültigen LLVolumeFace-Dreiecksfläche ergänzt.
17. Übliche TextureEntry-Rotation/Skalierung/Offset gemäß Firestorm-LLFace-Konvention umgesetzt; Terrain, Ray-Anker, Planarprojektion und ungültige UVs verwenden vorsichtige neutrale UV-Daten.
18. Der Firestorm-Host reicht XYZ, RGB und UV innerhalb eines einzigen synchronen, quellqualifizierten Scans weiter, prüft die aktuelle Meshgeneration und sperrt Live-Quittungen bei UV-Ausfall.
19. Native echte Vulkan-GPU-Tests: UV-Rot- und Grünfeld, negative Koordinaten/Wrapping, unzulässige Anzahl, NaN, ungültige Freigabe und neue Meshgeneration mit Neuabgleich bestanden.
20. Linux-Vollviewer gebaut und 374 Python-Tests, 41 native CTests, 62 Grafik- sowie 45 Report-Tests bestanden; zusätzliche isolierte SDL/X11-Viewerstart-Fälle für UV-Freigabe und Rückfallpfade vorbereitet und ausgeführt.

**Gemessene native Vulkan-GPU-Abnahme:** 30 private
VkDrawIndexed-World-/Depth-Frames, **16 GPU-Cache-Builds,
14 Reuses, 16 Freigaben**. Bei identischem GPU-Mesh,
RGB-Material und Kameraframe erzeugen unterschiedliche
UV-Werte zwei unterschiedliche GPU-Texturprüffelder:
**rot R48/G6, grün R6/G47**. Eine äquivalente negative
UV-Wiederholung erzeugte denselben grünen Pixelwert.
Der UV-Buffer wurde viermal ausgetauscht, ohne
neuen GPU-XYZ-Meshupload; NaN, falsche Dreieckszahl
und ungültiges Flag wurden abgelehnt. Nach
Meshgenerationserhöhung war das alte UV-Material
nicht darstellbar, bis neue UV- und RGB-Daten
erneut passend bereitgestellt wurden. Vorige
Vulkan-Depth-/Regions-/WSI-X11-Abnahmen weiter grün.

**GUI-Abnahme:** 43/43 echte isolierte SDL/X11-Viewerstarts bestanden, einschließlich fünf neuer UV-Opt-in-/Missing-Source-/Invalid-Flag-Fälle, normalen OpenGL-Logins und sämtlicher vorigen Vulkan-X11-WSI-/Depth-/Region-/Treiber-Fallbacks. Keine Userprofile oder Userprozesse geändert.

**Linux-Volltest:** 374/374 Python-Tests, 41/41 native
CTests, 62 gezielte Grafik- und 45 Report-Tests grün.
Vollständige isolierte SDL/X11-GUI-Abnahme mit
bestehenden Fallbacks und fünf zusätzlichen
UV-Flag-/Auflösungsfällen läuft unabhängig von
Nutzerprofil, Nutzer-Viewer und Live-Logins.

**Noch offen:** Der Firestorm-Datenpfad ist mit echten
OpenSim-UV-Quellen verbunden, aber er wurde NICHT
gegen einen authentifizierten Live-OpenSim-Vulkan-
Weltframe abgenommen. Die gemessenen Prüfpixel
stammen aus kontrollierter synthetischer Geometrie.
Es gibt NOCH KEINE echte GPU-Texturpixelübernahme,
keinen VkImage-Sampler aus OpenSim-Texture-Assets,
keine vollständige Material-/PBR-/Terrain-/
Wasser-/Avatar-/Animation-/UI-Darstellung
und keine Vulkan-Fensterübernahme. Produktiv bleibt
OpenGL. P05 formal weiterhin 2/20 operative
Hauptblöcke; Block3/4 nicht komplett abgenommen.
Code-Commit: ee2d7544d5cd9f31c74de68297a1f81c58312181.

## P05/3–4 – 20 technische Schritte: OpenSim-Face-Farbwerte an Vulkan-World-Shader (07.11.2026)

**Entwicklungsinhalt:** Der tatsächliche Firestorm-Deferred-Szenenscanner
besitzt pro LLVOVolume-Oberfläche echte LLTextureEntry-Grundfarben.
Bisher lud die Vulkan-World-Probe nur XYZ-Dreiecke und zeichnete sie
mit einheitlicher Entwicklerfarbe. Jetzt kann der Scanner unter dem
separaten Entwicklerflag OGL_VULKAN_WORLD_FACE_TINT_PROBE=1
ein RGB-Tripel pro akzeptiertem Dreieck mitgeben. Der Shader
kombiniert dieses Farb-Attribut mit der bereits vorhandenen
GPU-Flächennormalen-/OpenSim-Sonnenbeleuchtung.

**Quelltreue und Grenzen:** Die Positionen-/RTX-ABI wurde nicht
verändert. Eine neue optionale ogl_rtx_face_tint-C-ABI bildet einen
separaten GPU-Vertexbuffer mit dreifach wiederholten RGB-Attributen
pro Dreieck. Nur vorhandene und valide opake Texture-Entry-RGB-Werte
werden übernommen. Terrain-/exakte Kameraanker- und transparente
Flächen erhalten eine neutrale Ersatzfarbe, da die aktuelle
Sampling-/Transparenz-/Terrain-Materialpipeline noch kein
vollständiges Abbild der echten Firestorm-Welt bereitstellt.
**Es handelt sich ausdrücklich NICHT um das Laden/Rendern
echter Diffuse- oder PBR-Texturen.**

**Vulkan-Pipeline:** Ein neuer privater Shadermodus 3 bindet die
vorhandenen World-XYZ-Vertexdaten an Binding 0 und neue Face-RGB-
Daten an Binding 1. Zwei originale GLSL-450-SPIR-V-Shader wurden
für Vulkan 1.0 kompiliert und mit spirv-val geprüft und in die
bestehende C++-Quelldatei eingebettet. Die normierte OpenSim-
Sonnenrichtung wird weiter als GPU-PushConstant übergeben.
Die RGB-Werte werden bei jedem gültigen Frame mit dem
richtungsabhängigen Diffuslicht multipliziert. Die bisherigen
Depth-Debug-/Lambert-/Sun-Diagnosen bleiben unangetastet.

**Sicherheitsgates:** FACE_TINT_PROBE=1 benötigt außerdem
WORLD_LAMBERT_PROBE=1, WORLD_SUN_LIGHT_PROBE=1 sowie alle
bestehenden unabhängigen Device-/Camera-/Mesh-/Offscreen-
Freigaben. Der Modul-Upload überprüft jedes Farbtripel und
verlangt identische Anzahl und aktuelle Meshgeneration.
Bei Region-/Meshersatz wird die Farbbindung invalidiert;
bei fehlgeschlagener Quellübertragung wird ein vorheriger
Farbbuffer explizit verworfen, statt alte Materialfarben
einer neuen Region zuzuordnen. Kein sichtbares Vulkan-
Fenster oder OpenGL-Renderziel wird übernommen.

**20 tatsächlich ausgeführte Implementierungs-/Testaufgaben:**

01. Branch ogl-develop, sauberen Ausgangsstand, aktuelle Roadmap/Handoff/AGENTS und erfolgreichen vorherigen plattformübergreifenden GitHub-CI-Lauf geprüft.
02. Bestehenden realen Firestorm-Deferred-Szenenscanner einschließlich LLVOVolume, LLVolumeFace und Texture-Entries auf verfügbare Farbattribute geprüft.
03. Unveränderte historische OpenSim-/RTX-Triangle-ABI für dreimal drei Weltpositions-Floats als bindende Abwärtskompatibilität beibehalten.
04. Additiven C-ABI-Datentyp ogl_rtx_face_tint mit einem streng gepackten RGB-Tripel pro tatsächlich akzeptiertem Dreieck ergänzt.
05. Neuen separaten, nur nach Opt-in freigegebenen Vulkan-GPU-RGB-Vertexbuffer für exakt drei Farbvertices je World-Dreieck eingeführt.
06. RGB-Eingaben vor jeder GPU-Allokation auf gültige Dreieckszahl, aktuelle GPU-Meshgeneration, endliche Zahlen und Wertebereich 0 bis 1 geprüft.
07. GPU-Seitenbuffer via VkBuffer, Host-Visible- und Host-Coherent-VkDeviceMemory mit sicherem Rollback bei partiellen Allokationsfehlern eingerichtet.
08. GPU-Farbressourcen bei Region-Clear, Meshersatz und Modul-Shutdown automatisch freigegeben und bei Farbenwechsel unabhängig vom Positionsmesh ersetzt.
09. Separate Revision/Farb-Uploadzähler eingeführt, damit ohne neuen GPU-Positionsmeshupload eine aktualisierte Textur-Entry-Grundfarbe angenommen werden kann.
10. Zwei GLSL-450-Shader für positionierte Welt-Vertices mit zusätzlicher RGB-Attribute-Location und Lambert-Sonnenbeleuchtung geschrieben und als Vulkan-1.0-SPIR-V geprüft.
11. Beide validierten SPIR-V-Programme in die bestehende Vulkan-Modulquelle eingebettet; keine neuen Runtime-Shaderdateien und keine weiteren Viewer-Abhängigkeiten hinzugefügt.
12. Vulkan-Vertex-Input-Binding 1 zusätzlich zu unverändertem Positions-Binding 0 eingerichtet und nur im neuen Entwickler-Shadermodus aktiviert.
13. Vierten GPU-Shader-/Cache-Modus ergänzt, der Face-Tint-Farbe und aktuelle normierte OpenSim-Sonne kombiniert; alle drei bisherigen Shaderpfade funktionieren unverändert.
14. Neuen Shadermodus an OGL_VULKAN_WORLD_FACE_TINT_PROBE=1 plus vorhandenes Lambert-/Sun-/Mesh-/Camera-/Offscreen-Opt-in gebunden; unvollständige Berechtigungen fail-closed.
15. Firestorm-Deferred-Scanner auf echte opake Texture-Entry-RGB-Werte pro Volume-Face erweitert; Terrain/Ray-Anker/Transparenz ohne sicheren Farbwert bekommen neutrale Ersatzfarbe.
16. Snapshot-Arrays bei Terrain-, Vorpass- und Volume-Dreiecksanfügen synchronisiert und Farb-/Positionsanzahl vor der Deferred-Quellübergabe auf Gleichheit überprüft.
17. Firestorm-Host über neuen Source-qualified-Sidecar-Pfad verbunden, sodass Farb-Uploads vor Kamerarebind und Vulkan-Offscreen-Probe erfolgen; ungültiger Farbsource invalidiert alte Quittungen.
18. Additive Read-only-Vulkan-C-ABI für aktive Farb-Meshgeneration, Farb-Uploadzähler und mittlere GPU-Rot-/Blaukanalwerte ergänzt.
19. Native Vulkan-GPU mit gleichen World-Koordinaten und rotem beziehungsweise blauem Farbbuffer geprüft, Negativkontrollen für falsche Farben und Dreieckszahl, neue Region-/Meshrevision, 256x144-Resize und GPU-Cache-Lebenszyklus bestanden.
20. Linux-Vollviewer mit 373 Python-Tests und 41 nativen CTests bestanden; Quellcodeprüfung 61 Grafikverträge und 45 Berichttests grün, isolierte SDL/X11-Viewerstart-Fallbacks mit neuen Farb-Flags ausgeführt.

**Native echte Vulkan-GPU-Abnahme:** Die GPU rendert
mit identischem World-Mesh, identischer Kamera und
derselben gültigen Sonnenrichtung zwei verschiedene
RGB-Farbbuffer: rot ergibt im tatsächlich gezeichneten
Bild einen mittleren Rot-/Blaukanal von **51/0**,
blau einen mittleren Rot-/Blaukanal von **0/51**.
Ein reiner Farbwechsel lädt **kein neues GPU-Mesh**
und baut die vorhandene Shader-Pipeline nicht neu.
Die GPU-Abnahme umfasste **26 private World-/Depth-Frames,
15 GPU-Cache-Aufbauten, 11 Wiederverwendungen und
15 Freigaben**. Auch ein 256x144-Frame, ein ungültiger
Farbwert, ein falscher Triangle-Count, ein ungültiges
Entwicklerflag, eine echte neue GPU-Meshgeneration
mit invalider alter Farbbindung und das Neuhochladen
passender Farben wurden auf der GPU nachgewiesen.

**38/38 isolierte SDL/X11-Viewer-GUI-Starts bestanden**, darunter fünf neue Farb-Opt-in- und Invalid-Flag-Tests, normale OpenGL-Logins und alle Vulkan-WSI-/Regions-/Treiber-Rückfälle.

**Voll-Linux-Build:** 373/373 Python-Tests, 41/41
native CTests und 61/61 gezielte Grafik-
Quellcodekontrakte bestanden. Die bisherigen
45/45 Report-/Herkunftstests blieben grün.
Separate SDL/X11-GUI-Tests mit normalem
OpenGL-Login, Farbentwicklerflags und
Treiber-/Regionswechsel-Fallbacks laufen
ausschließlich in isolierten Profilen.

**Offenes P05-Gate:** Die Codeübergabe ist mit dem
realen OpenSim-Deferred-Scanner verbunden,
aber es wurde **keine authentifizierte Stadt-
NexVerse-Live-Region mit tatsächlichen Vulkan-
Face-Farb-GPU-Bildern abgenommen**.
Die nachgemessenen roten/blauen GPU-Testpixel
stammen aus kontrollierter synthetischer
Entwicklergeometrie. Der echte Texture-/PBR-/
Terrain-/Wasser-/Avatar-/UI-Renderer, Vulkan-
Fensterhoheit, produktive Vulkan-Grafikauswahl
und hardwareübergreifende Live-Abnahme fehlen.
OpenGL bleibt der einzige sichtbare Welt-Renderer.
P05 formal 2/20; Block 3 und vollständiger
Block 4 weiterhin offen.
Code-Commit: 115294005da019ff2f658a853d8f4c0e56c1a649.

## P05/3–4 – 20 technische Schritte: Sonnenrichtung aus Firestorm auf Vulkan-GPU (07.11.2026)

**Neue Umsetzung:** Die privat gerenderte Vulkan-World-XYZ-Lambert-Pipeline
kann unter zwei separaten Entwicklerfreigaben den tatsächlichen
Sonnenrichtungsvektor des Firestorm-Deferred-Kameraframes verwenden.
Der neue Vulkan-1.0-Fragmentshader wurde mittels glslangValidator
kompiliert und mit spirv-val validiert. Ein 80-Byte-Vulkan-PushConstant-
Paket enthält die bestehende 64-Byte-Kameramatrix und 16 Byte
normalisierte Sonnenrichtung. Bei Lichtwechsel sind weder neue
GPU-Meshuploads noch Shader-/Image-/Pipeline-Rebuilds notwendig.

**Gates:** OGL_VULKAN_WORLD_SUN_LIGHT_PROBE=1 erfordert zusätzlich
OGL_VULKAN_WORLD_LAMBERT_PROBE=1 sowie die bisherigen unabhängigen
Device-/Mesh-/Camera-/Offscreen-Freigaben. Ungültige Flags, Nullsonne,
NaN/Inf und unplausible Sonnenlängen werden verworfen. Drei
Shadermodi bleiben unabhängig: Depth-Debug als Standard, feste
Lambert-Testbeleuchtung, dynamische Sonne. Regions-/Mesh-/WSI-/
Kamerainvalidierung verwirft auch die gekoppelte Sonnendatenquelle.
Der 120-Deferred-Frame-Hosttick reagiert nur bei explizit aktiviertem
dynamischem Licht auf reine Sonnenänderungen.

**20 tatsächlich durchgeführte technische Arbeitsschritte:**

01. Branch, CI auf Linux/Windows/macOS und kanonische Projektdokumentation geprüft.
02. Firestorm-Deferred-Pass und echten LLEnvironment-Sonnenvektor bis zur vorhandenen v3-Kamera-ABI geprüft.
03. Starre Entwickler-Lambert-Lichtquelle als nächste fehlende reale Weltquelle identifiziert.
04. Neue GLSL-450-Vulkan-Fragmentshader-Variante für veränderbare Sonnenrichtung und GPU-Weltflächennormalen erstellt.
05. GPU-Normalen weiter aus dFdx/dFdy der Weltpositionsdaten berechnet.
06. Sonnenlicht als zusätzliche vec4 nach der 64-Byte-World-to-Clip-Kamera-PushConstant angebunden.
07. Neuen GLSL-Shader für Vulkan 1.0 kompiliert und mit Khronos spirv-val erfolgreich validiert.
08. Validierte 460 SPIR-V-Wörter in bestehender Vulkan-C++-Moduldatei eingebettet, keine neuen Repo-Dateien.
09. Eigenes OGL_VULKAN_WORLD_SUN_LIGHT_PROBE-Opt-in zusätzlich zu LAMBERT_PROBE und bestehenden GPU-Gates erzwungen.
10. Ungültige Opt-ins, Null-/nicht endliche Sonnenrichtung und außerhalb der geprüften Vektorlänge liegende Werte fail-closed abgelehnt.
11. Den akzeptierten Kamerasonnenvektor normiert und nur intern als vier Float-Werte für die GPU bereitgestellt.
12. Nur für dynamisches Sonnenlicht das Vulkan-Pipeline-Push-Layout auf 80 Byte und Vertex-/Fragment-Stages erweitert; alte Modi bleiben 64 Byte.
13. Shader-Cache strikt nach den drei Modi Depth-Debug, feste Lambert-Normale und dynamische Sonne getrennt.
14. Beim Sonnenrichtungswechsel lediglich PushConstants erneuert; GPU-Image, Pipeline und Meshbuffer wiederverwendet.
15. Ungültige Sonnen-Kamera-Daten bei Regionswechsel, Meshersatz, Scene-Clear, WSI-Kamerainvalidierung und fehlerhaftem Kameraframe verworfen.
16. Vorhandenen gedrosselten 120-Frame-Kamera-Tick erweitert: reine Sonnenbewegung zählt nur bei beiden dynamischen Entwicklerfreigaben.
17. Grafik-Quellcode-Regressionsprüfung für 80-Byte-PushConstant, Shader, Freigaben, Cache-Modi und Sicherheitsgrenzen ergänzt.
18. Native Vulkan-GPU: zwei verschiedene Sonnenrichtungen auf identischem Mesh verglichen, mittlere RGB-Summe 70 gegenüber 349 bei unveränderter GPU-Meshgeneration.
19. Negative GPU-Abnahmen für fehlende Lambert-Freigabe, ungültiges Sonnenflag, Nullsonne und anschließende erfolgreiche Wiederherstellung umgesetzt.
20. Native Vulkan-GPU mit 22 World-/Depth-Frames, 12 Builds/10 Reuses/12 Releases und alten Vulkan-X11-/Regionstests bestanden; Linux-Vollbuild 372 Python-, 41 native CTests, 60 Grafiktests und 45 Reporter-Tests grün.

**GPU-Abnahme:** Echte native Vulkan-X11-Tests:
22 private Welt-/Depth-GPU-Frames, 12 Cache-Builds,
10 Reuses und 12 Releases. Bei gleicher Kamera und
gleichem GPU-Mesh brachten Sonnenvektoren entlang
X und Z nach GPU-Auswertung mittlere RGB-Summen
von **70 bzw. 349**. GPU-Mesh-Uploadzähler und
Meshgeneration blieben unverändert. Ein Vektor
mit Länge 2 wurde normiert; Null-Sonnenvektor
und ungültige oder unvollständige Developer-
Flags wurden abgewiesen. Bisherige Vulkan-
Region-/Depth-/X11-Present-Tests bestanden.

**GUI-Abnahme: 33/33 isolierte reale SDL/X11-Viewerstarts bestanden**, einschließlich vier neuer SUN-/LAMBERT-Opt-in- und Invalid-Flag-Fälle.

**Voll-Linux-Build:** 372/372 Python-Tests und
41/41 native CTests erfolgreich; 60/60
gezielte Grafik-Quellcode-Kontrakte und
45/45 GPU-Report-Tests ebenfalls grün.
Isolierte echte SDL/X11-Viewer-GUI-Smokes
prüfen vier neue Sonnenlicht-/Invalid-Flag-
Kombinationen, normalen OpenGL-Login sowie
bisherige Vulkan-Fallbacks. Keine Nutzer-
Viewer-Prozesse, Profile oder Secrets berührt.

**Offen:** Die echten GPU-Testpixel stammen
aus synthetischen Entwickler-World-Meshes.
Kein authentifiziertes OpenSim-Login mit
nachgewiesenem Vulkan-Live-Weltrendering.
Terrain, reale Materialien/Texturen,
Wasser, Avatare und Benutzeroberfläche
werden nicht über Vulkan gezeichnet.
Keine sichere produktive Vulkan-
Fensterübernahme. Die sichtbare Welt
verwendet weiter OpenGL.
**P05 formal 2/20 operative Blöcke,
Block 3 und vollständiger Block 4 offen.**
Code-Commit: 043efe22e4b39bbb64c1126ea255e7aba247d707.

## P05/3–4 – 20 technische Schritte: GPU-Normalen und einfaches Vulkan-Weltlicht (06.11.2026)

**Ziel dieser Runde:** Der private Vulkan-World-/Depth-GPU-
Raster zeichnet bisher nur nahe rote und ferne grüne
Debugflächen. Für die Weiterentwicklung zur Weltgrafik
wurde eine **separate, opt-in 3D-Normalen-Lichtpipeline**
implementiert. Sie rendert aktuell hochgeladene GPU-
World-XYZ-Dreiecke tatsächlich mit richtungsabhängiger
Lambert-Beleuchtung. Das ist ein UNTEXTURIERTER
technischer Zwischenschritt, KEIN fertiger
Material-/Textur-/PBR-/Raytracing-Renderer.

**GPU-Implementierung:** Ein neuer GLSL-450-Vertexshader
gibt pro Vertex die Weltposition weiter; der Fragment-
shader berechnet per dFdx/dFdy und Kreuzprodukt
eine eigene Flächennormale und beleuchtet sie
mit einem festen gerichteten Licht plus
Ambient-Anteil. Degenerierte Normale wird
begrenzt behandelt. Zwei Shader wurden
als Vulkan-1.0-SPIR-V kompiliert und mittels
spirv-val überprüft; ihre 719 32-Bit-Wörter
sind als unveränderliche Arrays direkt im
bestehenden Modul eingebettet. Keine neuen
Repository-Shaderdateien, kein Shader-Compiler
oder zusätzliche GPU-Software beim Nutzer.

**Opt-in und Shadercache:** Die neue Beleuchtung
läuft ausschließlich unter dem getrennten
Entwicklerflag OGL_VULKAN_WORLD_LAMBERT_PROBE=1
und allen bisherigen GPU-World-/Mesh-/Kamera-/
Offscreen-Freigaben. Unzulässiger Flagwert
scheitert geschlossen. Standard ohne Flag:
bisherige rot/grüne Tiefen-Debugdarstellung.
Der Vulkan-Gerätecache unterscheidet
Tiefen- und Lambert-Pipeline; nur bei
Modus-/Auflösungsänderung wird nach
erfolgreichem GPU-Fence Shader/Pipeline/
Framebuffer neu aufgebaut, ansonsten
weiterverwendet. Der sichtbare OpenGL-
Renderer wird NIE ersetzt und kein
VkQueuePresentKHR auf dem Nutzerfenster
ausgeführt.

**Numerische GPU-Nachweise:** Das Modul
berechnet nach realem VkDrawIndexed und
VkReadback nur die Anzahl beleuchteter
GPU-Pixel und die mittlere RGB-Summe
(0 bis 765) über nichtleere Pixel.
Zwei additive C-ABI-Read-only-Funktionen
melden diese Werte. Der echte Firestorm-
Host führt sie in der anonymen, bereits
quellqualifizierten Deferred-GPU-Receipt-
Kette mit, ohne die eigentliche Bitmap,
den User, eine Regionkennung, Kamera-
positionen oder Assetdaten zu loggen.
Der lokale Bericht verwirft unzulässige
Licht-/Pixelwerte und akzeptiert alte
Logs ohne die neuen Felder weiter.

**20 tatsächlich umgesetzte oder getestete Arbeitsschritte:**

01. Git-Branch ogl-develop, kanonische Dokumente, sauberen Ausgangsstand, reale Viewer-Prozesse und letzte vollständig grüne Linux-/Windows-/macOS-CI geprüft.
02. Vulkan-World-XYZ-Pipeline, privaten Farb-/Depth-Framebuffer, World-Camera-PushConstant und gerätegebundenen Shader-Pipeline-Cache analysiert.
03. Separate Vertex-Shader-Quelle für aktuelle World-XYZ-Position, Vulkan-Welt-Projektionsmatrix und 3D-Weltpositions-Interpolationskanal erstellt.
04. Separate Fragment-Shader-Quelle für GPU-Fragmentableitungen dFdx/dFdy aus der tatsächlichen interpolierten 3D-Weltposition erstellt.
05. Vektorprodukt der Weltkoordinaten-Ableitungen als geometrische Pixel-Flächennormale implementiert.
06. Degenerierte Normale mit begrenzter Längenprüfung und Ersatznormalen behandelt, damit aus degenerierter Geometrie keine NaN-Lichtfarbe entsteht.
07. Richtungsabhängiges zweiseitiges Lambert-Diffuslicht mit begrenztem Ambient-Anteil und kontrollierter untexturierter Entwicklerfarbpalette implementiert.
08. Beide originalen GLSL-450-Shader als SPIR-V für Vulkan 1.0 kompiliert und mittels Khronos spirv-val erfolgreich validiert.
09. Validierte SPIR-V-Arrays in die bereits bestehende Vulkan-Modulquelldatei eingebettet; keine neue Shader-Datei oder Laufzeit-Abhängigkeit im Repository angelegt.
10. Explizites Developer-Flag OGL_VULKAN_WORLD_LAMBERT_PROBE eingeführt; nur Wert 1 aktiviert Beleuchtung, ungültige Werte werden ohne GPU-Frame fail-closed abgewiesen.
11. Originalen roten/grünen Tiefen-Debugshader als Standard unverändert beibehalten, kein automatischer Wechsel im normalen Viewer oder Benutzer-Grafikmenü.
12. Vulkan-GPU-Cache gegen den gewählten Shader-Modus abgesichert: bei Wechsel nach abgeschlossenem GPU-Fence neu aufbauen, sonst bestehende Bilder, Pipeline und Readback wiederverwenden.
13. Das World-XYZ-VkDrawIndexed-Rendering, GPU-Tiefentest VK_COMPARE_OP_LESS und die vorhandene 64-Byte-Kamera-PushConstant für beide Shader-Modi beibehalten.
14. GPU-Readback um zusammengefasste normalbeleuchtete Pixelzahl und Mittelwert der RGB-Helligkeit pro abgeschlossenem privaten Bild erweitert, ohne Bildexport.
15. Zwei additive Read-only-C-ABI-Funktionen für die aggregierten beleuchteten Pixel und mittlere RGB-Helligkeit des letzten GPU-Weltframes ergänzt.
16. Den echten Firestorm-Deferred-Host um optionale Vulkan-ABI-Auflösung und anonymisierte lit_pixels-/mean_luminance-Diagnose erweitert.
17. Den vorhandenen lokalen GPU-Herkunftsreport für ältere und neue OpenSim-GPU-Quittungen kompatibel erweitert; Grenzen lit_pixels <= nonclear_pixels und mittlere RGB-Summe 0..765 geprüft.
18. Vier neue Report-Tests und einen neuen Grafik-Quellcodevertrag für Shader, Opt-in, GPU-Cache, echte Draws und datensparsame Diagnose ergänzt.
19. Native echte Vulkan-X11-GPU-Abnahme: 18 World-/Depth-Frames, Lambert bei 32x32 und 256x144, Rückkehr zum Depth-Debugshader, 10 Cache-Builds/8 Reuses/10 Releases, bei gekipptem vs. frontalem Dreieck RGB-Mittel 274 vs. 327.
20. Kompletten Linux-Viewer mit 371 Python-Tests, 41 nativen CTests, 59 Grafikverträgen und 45 Report-Tests gebaut; isolierte echte Viewer-GUI-Starts einschließlich neuer Lambert-Freigaben und fehlender Vulkan-Treiber/Fallbacks durchgeführt.

**Native GPU-Belege:** Die reale Vulkan-GPU
hat 18 private, indexierte World-/Depth-
GPU-Bilder erfolgreich gerendert:
10 GPU-Pipeline-Cache-Aufbauten,
8 Wiederverwendungen,
10 kontrollierte Freigaben.
Mit dem neuen Beleuchtungsshader
lieferten zwei unterschiedlich orientierte,
echte GPU-Mesh-Dreiecke bei identischer
Kamera messbar verschiedene RGB-
Mittelwerte **274 (geneigt) und
327 (frontal)**. Dieser Unterschied
ist aus GPU-Fragmentableitungen der
Weltkoordinaten entstanden; eine
konstante Einfärbung würde ihn nicht
erklären. Bei gleicher Geometrie wurde
erfolgreich auf die bisherige
Depth-Debugpipeline zurückgeschaltet.
Alle vorherigen X11-/Depth-/
Window-/Regionswechseltests blieben grün.

**Linux-Abnahme:** Vollviewer-Build
und 371/371 Python-Tests,
41/41 native CTests,
59 gezielte Grafik-/Quellcode-Checks
und 45 Report-Event-Tests grün.
**29/29 isolierte reale SDL/X11-Viewer-GUI-Starts** grün,
darunter drei neue Shader-/Invalid-Flag-Fälle,
alle vorherigen Vulkan-WSI-/Region-/Treiber-Fallbacks.
CLI-Logtests: alte GPU-Quittung
ohne Lichtfelder akzeptiert;
aktuelle Lichtstatistik 254/1024
und 327, größerer 256x144-
Frame mit 17500 beleuchteten
von 18000 nichtleeren Pixeln
und mittlerer RGB-Summe 274
akzeptiert; unzulässige 18001
beleuchtete Pixel und zu großer
RGB-Wert 999 verworfen.
Temporäre QA-Logs entfernt.

**Noch nicht erreicht:** Es gibt in
dieser Runde keinen authentifizierten
OpenSim-Login mit tatsächlich gemessenem
GPU-Lichtbild einer Live-Region.
Die Pixel der GPU-Abnahme stammen
aus synthetischen Testmeshes.
Die normale 3D-Welt läuft weiter
über OpenGL; Vulkan hat weder
Materialtexturen, Terrain,
Wasser, animierte Avatare,
Realtime-Weltbeleuchtung,
GPU-Textur-Upload noch die
exklusive In-World-Fensterhoheit.
Die neue Beleuchtung ist als
Vulkan-1.0-Shader auch NICHT
NVIDIA-RTX-spezifisch.
**P05 formal weiterhin 2/20;
Block 3 und vollständiger
Block 4 bleiben offen.**
Nächstes /goal-Gate: tatsächliche
eingeloggte Region-Capture-/GPU-
Quittung plus aussagekräftige
Vulkan-World-/Depth-/Licht-
GPU-Diagnose vor sichtbarer
Window-Ownership.
Code-Commit: 5938a39500bcfab6cba9cbba9ebf7ae0655847c6.

## P05/3–4 – 20 technische Schritte: private Vulkan-Welt-GPU-Rasterfläche mit vier Auflösungen (06.11.2026)

**Technische Umsetzung:** Der vorher auf 32x32 Pixel begrenzte private
Vulkan-World-/Depth-Renderer kann jetzt per ausdrücklich aktivierter
Entwicklerauflösung vier feste GPU-Rendergrößen verwenden: 32x32
(Standard), 128x96, 256x144 und 512x288. Das neue Flag heißt
OGL_VULKAN_WORLD_OFFSCREEN_RESOLUTION; unbekannte und übergroße
Werte werden vor GPU-Allokationen abgewiesen. Dieses Diagnostikbild
gehört ausschließlich Vulkan und verändert kein SDL-Fenster,
keine aktive OpenGL-Renderfläche und keine Logindaten.

**GPU-Bilder:** VkImage, GPU-Memory, Views, Depth-Image,
VkFramebuffer, Viewport, Scissor, Image-zu-Buffer-Kopie und
RGBA-Readback werden in tatsächlicher Breite/Höhe erstellt.
Echte indexierte World-XYZ-Triangles werden mit
VK_COMPARE_OP_LESS im privaten GPU-Depth-Test gezeichnet.
Vulkan-GPU-Cache-Ressourcen werden nur bei geänderter Größe
nach gesichertem GPU-Fence neu erzeugt, nicht bei jedem
Kameraframe. Der Shader bekommt eine Kopie der gültigen
64-Byte-Firestorm-Weltkameramatrix mit angepasstem
X-Projektionsmaßstab für das Zielseitenverhältnis.
Originalkamera, Regions-/Meshgeneration und OpenGL-Fenster
bleiben unverändert.

**GPU-Schnittstelle und Bericht:** Zwei additive C-ABI-Funktionen
liefern Breite und Höhe des zuletzt tatsächlich fertiggestellten
privaten Weltframes. Die bestehende OpenSim-Deferred-Capture-
Herkunftskette protokolliert anonym die GPU-Abmessungen.
ogl/scripts/ogl-test-report.py akzeptiert alte 32x32-Quittungen,
prüft aber neue Pixelzahlen gegen ihre tatsächliche
allowlistierte Bildkapazität. Der Bericht verwirft
ungültige Bildmaße und überzählige GPU-Pixel.

**20 tatsächlich implementierte bzw. getestete Arbeitsschritte:**

01. Vorherigen sauberen Git-Branch, AGENTS, Handoff, Roadmap, Prozesszustand und vollständig erfolgreiche letzte GitHub-CI geprüft.
02. Bisherigen festen privaten 32x32-Vulkan-World-/Depth-Renderer und dessen gerätegebundene Ressourcen analysiert.
03. Entwicklerflag OGL_VULKAN_WORLD_OFFSCREEN_RESOLUTION mit exakt vier erlaubten Werten 32x32/128x96/256x144/512x288 eingeführt.
04. Ungültige oder übergroße Werte wie 8192x8192 vor GPU-Allokation fail-closed abgewiesen, ohne bisherigen Cache anzutasten.
05. GPU-Pixelobergrenze 512x288 und VkDeviceSize-sichere RGBA-Readback-Byteberechnung eingesetzt.
06. GPU-Farbimage, Speicher und ImageView auf tatsächliche Breite und Höhe ausgelegt.
07. Privates Vulkan-Depth-Attachment D32/D16 und Framebuffer auf dieselben Bildmaße skaliert.
08. VkRenderPass-RenderArea, Framebuffer-Extent und Raster-Viewport dynamisch angepasst.
09. GPU-Scissor, Image-to-Buffer-Kopie und Transfer-to-Host-Synchronisation auf das private Bildformat angepasst.
10. Vorhandenen World-XYZ-VkDrawIndexed-Mesh-Datenpfad und echten Vulkan-Depth-Test bei allen vier Bildgrößen verwendet.
11. Gerätegebundenen Vulkan-GPU-Cache bei Auflösungswechsel kontrolliert nach beendetem Fence freigegeben und neu aufgebaut.
12. Cache bei unveränderter Auflösung für Folgeframes samt CommandBuffer, Shader, Depth und Readback wiederverwendet.
13. Kopie der 64-Byte-Firestorm-Kamera-PushConstant für jedes private Offscreen-Seitenverhältnis korrekt neu projiziert, ohne Originalkamera zu ändern.
14. Read-only-C-ABI für Breite und Höhe des jeweils letzten fertig gerenderten Vulkan-GPU-Weltbilds ergänzt.
15. Echte Deferred-Capture-GPU-Quittungen und anonyme Host-Diagnose um die privaten GPU-Bildabmessungen erweitert.
16. Bestehenden lokalen Viewer-Testbericht auf allowlistierte GPU-Pixelkapazität statt feste 1024 Pixel erweitert und alte 32x32-Logs weiter unterstützt.
17. Vier weitere Reporter-Unit-Tests für alle Bildgrößen, alte Logs, fehlerhafte Abmessungen und Pixelwerte über Kapazität implementiert.
18. Native echte Vulkan-GPU-Tests auf drei größere Renderflächen sowie ungültiges Format, Rückwechsel zu 32x32 und unveränderte GPU-Meshgeneration erweitert.
19. Hardware-Tests mit zwölf World-/Depth-GPU-Bildern, sechs Cache-Aufbauten, sechs Wiederverwendungen und sechs Freigaben bestanden; alte X11-/Depth-/Region-GPU-Tests grün.
20. Linux-Vollviewer mit 366 Python-Tests, 41 nativen CTests, 58 Grafik-/Quellcodekontrakten und 41 Reporter-Tests bestanden; 26 isolierte Viewer-GUI-Regressionen einschließlich fünf Auflösungsfällen ausgeführt.

**Hardware- und Testabnahme:** Native Vulkan-GPU:
12 private World-/Depth-GPU-Bilder, 6 GPU-Cache-Aufbauten,
6 Wiederverwendungen und 6 kontrollierte Freigaben.
Vier Bildgrößen und ein ungültiges 8192x8192-Preset
sind getestet. Die Rückkehr zur korrekt seitenverhältnis-
projizierten 32x32-Kamera ergab 254/1024 nichtleere
GPU-Testpixel (vor Aspect-Fix 192). X11-Presents,
GPU-Fences, Depth, Meshwechsel und Regionsbarriere
blieben grün. Vollständiger Linux-Viewer-Build:
366/366 Python, 41/41 native CTests,
58/58 gezielte Grafiktests, 41/41 Reporttests.
Reporter-End-to-End-Fixtures: 18000/36864
bei 256x144, 147455/147456 bei 512x288,
überschüssige Pixel verworfen. GUI-Abnahme
mit separaten Xvfb-Profilen und 26 Fällen
einschließlich aller neuen Auflösungsflags.
Alle Fixtures wurden bereinigt, keine aktive
Nutzersitzung verändert.

**Abgrenzung:** Vulkan zeichnet noch nicht die für den
Benutzer sichtbare OpenSim-Welt. Die tatsächliche
Welt inkl. Terrain, Wasser, Materialien, Meshtexturen,
Avataren und UI wird weiter über OpenGL gerendert.
Die neuen GPU-Pixeltests verwenden synthetische
Entwicklergeometrie. Ein authentifizierter
Stadt-NexVerse-Live-Login mit positiv korrelierten
Capture-/GPU-Quittungen wurde noch nicht geprüft.
P05 bleibt formal 2/20; Block 3 und der
vollständige World-Renderer-Block 4 bleiben offen.
Code-Commit: 0e86d6fdd0254f79ae03d628dc8c9c85ff54152f.

## P05/3–4 – 20 technische Schritte: echte Deferred-Capture-Herkunft gegen synthetische Vulkan-GPU-Tests abgrenzen (06.11.2026)

**Kernänderung:** Das tatsächliche Firestorm-Deferred-Weltrendering
kann weiterhin OpenSim-Dreiecke an die vorhandene Vulkan-GPU-
Staging-ABI weitergeben. Zusätzlich erhält NUR der echte
Deferred-Scanner eine eigene C++-Host-Entrypoint-Funktion.
Ein erfolgreicher GPU-Mesh-Upload aus dieser Quelle erzeugt
eine **numerische Capture-Quittung**: nur lokale monoton
steigende Capture-Nummer, Regions-Epoche, Dreiecksanzahl und
exakte Vulkan-Meshgeneration. Das generische Modul-/Test-
Triangle-API kann eine solche Live-Capture-Quittung
**weder erzeugen noch von einem alten Scan übernehmen**.
Dafür bleiben OpenGL und alle bestehenden
GPU-/Modul-Diagnostikpfade unverändert.

**GPU-Herkunft:** Ein hostgesteuerter privater 32x32-
Vulkan-World-/Depth-Offscreen-GPU-Frame darf erst dann
als Deferred-GPU-Quittung gelten, wenn eine aktuelle
Capture-Quittung existiert, Regions-Epoche und GPU-
Meshgeneration exakt passen, die Kamera-Mesh-Kopplung
bereit ist und ein echter Vulkan-GPU-Abschluss mit
erhöhtem Framezähler bestätigt wurde. Als Herkunft
wird **mesh-rebind** oder **camera-tick** geloggt.
Der reine GPU-Modultest, ein Vorlogin-Vulkan-WSI-
Present oder beliebige künstliche Dreiecke werden
dadurch KEINE vermeintlichen echten OpenSim-World-
Frames. Bei Regionwechsel, neuer generischer
Meshquelle oder explizitem Scene-Clear werden
die vorherigen Herkunftsmarker verworfen.

**Vertraulichkeit und Diagnose:** Alle Quittungen
enthalten AUSSCHLIESSLICH Zähler, Epochen,
GPU-Meshgenerationen und 0–1024 Bildabdeckungs-
Pixel. Keine OpenSim-Region-UUID, Avatar-Namen,
Accountdaten, Grid-Adressen, Geokoordinaten,
Kamerapositionen, Bild- oder Meshdateien werden
geloggt. Die vorhandene Datei
ogl/scripts/ogl-test-report.py analysiert diese
Ereignisse jetzt in chronologischer Reihenfolge
und trennt: keine Diagnose-Sitzung,
kein beobachteter Weltstart, keine Capture-
Quittung, akzeptierte Capture mit GPU noch
offen, und source-qualified GPU-Abschluss.
Auch eine solche Logkette ist KEIN
unabhängiger kryptographischer Nachweis
eines authentifizierten OpenSim-Logins
und KEIN sichtbarer Vulkan-Weltrenderer.

**20 tatsächlich ausgeführte technische Teilaufgaben:**

01. Sauberen Ausgangsbranch ogl-develop sowie AGENTS, Projektstatus, Handoff, P05-Roadmap und vollständig erfolgreiche vorherige GitHub-CI überprüft.
02. Firestorm-Deferred-Kamera-Hook und realen periodischen OpenSim-Regionsmesh-Scanner bis zur Vulkan-ABI-Nachverarbeitung verfolgt.
03. Die fehlende Unterscheidung zwischen Entwickler-GPU-Proben und tatsächlich vom Deferred-Weltpass gelieferten Meshdaten identifiziert.
04. Im bestehenden Host eine eigene Source-Provenienz für akzeptierte echte OpenSim-Deferred-Meshsnapshots eingeführt, getrennt von der generischen Triangle-Test-ABI.
05. Den bestehenden LLPipeline-Mesh-Scan auf die neue explizite Deferred-Scanner-Host-Funktion umgestellt.
06. Den Deferred-Scanner-Aufruf zusätzlich an eine existierende LLViewerRegion und nichtleere OpenSim-Region-UUID gebunden.
07. Nur nach erfolgreich akzeptiertem Vulkan-GPU-Mesh-Upload eine anonymisierte Capture-Quittung mit lokaler monotoner Nummer erstellt.
08. Jede Capture-Quittung mit der aktuellen Regions-Epoche und der exakt akzeptierten Vulkan-GPU-Meshgeneration verknüpft.
09. Die generische Triangle-ABI weiterhin unterstützt, aber deren Aufrufe können niemals eine gültige Deferred-Quittung ausstellen oder übernehmen.
10. Bei explizitem Regions-Clear eine bisherige Deferred-Quittung, deren Epochenbindung und Meshgeneration ungültig gemacht.
11. Bei echtem OpenSim-Regionwechsel die bisherige Quittung sofort invalidiert und einen anonymisierten Region-Grenzmarker für die Diagnose geschrieben.
12. Den Diagnose-Sessionbeginn gesondert markiert, sodass spätere Viewerstarts keine alten Herkunftsnachweise übernehmen.
13. Den tatsächlichen privaten Vulkan-Offscreen-Draw im Host zusätzlich an eine aktuelle Deferred-Quittung und passende Regions-/Meshgeneration gebunden.
14. Nur nach erfolgreich bestätigtem Vulkan-GPU-Fence und dem Fortschritt des GPU-Framezählers eine abgeschlossene GPU-Quittung mit Quelle mesh-rebind oder camera-tick protokolliert.
15. Nur numerische Epochen, Capture-/GPU-Zähler, Meshgenerationen und 0–1024 Pixelwerte dokumentiert; keine Avatar-/Regions-IDs, Kamerakoordinaten, Bilddaten oder Accountinformationen.
16. Den vorhandenen lokalen Viewer-Testreport um separate Parser für Session-, Regions-, Capture- und GPU-Quittungen erweitert, ohne neue Quelldateien anzulegen.
17. Die Reporter-Auswertung auf die jüngste Sitzung, aktuelle Region und zuletzt akzeptierte Capture-/Meshgeneration begrenzt; jeder neuere Snapshot oder Regionswechsel verwirft alte GPU-Erfolge.
18. 13 neue Unit-Tests für Sessionstart, Weltstart, fehlende Captures, Generation-/Quittungsmismatch, Regionswechsel, neue Meshe, Camera-only-Frames, ungültige Pixelwerte, Reihenfolge und Datenschutz ergänzt.
19. Vier isolierte CLI-Reporter-Szenarien mit temporären anonymen Logs geprüft: synthetischer Login, Deferred-Snapshot mit GPU ausstehend, paariger GPU-Erfolg und Regionswechsel-Invalidierung.
20. Linux-Vollviewer-Build 361/361 Python-Tests und 41/41 native CTests; 57/57 Grafik-Quellcode-Prüfungen, 21/21 isolierte GUI-Starts, drei Negativbeweise gegen falsche Live-Captures und alle nativen Vulkan-GPU-Regressionen erfolgreich.

**Testnachweise:**
- **361/361 Python-Tests**, davon **57/57 gezielte
  Grafik-/Quellcode-Vertragstests** und **37/37
  RTX-/Report-Event-Tests**.
- **41/41 native CTests** und erfolgreicher
  Linux-Vollviewer-Build.
- **21/21 echte isolierte SDL/X11-GUI-Starts**,
  darunter normale OpenGL-Anmeldung, 20 Vulkan-
  WSI-GPU-Presents mit 100 X11-Pixelproben,
  persistenten Vulkan-World-/Depth-Tests,
  Meshrotation, Regionswechsel und Vulkan-ICD-
  /Opt-in-Fallbacks. Drei neue Tests bestätigen
  explizit **0 falsche Deferred-Capture-/GPU-
  Quittungen vor dem Login**.
- **Native Vulkan-GPU-Regressionen grün**:
  sechs private World-/Depth-GPU-Frames,
  Vulkan-Cache 2 Builds/4 Reuses/2 Releases,
  erfolgreiche Regionsbarrieren und
  weiterhin **192/1024 sichtbare Testwelt-Pixel**.
- **Vier CLI-Reporter-End-to-End-Fälle**:
  synthetischer Login = keine Deferred-Evidenz;
  echter-Quelle-Fixture-Snapshot = GPU offen;
  numerisch gepaarter GPU-Fixture-Erfolg =
  Deferred-Source-GPU vorhanden; neuer
  Region-Marker = alter GPU-Beweis verworfen.
  Alle Fixtures wurden nach Test entfernt.

**Grenze und nächstes Gate:** Der echte Firestorm-
Weltpass kann jetzt quellqualifizierte Quittungen
ausgeben, **aber diese Runde hat keinen
authentifizierten Login in Stadt NexVerse
durchgeführt und keine tatsächliche
Live-Region-GPU-Quittung aus einem eingeloggten
Viewer protokolliert**. Die aktiven GPU-Pixeltests
nutzen weiterhin synthetische Entwicklergeometrie,
und sichtbare Welt, Terrain, Wasser, Materialien,
Texturen, Avatare und UI werden weiterhin
über OpenGL dargestellt. Erst die manuelle
Anmeldung in einer isolierten zulässigen
NexVerse-Testsession und danach eine
positiv korrelierte Deferred-Capture-
und GPU-Quittung im lokalen Report
würden das Live-Input-Gate schließen.
**P05 formal weiterhin 2/20; Block 3
und vollständiger Block 4 noch offen.**
Code-Commit: 68f7ac39d5e1378bf448430020a31992d08274f0.

## P05/3–4 – 20 technische Schritte: Kamera-only Vulkan-GPU-World-Frames ohne Meshupload (06.11.2026)

**Implementiert:** Der echte Firestorm-Deferred-Weltpass liefert regelmäßig
Kameraframes, während OpenSim-Regionsmesh-Snapshots nur periodisch
erneuert werden. Bisher lief die private Vulkan-Offscreen-World-GPU-
Diagnose nur direkt nach einem Mesh-Rebind. Nun wird der neue
Camera-only-Hook **NACH der echten OpenSim-Region-UUID-Prüfung**
aufgerufen. Er kann ein aktuelles Kamerabild mit **den bereits
hochgeladenen, gültigen GPU-Meshdaten** in den bestehenden
32×32-Vulkan-World-/Depth-Offscreen-Framebuffer zeichnen, ohne
GPU-Meshupload, neue Fence-Retirement-Waits oder Vulkan-Fenster-
Übernahme. OpenGL rendert unverändert das sichtbare Viewerfenster.

**Sicherheit und CPU/GPU-Last:** Separates Developer-Flag
OGL_VULKAN_LIVE_CAMERA_TICK=1 sowie bisherige Device-, Mesh-,
Camera-, Offscreen-, Raster- und Host-Freigaben erforderlich.
Die aktuelle Kamera-/Meshgeneration muss gültig sein.
Maximal **ein Tick je 120 geeignete Deferred-Frames**;
bei identischer Kamerapose erfolgt gar kein GPU-Draw.
Sonnenrichtung allein verändert den untexturierten
World-Depth-GPU-Test nicht und wird ignoriert. Regionwechsel
verwirft Pose, Generation und Tick-Zähler; eine abgewiesene
Kamera kann nicht weiterhin als gekoppelt/ready gelten.
Keine User-/Regions-/Positionsdaten oder GPU-Bilder im Log.
Gemeinsamer Host-Helper für alte Mesh-Rebind-Probe und
neuen Camera-only-Tick.

**20 tatsächlich implementierte oder überprüfte Teilaufgaben:**

01. Branch ogl-develop, sauberen Git-Ausgangsstand, AGENTS, Handoff, Roadmap und zuletzt vollständig erfolgreiche GitHub-CI kontrolliert.
02. Die Kameradatenquelle im echten Firestorm-Deferred-Pass und deren validierte Vulkan-Kamera-Mesh-ABI überprüft.
03. Den periodischen OpenSim-Regionsmesh-Scanner und die wesentlich häufigeren Kameraframes im selben Weltpass verglichen.
04. Fehlende Vulkan-GPU-Aktualisierung bei reiner Kamerabewegung ohne neuen Meshsnapshot als Ausgangsfehler lokalisiert.
05. Separate Host-Funktion ogl_rtx_after_region_camera_tick im bestehenden Viewer ergänzt.
06. Den Kamera-Tick NACH der realen Region-UUID-Prüfung und VOR dem periodischen Meshscan in LLPipeline eingebunden.
07. Ungültige/fehlende Region-ID vom Camera-Tick ausgeschlossen, um neue Kameras nicht mit alter Regionsgeometrie zu mischen.
08. Neues explizites Entwicklerflag OGL_VULKAN_LIVE_CAMERA_TICK mit den bisherigen GPU-, Mesh-, Kamera- und Offscreen-Berechtigungen kombiniert.
09. Nur vom Vulkan-Modul validierte Kameraframes und zur aktuellen GPU-Mesh-Generation passende Inputs akzeptiert.
10. Rate-Limit höchstens ein GPU-Kamera-Tick pro 120 geeignete Deferred-Frames in einer plattformneutralen constexpr-Policy eingebaut.
11. Die Frame-Grenzen 0, 1, 2, 120, 121, 240 und 241 durch sieben Compile-Time-C++-Asserts überprüft.
12. Semantische Kamera-Pose-Prüfung für Position, Achsen, Viewport, FOV, Aspect und Near/Far-Distanzen eingeführt.
13. Unveränderte Kameraposen übersprungen; reine Änderung der Sonnenrichtung löst keine World-Depth-GPU-Diagnose aus.
14. Bei echtem Regionswechsel den Kamera-Tick-Zähler, vorherige GPU-Probe-Pose und Generation/Epoche invalidiert.
15. Nach Ablehnung eines Vulkan-Kameraframes die vorherige Live-Inputs-Ready-Kopplung als ungültig markiert.
16. Mesh-Rebind und neue Camera-only-Probe auf einen gemeinsamen, anonym protokollierenden GPU-Offscreen-Helper umgestellt.
17. Den bestehenden Vulkan-Offscreen-GPU-Cache ohne neuen Mesh-Upload, GPU-Bufferwechsel, Queue-Idle oder SDL/OpenGL-Fensterzugriff weiterverwendet.
18. Native echte Vulkan-GPU: 2 zusätzliche Kamerabewegungen bei identischen VkBuffer, Meshgeneration, Upload- und Fence-Wait-Zählern geprüft; zuerst 0, dann wieder sichtbare Weltpixel.
19. Native GPU insgesamt 6 World-/Depth-Offscreen-Frames mit 2 Cache-Aufbauten/4 Reuses/2 Releases; Linux-Vollbuild 347 Python-/41 native Tests und 56 Grafik-Kontrakte bestanden.
20. 18 isolierte echte SDL/X11-Viewer-GUI-Fälle inkl. 3 neuer Kamera-Tick-Tests, OpenGL-Login, Vulkan-WSI-Frames, Meshrotation, Region-Clear und Treiber-Fallback bestanden.

**GPU-Abnahme:** Native echte Vulkan-GPU:
insgesamt **6 Offscreen-World-/Depth-GPU-Frames**,
darunter **2 Kamera-only-Frames mit unveränderter
Meshgeneration, unveränderten GPU-Meshuploads
und GPU-Retirement-Waits**. Weggedrehte Kamera:
**0 nichtleere Pixel**; zurückgedreht:
**192/1024 nichtleere Pixel**. Wiederverwendbarer
Vulkan-Cache: 2 Builds, 4 Reuses, 2 Releases.
Auch die bisherigen GPU-Tests für X11-Present,
Depth, Regionsbarriere, Mesh-Rotation und WSI-
Resize waren erfolgreich.

**Build und Viewer-Abnahme:** Linux-Vollviewer
**347/347 Python-Tests**, **41/41 native CTests**,
**56/56 gezielte Quellcode-/Grafikprüfungen**.
**18/18 isolierte SDL/X11-GUI-Starts** bestanden,
darunter die 3 neuen Tick-Opt-in-/Fallback-Fälle,
normaler OpenGL-Login/Exit 0, 20 X11-GPU-
Presents mit 100 RGB-Pixelproben sowie die
bestehenden Regionen-, Mesh- und Vulkan-ICD-
Rückfalltests. Kein Nutzer-Viewer, Cache oder
bestehendes Loginprofil wurde berührt.

**Wichtig:** Der Host ist an den echten
Firestorm-/OpenSim-Scene-Capture-Code gebunden.
Es wurde aber **kein authentifizierter OpenSim-
Login mit echten Vulkan-GPU-Regionbildern**
abgenommen. Die konkret nachgemessenen GPU-
Pixel stammen aus synthetischen Entwickler-
Meshdaten; die Vulkan-Bilder sind privat und
NICHT im Viewer sichtbar. Produktive Vulkan-
Fensterhoheit und Renderer für Terrain,
Texturen, Materialien, Wasser, Avatare und
UI stehen aus. **P05 formal weiter 2/20;
Block 3 und der vollständige Block 4 offen.**
Nächstes Gate: authentifizierte Live-Region
mit echtem Scene-Capture und nachweisbarer
Kamerabewegung in der Offscreen-GPU-Probe,
danach exklusive Vulkan-Welt-Fensterübernahme.
Code-Commit: d527892b5abed3396fa44297b71095438fd75048.

## P05/3–4 – 20 technische Schritte: wiederverwendbarer Vulkan-World-Offscreen-GPU-Cache (06.11.2026)

**Implementiert:** Der private 32×32-Vulkan-World-/Depth-Renderer
hat seine GPU-Zielimages, GPU-Memory, Depth-Attachments,
SPIR-V-Shader, VkGraphicsPipeline, Framebuffer, VkCommandPool,
VkCommandBuffer, Readback-Buffer und VkFence vorher **bei jedem
Diagnoseframe neu angelegt**. Der neue Cache ist jetzt an die
Lebensdauer genau eines VkDevice gebunden und wird auch nach
Kameraänderungen und GPU-sicheren OpenSim-Regionswechseln
wiederverwendet.

**Frame-Sicherheit:** VkCommandBuffer und VkFence werden erst
nach einer erfolgreich abgeschlossenen GPU-Submission
zurückgesetzt. Der Draw benutzt trotzdem bei JEDEM Frame die
aktuell validierte 64-Byte-World-to-Clip-Matrix und die jeweils
gültigen GPU-Vertex-/Indexbuffer. Bei Timeout bleiben noch
referenzierte Vulkan-Objekte quarantänisiert; die Freigabe
benutzt einen maximal 2s begrenzten Fence-Wait, **keinen
VkQueueWaitIdle im Frame-Loop**. Bei Modulende wird der
private Cache vor Regionsmesh und VkDevice freigegeben.
Der zuvor existierende Offscreen-CLEAR-Test bleibt weiterhin
temporär, ohne den neuen Weltcache aufzubauen.

**Diagnosen:** Additive read-only-C-ABI meldet, wie häufig
GPU-Cache-Ressourcen aufgebaut, weitergenutzt und freigegeben
wurden. Ein eigenes Opt-in erlaubt das vollständige,
sichere Cache-Release mit anschließendem Neuaufbau.
Die reale Firestorm-HOST-Anbindung protokolliert ausschließlich
anonyme Frame-/Pixel-/Cache-Zähler, keine Bildinhalte,
Regionsbezeichnungen, Benutzer- oder Positionsdaten.
OpenGL bleibt alleiniger Eigentümer des sichtbaren
Viewerfensters.

**20 tatsächlich durchgeführte technische Teilaufgaben:**

01. Ausgangsbranch ogl-develop, sauberen Git-Stand, AGENTS, Handoff, P05-Roadmap und plattformübergreifende Vor-CI kontrolliert.
02. Den funktionierenden 32x32-Vulkan-World-/Depth-Offscreen-Pfad auf pro Diagnose wiederholte GPU-Allokationen untersucht.
03. Eine gerätegebundene OglOffscreenWorldCache-Struktur in der vorhandenen Vulkan-Implementierung eingeführt, ohne neue Projektdateien.
04. VkImage-Farbfläche und die zugehörigen GPU-Memory-/ImageView-Objekte über mehrere Offscreen-Weltframes wiederverwendet.
05. Privates Vulkan-Depth-VkImage/Memory/View pro Cachelebenszyklus statt pro GPU-Diagnose verwendet.
06. World-SPIR-V-Vertex-/Depth-Fragmentshader und VkGraphicsPipeline in den gerätegebundenen Cache aufgenommen.
07. VkRenderPass, PipelineLayout und Vulkan-Framebuffer bei Folgeframes statt erneuter Allokation weiterverwendet.
08. GPU-Readback-VkBuffer und host-kohärenten Speicher des 32x32-RGBA-Bilds wiederverwendet.
09. Cache-eigenen VkCommandPool mit VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT und genau einem VkCommandBuffer aufgebaut.
10. Vor einem Folgeframe den nach GPU-Fence-Abschluss nutzbaren Command-Buffer mit vkResetCommandBuffer neu aufgezeichnet.
11. Die aktuelle 64-Byte-Kamera-Projektionsmatrix und die aktuellen GPU-Vertex-/Indexbuffer bei JEDEM neuen Weltframe erneut gebunden.
12. Den vorigen GPU-Fence nach erfolgreichem Abschluss mit vkResetFences für die nächste VkQueueSubmit-Verwendung vorbereitet.
13. Mit einem submitted-Flag die Freigabe bei ausstehender GPU-Submission gesperrt; maximal 2s begrenzter Fence-Wait statt QueueIdle.
14. Automatische Cache-Freigabe vor dem GPU-Mesh-Clear und VkDevice-Destroy im vorhandenen Modul-Shutdown abgesichert.
15. Den bisherigen einzelnen Offscreen-Clear-Probe-Pfad bewusst temporär gehalten und vom Weltcache getrennt.
16. Read-only-C-ABI für GPU-Cache-Aufbauten, Wiederverwendungen und Freigaben sowie explizit abgesicherten Cache-Release ergänzt.
17. Den echten Firestorm-Host mit anonymisierten Cache-Zählern verbunden; bestehende gedrosselte Offscreen-Diagnose bleibt unabhängig vom OpenGL-Fenster.
18. Native echte Vulkan-GPU-Abnahme: vier indexed World-/Depth-Frames, Kamera außerhalb des Blickfelds, Regionswechsel, 2 GPU-Builds/2 Reuses/2 Releases und Geräte-Reset bestanden.
19. Linux-Vollviewer-Build, 346/346 Python-Tests, 41/41 native CTests und 55/55 gezielte Grafikkontrakte erfolgreich.
20. 15/15 isolierte echte SDL/X11-Viewer-GUI-Fälle bestanden, inklusive Standard-OpenGL, Vulkan-Welt/Regionswechsel, Offscreen-Flags, fehlender Treiber und Berechtigungs-Fallbacks.

**Native echte Vulkan-GPU-Abnahme:** Vier GPU-World-/Depth-
Offscreen-Frames wurden mit tatsächlichem VkDrawIndexed,
GPU-Kameraprojektion und RGBA-Readback geprüft. Zuerst
sichtbare GPU-Weltpixel, dann bei abgewendeter Kamera
0 sichtbare Pixel, danach neue Regionsgeometrie und
erneut **192 von 1024 nicht-Hintergrund-Pixeln**.
Die GPU meldete **2 Cache-Aufbauten, 2 Wiederverwendungen
und 2 Freigaben**. Ein expliziter Release erlaubte
eine vollständige neue GPU-Ressourcenallokation; beim
Modul-Stop wurden die Zähler auf 0 zurückgesetzt.
Die vorherigen Vulkan-X11-Present-, Depth-, Kamera-,
Mesh-Rotation-, Regionsbarrieren- und WSI-Resize-
Regressionen blieben grün.

**Linux-Vollviewer:** Erfolgreicher Build,
**346/346 Python-Tests, 41/41 native CTests**,
darunter **55/55 gezielte Grafik-/Quellcode-
Vertragstests**. 15/15 isolierte echte
SDL/X11-Viewer-GUI-Starts erfolgreich:
Standard-OpenGL, Live-Offscreen-Opt-ins,
persistente Vulkan-Welt-/Depth-Tests
(20 GPU-Frames, 100 X11-Pixelproben),
Mesh-Rotation, Regionsbarriere und
fehlerfreie OpenGL-Fallbacks bei
fehlendem Vulkan-Treiber oder
fehlenden Freigaben. Alle Tests
endeten nach Login-Wait mit Exit 0.
Die eigentliche Benutzer-Session,
Cache und Accounts wurden nicht
gestartet, geändert oder beendet.

**Grenzen:** Die GPU-Ressourcen werden für
DEN PRIVATEN diagnostischen 32×32-Offscreen-
Renderpfad wiederverwendet, **nicht**
für einen produktiven, im sichtbaren
Viewerfenster laufenden Vulkan-Renderer.
Die dokumentierten GPU-Pixeltests
verwenden synthetische Testmeshes. Der
echte Firestorm-Capture-Hook ist im
Viewer-Quellcode angebunden, aber
es ist weiterhin **kein tatsächlicher
authentifizierter OpenSim-Region-GPU-
Vulkan-Frame unter realem Login geprüft**.
Die Vulkan-Engine für Terrain, Materialien,
Texturen, Avatare, Wasser, UI und sichere
exklusive Fensterhoheit fehlt.
**P05 formal unverändert 2/20;
Block 3 und der vollständige Block 4 offen.**
Nächstes Gate: echte eingeloggte Region-
Eingangsdaten nachweisbar mit Offscreen-
World-GPU-Diagnose koppeln und erst dann
In-World-Vulkan-Fenster-Ownership aufbauen.
Code-Commit: cb364549ca3764cc04e580db785da269e589b10e.

## P05/3–4 – 20 technische Schritte: echte OpenSim-GPU-World-Inputs ohne Vulkan-Fensterübernahme (06.11.2026)

**Implementiert:** Ein zusätzlicher Vulkan-Welt-Renderpfad zeichnet die
vorhandenen, aus dem echten Firestorm-Deferred-Scanner übernommenen
GPU-Vertex-/Indexdaten mit der bereits validierten OpenSim-Kameramatrix
und einem GPU-Depth-Test in eine **private 32×32-VkImage-Fläche**.
Er benötigt KEINE Vulkan-X11-Surface, keine VkSwapchainKHR, keine
SDL-Fensterübernahme und keinen VkQueuePresentKHR. Das eigentliche
Viewerfenster und die sichtbare 3D-Welt bleiben ausschließlich bei
OpenGL. Der Pfad nutzt die vorhandenen shaderbasierten
World-XYZ-Grafikressourcen sowie die frühere sichere GPU-Mesh-
Generationsverwaltung. Bildinformationen werden NICHT nach außen
exportiert oder gespeichert.

**Sicherheit und Aufwand:** Nur mit allen ausdrücklichen Freigaben
OGL_VULKAN_OFFSCREEN_FRAME_PROBE=1,
OGL_VULKAN_WORLD_OFFSCREEN_RASTER=1,
OGL_VULKAN_WORLD_MESH_UPLOAD=1 und
OGL_VULKAN_WORLD_CAMERA_PROBE=1 kann das Vulkan-Modul einen
Offscreen-Weltframe rendern. Der echte Firestorm-Host benötigt
zusätzlich OGL_VULKAN_LIVE_WORLD_OFFSCREEN=1 und die
zuvor freigegebene Kamera-/Mesh-Neubindung. Die Diagnose ist
auf die ersten drei erfolgreichen neuen Szene-Generationen
und danach maximal jeden 64. erfolgreichen Szenenwechsel
begrenzt; eine GPU-Fence-Warteoperation ist auf maximal 2 s
beschränkt. Die vorhandenen OpenGL-Anwender werden damit
nicht zusätzlich belastet. Read-only-C-ABI meldet abgeschlossene
Frames und die Anzahl sichtbarer, vom Hintergrund abweichender
Pixel (0..1024). Null sichtbare Pixel sind ein gültiger
GPU-Renderabschluss, NICHT der Nachweis sichtbarer Weltgeometrie.

**20 tatsächlich implementierte oder geprüfte Teilaufgaben:**

01. Branch, AGENTS, Projekt-Handoff, P05-Roadmap und die vorige vollständige GitHub-CI geprüft; ursprünglicher Checkout sauber.
02. Bestehenden 32×32 Vulkan-Offscreen-Clear-Pfad und die vorherige echte World-XYZ-/Depth-Swapchain-Pipeline analysiert.
03. Separate Entwicklerfreigabe OGL_VULKAN_WORLD_OFFSCREEN_RASTER eingeführt; vorhandene Offscreen-, Kamera- und Meshflags bleiben zusätzliche Pflicht.
04. Neuen Vulkan-Weltrenderpfad an bereits akzeptierte GPU-Vertex-/Indexbuffer mit passender Kamera-/Mesh-Generation gebunden.
05. Privates 32×32-VkImage in VK_FORMAT_R8G8B8A8_UNORM verwendet; kein VkSurfaceKHR, keine VkSwapchainKHR und keine SDL-Fensterinteraktion.
06. GPU-Farbimage mit VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT und VK_IMAGE_USAGE_TRANSFER_SRC_BIT sowie eigenem Device-Speicher erzeugt.
07. Unterstütztes GPU-Depth-Format D32_SFLOAT beziehungsweise D16_UNORM gesucht und private Depth-Image-/Memory-/ImageView-Ressourcen angelegt.
08. Den Offscreen-VkRenderPass und VkFramebuffer mit separatem Farb- und Depth-Attachment erweitert.
09. Die bereits validierten Vulkan-World-Vertex- und Depth-Fragment-SPIR-V-Shader ohne neue Shaderdateien wiederverwendet.
10. GPU-VkGraphicsPipeline mit World-Kamera-PushConstant, indexed Triangle-List und VK_COMPARE_OP_LESS erzeugt.
11. 64-Byte-World-to-Clip-Matrix aus dem vorhandenen Firestorm-Kamera-ABI in den Vertexshader übergeben.
12. Reale GPU-VkVertex- und VkIndexBuffer aus dem OpenSim-Mesh-Staging per vkCmdBindVertexBuffers, vkCmdBindIndexBuffer und vkCmdDrawIndexed gerendert.
13. Offscreen-Farbbild nach dem Renderpass per Image-Barrier und vkCmdCopyImageToBuffer in einen privaten Readback-Buffer übertragen.
14. GPU-Submit-Fence mit höchstens zwei Sekunden Wartezeit abgesichert; bei Timeout keine möglicherweise noch referenzierten GPU-Objekte vorzeitig freigegeben.
15. Read-only-C-ABI für abgeschlossene indexed World-Offscreen-Frames und die Zahl tatsächlich nicht-hintergrundfarbiger Pixel implementiert.
16. Ein Bild ohne sichtbare Geometrie wird als abgeschlossener GPU-Render gemeldet, aber korrekt mit null sichtbaren Weltpixeln ausgewiesen.
17. Echten Firestorm-Host mit der neuen Offscreen-ABI verbunden und als Entwicklerlauf nur nach erfolgreicher Camera-/Mesh-Generationskopplung ausgeführt.
18. Live-Entwicklertrigger OGL_VULKAN_LIVE_WORLD_OFFSCREEN mit höchstens drei frühen und danach einer Probe pro 64 neue Meshgenerationen begrenzt; Logs ohne Spieler-/Regions-/Bilddaten.
19. Native Vulkan-GPU: drei private World-/Depth-Offscreen-Frames ohne vorherige Swapchain, sichtbare Pixel, Kamera außerhalb des Blickfelds mit null sichtbaren Pixeln, neue Region und Negativfreigaben erfolgreich.
20. Linux-Vollviewer-Build, 345/345 Python-Tests einschließlich 54/54 gezielter Quellcodeprüfungen, 41/41 native CTests und 15/15 isolierte Viewer-GUI-Tests einschließlich normalem OpenGL-Login/Exit 0 bestanden.

**Native echte Vulkan-Abnahme:** Drei GPU-World-/Depth-Draws
im privaten 32×32-Image, ohne dass vorher eine
X11-Vulkan-Swapchain angelegt werden musste.
Die Kamera-Probe meldete nach sichtbarer vorderer
Region Weltpixel, nach vollständiger Abwendung der
Kamera **0** sichtbare Weltpixel und nach neuer
Region erneut sichtbare GPU-Mesh-Pixel.
Der zuletzt gemessene Wert: **192 von 1024 Pixeln**,
die nicht der Hintergrundfarbe entsprechen.
Weder fehlende Freigaben noch Regions-Clear dürfen
einen Framezähler erhöhen. Bestehende echte
Vulkan-Tests für X11-Present, Depth-Z-Test,
Kamera-Move, Fence-Retirement, GPU-Meshrotation,
Resize und zwei sichere Regionsbarrieren erneut
bestanden. Offscreen-Ergebnisse sind keine
sichtbaren Vulkan-Viewerbilder.

**Echter SDL/X11-Viewer in isolierter Xvfb-Sitzung:**
15/15 GUI-Fälle bestanden, darunter Standard-
OpenGL-Start, Offscreen-Entwicklerflags mit
fehlender Einzelfreigabe, Vulkan-GPU-Weltstart
unter gleichzeitigen Offscreen-Flags, 20
Vulkan-Welt-/Depth-Presents mit 100 echten
X11-Pixelproben und alle vorherigen
Regions-, Mesh-, Treiber- und Berechtigungs-
Fallbacks. Jeder Fall kam bis zum normalen
OpenGL-Login-Wait und Exit 0. Kein normales
Benutzerprofil/Cache/Account wurde angerührt.

**Build:** Linux-Vollviewer erfolgreich;
345/345 Python-Tests (darunter 54/54 gezielte
Grafik-/Quellcode-Tests) und 41/41 native
CTests bestanden. Vorige plattformübergreifende
GitHub-CI vor den neuen Commits vollständig
grün; für den neuen Branch-HEAD separat prüfen.

**Grenzen:** Die HOST-VERBINDUNG zum tatsächlichen
Firestorm-/OpenSim-Capture-ABI ist gebaut, aber es
wurde **kein authentifizierter Regionslogin und
kein Live-OpenSim-GPU-Bild aus einer angemeldeten
Nutzersitzung getestet**. Die konkreten
GPU-Pixeltests verwenden absichtlich synthetische
Weltgeometrie. Der neue Offscreen-Pfad rendert
keine auf dem Viewerfenster sichtbare Welt.
Terrain, Texturen, Materialien, Wasser, Avatare,
UI und produktive Vulkan-In-World-
Fensterübernahme stehen weiter aus.
**P05 formal 2/20, Block 3 und die vollständige
Weltengine noch offen.** Nächstes Gate:
einen tatsächlichen eingeloggten Region-Mesh-/
Kamera-GPU-Probe-Log aus isoliertem Live-
Viewer nachweisen, dann sichere Vulkan-
Fensterhoheit und Präsentation nach Login.
Code-Commit: 525c09457ab7b369d761f7a6a54449c303ec2d41.

## P05/3–4 – 20 technische Schritte: fence-sichere echte OpenSim-Regionsbarriere (06.11.2026)

**Implementiert:** Beim tatsächlichen OpenSim-Region-ID-Wechsel
war die vorherige generische Vulkan-Regionslöschung unter
ausstehenden GPU-Frames ohne allgemeine Mesh-Rotationsfreigabe
nicht zuverlässig möglich. Nun gibt es eine eigene
**OGL_VULKAN_LIVE_REGION_BARRIER=1** Developer-Operation:
Sie invalidiert die alte Vulkan-Kamera SOFORT, wartet danach
höchstens zwei Sekunden auf die Submit-Fences der bisherigen
Mesh-Generation in beiden Frame-Slots und gibt die alten
VkVertex-/Indexbuffer nur bei erfolgreich abgeschlossener
GPU-Arbeit frei. Bei Fehler/Timeout wird der persistente
Vulkan-Pfad blockiert und der alte GPU-Speicher nicht vorzeitig
freigegeben. Es gibt keinen vkQueueWaitIdle pro Frame oder
Regionswechsel.

Der vorhandene Firestorm-Regionwechsel-Callback meldet den
erfolgreichen GPU-Barriereschritt als boolesches Ergebnis.
Nur dann entfällt der zweite, generische GPU-Clear. OpenGL
bleibt unabhängig vom Vulkan-Ergebnis der laufende
Welt-Renderer. Das erste Mesh einer neuen Region kann
nach erfolgreich freigegebenen alten Buffern übernommen
werden, **ohne** dadurch die allgemeine Erlaubnis zu
beliebigen Meshwechseln während GPU-Frames zu erteilen.
Drei read-only C-ABI-Funktionen machen Barrier-Erfolg und
Fehler zählbar.

**20 tatsächlich erledigte und geprüfte Teilaufgaben:**

01. Git-Branch ogl-develop, sauberer Ausgangsstand, AGENTS, Handoff, P05-Roadmap und GitHub-CI geprüft; vorherige CI vollständig erfolgreich.
02. Den tatsächlichen OpenSim-Region-ID-Wechsel im bestehenden Firestorm-Deferred-Pfad und den bisherigen Vulkan-Region-Clear verfolgt.
03. Sicherheitslücke reproduzierbar eingegrenzt: Ein GPU-Regions-Clear konnte wegen noch ausstehender Vulkan-Frame-Submissions ohne Mesh-Rotationsfreigabe abgewiesen werden.
04. Eine eigenständige C-ABI für echten GPU-Regionswechsel mit OGL_VULKAN_LIVE_REGION_BARRIER als separater Entwicklerfreigabe ergänzt.
05. Zusätzlich OGL_VULKAN_WORLD_MESH_UPLOAD=1 und ein existierendes Vulkan-Device für jede GPU-Regionsbarriere vorgeschrieben.
06. Vor jedem GPU-Fence-Wait die alte Vulkan-Kamera, deren Mesh-Generation und die Projektionsmatrix invalidiert.
07. Nur die noch ausstehenden Submit-Fences der alten Mesh-Generation aus beiden Frame-Slots identifiziert.
08. Diese Fences mit maximal zwei Sekunden waitAll abgewartet, ohne vkQueueWaitIdle pro Frame oder GPU-Regionswechsel.
09. Bei fehlgeschlagenem Fence-Wait den persistenten Vulkan-Pfad blockiert und noch referenzierte GPU-Buffer im Speicher belassen.
10. Nach erfolgreichem Fence-Retirement Vertex-/Indexbuffer und CPU-Snapshot der vorherigen Region freigegeben und Mesh-Generation erhöht.
11. Additive read-only-C-ABI für akzeptierte Regionsbarrieren und GPU-Retirement-Fehler implementiert.
12. Die GPU-Mesh-Upload-Regel angepasst: Erstes Mesh nach erfolgreichem Regions-Clear benötigt keine allgemeine Live-Mesh-Rotationsfreigabe.
13. Das bestehende strikte Upload-Verbot bei laufenden alten GPU-Mesh-Referenzen und gesperrtem GPU-Pfad beibehalten.
14. Das Vulkan-Regionsbarrier-Symbol im Firestorm-Host geladen und beim Shutdown zurückgesetzt.
15. Den echten Firestorm-Regionwechsel-Callback um booleschen GPU-Barriere-Erfolg ergänzt und die alte zwischengespeicherte Kamera dort weiterhin invalidiert.
16. Bei erfolgreich durchgeführter GPU-Regionsbarriere den doppelten generischen Regions-Clear im Firestorm-Pipeline-Code übersprungen; OpenGL bleibt immer aktiv.
17. Native Vulkan-X11-Abnahme: zwei ausstehende Frame-Slot-Submissions, Regionsbarriere ohne allgemeine Rotation, Kamerasperre und neues Regionsmesh mit grünem GPU-Pixel bestanden.
18. Zweite native Regionsbarriere, zwei akzeptierte Barrieren, null Fehler, kein zusätzlicher Frame-QueueIdle und vollständige GPU-Welt-/Depth-Regression bestanden.
19. Reale SDL/X11-Vorlogin-Regionstestsequenz unter zwei separaten Dev-Freigaben implementiert: 20 World-Frames, 100 RGB-Samples und drei Fenstergrößen; fehlende Freigaben oder ICD fallen sauber auf OpenGL zurück.
20. Linux-Vollviewer-Build, 344/344 Python-Tests, 41/41 native CTests, 53 gezielte Quellcode-Tests und 12/12 isolierte GUI-Regressionen bestanden; keine Nutzersitzung oder Nutzerdateien verändert.

**Native echte Vulkan-X11-GPU-Abnahme:** Vor einem echten
Regionswechsel werden beide persistenten GPU-Frame-Slots
benutzt. Ohne separates Regions-Opt-in bleiben alte
Meshdaten unangetastet; mit aktivierter Barriere werden
beide alten Frames über Submit-Fences sicher abgeschlossen.
Die alte Kamera wird ungültig und ohne Mesh ist jeder
Vulkan-World-Present gesperrt. Das erste Mesh der
neuen Region (ohne allgemeines Mesh-Rotationsflag)
wird erfolgreich geladen und der Depth-Renderer zeigt
ein überprüftes grünes X11-GPU-Pixel. Nach einer
zweiten Barriere: **2 akzeptiert, 0 Fehler**,
GPU-Retirement-Waits seit dem gesamten Testbeginn
von 3 auf 5 gestiegen, QueueIdle-Zähler im laufenden
Frame-Pfad unverändert. Alle vorherigen GPU-Raster-,
Depth-, Kamera-, Resize- und Meshrotations-Gates
erneut bestanden.

**Tatsächlicher SDL/X11-Viewer VOR GL-Kontext:** Eine
neue, nur über ZWEI Entwicklerfreigaben erreichbare
Regionswechsel-Entwicklerprobe führt zwei Region-Clears
und erneute Mesh-/Kamera-Übergaben im selben
Vulkan-Swapchain-Lebenszyklus aus. 20/20 echte
Vulkan-Presents, 100 validierte X11-RGB-Pixel,
drei Fenstergrößen und anschließend normaler
OpenGL-Login/Exit 0. Fehlende Freigaben,
unzulässige Kombination mit Meshrotation und
ungültiger Vulkan-Treiber führen sicher auf
OpenGL zurück. **5 neue und 7 bestehende
GUI-Fälle: 12/12 bestanden**, alle mit
GL-Login-Wait und Prozess-Exit 0.
Die Tests liefen in separaten Xvfb-Sitzungen mit
isolierten Profilen; kein User-Viewer,
Login, Cache oder Secret wurde angerührt.

**Linux-Vollviewer-Build 344/344 Python-Tests,
41/41 native CTests**, einschließlich
**53/53 gezielter Grafik-/Quellcode-Tests**
erfolgreich. Vorherige plattformübergreifende
GitHub-CI war grün; die neue Commit-CI
muss gesondert bestätigt werden.

**Noch offen:** Kein tatsächlicher Login in eine
OpenSim-Region wurde automatisiert oder mit
Vulkan als Weltbackend abgenommen. Der
GPU-Regionsbarriere-Test verwendet SYNTHETISCHE
Weltmeshes vor dem GL-Kontext. Dauerhafte
Vulkan-Fensterhoheit nach Login, Terrain,
Wasser, Texturen/Materialien, Mesh-Streaming,
Avatare und UI unter Vulkan fehlen weiterhin.
**P05 formal 2/20; Block 3 und der vollständige
Block 4 bleiben offen.** Nächstes technisches
Gate: nachgewiesene authentifizierte
Firestorm-Regionsdaten im Live-Vulkan-Input
und danach sichere exklusive Vulkan-In-World-
Fensterübernahme mit OpenGL-Fallback.
Code-Commit: 4520ce33864bc23feeab066d4c522150633835d7.

## P05/3–4 – 20 technische Schritte: echter Firestorm-Kamera-/Regionsmesh-Handoff (06.11.2026)

Der echte Firestorm-Deferred-Pass erfasst die Kamera vor dem späteren
OpenSim-Regionsmesh-Snapshot. Der nachgelagerte Vulkan-Meshupload
invalidierte bisher die bereits eingereichte Kamera. Der Host
speichert deshalb nur erfolgreich validierte Kameraframes und kann
unter eigenem Entwicklerflag OGL_VULKAN_LIVE_CAMERA_REBIND=1 die
Kamera nach akzeptiertem Mesh erneut zur aktuellen GPU-Generation
binden. Ein echter Region-ID-Wechsel verwirft den Cache vor
Region-Clear; leere Scanbatches sind keine Regionwechsel.

World-Present und das GPU-Inputs-Ready-Gate akzeptieren ausschließlich
zusammenpassende Kamera-/Mesh-Generationen. Eine neue read-only-C-ABI
macht die Kamerageneration prüfbar.

Mit OGL_VULKAN_WORLD_MESH_DEDUP=1 können identische Snapshots
bytegenau verglichen werden, ohne neue VkBuffer, GPU-Fence-Waits
oder Kamera-Invalidierung. Ein CPU-Snapshot wird vor der GPU-
Veröffentlichung vollständig vorbereitet und ohne erneute
Allokation übernommen. Der normale OpenGL-Modus wird nicht verändert.

**20 durchgeführte und geprüfte technische Teilaufgaben:**

01. Git, Branch ogl-develop, Arbeitsverzeichnis, AGENTS, Handoff und P05-Roadmap vor allen Änderungen geprüft.
02. Vorherige GitHub-CI für Linux, Windows und macOS als vollständig grün bestätigt.
03. Den echten Firestorm-Deferred-Kamera-Hook mit Kameraachsen, FOV und Clipping analysiert.
04. Den tatsächlichen OpenSim-Region-Meshscanner und den nachgelagerten GPU-Upload untersucht.
05. Den Reihenfolgefehler ermittelt: Kamera kommt vor Mesh, Mesh-Upload invalidiert die Kamera.
06. Nur erfolgreich im Vulkan-Modul validierte Kameraframes im vorhandenen Firestorm-Host zwischengespeichert.
07. Den realen Region-ID-Wechsel im Firestorm-Pipeline-Code als expliziten Cache-Invalidierungs-Hook angebunden.
08. Vor Region-Clear den Kameracache und Live-Input-Status verworfen; leere Scanbatches nicht als Regionswechsel behandelt.
09. Eigenes Entwicklerflag OGL_VULKAN_LIVE_CAMERA_REBIND zum gezielten Kamera-Rebind nach akzeptiertem Regionsmesh eingeführt.
10. Kamera-Neubindung auf vorhandenes Vulkan-Device, gültigen Kameracache und erfolgreiche Mesh-Übergabe begrenzt.
11. Bei jeder gültigen GPU-Kameraprojektion die aktuell akzeptierte Mesh-Generation mitgespeichert.
12. Vulkan-World-Present und Live-Inputs-Ready lehnen unpassende Kamera-/Mesh-Generationen ausdrücklich ab.
13. Kamerageneration bei ungültigem Kamera-Frame, Meshwechsel, Region-Clear und unpassendem Swapchain-Aspect zurückgesetzt.
14. Read-only Vulkan-C-ABI für die Kamera-Mesh-Generation ergänzt und durch Firestorm-Host sowie Native-Test geprüft.
15. Separates OGL_VULKAN_WORLD_MESH_DEDUP Entwicklerflag implementiert: voller bitgenauer CPU-Meshvergleich ohne Hash.
16. Identische GPU-Meshes behalten Buffer, Kamera, Generation und Fences ohne unnötige Warteoperation.
17. CPU-Meshsnapshot vollständig vor GPU-Publish alloziert und erst nach erfolgreicher GPU-Übergabe ohne neue Allokation getauscht.
18. Native echte Vulkan-GPU: Kameragenerationsgates, dedup=1, Region-Clear, GPU-Meshrotation, Depth und X11-Pixel bestanden.
19. Vollständiger Linux-Viewer-Build, 343/343 Python-Tests, 41/41 native CTests und 52 gezielte Vertragsprüfungen bestanden.
20. Sieben isolierte reale Viewer-GUI-Fälle mit OpenGL-Login/Exit 0 bestanden; nach abschließendem Codefix beide 20-Frame-/100-Pixel-GPU-Hauptfälle erneut grün.

**Native GPU-Abnahme:** Kamerageneration nach Meshwechsel/Region-Clear
ungültig, nach neuer akzeptierter Kamera wieder passend.
Byte-identische Szenen werden ohne GPU-Retirement-Wait übersprungen.
Die GPU-Tests für persistente Vulkan-World-XYZ-Frames, Kamera,
Depth, Resize und Rot/Grün/Leer/Rot/Grün-Meshwechsel bestanden;
dedup=1, rotation=1 und no-frame-idle=1 durch echte GPU bestätigt.

**Tatsächlicher SDL/X11-Viewer:** 7/7 isolierte Testfälle bestanden:
normaler OpenGL-Start, neue Live-Capture-Flags ohne Vulkan-WSI,
persistente Vulkan-World-Frames mit und ohne Capture-Flags,
Meshrotation mit Capture-Flags, fehlender Vulkan-Treiber und
unvollständige Developer-Freigaben. Die GPU-Erfolgsfälle
präsentierten 20/20 synthetische Weltframes bei drei Auflösungen
mit 100 echten X11-RGB-Samples; danach OpenGL-Login und Exit 0.
Nach dem finalen CPU-Snapshot-Sicherheitsfix wurden beide
20-Frame-/100-Pixel-Hauptfälle erneut erfolgreich gestartet.
Keine laufende Usersitzung und kein persönlicher Cache berührt.

**Abnahme:** Linux-Vollviewer-Build, 343/343 Python-Tests,
41/41 native CTests und 52 gezielte Vertragstests erfolgreich.
Vorige plattformübergreifende GitHub-CI vollständig grün;
neue Commit-CI noch zu prüfen.

**Grenze:** Der echte OpenSim-Capture-Handoff ist jetzt im
Viewer-Code verbunden, aber KEINE authentifizierte Live-Region
wurde mit dem Assistenten eingeloggt und unter Vulkan gerendert.
Die sichtbaren Vulkan-GUI-GPU-Bilder verwenden weiter
synthetische Entwickler-Meshes VOR dem OpenGL-Kontext.
Terrain, Materialien, Texturen, Avatare, UI, ein produktiver
Vulkan-In-World-Switch und dauerhafte Vulkan-Fenster-Ownership
fehlen. **P05 formal weiter 2/20; Block 3 und der vollständige
Block 4 bleiben offen.** Nächstes Gate: echte Live-Region-
Eingangsdaten nach Login prüfen und sicheren Vulkan-Fenster-
Handoff mit OpenGL-Fallback implementieren.
Code-Commit: bf85bff580a529354fd203624324792944f5c337.

## P05/3–4 – 20 technische Aufgaben: GPU-Fence-gesicherte Mesh-Generationen und Regionsverlust (05.11.2026)

**Implementiert:** Die bisher beim ersten persistenten Vulkan-Present
gesperrte Mesh-Aktualisierung kann unter separatem Entwickler-Opt-in
nun während derselben laufenden X11-Swapchain erfolgen. Vor dem
Freigeben alter VkVertex/VkIndexBuffer wartet der Renderer **gezielt
auf die noch ausstehenden Submit-Fences der bisherigen GPU-Mesh-
Generation** (waitAll, maximal 2 Sekunden). Ein globales
vkQueueWaitIdle wird nicht im Frame-Loop oder beim Meshwechsel
verwendet. Ein Timeout sperrt die weitere Entwicklerdiagnose und
lässt die alten GPU-Buffers unangetastet.

Jeder akzeptierte Meshsnapshot und jedes Regions-Clear erhält
eine neue Scene-Generation; jedes von zwei GPU-Frame-Slots merkt
sich seine referenzierte Meshgeneration. Ungültige Geometrie
wird bereits VOR jeder Fence-Warteoperation abgewiesen. Nach
einem gültigen Meshwechsel ist der vorherige Kameraframe ungültig,
bis ein zur aktuellen Swapchain passendes neues Kameraupdate vorliegt.
Ein explizit geleertes Regionsmesh lässt keinen weiteren
World-Present zu, bis ein neuer Meshsnapshot vorliegt.

**20 konkret durchgeführte technische Aufgaben und Abnahmen:**

01. Sauberen Branch, kanonischen Projektstatus, Handoff, AGENTS und den P05-Plan erneut geprüft; vorige GitHub-CI vollständig grün.
02. Den bestehenden Vulkan-Mesh-Upload auf vorzeitiges Freigeben noch von GPU-Submissions referenzierter Buffer untersucht.
03. Separates Entwickler-Opt-in OGL_VULKAN_PERSISTENT_MESH_ROTATION_PROBE für Meshwechsel im persistenten Welt-Framepfad eingeführt.
04. Bei fehlendem Entwicklerflag oder inkompatiblem/nicht freigegebenem persistenten World-Framepfad das bisherige Fail-Closed-Verhalten erhalten.
05. Monotone Scene-Generation für akzeptierte GPU-Mesh-Uploads und explizites Regions-Clear eingeführt.
06. Die zugehörige Mesh-Generation bei jedem erfolgreichen VkQueueSubmit in beiden GPU-Frame-Slots erfasst.
07. Ungültige Welt-XYZ-Geometrie vor jeglichem GPU-Fence-Wait abgewiesen, ohne alte Meshdaten oder den Generationszähler zu ändern.
08. Nur tatsächlich ausstehende VkFence-Objekte der noch referenzierten bisherigen Mesh-Generation ermittelt.
09. Diese Fences per VkWaitForFences waitAll mit maximal zwei Sekunden Wait freigegeben, ohne QueueIdle und ohne Fenster-Neuaufbau.
10. Bei GPU-Wait-Fehlern den persistenten Entwicklerpfad gesperrt und alte GPU-Buffer vor vorzeitiger Freigabe geschützt.
11. Nach erfolgreichem Fence-Wait die entsprechenden Pending-Marker aufgehoben und gezielte Mesh-Retirement-Zähler fortgeschrieben.
12. Bereits vorhandene atomare GPU-Vertex-/Indexbuffer-Allokation vor Veröffentlichung einer neuen Scene-Generation beibehalten und geprüft.
13. Bei erfolgreichem Meshwechsel die vorherige Kameraprojektion invalidiert und einen frischen Kameraframe vor dem nächsten Vulkan-Present verlangt.
14. Regionsverlust und Region-Clear während der laufenden Swapchain zugelassen, aber ohne Mesh/Kamera jeden weiteren World-Present verweigert.
15. Read-only-C-ABI für Scene-Generation, tatsächliche Mesh-Retirement-Waits und GPU-Wait-Fehler implementiert.
16. Native Vulkan-GPU: zwei vorherige Frame-Slot-Submissions, anschließend sicherer Meshwechsel Rot-zu-Grün bei unveränderter Swapchain bestanden.
17. Native GPU: Regions-Clear, neues rotes Mesh, erneutes grünes Mesh und drei begrenzte GPU-Fence-Waits mit unverändertem Per-Frame-QueueIdle-Zähler bestätigt.
18. Echte SDL/X11-Vorlogin-Probe um gezielte Mesh-Generationswechsel, Regionsverlust, Kameraneufreigabe und RGB-Kontrolle erweitert.
19. 7/7 neue GUI-Testfälle mit 20 Vulkan-Frames, 100 RGB-Samples, drei Fenstergrößen und fehlerfreiem OpenGL-Fallback bestanden; weitere 31 ältere Fälle seriell erneut grün.
20. Linux-Vollviewer-Build, 342/342 Python-Tests und 41/41 native CTests bestanden; Code unter fb53a9e9 committed, produktiver Vulkan-Live-World-Gate weiter offen.

**Native Vulkan-GPU-Abnahme:** Erst zwei GPU-Submissions
aus den zwei persistenten Frame-Slots, dann geordneter
Meshwechsel auf der gleichen VkSwapchainKHR. Sichtbare
Bildfolge mit echten X11-RGB-Pixeln:
vorderes Rot, hinteres Grün, Region-Clear,
neues Rot und neues Grün. Insgesamt **drei
gezielte Fence-Retirement-Waits** und keine
GPU-Retirement-Fehler. Der QueueIdle-Zähler
blieb während des Frame-Loops unverändert.
Ungültige NaN-Geometrie wurde abgewiesen,
ohne Meshgeneration, alte VkBuffers oder
Retirement-Wait-Zähler zu verändern.

**Echte SDL/X11-Viewer-Abnahme VOR OpenGL:**
20 GPU-Welt-/Depth-Presents über drei
Window-Extents (8/8/4) mit **100 echten
X11-RGB-Samples**, gezielten Mesh-Rotationen
und Region-Clear im ersten Abschnitt.
mesh-rotation, raster, depth, camera,
persistent-world und resize jeweils
verified; anschließend regulärer
OpenGL-Login und Exit 0. Fehlende Modul-
oder SDL-Freigaben, fehlender Vulkan-ICD
und der normale Legacy-Pfad erfolgreich
auf OpenGL zurückgeführt. **7/7 neue**
und **31/31 ältere** GUI-Fälle bestanden:
**38/38** insgesamt. Kein User-Viewer,
Login, Cache oder persönlicher Zustand
wurde verändert.

**Linux-Vollviewer-Build, 342/342 Python-Tests,
41/41 native CTests** bestanden. Die letzte
GitHub-CI vor diesem Code-Commit war
vollständig erfolgreich. Die neue CI muss
noch den veröffentlichten Stand prüfen.

**Grenze:** Auch dies ist eine ausdrücklich
freigegebene, vor dem GL-Login ausgeführte
Vulkan-Entwicklerdiagnose mit synthetischen
OpenSim-artigen Weltmeshes. Keine echte
authentifizierte OpenSim-In-World-Vulkan-
Darstellung, kein vollständiger Terrain-,
Material-, Textur-, Avatar- oder UI-Renderer
und kein dauerhaft aktiver Vulkan-Backend-
Switch in der laufenden Viewer-Welt.
P05 formal weiter **2/20**, Block 3 und
vollständiger Block 4 noch NICHT abgenommen.
Nächster Gate: echte nachgewiesene Live-
Regionsmesh-/Kamera-Snapshots und sichere
persistente Vulkan-Fenster-Ownership nach Login.
Code-Commit: fb53a9e94d91b827f3ec9c0ec26171eb2a5139b2.

## P05/3–4 – 20 technische Aufgaben: persistente Vulkan-World-Geometrie mit GPU-Tiefentest (05.11.2026)

Der bestehende persistente Vulkan-X11-Swapchain-Renderer zeichnet
jetzt nicht nur Clear-Bilder, sondern echte indexierte Welt-XYZ-Dreiecke
mit World-to-Clip-Kamera-PushConstants und GPU-Z-Depth-Test.
VkGraphicsPipeline, ShaderModule, PipelineLayout, Renderpass und
Framebuffers bleiben über mehrere GPU-Presents erhalten.
Jedes Swapchain-Image hat ein eigenes DepthImage, DepthMemory und
DepthImageView: zwei Frame-in-Flight-Slots schreiben dadurch
nicht in dieselbe Depth-Fläche. Kein vkQueueWaitIdle pro Frame;
beim Detach einmal QueueIdle zur Freigabe der Present-Semaphores.
GPU-Meshmutation/Region-Clear wird während möglicher in-flight-
Buffer-Referenzen bis zum Detach abgewiesen.

**20 tatsächlich umgesetzte/geprüfte Schritte:**

01. Projektstand, Branch, AGENTS, Handoff, Roadmap und vorige GitHub-CI geprüft; letzter Stand CI grün.
02. Die vorhandene persistente Clear-Swapchain und frühere Vulkan-World-/Depth-Pipeline auf gemeinsame Ressourcen geprüft.
03. Einen expliziten persistenten World-Depth-Entwicklermodus zum bestehenden Vulkan-Present-Pfad ergänzt.
04. Unabhängige World-, Raster-, Depth-, Meshupload-, Kamera- und Persistent-Freigaben gefordert.
05. Echte GPU-Vertex-/Indexbuffer, Kameramatrix und passendes Swapchain-Seitenverhältnis vor GPU-Present validiert.
06. Getrennte Depth-Images für jeden Swapchain-Image-Index zur sicheren Frame-in-Flight-Nutzung vorgesehen.
07. D32_SFLOAT/D16_UNORM-Unterstützung geprüft, pro Image GPU-Speicher alloziert und gebunden.
08. Jeder Depth-Speicherfläche eigene VkImageView und eigenes Zwei-Attachment-Framebuffer zugeordnet.
09. Renderpass um Depth-Attachment, Depth-Clear 1.0 und VK_COMPARE_OP_LESS erweitert.
10. Die vorhandenen validierten World-Vertex- und Depth-Fragmentshader in persistente VkGraphicsPipeline eingebaut.
11. Persistentes PipelineLayout mit 64-Byte-World-to-Clip-Kamera-PushConstant erstellt.
12. VkCmdBindPipeline, VkCmdPushConstants, BindVertexBuffers, BindIndexBuffer und vkCmdDrawIndexed im persistenten Frame aufgezeichnet.
13. GPU-Meshsnapshot und Kamera über die bestehenden C-ABI-Eingänge für alle World-Frames genutzt.
14. Vorzeitigen Mesh-Austausch und Regions-Clear verhindert, solange GPU-Frame-Submissions die Buffer referenzieren.
15. Shader, Pipeline, Layout und Depth-Images beim Detach nach Queue-Synchronisation kontrolliert freigegeben.
16. Native GPU-X11-Abnahme mit 16 persistierenden indexierten World-/Depth-Frames und echten Farbproben bestanden.
17. Native Kamera-Verschiebung, GPU-Mesh-Lebenszeitsperre, Regions-Clear nach Detach und Resize/Recreate erfolgreich geprüft.
18. SDL/X11-Vorlogin-Probe: 20 World-/Depth-Frames über drei Fenstergrößen und 100 echte RGB-Proben erfolgreich.
19. 5 neue World-/Depth-/Fallback-GUI-Fälle und 26 bestehende GUI-Regressionen nach serieller Wiederholung bestanden.
20. Linux-Vollviewer-Build, 341/341 Python-Tests und 41/41 native CTests erfolgreich; keine Nutzersitzung verändert.

**Native Vulkan-GPU-Abnahme:** 16 reale persistente
World-XYZ/Depth-Rasterframes, sichtbare rot-vorne/grün-hinten-
Projektion, Kamerabewegung und Rückbewegung, Mesh-Mutationssperre,
Resize/Recreate, reguläre Speicherfreigabe nach Detach bestanden.

**SDL-X11-Viewer-Abnahme VOR OpenGL:** 20 echte World-/Depth-
GPU-Presents über drei Fenstergrößen (8/8/4) mit 100 verifizierten
X11-RGB-Pixeln und gültiger Kamera nach jedem Attach;
camera/depth/raster/persistent-world=verified. Danach OpenGL-
Login und Exit 0. 5/5 neue Fälle; zusätzlich 7/7 alte Persistent-
Clear-, 14/14 Kamera-/Depth-/Raster- und 5/5 Dual-Fence-
GUI-Fälle bestanden: **31/31** insgesamt. Ein erster alter
Regressionstest wurde während eines parallelen Paket-Builds durch
kurzzeitig fehlende Viewer-Paketressourcen abgebrochen; nach
vollständigem Build erneut seriell ausgeführt und bestanden.

**Linux-Vollbuild, 341/341 Python-Tests, 41/41 native CTests**
erfolgreich. Vorige GitHub-CI für ogl-develop war vollständig
grün. Neue Commit-CI noch separat prüfen.

**Nicht nachgewiesen:** Es sind weiterhin SYNTHETISCHE
Testmeshes, keine authentifizierte OpenSim-Region. Keine
Terrain-/Textur-/Material-/Avatar-/UI-Vulkan-Weltdarstellung,
keine produktive Engine-Auswahl und keine dauerhaft aktive
Vulkan-Fenster-Ownership während des In-World-Betriebs.
Kein dynamischer Live-Meshupload während des GPU-Frame-Loops.
P05 formal **2/20**, Block 3 und vollständiger Block 4 offen.
Nächstes Gate: sichere generationenbasierte GPU-Mesh-Aktualisierung,
tatsächliche eingeloggte Region-Frames und Vulkan-Live-Handoff.
Code-Commit: 6734cdefdc8fad61ac63c5e124bfc2fa29498af8.

## P05/3 – 20 technische Aufgaben: persistente Vulkan-Swapchain-Frames ohne Queue-Idle pro Bild (05.11.2026)

**Implementiert:** Ein neuer, ausschließlich über Entwicklerflags
aktivierbarer Vulkan-X11-WSI-Pfad verwendet dieselben
VkRenderPass-, VkFramebuffer-, VkCommandPool-,
Command-Buffer-, Semaphore- und Fence-Ressourcen
über mehrere tatsächlich präsentierte GPU-Frames.
Zwei CPU-Frame-Slots und per Swapchain-Image
zugewiesene Render-Finished-Semaphores verhindern
die vorzeitige Wiederverwendung von Präsentations-
und Command-Buffer-Ressourcen. Die GPU-Fence-
Wartezeit ist begrenzt; die Funktion verwendet
KEIN vkQueueWaitIdle pro Frame. Nach dem letzten
Present wird genau EIN Queue-Idle beim kontrollierten
Detach ausgeführt, bevor die Objekte freigegeben werden.

**20 konkrete technische Arbeitsschritte:**

01. Bestehenden Branch, sauberen Git-Ausgangsstand, P05-Roadmap, Handoff und AGENTS vor Codeänderungen geprüft.
02. Vorherigen GitHub-CI-Fehlschlag isoliert: Windows x64 und ARM64 scheiterten; Linux x64/ARM64 und macOS-Varianten waren grün.
03. Über GitHub-Joblogs den exakten MSVC-Fehler C2589/C2059 in ogl_rtx_module.cpp auf das Windows-Makro max zurückgeführt.
04. Neuen std::max-Aufruf auf die MSVC-sichere geklammerte Funktionsform umgestellt.
05. Bisherigen GPU-Present-Code auf per-Frame-Ressourcenerzeugung und vkQueueWaitIdle nach jedem Bild untersucht.
06. Vorhandenes Vulkan-Modul um einen an die aktive Swapchain gebundenen persistenten Ressourcencontainer ergänzt.
07. Genau einen VkRenderPass und einen VkCommandPool für die gesamte angehängte Swapchain erstellt.
08. Separate VkImageViews und VkFramebuffers für jedes tatsächlich vorhandene Swapchain-Image angelegt.
09. Zwei wiederverwendbare primäre VkCommandBuffer mit Reset-Fähigkeit sowie zwei eigene Submit-Fences initialisiert.
10. Zwei Acquire-Semaphores entsprechend den CPU-Frame-Slots zur sicheren Übergabe an die GPU angelegt.
11. Present-Semaphores pro Swapchain-Image statt pro CPU-Frame-Slot reserviert, um eine vorzeitige Wiederverwendung zu verhindern.
12. Jeden CPU-Frame-Slot erst nach begrenztem Wait seines Submit-Fence und bei Bedarf seines vorherigen Image-Fence wiederverwendet.
13. Swapchain-Image mit VkAcquireNextImageKHR bezogen, Command-Buffer zurückgesetzt und GPU-Clear per wiederverwendbarem Renderpass aufgezeichnet.
14. Queue-Submit, Fence-Signal und VkQueuePresentKHR getrennt; keinen vkQueueWaitIdle im tatsächlichen persistenten Frame-Funktionskörper eingesetzt.
15. Maximal 16 persistente Entwickler-Frames pro Swapchain-Attach zugelassen; Folgeaufruf wird sicher abgewiesen.
16. Auf Detach genau einen Queue-Idle zur Absicherung der Present-Semaphore-Lifetime vorgesehen und Framebuffers, ImageViews, Renderpass, Pool und Fences freigegeben.
17. Bei Resize die alte Swapchain samt persistenten GPU-Ressourcen vollständig abgebaut und für die neue Größe frisch aufgebaut.
18. Nativer X11-Vulkan-GPU-Test: 16 reale abwechselnde Bildfarben, sichtbare Pixel, nur ein Build, ein Teardown, anschließend Resize/Recreate bestanden.
19. Die neue Persistent-Clear-Probe im echten SDL-Fenster ausschließlich vor GL-Kontexterzeugung und unter zwei separaten Entwicklerfreigaben eingebunden: 20 GPU-Frames, 100 RGB-Pixel und drei Fenstergrößen verifiziert.
20. Linux-Vollviewer-Build, 340/340 Python-Tests, 41/41 native CTests, sieben neue und 19 ältere isolierte Viewer-GUI-/Fallback-Tests bestanden; Windows-Reparatur wartet auf neue GitHub-CI.

**Abnahme echte GPU:** Nativer X11-Vulkan-Test:
16 reale Presents auf einer Swapchain, abwechselnde
RGB-Farbpixel, nur eine Ressourceninitialisierung,
der 17. Present wird abgewiesen. Danach genau
eine Freigabe mit einer Queue-Idle-Abschlussprüfung.
Ein X11-Resize erzeugt die zweite Swapchain und
liefert ein weiteres korrektes GPU-Pixel; Zähler:
builds=2, releases=2, teardown_idle=2.

**Abnahme tatsächlicher SDL-Viewer:** Die separate
Vor-OpenGL-Probe zeichnet 20 GPU-Bilder über drei
Fenstergrößen (8/8/4) auf der vorhandenen SDL/X11-
Viewer-XID. 100 echte X11-RGB-Pixel, drei
Ressourcen-Lifecycles und die Wiederherstellung
der ursprünglichen Fenstergröße wurden bestätigt.
Danach regulärer OpenGL-Login und Exit 0.
**7/7 neue** SDL-GUI-Fälle inkl. fehlender
Entwicklerfreigaben, unzulässiger Mischung
mit Raster-Probe und fehlendem Vulkan-Treiber.
**14/14 ältere** Kamera-/Depth-/Raster-Fälle
und **5/5** Doppelfence-Fälle erneut grün:
insgesamt 26/26 isolierte GUI-Smokes.

Linux-Vollviewer-Build, **340/340 Python-Tests**,
**41/41 native CTests** bestanden. Kein Login,
persönlicher Cache oder Nutzer-Viewer wurde geändert.

**Windows-CI-Blocker gefunden und im Code korrigiert:**
Die vorherige GitHub-CI für Code 49b28a90 scheiterte
auf Windows x64/ARM64 mit MSVC C2589/C2059 wegen
std::max und dem Windows-Makro max. Die Korrektur
wurde als portable geklammerte Form implementiert.
Erst die neue GitHub-CI kann diese Korrektur
für Windows bestätigen; lokale Linux-Ergebnisse
ersetzen die Windows-Prüfung nicht.

**Grenzen:** Der persistente Pfad zeigt echte
Vulkan-Clear-Bilder, aber noch keine eingerichtete
OpenSim-Welt mit Terrain, Texturen, Avataren und UI.
Er läuft vor SDL_GL_CreateContext als sichere
Entwicklerdiagnose, NICHT während einer eingeloggten
OpenSim-World-Session. Der alte Raster-/Depth-Testpfad
verwendet weiter seinen gesonderten Synchronisations-
mechanismus; der neue langlebige Clear-Pfad ist noch
nicht mit der eigentlichen Welt-Grafikpipeline verbunden.
Es gibt keine fertig auswählbare Vulkan-Engine.
**P05 formal 2/20; operativer Block 3 bleibt bis
zur dauerhaften Vulkan-World-/Window-Ownership offen.**
Nächstes Gate: die bestehende Welt-Raster-/Depth-
Pipeline auf die persistente WSI-Frame-Verwaltung
migrieren, danach echte In-World-Eingangsframes
bei angemeldetem Viewer nachweisen.
Code-Commit: 8f33196ea3febf7e75e17a68fbe659e3fbe3609a.

## P05/3 – 20 technische Aufgaben: Zwei-GPU-Submissions und sichere Kamera-/Swapchain-Zuordnung (05.11.2026)

**Neu:** Die GPU-Queue-Diagnose reiht jetzt zwei unterschiedliche
Command-Buffer mit je eigener Fence ein, bevor EIN
zeitlich begrenzter wait-all-Fence-Wait (2 Sekunden)
die Fertigstellung abfragt. Es gibt keinen
vkQueueWaitIdle in diesem separaten Sync-Probe-Pfad.
Read-only-C-ABI zählt die maximal vor dem Warten
ausstehenden Submissions sowie die eingereihten
und abgeschlossenen Fence-Submissions. Zwei
GPU-Submissions vor dem Wait bedeuten nicht,
dass die GPU sie wirklich gleichzeitig ausführt.

**Resize-Sicherheit:** Die Vulkan-Kameraprojektion
ist jetzt an das Seitenverhältnis der tatsächlichen
Swapchain gebunden. Nach einem Wechsel von 4:3
auf 16:9 wird die bisherige Kamera ungültig,
bis ein neuer Kameraframe zur aktiven Swapchain
passt. Kontrollpunkte liegen bei Swapchain-Attach,
Kameraupdate und vor dem GPU-Welt-Present.

**20 tatsächlich umgesetzte/geprüfte Arbeitsschritte:**

01. Branch ogl-develop, Git-Status, P05-Roadmap, AGENTS, Status und Handoff vor Änderungen geprüft.
02. Erfolgreiche vorherige GitHub-CI verifiziert.
03. Serielle Submit-Wait-Submit-Wait-Abfolge als GPU-Queue-Bottleneck ermittelt.
04. Zwei unabhängige Vulkan-Command-Buffer und VkFence-Objekte erhalten.
05. Ersten VkQueueSubmit auf echter GPU vor jedem CPU-Wait ausgeführt.
06. Zweiten VkQueueSubmit ebenfalls vor dem ersten CPU-Fence-Wait eingereiht.
07. Einzelne vkWaitForFences-Abfrage mit waitAll für beide Einreichungen eingebaut.
08. Gemeinsamen GPU-Fence-Wait auf zwei Sekunden begrenzt.
09. Timeout quarantänisiert potentiell noch aktive GPU-Ressourcen und sperrt weitere Probe-Versuche.
10. Read-only-Messgröße max_outstanding für GPU-Einreichungen vor dem Wait ergänzt.
11. Read-only-Messgrößen last_submitted und last_completed eingeführt.
12. Alle drei Zähler über additive C-ABI exportiert und beim Modulstop zurückgesetzt.
13. Zweimal zwei GPU-Einreichungen mit je zwei gültigen Fence-Completions im nativen Test bestanden.
14. Seitenverhältnis der Vulkan-Kameramatrix zusätzlich zum World-to-Clip-Transform gespeichert.
15. Beim realen Swapchain-Attach mit verändertem Aspect die alte Kamera invalidiert.
16. Bei Kameraupdate mit bereits aktiver Swapchain ein unpassendes Aspect abgewiesen.
17. World-Probe-GPU-Present durch zusätzlichen Camera/Swapchain-Aspect-Check geschützt.
18. Echten X11-Resize 4:3 auf 16:9 getestet: stale Kamera verweigert, neuer Kameraframe rendert wieder sichtbar.
19. Zwei-Fence-Prüfung im echten SDL-Viewer nur vor GL-Start und mit separatem Entwickler-Opt-in integriert; fünf GUI-Abnahmen bestanden.
20. Linux-Vollbuild, 339/339 Python- und 41/41 native Tests sowie 14 bestehende GUI-Regressionen zusätzlich bestanden.

**GPU-Hardware-Nachweise:** Zwei getrennte GPU-Testdurchläufe
mit jeweils zwei GPU-Submissions vor dem gemeinsamen
Fence-Wait; die drei Synchronisationszähler melden
max_outstanding=2, last_submitted=2 und last_completed=2.
Nach Modulstop sind alle drei zurückgesetzt.
Ein echter X11-Resize von 4:3 auf 16:9 verweigert
den alten Welt-Kameraframe; nach gültigem
16:9-Update wird die Welt-Testgeometrie wieder
mit korrekten X11-RGB-Pixeln präsentiert.
Bisherige 16-Frame-Swapchain-, Depth-,
Kameraprojektions- und Clear-Tests sind grün.

**Echter SDL/X11-Viewer:** Mit zwei ausdrücklichen
Entwicklerfreigaben ist die Zwei-Fence-Prüfung
VOR einem optionalen Vulkan-GPU-Testbild integriert.
5/5 neue GUI-Smokes bestanden (mit World/Depth,
mit Raster, ohne Freigabe, ohne Vulkan-Treiber
und Legacy ohne Dual-Sync); zusätzlich 14/14
bestehende GUI-Smokes erneut bestanden.
Alle 19/19 bis zum OpenGL-Login und Exit 0.
Linux-Vollviewer-Build, 339/339 Python-Tests
und 41/41 native CTests bestanden.

**Abnahmegrenze:** Die Zwei-Fence-Änderung betrifft
nur die abgekoppelte Vulkan-GPU-Queue-Diagnose.
Der bestehende Vulkan-Swapchain-Present-Pfad
verwendet weiterhin vkQueueWaitIdle pro Frame
und ist ausdrücklich KEIN produktiver, dauerhaft
nonblocking Vulkan-World-Frame-Loop. Es gibt
weiterhin KEINE getestete authentifizierte
OpenSim-In-World-Vulkan-Präsentation und keine
fertige Vulkan-Engine mit Terrain, Texturen,
Avataren und UI. Formale P05-Zählung weiterhin
2/20, Block 3 und vollständiger Block 4 offen.
Nächstes Gate: persistent für Vulkan reservierte
Swapchain-Frame-Ressourcen, Present-Semaphore-
Lifetime, kontrolliertes Acquire/Present sowie
echte OpenSim-World-Eingänge im laufenden Viewer.
Code-Commit: 49b28a906930db9ace693580390bcb654bd418df.

## P05/3–4 – 20 technische Teilaufgaben: gemeinsamer Vulkan-Weltkamera-/Tiefenpfad (05.11.2026)

**Code-Fortschritt:** Die vorhandene Vulkan-World-to-Clip-Kamera-
Projektion und der echte VkDepth-Test wurden in einer einzigen
Grafikpipeline kombiniert. Zwei synthetische Welt-XYZ-Dreiecke
decken sich im Bild, liegen aber bei GPU-Z=0.25 (vorn) und
Z=0.75 (hinten). Die reale GPU zeichnet die vordere rote Fläche
und weist die danach gerasterte grüne Fläche zurück.

**Wesentlicher Negativnachweis:** Dieselben Weltkoordinaten, derselbe
Shader, dieselbe Kamera und die gleiche Dreiecksreihenfolge zeigen
OHNE Tiefenpuffer tatsächlich die zuletzt gezeichnete grüne Fläche.
Eine separate Entwickler-Farbkontrolle wählt deshalb denselben
GPU-Fragmentshader bewusst ohne Depth-Attachment.
Das ist kein zusätzlicher produktiver Grafikmodus.

**20 umgesetzte und geprüfte technische Teilaufgaben:**

01. Sauberen Branch ogl-develop, Projekthandoff, AGENTS.md, aktuellen Status und formalen P05-Plan geprüft.
02. Erfolg der vorangehenden portablen GitHub-CI für den bisherigen GPU-Depth-Baustein nachgewiesen.
03. Vorhandene Vulkan-World-to-Clip-Kamera-ABI und Z-/Depth-Renderpass auf gemeinsame Nutzbarkeit untersucht.
04. Zwei deckungsgleiche Dreiecke in synthetischen OpenSim-typischen Welt-XYZ-Koordinaten konstruiert.
05. Mit der v3-Kameraschnittstelle und Near=2 m/Far=10 m unterschiedliche GPU-NDC-Tiefen 0.25 und 0.75 festgelegt.
06. Den validierten 275-Wort-Welt-Vertexshader und 182-Wort-Depth-Fragmentshader erneut bytegenau geprüft.
07. Strikt separate opt-in Depth-Farbkontrolle ohne Tiefen-Image im Vulkan-GPU-Testpfad ergänzt.
08. Tiefentest und Negativkontrolle als nicht gleichzeitig zulässige Diagnosemodi abgesichert.
09. Die GPU-Z-Farbshaderauswahl für den normalen Tiefentest sowie den ausdrücklich depthlosen Kontrollfall vereinheitlicht.
10. Reale Vulkan-GPU-Vertex-/Indexbuffer mit beiden Welt-XYZ-Dreiecken bestückt und Meshcount/Gültigkeit geprüft.
11. Vulkan-Kamera nach dem neuen GPU-Meshsnapshot aktiviert; Bereitschaft vor gültiger Kamera sicher zurückgesetzt.
12. Native X11-GPU-Abnahme mit aktiviertem Depth-Test: vorderes Rot bleibt trotz zuletzt gezeichneter grüner Rückfläche sichtbar.
13. Gegenprobe mit identischen Vertexdaten, Kamera und Shader aber ohne Depth-Attachment: Rückfläche erscheint erwartungsgemäß grün.
14. Kamerabewegung bei unveränderten GPU-Vertexdaten geprüft: beide Dreiecke verlassen die Bildmitte, Hintergrund wird sichtbar.
15. Fenster-/Swapchain-Resize auf 640x480 mit erneut gültigem Kamera-Frame und neuem Depth-Attachment geprüft.
16. Ungültigen Kameraframe nach einem erfolgreichen Bild abgewiesen und weiteren Vulkan-Present ohne gültige Kamera verhindert.
17. Regions-/Mesh-Clear durchgeführt; Vertexbuffer, Kamerabereitschaft und Live-Input-Ready korrekt invalidiert.
18. Echte SDL/X11-Window-Probe um gleichzeitige World-Projektion und Depth-Test mit zwei Weltkoordinaten-Dreiecken erweitert.
19. SDL-/Modul-Depth-Freigaben symmetrisch geprüft; 14/14 reale isolierte GUI-Fälle zu GPU-Depth, fehlenden Freigaben, Kamera, Treiber und Legacy-Raster erfolgreich.
20. Linux-Vollviewer-Build, 338/338 Python-Tests, 41/41 native CTests, echte native GPU-Weltprojektion mit Depth-/Gegenprobe und Shader-Binärvergleich bestanden.

**Hardware-Nachweise:** Der native Vulkan/X11-Test bestätigt
near=1, no_depth_far=1, moved=1, resized=1,
invalid=1, clear=1. Bei 512x384 und 640x480 Pixeln
stimmen reale X11-Farbproben nach GPU-Present.
Der neue Test prüft auch die Kamera-Invalidierung
und die vollständige Geometriefreigabe nach Region-Clear.
Alle vorigen 16-Frame-Swapchain-, Index-Raster-
und Camera/Depth-Smoketests bleiben bestanden.

**Echte SDL-Viewer-Abnahme:** Unter voneinander unabhängigen
Entwicklerfreigaben läuft die kombinierte Projektion
und Tiefenprüfung AUF DEM VORHANDENEN SDL/X11-VIEWERFENSTER,
streng bevor SDL_GL_CreateContext aktiviert wird.
Ein echter GPU-Present lieferte pixel=verified,
raster=verified, camera=verified, depth=verified.
Auch fehlende Kamera, ungültiger Vulkan-ICD,
fehlende Depth-Freigaben und bestehende Clear-/Raster-
Modi starten OpenGL ohne Fehler.
**14/14 isolierte echte GUI-Tests erreichen
STATE_LOGIN_WAIT und Exit 0.**
Der vollständige Linux-Viewer-Build,
**338/338 Python-Tests**, **41/41 native CTests**
und SPIR-V-Binärnachweise sind erfolgreich.
Keine Nutzersessions, gespeicherten Profile
oder Zugangsdaten wurden angefasst.

**Ausstehende Arbeit:** Der Test benutzt bewusst
synthetische OpenSim-artige Weltkoordinaten und
eine Testkamera, keine angemeldete echte Region.
Eine authentifizierte OpenSim-World-Scene mit
realen Texturen, Terrain, Avataren, UI und
permanenter Vulkan-Window-Ownership ist nicht
nachgewiesen. Die echte Vulkan-Welt-Engine ist
nicht auswählbar. Jeder Vulkan-Present dieses
Tests endet vollständig VOR OpenGL. Der
Entwicklungs-Frame-Pfad verwendet weiterhin
vkQueueWaitIdle und ist nicht produktionsbereit.
**P05 formal weiterhin 2/20; Block 3 und
vollständiger Block 4 OFFEN.**
Nächster Gate: nachgewiesene echte In-World-
Mesh-/Kamera-Snapshots und langlebiger
exklusiver Vulkan-Window-/Frame-Loop mit
bounded nonblocking GPU-Synchronisation.
Code-Commit: c34a0d44aa26e70c0281274cb4075a72b977d513.

## P05/3–4 – 20 technische Teilaufgaben: echter Vulkan-Z-/Depth-Buffer im Viewerfenster (05.11.2026)

**Neuer GPU-Fortschritt:** Die bisherige Vulkan-Indexed-Grafikpipeline
unterstützt jetzt zusätzlich einen echten hardwarebeschleunigten
Tiefentest. Ein separates VkImage (D32_SFLOAT, Fallback D16_UNORM)
mit eigener VkDeviceMemory, VkImageView, VkRenderPass-Attachment,
Framebuffer-Depth-Bindung, VkPipelineDepthStencilState und
Tiefen-Clear auf 1.0 steht nur im ausdrücklich freigegebenen
Entwicklerpfad zur Verfügung. Depth-Compare ist LESS,
Depth-Write und Depth-Test sind aktiviert.

**20 nachweisbar bearbeitete technische Schritte:**

01. Branch, sauberen Git-Ausgangsstand, AGENTS.md, Handoff, Status und P05-Roadmap erneut kontrolliert.
02. Erfolgreiche vorangehende GitHub-CI des letzten P05-Kamerabridge-Commits geprüft.
03. Den vorhandenen VkRenderPass mit ausschließlich Color-Attachment auf fehlenden Tiefentest untersucht.
04. Depth-Fragmentshader für echtes GPU-Z-basiertes Rot-/Grün-Dreieck in GLSL 450 geschrieben.
05. Shader für Vulkan 1.0 mit lokalem Khronos-glslang kompiliert und mit spirv-val validiert.
06. Validierte 182 SPIR-V-Wörter in das bestehende Vulkan-Modul eingebettet, ohne neue Projektdatei.
07. Zusätzliches Entwicklerflag OGL_VULKAN_EXCLUSIVE_DEPTH_PROBE=1 eingeführt, das echten Indexed-Draw voraussetzt.
08. Fehlende Vulkan-Image-/Memory-Prozeduren und fehlende physische Format-/Memory-Abfragen sicher abgewiesen.
09. Unterstützte D32_SFLOAT-Depth-Tiling-Eigenschaft abgefragt und D16_UNORM als Fallback vorgesehen.
10. Eigenes Vulkan-Depth-Image in der tatsächlichen Swapchain-Fenstergröße mit DEPTH_STENCIL_ATTACHMENT_USAGE angelegt.
11. Kompatiblen VkDeviceMemory-Typ unter Bevorzugung von DEVICE_LOCAL gewählt, zugewiesen und an das Image gebunden.
12. Depth-VkImageView mit VK_IMAGE_ASPECT_DEPTH_BIT erzeugt und bei Fehlern fail-closed.
13. VkRenderPass um Depth-Attachment und eindeutiges VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ergänzt.
14. VkFramebuffer um separate Depth-ImageView erweitert und Imagegröße an Swapchain-Resize gekoppelt.
15. VkGraphicsPipeline mit depthTestEnable, depthWriteEnable und VK_COMPARE_OP_LESS ausgerüstet.
16. VkRenderPassBegin um Depth-Clear 1.0 ergänzt; Legacy-Clear-/Rasterproben ohne Depth weiterhin beibehalten.
17. Depth-View, Image und DeviceMemory im bestehenden GPU-Cleanup in korrekter Eigentumsreihenfolge freigegeben.
18. Native GPU-X11-Abnahme mit nahem rotem Dreieck zuerst, entferntem grünem Dreieck zuletzt durchgeführt: Rot vorne sichtbar; 2 Fenstergrößen inklusive Resize und Clear bestanden.
19. Echten SDL/X11-Viewer-Startup nur bei separatem OGL_VULKAN_SDL_PREGL_DEPTH_PROBE freigegeben; fehlende Flags und ungültiger Treiber mit OpenGL-Fallback geprüft.
20. Linux-Vollviewer-Build, 337/337 Python-Tests, 41/41 CTests, Khronos-Shaderprüfung, native GPU-Depthtests und 12/12 isolierte Viewer-GUI-Starts bestanden.

**Reale GPU-Abnahme:** Das vordere rote Dreieck (z=0.25) wurde
zuerst, danach das überlappende entfernte grüne Dreieck
(z=0.75) in echte GPU-Vertex-/Indexbuffer hochgeladen.
Der native Vulkan-GPU-Test meldet near_over_far=1,
resized=1 und cleared=1. Es wurde durch tatsächliche
X11-Vordergrundpixel gezeigt, dass das rote Dreieck trotz
zuletzt gezeichneter grüner Fläche sichtbar bleibt.
Bei 512x384 und 640x400 sind Farbe und Hintergrund
verifiziert; der zweite Lauf benutzt eine neue
Swapchain-/Depth-Fläche nach Resize.

**Viewer-GUI-Abnahme:** Im echten SDL/X11-Window des
OGL-Viewers VOR GL-Erzeugung: 1/1 Vulkan-Present,
pixel=verified, depth=verified, danach OpenGL-Login
und regulärer Exit 0. Beide Depth-Opt-ins sind
erforderlich; fehlende SDL-Freigabe, fehlender Raster-
Opt-in und ungültiger Vulkan-Treiber behalten den
normalen OpenGL-Start. Bestehende Kamera-/Indexed-
Raster-/Clear-/Fallback-GUI-Fälle erneut geprüft:
**12/12** erfolgreich. Linux-Vollbuild,
**337/337 Python-Tests**, **41/41 native CTests**
bestanden. Keine Benutzerprofile, laufenden
Viewer oder persönliche Daten angefasst.

**Wichtig:** Noch immer ausschließlich eine
explizite Entwicklerdiagnose VOR dem Start des
OpenGL-Kontexts. Es ist ein echter Vulkan-Depth-
/Raster-Draw, aber KEIN eingeloggter OpenSim-
World-Frame, keine vollständige Vulkan-Engine,
kein produktiver Vulkan-Frame-Loop und keine
dauerhafte exklusive Vulkan-Fensterkontrolle.
Echte Live-Regionsabnahme bleibt offen.
**P05 operativ 2/20; Block 3 und vollständiger
Block 4 bleiben offen.** Nächster technischer
Schwerpunkt: Depth-geprüfte Welt-Meshes mit
aktueller Firestorm-Kamera im exklusiven
Live-Vulkan-Renderer; danach nonblocking
Swapchain-Frame-Scheduling und GL-Fallback.
Code-Commit: cf2cc17fbfa575a64ff2b1bbb5f66e9f7f5ea1ba.

## P05/3–4 – 20 technische Teilaufgaben: Firestorm-Live-Kamera und Vulkan-Regionsmesh-Kopplung (05.11.2026)

**Code-Fortschritt:** Der vorhandene OpenGL-Deferred-Renderpfad übergibt
aktuelle Firestorm-Kameraachsen, Viewport, FOV, Near/Far und Sonne in
der bereits bestehenden v3-Kamera-ABI nun optional auch an das
Vulkan-Grafikmodul. Diese Weiterleitung ist unabhängig von einer
RTX-/Raytracing-Fähigkeit der GPU und ausschließlich bei drei expliziten
Entwicklerflags und echtem Vulkan-Gerät aktiv. Das Rendern selbst
bleibt OpenGL; es wird KEIN Vulkan-Weltbild präsentiert.

Die vorhandene OpenSim-Regionsgeometrie wurde bereits zuvor optional
als Dreiecksliste in echte GPU-Vertex-/Indexbuffer hochgeladen. Neu:
ein GPU-Snapshot löscht die alte Kamerabereitschaft; erst eine
danach akzeptierte Kamera qualifiziert das gemeinsame Datenpaar.
Region-Clear, ungültige Kamera und Modulstop verhindern stale Daten.
Die zusätzlichen C-ABI-Zähler melden angenommene Eingaben, KEINE FPS
oder sichtbaren Vulkan-Frames.

**20 umgesetzte technische Teilaufgaben und Prüfgates:**

01. Branch ogl-develop, sauberen Ausgangsstand, AGENTS, Handoff, Status, P05-Plan und vorherige CI geprüft.
02. Tatsächliche OpenGL-Deferred-Pipeline und Real-Kamera-Aufruf in pipeline.cpp lokalisiert.
03. Vorhandene Regions-ID-Umschaltung und gecappte Live-OpenSim-Mesh-Erfassung nachvollzogen.
04. Unbeabsichtigte Kopplung der Kamerafreigabe an RTX-Hardware in oglrtxhost.cpp identifiziert.
05. Dreifache ausdrückliche Entwicklerfreigabe für Vulkan-Device, Meshupload und Kamera implementiert.
06. Gültiges Vulkan-Gerät als Voraussetzung der neuen Kameraweitergabe unabhängig von NVIDIA/RTX geprüft.
07. Kamera-ABI des vertrauten Moduls zur Laufzeit dynamisch aufgelöst und bei fehlender Schnittstelle auf GL zurückgeführt.
08. Neue Read-only-Eingangsbereitschaft des Vulkan-Moduls über ein eigenes C-ABI-Symbol aufgelöst.
09. Real-Firestorm-Kamera mit Viewport, Position, Blick-/Rechts-/Hochachse, FOV, Aspect, Near/Far und Sonne in bestehende v3-Frame-Struktur übernommen.
10. Nur bei echter Opt-in-Freigabe die aktuelle Kamera aus dem vorhandenen Rendererpfad an Vulkan-CPU-Projektion weitergereicht.
11. RTX-Kamera- und Raytracing-Diagnostik weiterhin ausschließlich bei gültiger RTX-Hardwarefähigkeit ausgeführt.
12. Erste angenommene Kamera und erstes qualifiziertes Kamera/Mesh-Eingangspaar separat und ohne falsche Vulkan-Present-Meldung logbar gemacht.
13. Im Vulkan-Modul Zähler für erfolgreich angenommene Kameratransformationen und erfolgreich hochgeladene GPU-Meshsnapshots ergänzt.
14. Echte Read-only-Ready-Prüfung verlangt gültige Kamera, Dreiecke und vorhandene Vulkan-Vertex-/Indexbuffer.
15. Jeder angenommene neue GPU-Meshsnapshot invalidiert die vorherige Projektionskamera bis zum nächsten gültigen Frame.
16. Regions-/Szenen-Clear invalidiert Kamerabereitschaft und Matrix, während GPU-Meshbuffer freigegeben werden.
17. Ungültige Kameraupdates löschen Bereitschaft; atomar abgelehnte Meshuploads behalten dagegen das letzte gültige Datenpaar.
18. Native-GPU-Tests prüfen Opt-in, Kamera, drei erfolgreiche Kameraframes, Meshwechsel, Invalidierung, Regions-Clear und Geräte-Shutdown.
19. Portabler Quellcode-Regresstest prüft echten Pipeline-Firestorm-Aufruf, RTX-unabhängige Weiterleitung, C-ABI und fehlende Welt-Präsentation.
20. Linux-Vollbuild, 336 Python-/41 native Tests, native Vulkan-GPU-Abnahme sowie 12 getrennte isolierte GL-GUI-/Fallback-Starts bestanden.

**Verifizierte Ergebnisse:** Native Vulkan-GPU-Tests mit echter
Mesh-/Kamerapaarung, gültiger und ungültiger Kamera, mehreren
GPU-Buffer-Uploads, atomar abgewiesenem Mesh, Region-Clear
und Modul-Shutdown bestanden. Die bisherige echte Kamera-Projektions-
GPU-Abnahme mit bewegter Testkamera, Rasterpixel und dem nativen
16-Frame-WSI-Test bleibt erfolgreich. Linux-Vollviewer-Build,
336/336 Python-Tests und 41/41 native CTests bestanden.
Vier neue isolierte Xvfb-Starts (Live-Kamera-Flags an/aus,
ungültiger Vulkan-ICD, reines OpenGL) und acht bestehende
Vulkan-Raster-/Fallback-GUI-Starts: **12/12** zum
Login-Wartezustand und regulärem Exit-Code 0. Keine
Nutzerprofile, Zugänge oder laufenden Viewer berührt.

**Nicht nachgewiesen:** Die Xvfb-Starts erreichten nur das
Vorlogin-Fenster. Es wurde KEINE OpenSim-Region authentifiziert;
daher ist in diesen GUI-Smokes auch kein Live-Kamera-und-
Regionsmesh-Paar tatsächlich beobachtet worden. Der reale
Firestorm-Eingangspfad ist im Quellcode implementiert und
statisch geprüft; das Datenpaar und dessen Invalidierung
sind nativ auf echter Vulkan-GPU mit definierten Testwerten
geprüft. Ein authentifizierter In-World-Snapshot bleibt
ein separates zwingendes Abnahme-Gate. Vollständiges
Terrain-/Material-/Avatar-/UI-Rendering, dauerhaftes
exklusives Vulkan-Fenster-Ownership und nichtblockierende
Frame-Synchronisation fehlen. **Operative P05-Blöcke
weiterhin 2/20; Block 3 und vollständiger Block 4 offen.**
Nächster Schritt: authentifizierter echter OpenSim-
Regionstest mit im Log belegten Camera+Mesh-Inputs,
anschließend exklusive Vulkan-Weltpräsentation.
Code-Commit: ae00535c6ed75af3f9c0bb0fd73793270bb83bbe.

## P05/3–4 – 20 technische Schritte: Vulkan-Projektion aus Welt-X/Y/Z und Kamera (05.11.2026)

**Ergebnis:** Die bestehende Vulkan-Indexed-Draw-Pipeline verarbeitet
jetzt eine validierte 4x4-Welt-zu-Clip-Kameramatrix über echte
GPU-Push-Constants. Ein neues Shaderprofil projiziert Welt-XYZ
mittels Blickrichtung, Rechts-/Hochachse, Sichtwinkel,
Seitenverhältnis sowie Near/Far in Vulkan-NDC mit Z in 0..1.
Die Kameradaten verwenden die bestehende v3-Firestorm-Kamera-ABI,
die Projektion ist unabhängig von NVIDIA-/RTX-Fähigkeiten.

**20 einzeln umgesetzte/überprüfte technische Arbeitsschritte:**

01. Repository- und Branchzustand, P05-Roadmap, AGENTS.md und Projekthandoff vor Beginn kontrolliert; vorherige portable GitHub-CI erfolgreich.
02. Vorhandene v3-Firestorm-Kameraschnittstelle, Kamera-Validator und Welt-Dreiecksdaten-ABI untersucht.
03. Vorhandenes lokales Khronos-glslang/spirv-val verwendet; keine zusätzlichen Programmabhängigkeiten installiert.
04. GLSL-450-Vertexshader mit 4x4-World-to-Clip-Matrix als Vulkan-Push-Constant erstellt.
05. Shader als SPIR-V für Vulkan 1.0 kompiliert und durch Khronos SPIR-V-Validator geprüft.
06. 275 SPIR-V-Shaderwörter direkt im bestehenden Vulkan-Modul eingebettet; bytegenau mit validierter Shaderdatei abgeglichen.
07. Eigenständige additive Vulkan-C-ABI zum Setzen und Abfragen der projektionsbereiten Kamera eingebaut.
08. Exakte Strukturversion, Kameraposition, FOV, Near/Far, Blickachsen und numerische Gültigkeit mit vorhandenem Kamera-Validator geprüft.
09. Seitenverhältnis der Kamera mit den echten Frame-Bildmaßen abgeglichen und unpassende Eingaben abgewiesen.
10. Drei Kameraachsen auf näherungsweise Einheitslänge und paarweise Orthogonalität geprüft.
11. Handedness über das Kreuzprodukt aus Rechts-/Hochachse gegen Blickrichtung zusätzlich geprüft.
12. Vulkan-Perspektivprojektion mit Tiefenbereich 0..1 und korrektem Y-Vorzeichen implementiert.
13. Welt-zu-Clip-Kameramatrix aus echten XYZ-Basisachsen, Position und Clipping-Ebenen spaltenweise aufgebaut.
14. Ungültige Aktualisierungen lassen keine frühere gültige Vulkan-Kameramatrix als veralteten Frame weiterverwenden.
15. VkPipelineLayout und Vulkan-Render-Befehl um 64 Byte VkPushConstantRange sowie vkCmdPushConstants ergänzt.
16. World-Shader und Clip-Space-Shader klar getrennt; Weltprojektion nur unter zusätzlicher Entwicklerfreigabe mit Vulkan-GPU und vorhandenem Mesh.
17. Nativer Vulkan-GPU-Test rendert ein reales indexiertes Dreieck aus Weltkoordinaten und prüft echte X11-Vordergrund-/Hintergrundpixel.
18. Mit unveränderten Welt-Vertexdaten nach seitlicher Kameraverschiebung die erwartete leere Bildmitte GPU-seitig nachgewiesen; ungültige Achsen/Aspect/FOV abgewiesen.
19. Auf tatsächlicher SDL-Viewer-XID den synthetischen Weltkamera-Test vor OpenGL geprüft; 8/8 neue Raster-/Fallback-GUI-Fälle und 7/7 bisherige Resize-/Fehler-GUI-Fälle bestanden.
20. Linux-Vollviewer-Build, 335/335 Python-Tests, 41/41 native CTests, native Vulkan-Projektion und negative GPU-Treiberprüfung bestanden.

**GPU-/GUI-Nachweise:** Der echte native Vulkan-GPU-Test präsentiert
ein rotes indexiertes Dreieck aus Weltkoordinaten und überprüft
die Mitte des X11-Fensters sowie den blauen Hintergrund.
Bei unveränderten GPU-Vertexdaten verschwindet das Dreieck
nach einer seitlichen Kameraverschiebung erwartungsgemäß
aus der Bildmitte (visible=1 moved=1 invalid=1).
Ungültige Achsen, falsches Aspect und ungültiges FOV
werden verweigert; der zuletzt gültige Kamera-Transform
wird bei invalidem Update gelöscht. In der tatsächlichen
SDL/X11-Viewer-Window-ID: synthetische World-Kamera
1/1 presented, pixel=verified, camera=verified,
danach regulär World backend: OpenGL (actual),
Login-Wartezustand und Exit 0. Weitere GUI-Fallbacks
ohne Kamera-Freigabe, ohne Vulkan-Treiber, ohne Mesh,
mit altem Rastershader und ausgeschalteter Vulkan-Probe:
**8/8 GUI-Tests erfolgreich**. Ältere 20-Frame-/Resize-
und Fehlereinleitungs-Gates: **7/7 weitere GUI-Tests**
erfolgreich. Linux-Vollbuild, **335 Python-Tests,
41 native CTests** und Shader-Binärvergleich bestanden.
Vulkan-less native Hardwarecheck schlägt mit erwartetem
Exit-Code 12 fehl, nicht stillschweigend als GPU-Erfolg.

**Strenge Grenze:** Die Kamera und das Dreieck im SDL-Test
sind bewusst synthetische World-XYZ-Daten, keine live
eingeloggten OpenSim-Szenen oder Avatar-/Terrain-Daten.
Die Kamera-Schnittstelle kann künftig aktuelle Firestorm-
Frames entgegennehmen; tatsächliche Live-Einspeisung in
eine dauerhaft Vulkan-eigene Welt-Rendering-Session,
Terrain, Texturen, Avatare, UI-Compositing, Vulkan-Frame-
Scheduling ohne vkQueueWaitIdle sowie Windows/macOS/Wayland
sind weiterhin offen. Der produktive OpenGL-Renderer und
dessen Login bleiben unverändert. **P05 formal 2/20;
P05/3 und der vollständige P05/4 weiterhin nicht abgenommen.**
Nächster Gate: realer in-world Kameraframe plus Regions-
Dreieckdaten in einer exklusiven Vulkan-Renderer-Session.
Code-Commit: 439b8a2710722da93155fbe520ba5158d1356986.

## P05/3 und P05/4 – 20 technische Raster- und Grafik-Nachweise (05.11.2026)

**Ergebnis:** Erstmals ein ECHTER indexierter Vulkan-Grafikdraw
aus vorhandenen VkVertex-/VkIndex-Bufferdaten in einer
VkGraphicsPipeline, mit GLSL-450-Vertex-/Fragmentshadern
und vkCmdDrawIndexed. Der native GPU-Test liest ein rotes
Dreieck (RGB 237/48/38) und den unveränderten blauen
Hintergrund (RGB 20/54/115) aus dem tatsächlichen X11-
Fenster. Dasselbe Dreieck wurde mit strengem Entwickler-
Opt-in im tatsächlichen SDL-Viewerfenster vor
SDL_GL_CreateContext gerastert, fünf sichtbare Pixel
verifiziert und danach OpenGL bis zum Login/Exit 0 gestartet.

**20 umgesetzte und verifizierte technische Teilaufgaben:**

01. Vor Entwicklungsbeginn Branch, sauberen Git-Stand, AGENTS.md, Handoff, Status und P05-Roadmap verifiziert.
02. Bestehende OpenSim-Dreieck-ABI und bereits GPU-gestagte Vulkan-Vertex-/Indexbuffer untersucht.
03. Vorhandenen lokal entpackten Khronos-glslang- und SPIR-V-Validator ohne Installation benutzt.
04. GLSL-450-Vertexshader für ein eindeutig als Clip-Space markiertes Testdreieck erstellt.
05. GLSL-450-Fragmentshader für sichtbare rote Rasterfarbe erstellt.
06. Beide Shader mit glslang für Vulkan 1.0 in echte SPIR-V-Module übersetzt.
07. Vertex- und Fragment-SPIR-V per Khronos spirv-val erfolgreich validiert.
08. Shader-SPIR-V in bestehendes Vulkan-Modul eingebettet, ohne zusätzliche Projektdateien oder Laufzeit-Compiler.
09. Bytegenauen Vergleich der eingebetteten Shaderdaten gegen geprüfte SPIR-V-Binärdateien bestanden.
10. Rasterspezifisches Entwickler-Opt-in OGL_VULKAN_EXCLUSIVE_RASTER_PROBE=1 implementiert.
11. Vor vkAcquireNextImageKHR fehlende Mesh-/Vertex-/Indexbuffer und ungültige Dreieckanzahl kontrolliert abgewiesen.
12. Neue VkShaderModule-, VkPipelineLayout-, VkGraphicsPipeline- und Renderbefehle dynamisch geladen.
13. VkShaderModule für beide Shader und VkPipelineLayout in den bestehenden WSI-Renderpass integriert.
14. Vertex-Format R32G32B32_SFLOAT, 12-Byte-Stride und Dreiecklisten-Indexing festgelegt.
15. Rasterizer, Viewport, Scissor, MSAA-1 und RGBA-Blend-Ziel für echte VkGraphicsPipeline eingerichtet.
16. Im Renderpass Vertex-/Indexbuffer an VkCommandBuffer gebunden und realen vkCmdDrawIndexed-Befehl aufgezeichnet.
17. GPU-Fence-/Queue-Synchronisation der WSI beibehalten und Pipeline-/Shader-Ressourcen vor Device-Abbau freigegeben.
18. Nativen X11-GPU-Test um Raster-Opt-in, Ablehnung ohne Mesh, Clip-Space-Dreieck und anschließenden Mesh-Clear erweitert.
19. Echte GPU/X11-Pixel bestätigt: Dreiecksmitte RGB(237,48,38), Hintergrund RGB(20,54,115); SDL-Viewer-GUI mit GL-Login/Exit 0.
20. Linux-Vollbuild, 334 Python-Tests, 41 native CTests und fünf reale SDL-GUI-Fallback/Raster-Fälle erfolgreich geprüft.

**Zusätzliche Prüfergebnisse:** Der native X11-WSI-Test
behält die bereits funktionierenden 16 aufeinanderfolgenden
Swapchain-Presents inklusive Resize und Einzel-Frame-
Schutz. Im neuen Rastermodus wird eine fehlende
hochgeladene Geometrie vor dem Acquire abgewiesen.
Der neue SDL-Entwicklertest prüfte fünf Fälle:
echte Dreiecks-Rasterisierung 1/1, normales Clear 1/1,
fehlender Mesh-Upload 0/1, kein Vulkan-Treiber 0/1
und ausgeschaltete Rasterprobe; alle erreichten
das tatsächliche OpenGL-Login und Exit 0.
Linux-Vollviewer-Build, 334/334 Python-Tests,
41/41 native CTests und Shader-Binärvergleich grün.

**Freigabegrenze:** Die neuen XYZ-Daten sind künstliche
CLIP-SPACE-Testkoordinaten (nur im Entwicklerprobe-Pfad),
KEINE korrekt projizierte OpenSim-Szene.
Die GLSL-Shader demonstrieren eine reale Rasterpipeline,
aber OpenSim-Kamera-/Weltmatrizen, Terrain, Meshmaterialien,
Texturen, Sky, Wasser, Avatare, UI-Compositing und dauerhafter
Vulkan-Welt-Frame-Loop sind weiterhin NICHT integriert.
Der Grafik-Backend-Schalter bleibt auf OpenGL.
P05/3 ist technisch weiter vorbereitet, **P05 formal 2/20,
Block 3 und vollwertiger Block 4 bleiben offen**.
Nächster Gate: echte Agent-/Weltgeometrie über aktuelle
Kameramatrizen projizieren und innerhalb einer exklusiv
Vulkan-eigenen Display-Session rendern; danach produktive
Swapchain-Frames ohne blockierendes vkQueueWaitIdle.
Code-Commit: 410f7d9a80129fa9f86f2e1328480deed597482f.

## P05/3 – 20 technische Arbeitsschritte: fünf GPU-Pixel/Frame und kontrollierte Fehler (05.11.2026)

**Ziel dieser Runde:** Den exklusiven Vulkan-WSI-Entwicklerpfad auf der
echten SDL/X11-Fenster-ID vor OpenGL sicherer machen. Statt nur des
Mittelpixels werden nun fünf verteilte X11-Pixel aus jedem der 20
nacheinander präsentierten GPU-Bilder geprüft (100 Pixel).
Zusätzlich prüft XGetGeometry die tatsächliche Fenstergröße und
kontrollierte Entwickler-Fehlerinjektion den sicheren Abbruch
jeder der drei Swapchain-/Resize-Phasen.

**20 einzeln umgesetzte/überprüfte technische Schritte:**

01. Projektregeln, Hand-off, Status und P05-Roadmap erneut geprüft; origin/ogl-develop und Ausgangsstand sauber.
02. Vorhandenen tatsächlichen SDL/X11-Vulkan-Pre-GL-Frame-Pfad und GL-Initialisierung untersucht.
03. Pre-GL-Vulkan-Startprobe pro Prozess auf einen Versuch begrenzt, um nachträgliche GL-Kontextwechsel auszuschließen.
04. Bei bereits aktivem OpenGL-Kontext weiterhin keine Vulkan-Präsentation erlaubt.
05. Fünf über die Fensterfläche verteilte X11-Pixelpositionen für jedes GPU-Bild definiert.
06. Alle Pixel mit echtem XGetImage/XGetPixel aus dem tatsächlichen Viewerfenster zurückgelesen.
07. X11-Visual-Farbmasken pro Pixel in normale RGB-Werte umgerechnet.
08. Blaue und warme GPU-Clear-Farbwechsel mit vorgegebener Toleranz für jedes Sample validiert.
09. Nach jedem gültigen Vulkan-Frame den realen Zähler geprüfter Bildpunkte fortgeschrieben.
10. Volle 20-Frame-Sequenz nur bei 100 bestätigten X11-Pixelsamples akzeptiert.
11. XGetGeometry als unabhängige Kontrolle der tatsächlichen X11-Fenstergröße hinzugefügt.
12. Fenstergröße vor jedem Swapchain-Attach auf reale X11-Geometrie geprüft.
13. Fenstergröße nach temporärer SDL-Vergrößerung zusätzlich gegen X11-Geometrie geprüft.
14. Zurückgesetzte Originalgröße auch nach abgebrochenen Sequenzen gegen X11-Geometrie geprüft.
15. Explizite entwicklerseitige Fehler-Injektion nach drei echten Frames der Initialphase implementiert.
16. Explizite Fehler-Injektion nach drei Frames der Resize-Phase implementiert.
17. Explizite Fehler-Injektion nach drei Frames der Wiederherstellungsphase implementiert.
18. Reale GUI-Fallback-Nachweise: Initialfehler 3/20, Resizefehler 11/20, Rückkehrfehler 19/20, jeweils GL-Login und Exit 0.
19. Weitere reale GUI-Prüfungen: Normalfall 20/20 mit 100 Pixeln, ungültiger Vulkan-ICD 0/20, Einmalmodus 1/1 und deaktivierte Probe, jeweils GL-Login/Exit 0.
20. Linux-Vollbuild, 332 Python-Tests und 41 native CTests bestanden; nativer 16/16-Vulkan-Frame-Test weiterhin bestanden.

**Reale Nachweise:** Sieben getrennte isolierte Xvfb-Viewer-GUI-Tests
positiv abgeschlossen: regulär 20/20 echte Vulkan-Presents,
pixel=verified, samples=100, groups=3, resize=verified;
Fehlergruppe 1 nach 3/20 (15 Pixel), Gruppe 2 nach 11/20
(55 Pixel), Gruppe 3 nach 19/20 (95 Pixel), jeweils
pixel=unverified und keine fälschliche Vulkan-Erfolgsmeldung.
Ungültiger Vulkan-ICD: 0/20 und normaler GL-Login; alter
Einmalmodus: 1/1, samples=5; ohne Probe: kein Vulkan-Present.
Alle sieben erreichen STATE_LOGIN_WAIT und Exit 0.
Vollständiger Linux-Viewer-Build, 332/332 Python-Tests,
41/41 native CTests und separate 16/16 native WSI-GPU-
Präsentationsprüfung erfolgreich. Keine Nutzerprozesse
oder persönlichen Benutzerdateien geändert.

**Grenze / formale Zählung:** Die Vulkan-Testbilder werden
auf der echten SDL/X11-Window-ID ausschließlich vor
SDL_GL_CreateContext präsentiert und bleiben farbige Testframes,
kein Terrain-/Mesh-/Avatar- oder UI-World-Rendering.
Kein Live-Vulkan-Enginewechsel. Native GPU-Probe enthält noch
vkQueueWaitIdle und ist deshalb nicht für den
produktiven interaktiven Frame-Loop freigegeben.
Wayland/Windows/macOS sowie dauerhafte exklusive SDL-Engine-
Ownership bleiben offen. **P05 operativ weiterhin 2/20,
Block 3 nicht freigegeben.** Nächster Schwerpunkt:
eigentlicher Vulkan-Draw-Pipelineaufbau (Vertex-/Index-Buffer
mit Raster-/Shader-Pipeline, nicht nur Clear), danach
echte Welt-Renderframes und sichere GPU-Frame-Synchronisation.
Code-Commit: 2578754857a8e9328cc8376205a3daec50485eb1.

## P05/3 – 20 technische Entwicklungsschritte: SDL-Window-Resize, 20 echte Vulkan-Frames (05.11.2026)

**Technisches Ergebnis:** Eine ausdrücklich freigegebene
Entwicklerdiagnose präsentiert im tatsächlichen SDL/X11-Fenster
des OGL-Viewers insgesamt 20 echte Vulkan-Frames über drei
Swapchain-Lebenszyklen: 8 Frames bei Originalgröße,
8 nach Fenstergrößenänderung (+64x48), 4 nach Rückkehr zur
Originalgröße. Vulkan-Detach/Modulstop und vollständiger
Ressourcenabbau erfolgen vor der normalen OpenGL-Initialisierung.
Jeder präsentierte Frame wird am tatsächlichen X11-Fensterpixel
und am GPU-Present-Zähler geprüft.

**Abgeschlossene technische Teilaufgaben dieser Arbeitsrunde:**

01. AGENTS.md, Handoff, Projektstatus und aktive P05-Roadmap vor Bearbeitung gelesen; Arbeitsbranch und Status geprüft.
02. Exklusives SDL_WINDOW_OPENGL-Window-Ownership vor GL-Kontexterzeugung erneut kontrolliert.
03. Bereits vorhandene SDL-X11-Pre-GL-Vulkan-Probe direkt erweitert; keine parallele Projektimplementierung angelegt.
04. OGL_VULKAN_SDL_PREGL_SEQUENCE_PROBE=1 als zusätzlichen Entwickler-Opt-in-Gate umgesetzt.
05. OGL_VULKAN_EXCLUSIVE_FRAME_LOOP=1 als zweite Pflichtfreigabe für Multiframe-WSI gefordert.
06. Bestehenden normalen Einmal-Present-Modus bei fehlender Sequenzfreigabe beibehalten.
07. Echte SDL_GetWindowWMInfo-Display- und Window-ID auf allen drei Sequenzphasen wiederverwendet.
08. Vorhandene Grafik-/Present-ABI mit echtem Swapchain-Bildzähler erstmals pro Frame im SDL-Viewer ausgewertet.
09. Erste Vulkan-Sequenz mit acht tatsächlich präsentierten Frames in Originalgröße durchgeführt.
10. Swapchain vor dem Größenwechsel zerstört; kein Vulkan-Image-Handle über SDL_SetWindowSize hinweg gehalten.
11. Temporäre Fenstergröße um 64x48 erhöht und nach SDL/X11-Sync auf tatsächliche Größe geprüft.
12. Zweite Sequenz mit acht real präsentierten Vulkan-Frames auf neuer Swapchain durchgeführt.
13. Originalgröße unabhängig vom Sequenz-Erfolg wiederhergestellt und gegen SDL-Fenstergröße geprüft.
14. Dritte Sequenz mit vier Vulkan-Frames nach erneuter Swapchain-Erstellung durchgeführt.
15. Für jedes der 20 GPU-Bilder X11-Frontbuffer-Pixel in nativer Visual-Farbmaske geprüft.
16. Abwechselnde RGB-Farben mit Toleranz geprüft, GPU-Present-Zähler je Attachment validiert und danach zurückgesetzt.
17. Alle Vulkan-Surface-/Device-/Modul-Ressourcen vor SDL_GL_CreateContext freigegeben; GL-Welt-Renderer unverändert.
18. Echter isolierter Linux-Xvfb-Viewer: 20/20 GPU-Presents, Pixel-/Resize-Validierung, GL-Login-Wait und Exit 0.
19. Fallbacks: Einmal-Modus 1/1, ungültiger Vulkan-ICD 0/20, deaktivierte Hauptfreigabe ohne Probe; GL-Login und Exit 0.
20. Linux-Vollviewer-Build, 331 Python-Tests, 41 native CTests und separater 16-Frame-GPU-Test bestanden.

**Nachweise (Linux Mint / reale Vulkan-GPU):**
OGL-GUI-Xvfb-Log: 20/20 presented, pixel=verified,
resize=verified, surface=ready; anschließend
World backend: OpenGL (actual), STATE_LOGIN_WAIT und
regulärer Ctrl+Q-Exit 0. Drei separate negative
GUI-Läufe: ohne Sequenzflag 1/1, mit fehlendem ICD 0/20
und ohne Hauptflag keine Probe, jeweils GL-Login/Exit 0.
Der unabhängige native GPU-Test bestätigt 16/16 echte
Swapchain-Frames; ungültiger ICD ergibt erwartet Exit 12.
Vollständiger Linux-Build, 331/331 Python-Tests und
41/41 native CTests erfolgreich. Keine Änderungen an
Benutzerprofil, Zugangsdaten oder Cache.

**Wichtige Freigabegrenze:** Die 20 Frames sind farbige
GPU-Testbilder VOR SDL_GL_CreateContext, keine OpenSim-
Terrain-/Mesh-/Avatar-/Weltpixel. In der 3D-Welt ist
OpenGL weiterhin der einzige aktive Renderer. Die
native Diagnose verwendet vkQueueWaitIdle und ist
deshalb ausdrücklich NICHT als laufender produktiver
Vulkan-Frame-Loop freigegeben. Wayland/Windows/macOS
und persistentes Live-Window-Ownership sind offen.
P05 operativ weiterhin **2/20**, Block 3 offen.
Nächste P05-Aufgabe: dauerhaftes exklusives
SDL-Window-/Renderer-Ownership ohne gleichzeitigen
GL/Vulkan-Present, sichere Frame-/Resize-/
GPU-Error-Lifecycle-Integration und OpenGL-Fallback.
Code-Commit: aa71c8dd12b16b0f7ae9fc086ce2ef831942fa27.

## P05/3 – 20er-Runde: Vulkan-Present im echten SDL-Viewerfenster (05.11.2026)

**Ergebnis:** Mit vier ausdrücklichen Entwicklerflags kann Vulkan
vor SDL_GL_CreateContext ein sichtbares Bild im ECHTEN SDL/X11-
Viewerfenster präsentieren. Nach überprüftem X11-Pixel wurde
Vulkan vollständig abgebaut und OpenGL normal gestartet.
Dies ist kein produktiver Rendererwechsel oder OpenSim-Vulkan-World-Draw.

**20 erledigte technische Teilaufgaben und Prüfgates:**

01. AGENTS.md, Handoff, Status und P05-Roadmap gelesen; Branch und Ausgangsbaum geprüft.
02. Echten SDL-Viewerfenster-Erzeugungsort in LLWindowSDL::createContext gefunden.
03. SDL_WINDOW_OPENGL und SDL_GL_CreateContext-Reihenfolge untersucht.
04. Vor-GL-Entwicklerprobe zwischen SDL_CreateWindow und SDL_GL_CreateContext ergänzt.
05. OGL-Branding sowie LL_LINUX/LL_X11 als Plattformgrenzen eingesetzt.
06. Vier voneinander unabhängige Entwickler-Opt-ins, exakt Wert 1, verlangt.
07. Vulkan-Präsentation bei bereits aktivem GL-Kontext gesperrt.
08. Echte SDL_GetWindowWMInfo-X11-Display- und Window-Handles übernommen.
09. Ohne echte X11-Fenster-Handles auf OpenGL zurückgeführt.
10. Vorhandenes OGL-Vulkan-Modul über SDL_LoadObject und C-ABI verwendet.
11. Modul-Start über versionierte ogl_host_api und Log-Callback abgesichert.
12. Echte VkSurfaceKHR und VkSwapchainKHR auf der tatsächlichen SDL-Window-XID aufgebaut.
13. Echtes vkAcquireNextImageKHR/Renderpass/vkQueuePresentKHR vor GL-Kontext ausgeführt.
14. Per XGetImage/XGetPixel echte SDL/X11-Fensterbildpunkte gelesen.
15. Vulkan-Farbe RGB(20,54,115) nach X11-Farbmasken innerhalb Toleranz validiert.
16. Vulkan-Detach, Modulstop und SDL_UnloadObject vor SDL_GL_CreateContext umgesetzt.
17. Positiver Real-GUI-Test: 1/1 GPU-Bild, pixel=verified, danach OpenGL-Login und Exit 0.
18. Negativer Real-GUI-Test: fehlender Entwicklerflag und ungültiger Flagwert.
19. Negative Real-GUI-Tests: fehlender Device-Bootstrap und ungültiger Vulkan-ICD.
20. Linux-Vollbuild, 330 Python-/41 native Tests und vorheriger 16-Frame-GPU-Test bestanden.

**Reale Prüfergebnisse:** SDL/X11-Pre-GL-Vulkan-Present 1/1,
Pixel verified, Surface ready; danach normales OpenGL-Welt-
Backend und STATE_LOGIN_WAIT, Ctrl+Q Exit 0 im isolierten
Xvfb-Viewer. Vier negative GUI-Läufe (fehlender Flag,
ungültiger Flagwert, fehlender Bootstrap, ungültiger
Vulkan-ICD) jeweils mit GL-Login und Exit 0.
Ungültiger ICD meldet 0/1 Vulkan-Present. Linux-Vollbuild,
330/330 Python-Tests, 41/41 CTests und der native
16/16-Frame-GPU-Present-Test wurden bestanden.

**Grenze:** Vulkan präsentiert VOR dem GL-Kontext, niemals
gleichzeitig mit der OpenGL-Welt. Der Diagnosepfad verwendet
vkQueueWaitIdle und bleibt deshalb ausschließlich per
Entwickler-Opt-in erlaubt, nicht als Produktiv-Frame-Loop.
Eine permanente auswählbare Vulkan-Weltengine, Terrain-/Mesh-/
Textur-/Avatar-/UI-Darstellung, echter Live-Engine-Wechsel und
Windows/macOS/Wayland-Abnahmen fehlen. **P05 bleibt 2/20,
Block 3 offen** bis zur dauerhaften exklusiven SDL-Window-
Ownership mit Frame-/Resize-Lifecycle und GL-Fallback.
Code-Commit 7c533cc68d6879d3e95f31e6e2f01076118c9245.

## P05/3 – 20 technische Arbeitsblöcke: Vulkan-Multiframe und sichtbare GPU-Pixel (05.11.2026)

**Aktueller operativer Stand: P05 2/20, Block 3 weiterhin offen.**
Diese Runde umfasst 20 echte Teilaufgaben/Tests zur Frame-Synchronisation,
nicht den Abschluss der 20 operativen Vulkan-Weltphasen-Blöcke.
Code-Commit 1428a4d986b0b71ca74828344b7b3db4059429cc.

01. AGENTS.md, Handoff, Status und P05-Roadmap gelesen; korrekter Branch und sauberer Ausgangsbaum geprüft.
02. Aktuelles SDL_WINDOW_OPENGL-/SDL_GL_CreateContext-Ownership analysiert; keinen Vulkan-GL-Parallelbetrieb aktiviert.
03. Eigenen Entwicklerflag OGL_VULKAN_EXCLUSIVE_FRAME_LOOP=1 nur im isolierten nativen X11-Test eingebaut.
04. Bisherige Einmal-Present-Policy bleibt ohne Multi-Frame-Flag bestehen.
05. WSI-Zähler exclusive_present_count in bestehender WorldGraphicsDevice-Struktur ergänzt.
06. Read-only C-ABI ogl_vulkan_world_exclusive_present_count() für überprüfbare GPU-Präsentationen ergänzt.
07. Swapchain-Detach setzt Zähler und Einmal-Present-Schutz auf null zurück.
08. Multi-Frame-Limit hart auf 16 echte erfolgreich abgeschlossene Präsentationen begrenzt.
09. Unzulässiger 17. Present auf identischer Swapchain wird mit 0 abgewiesen.
10. GPU-Fehlersperre bleibt bestehen: Fehlversuche ohne erfolgreiche Präsentation erhöhen den Zähler nicht.
11. Vulkan-Clear-Farben wechseln je erfolgreichem Bild zwischen Blau und Warmton.
12. Echte vkAcquireNextImageKHR / vkQueueSubmit / Fence / vkQueuePresentKHR für 16 Frames nacheinander durchlaufen.
13. Fehlerhärtung: Auch nach Present-Fehler werden Queue-Referenzen vor Freigabe mit isoliertem QA-Queue-Wait behandelt.
14. XGetImage liest nach jedem Frame das sichtbare 1x1-Frontbuffer-Pixel aus dem echten X11-Testfenster.
15. XGetPixel/Farbmasken-Normalisierung prüft 16 wechselnde RGB-Werte mit begrenzter Toleranz.
16. Xvfb-GPU-Nachweis: Frame 1 RGB(20,54,115), Frame 2 und 16 RGB(166,89,31); 16/16 Bilder gültig.
17. Vorhandene Fenster-Resizes 320x240 -> 480x300 und erneuter Attach 640x360 mit 16 Frames geprüft.
18. Nach 256x192-Neuaufbau wiederhergestelltes Einmal-Present bestätigt; erneuter Versuch und Zähler nach Detach abgewiesen.
19. Ungültiger Vulkan-ICD reproduzierbar Exit 12; 329/329 Python-Tests, 41/41 native CTests und Linux-Vollviewer-Build grün.
20. Zwei isolierte OGL-GUI-Läufe: Xvfb-Login-Wait, OpenGL tatsächlich aktiv und Exit 0 trotz aller Entwicklerflags bzw. ungültigem ICD.

**Mess-/Testnachweise:** 16 aufeinanderfolgende real präsentierte
VkSwapchainKHR-Bilder auf ein und derselben nativen X11-Swapchain.
Jedes GPU-Bild wurde über XGetImage und RGB-Masken des X11-Visuals
pixelweise geprüft (16/16); Abwechseln der Bildfarbe bestätigt.
GPU-/Present-/Fence-Fehlerbehandlung erweitert, Kontingent 16,
17. Versuch korrekt abgelehnt. Nach Window-Resize und neuer Swapchain
setzt der Present-Zähler zurück, die Einmal-Frame-Policy ist wiederhergestellt.
Dies findet auf einer von der nativen QA eigens erzeugten
X11-Window-ID ohne OpenGL-Kontext statt. Keine echte OpenSim-Welt
oder Vulkan-Pixel im tatsächlichen SDL-Viewer behaupten.

**Regressionen:** Vollständiger Linux-Vollviewer-Build,
329 Python-Tests und 41 native CTests bestanden.
Im echten isolierten OGL-Viewer auf Xvfb bestätigten zwei Starts
mit Vulkan-Entwicklerflags einerseits und ungültigem Vulkan-ICD
andererseits jeweils Login-Wait, tatsächlich aktives OpenGL und
sauberen Ctrl+Q-Shutdown mit Exit-Code 0. Die präsentierende
native C-ABI ist NICHT im Viewer-Host aufgerufen.
GitHub-CI- und Windows/macOS-Vollviewer-Gates getrennt führen;
letztere bleiben P13.

**Nächster P05/3-Schritt:** Exklusives Ownership/Engine-Handoff des
tatsächlichen SDL-Viewerfensters: kein Vulkan-Present, solange dessen
OpenGL-Kontext die Welt zeichnet; Vulkan-Frame-Lifecycle, Resize,
GPU-Fehler und Fallback im realen Viewerfenster nachweisen.
Danach erst P05/4 Terrain-/Mesh-World-Draw. Keine produktive
Vulkan-Engine, Raytracing-Darstellung oder Grid-Live-Abnahme
ohne tatsächlichen Nachweis als fertig markieren.

## P05/3 – 20 technische Arbeits-/Prüfschritte: natives Vulkan-WSI-Present (05.11.2026)

**Fachliche Einordnung:** 20 überprüfte technische Teilaufgaben dieser Arbeitsrunde,
**NICHT** 20 abgeschlossene operative P05-Phasenblöcke. Die Entwicklung verbleibt
bei P05 **2/20**; operativ ist **Block 3** weiter offen. Alle Änderungen im
bestehenden Quellbaum, keine neuen Projektdateien, kein Eingriff in Nutzerprofile,
keine Löschung von Upstream-Lizenzen oder vermeintlich verwaisten Ressourcen.

01. Richtige GitHub-Quelle und Branch ogl-develop verifiziert; Ausgangsbaum sauber.
02. Passive OpenGL-/Vulkan-WSI-Grenze und SDL-Window-Ownership geprüft.
03. Opt-in OGL_VULKAN_EXCLUSIVE_PRESENT_PROBE nur für separaten nativen Test ergänzt.
04. Explizites gl_released=1 als erforderliches Test-Owner-Token umgesetzt; 0 wird abgewiesen.
05. Xlib/WSI-Code nur unter Linux/X11, portable Stub-Funktion für andere Systeme.
06. Vorhandene Present-Queue, Surface-Caps, FIFO-Format-/Extent-Wahl beibehalten.
07. Swapchain-Images mit Anzahlprüfung 2..16 real abgefragt.
08. Echtes vkAcquireNextImageKHR mit begrenztem 2-s-Timeout integriert.
09. ImageView nutzt das gewählte Swapchain-Bild und dessen VkFormat.
10. Vulkan-Renderpass mit Clear, UNDEFINED -> PRESENT_SRC_KHR konfiguriert.
11. Framebuffer verwendet tatsächliche Swapchain-Extent; kein festes 4K/HD-Profil.
12. Primärer Command Buffer: Renderpass starten, Color-Clear, Renderpass beenden.
13. Acquire-Semaphore wird erst nach erfolgreicher Erstellung für Image-Ready genutzt.
14. Graphics Queue wartet in COLOR_ATTACHMENT_OUTPUT auf Acquire und signalisiert Render-Fertig-Semaphore.
15. GPU-Submission mit VkFence, maximal zwei Sekunden vkWaitForFences.
16. Echtes vkQueuePresentKHR mit korrekt ausgewähltem Image-Index ausgeführt.
17. Nur im separaten nativen QA-Prozess Queue-Idle vor Resource-Cleanup; äußerer Test-Timeout.
18. Nach X11-Resize 320x240 -> 480x300 Swapchain neu erstellt und zweites Bild präsentiert; Wiederholung ohne Reattach abgelehnt.
19. Negativgates bestätigt: Owner 0, fehlendes Flag und ungültiger Vulkan-ICD dürfen nicht bestehen; 328 Python- und 41 CTests grün.
20. Vollständiger Linux-Vollviewer-Build 7.2.5.229; isolierter Xvfb-Viewer bis Login-Wait mit OpenGL und persistentem WSI, Ctrl+Q/Exit 0; Code-Commit e492f8be.

**Echte GPU-Nachweise:** Zwei erfolgreiche Swapchain-Presents auf dem
isolierten X11-Testfenster, jeweils als echte Vulkan-GPU-Writes;
Negativfall ohne Vulkan-ICD Exit 12. Das separate Testfenster wird nur
von frame_bridge_test.cpp für den QA-Lauf erzeugt, NICHT vom Viewer.
Linux-Paket und voller Viewer-Build bestanden, 328/328 Python-Tests,
41/41 CTests; GL-Viewer-GUI mit echtem OGL-Fenster 1024x680
(Xvfb 1280x720), GL-Backend und WSI-Attach, Login-Wait,
Ctrl+Q Shutdown Exit 0. Windows-/macOS-Vollviewer und GPU-Present
auf diesen Plattformen sind NICHT getestet; deren Volltests bleiben P13.

**Sicherheitsgrenze:** Dieses einmalige native Vulkan-Present läuft auf
einem *eigens erzeugten* X11-Fenster OHNE OpenGL-Kontext. Der echte
SDL/OpenGL-Viewer ruft die neue Präsentations-ABI nicht auf. Deshalb
existiert noch KEIN exklusiver Vulkan-Fenster-Renderer im Viewer,
keine 3D-OpenSim-Terrain-/Mesh-/Avatar-/UI-Darstellung mit Vulkan.
Der native Test verwendet für Present-Abschluss vkQueueWaitIdle,
aber ausschließlich im extern zeitbegrenzten QA-Prozess, niemals
in der interaktiven GL-Hauptschleife. Bei GPU-Fence-Timeout werden
in-flight Ressourcen nicht vorzeitig freigegeben.
**Nächster Schritt: /goal P05 Block 3** – GL/WSI-Fensterbesitz im
tatsächlichen SDL-Viewer sicher umschalten, Vulkan-Frame-/Resize-Lifecycle
im Viewer integrieren und echte präsentierte Bilder dort prüfen.
Dabei bleibt OpenGL-Fallback verpflichtend. P05/4-Welt-Draw folgt
erst nach bestandenem Block-3-Gate.
**Code-Commit:** e492f8beb5547ad73579a04803b7f91ada0a76a0 auf ogl-develop.

## Wartungsfix: Emoji-Daten im Chat wieder gefunden (04.11.2026)

Echter Fehler in OGL.log: Emoji file characters not found.
Ursache: Linux-Entwicklungsbuild sucht Skins in
indra/newview/skins, während die Emoji-Daten ausschließlich
in den paketierten App-Ressourcen unter
skins/default/xui/<Sprache>/emoji_characters.xml liegen.
Die Emoji-Buttons in Nearby/IM-Chat, Emoji-Schrift und
der aktivierte Standardwert FSShowEmojiButton waren vorhanden.

Korrektur in der bestehenden llemojidictionary.cpp:
Nur falls die reguläre Skin-Suche keine Emoji-Zeichendatei
liefert, wird die bereits vorhandene App-Ressource in der
validierten aktiven Sprache, ansonsten Englisch, geladen.
Normale Skin-Overrides bleiben unverändert. Kein neues
Projektfile, kein Eingriff in Nutzereinstellungen oder Cache.
Regression im bestehenden test_nexverse_chat_layout.py.

Nachweise: 322 Python-Tests bestanden, Linux-Vollviewer
erfolgreich gebaut und paketiert. Zwei echte, isolierte
Prelogin-GUI-Läufe bei 1280x720 in DE und EN laden
jeweils ihre Emoji-Datei aus dem Paket, keine Fehlermeldung,
Login-Wait erreicht, Exit 0. Die Emoji-Eingabe im
authentifizierten In-World-Chat ist noch nicht separat geprüft.
Ein bereits laufender Viewer benötigt Neustart.

Prozessprüfung: aktuell eine aktive nicht isolierte
User-Viewer-Instanz statt der früher gemeldeten elf;
diese wurde nicht gestoppt.

## Aktiver /goal-Ausführungsstand (maßgeblich)

**Aktiver Block:** /goal P05 Block 3/20 (Vulkan-Frame-Lifecycle und Synchronisation, später native Present-Integration).
P03 Block 16 abgeschlossen: Deutsch/
Englisch, 100/140 % UI-Skalierung,
korrigierte Farbvorschau und
Chat-/Menü-Farbzuordnung. Code-Commit
1f174bc7 auf origin/ogl-develop.
P03 Blöcke 9–15: 73565590.
Der erste native Import ist weiterhin
auf lokale, explizit ausgewählte,
unsignierte manifest-only-Farbpakete
begrenzt. Nativer GUI-Installations-,
Entfernungs- und Manipulationstest
in P03/17 erfolgreich abgeschlossen
(Code 59bc92d3, Linux 7.2.5.210).
Kryptografische Signaturen und
zusätzliche Bild-Assets bleiben
explizit außerhalb dieses
sicherheitsbegrenzten Importumfangs.
P01 unter Linux 20/20 abgeschlossen nach
Nutzerbestätigung am 02.11.2026: Live-Login,
3D-Welt, sauberer Shutdown und gespeicherte
Zugangsdaten funktionieren. Diese Abnahme
beruht auf der ausdrücklichen Nutzerprüfung;
der Assistent hat nur Prelogin und Linux-Build
eigenständig getestet. Windows/macOS bleiben
für spätere Plattform-Release-Gates in P13 offen;
Beta-TLS-Zertifikat ist P08 zugeordnet.
P02 als Linux-Code-/Regressionsphase
20/20 abgeschlossen, authentifizierte
Chat-/Voice-In-World-Gates weiterhin
in P13 offen.

Nach technischer Fertigstellung einer Phase sofort
Abschlussmeldung mit Commit/Tests und, soweit im laufenden
Arbeitsauftrag möglich, direkter Start der Folgephase.
Ohne laufenden Chat findet keine Hintergrundentwicklung statt.

| Phase | Status | Fortschritt | Nächster Schritt |
| --- | --- | --- | --- |
| P01 – Branding und modernes Login | Linux abgeschlossen | 20/20 | Windows/macOS vor Release in P13 testen |
| P02 – OGL-Navigation, Fenster, Chat und Avatare | Linux-Code/Tests abgeschlossen | 20/20 | In-World-GUI noch in P13 end-to-end prüfen |
| P03 – Theme-System und sichere Designinstallation | Linux-Code/QA abgeschlossen | 20/20 | Vollviewer-Windows/macOS, Signaturen und Asset-Erweiterungen bleiben separate Gates |
| P04 – Renderer-Abstraktion und OpenGL-Fallback | Linux-Code/portable Module abgeschlossen | 20/20 | Vulkan-Vollrenderer erst P05, Windows/macOS-Full-Viewer P13 |
| P05 – Vulkan als vollständige auswählbare 3D-Engine | aktiv | 2/20 | Block 3: Vulkan-Frame-Lifecycle, Fence-/Semaphore-/Present-Synchronisation und Resize prüfen |
| P06 – Vulkan-Raytracing und Grafikqualität | offen | 0/20 | nach Abschluss der Vorphase |
| P07 – CPU/GPU-Leistung, Textur-, Mesh- und Avatarcache | offen | 0/20 | nach Abschluss der Vorphase |
| P08 – Beta-Protokoll und getrennte NexVerse-Anmeldung | offen | 0/20 | nach Abschluss der Vorphase |
| P09 – Abgesicherter Plugin-Manager | offen | 0/20 | nach Abschluss der Vorphase |
| P10 – Automatisierte signierte GitHub-Releases | offen | 0/20 | nach Abschluss der Vorphase |
| P11 – Clientseitiger Pflicht-Updater und Update-UI | offen | 0/20 | nach Abschluss der Vorphase |
| P12 – Serverseitige Mindestversion für beide Grids | offen | 0/20 | nach Abschluss der Vorphase |
| P13 – Portal-Releases, End-to-End-QA und Dauerbereinigung | offen | 0/20 | nach Abschluss der Vorphase |

## P05 Block 3 – realer Vulkan-Offscreen-Renderpass mit GPU-Pixelnachweis (05.11.2026)

Weiterarbeit am bestehenden Vulkan-Modul (ogl_rtx_module.cpp,
ogl_rtx_api.h), Host (oglrtxhost.cpp), nativem
frame_bridge_test.cpp und test_public_graphics.py.
Unter explizitem Entwicklerflag
OGL_VULKAN_OFFSCREEN_FRAME_PROBE=1 und vorhandenem
Vulkan-Device erstellt das Modul ein 32x32 RGBA8-
VkImage, VkDeviceMemory, VkImageView, VkRenderPass
und VkFramebuffer, zeichnet per Graphics-Renderpass
eine Clear-Farbe, führt die Image-/Transfer-/Host-
Memory-Barriers aus, kopiert in einen VkBuffer und
liest nach GPU-Fence-Fertigstellung die Daten zurück.
Erst bei erfolgreicher Pixelprüfung an drei
Bildpositionen meldet die neue optionale C-ABI-
Funktion ogl_vulkan_world_render_offscreen_probe()
Erfolg (1), sonst 0. Zwei direkt aufeinanderfolgende
native GPU-Frames erfolgreich. GPU-Wait auf 2 s
begrenzt; im Timeoutfall keine unsichere Freigabe
noch aktiver Vulkan-Commands. Es werden weder
Swapchain-Bilder noch GL-Fenster-/Weltpixel angefasst.

**Linux-Nachweise:** Vollviewer neu kompiliert/gelinkt
und gepackt. 327/327 Python-Tests und 41/41
native CTests bestanden, inklusive native ABI-/GPU-
Bildnachweis und Test mit ungültigem Vulkan-ICD.
Drei echte isolierte Xvfb-DE-GUI-Starts (1280x720):
GPU-Fall 1/1 verifiziertes Offscreen-Bild, ungültiger
Treiber 0/1, Standardstart ohne Entwicklerflag keine
Offscreen-Aktivierung. Alle drei mit OpenGL-Backend,
Login-Wait und Exit 0. Keine Änderung an
Nutzerprofilen, Cache oder aktiven Nutzerprozessen.
Plattformübergreifender CI-Lauf für diesen Commit
erst nach Push auswertbar; kein Windows/macOS-
Vollviewer-Login behauptet.

**Zusätzliche GPU-Pflichtprüfung (05.11.2026):** Der vorhandene
native `ogl_rtx_frame_bridge_test` akzeptiert jetzt optional
`OGL_REQUIRE_VULKAN_OFFSCREEN_FRAME=1`. Auf einem für den
Test vorgesehenen Vulkan-GPU-Rechner führt `device!=1` oder
ein fehlender GPU-Readback-Erfolg damit zu Exit-Code 11,
statt als optionales 0-Ergebnis still durchzugehen.
Auf dem Linux-Mint-Rechner: expliziter Hardwaretest PASS;
mit ungültigem Vulkan-ICD erwarteter Exit-Code 11.
Portabler Standard ohne Pflichtflag bleibt grün.
327/327 Python-Tests und 41/41 native CTests erneut bestanden.
Das ist zusätzliche Testabsicherung innerhalb P05/3,
kein Vulkan-Swapchain-Present und keine neue Blockfreigabe.

**Abnahmegrenze:** Noch kein Vulkan-Swapchain-Acquire,
vkQueuePresentKHR, vollständiger World-Draw,
Textur-/Mesh-/Avatar-Pipeline oder anwählbarer
Vulkan-Renderer. Dies ist ein ECHTER offscreen
GPU-Renderpass, nicht die sichtbare NexVerse-Welt.
P05 formal weiterhin 2/20, Block 3 offen.
Nächster konkreter Schritt: exklusives Vulkan-
Fenster-/Present-Ownership statt gleichzeitiger
Präsentation in das SDL-OpenGL-Fenster; danach
vollständige Frame-Synchronisation und World-Draw.

## P05 Block 4/20 – realer GPU-Vertex-/Indexbuffer-Stagingpfad; Welt-Draw OFFEN (04.11.2026)

Die bereits vorhandenen Dateien ogl_rtx_module.cpp, ogl_rtx_api.h,
oglrtxhost.cpp/.h und pipeline.cpp wurden für eine getrennte,
nicht-Raytracing-gebundene Vulkan-Geometrieablage erweitert.
Über OGL_VULKAN_WORLD_BOOTSTRAP=1 und
OGL_VULKAN_WORLD_MESH_UPLOAD=1 kopiert
ogl_vulkan_world_upload_triangles() die validierten,
auf maximal 8192 begrenzten bestehenden OpenSim-
Dreiecke in echte VkBuffer mit VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
bzw. VK_BUFFER_USAGE_INDEX_BUFFER_BIT und gebundenem
HOST_VISIBLE|HOST_COHERENT VkDeviceMemory. Sequentielle
32-Bit-Indizes werden für drei Eckpunkte pro Dreieck erzeugt.
Fehlerhafte/übergroße Pakete werden abgelehnt, ohne die
vorherige GPU-Szene zu überschreiben. count=0, Modulstop
und Device-Neustart geben GPU-Speicher vollständig frei.
Das bisherige RTX-Verhalten bleibt unabhängig und unverändert.
Der bestehende CPU-Szenencollector ist nun explizit auch
auf nicht-RTX-Vulkan-Geräten mit Opt-in aktivierbar.
Eine zusätzliche Leermengenprüfung schützt vor front()
auf einer noch leeren OpenSim-Region.

**Real nachgewiesen:** VkBuffer/VkDeviceMemory auf
Vulkan-Gerät für 1 Dreieck und 8192 Dreiecke mit
24576 Vertices, Ablehnung von NaN und >8192,
Zählersicherheit und kompletter Cleanup. Native CTest
mit/ohne Vulkan-ICD erfolgreich. Linux-Vollviewer
erneut kompiliert/gelinkt, 326/326 Python-Tests und
41/41 native CTests bestanden, isolierter DE-GUI-
Start mit Opt-in, OpenGL aktiv, Login-Wait und Exit 0.

**Nicht behaupten:** Im anonymen Prelogin-GUI-Smoke
gab es noch keine echte OpenSim-Szene; der End-to-End-
Upload aus angemeldeter 3D-Welt wurde deshalb NICHT
nachgewiesen. Es gibt noch keine GPU Draw Calls,
VkRenderPass/Viewport/Shader, keinen Vulkan-Present
und keine Anzeige von Meshes/Terrain/Avataren.
P05 verbleibt formal bei 2/20; Block 3 Present
weiter offen, Block 4 ist verifizierte Vorarbeit.

## P05 Block 3/20 – echte GPU-Queue-/Fence-Prüfung; Vulkan-Present weiterhin OFFEN (04.11.2026)

Der bestehende Vulkan-Modulcode enthält nun einen
expliziten, nicht nutzerseitig aktivierten Test für
OGL_VULKAN_FRAME_SYNC_PROBE=1. Das reale VkDevice
erzeugt VkCommandPool, ZWEI unabhängige primäre
Command Buffers und VkFences. Vulkan vkQueueSubmit
reicht beide Befehle ein; vkWaitForFences prüft
deren GPU-Fertigstellung jeweils mit einem
begrenzten 2-Sekunden-Wait. Fehler sperren weitere
Diagnose-Submissions und blockieren nicht per
unbegrenztem vkQueueWaitIdle den GL-Viewer.
Keine Weltobjekte, uninitialisierten Swapchain-Pixel,
Bild-Acquires oder Vulkan-Presents werden verwendet.
Die C-ABI-Funktion ogl_vulkan_world_submit_sync_probe
liefert nur 2 für zwei echte fertige GPU-Submissions,
ansonsten 0. Die OpenGL-Engine bleibt unverändert.

**Realer Nachweis:** Isolierter nativer Modul-/GPU-
Test 2/2, ohne gesetztes Opt-in 0/2 und mit
absichtlich ungültigem Vulkan-Treiber 0/2.
Zwei tatsächliche Linux-Viewerstarts auf
eigenem Xvfb bei GPU 2/2 und invalidem ICD 0/2
mit OpenGL-Backend, Login-Wait und Exit 0.
Linux-Vollviewer neu kompiliert und gepackt.
325 Python-Tests / 41 native CTests bestanden,
inklusive zusätzlichem Native-Test mit und ohne
gültige Vulkan-GPU sowie Python-Vertragsprüfung.

**P05-Fortschritt bleibt ausdrücklich 2/20.**
Block 3 benötigt noch echten Swapchain-Acquire,
Vulkan-Bildaufbau/Renderpass, GPU-Frame-Semaphore-
Synchronisation und vkQueuePresentKHR. Nur
Zwei-Fence-Submissions sind nicht gleichzusetzen
mit einem gerenderten Vulkan-Weltframe.

## P05 Block 2/20 – persistente Vulkan-Swapchain, SDL-Resize und Shutdown (04.11.2026)

Die bestehenden llwindowsdl2.cpp/.h und oglrtxhost.cpp erhalten
optionale synchrone WindowWillDestroy- und Resize-Callbacks.
Die WSI wird vor SDL_QuitSubSystem(SDL_INIT_VIDEO) freigegeben.
Direkt nach SDL_WINDOWEVENT_RESIZED wird die Vulkan-
Surface/Swapchain mit der echten X11-ID und dem neuen
SDL-Fenstermaß erneuert; zusätzlich eine ID/Größenprüfung
beim späteren Welt-Frame. Standardbetrieb bleibt OpenGL.
Dauerbetrieb nur bei drei expliziten Entwicklerflags:
OGL_VULKAN_WORLD_BOOTSTRAP=1,
OGL_VULKAN_WINDOW_SURFACE=1,
OGL_VULKAN_WINDOW_PERSISTENT=1.
Ohne das dritte Flag bleibt der einmalige P05/1-Probe aktiv.

**Realer GUI-Nachweis:** Eigenes Xvfb-Display,
echter OGL-Login, persistente VkSurfaceKHR/VkSwapchainKHR,
Fenster zuerst 1230x820, danach 1050x680; für BEIDE
Resize-Ereignisse swapchain=ready im Log. Login-Wait,
aktive OpenGL-Engine, sauberer Exit 0. Keine weiteren
Viewer-Prozesse oder Benutzerdaten geändert.
Linux-Vollviewer gebaut und gelinkt; 324/324 Python-
und 41/41 native CTests erfolgreich.

**Phasengrenze:** P05 nun 2/20, Block 3 als nächster.
Noch KEIN Vulkan-Bild-Acquire, vkQueuePresentKHR,
Vulkan-Weltframe, Terrain-/Avatar-Renderer oder
Vulkan-Engine-Auswahl. Nur Linux/X11-WSI vorhanden;
Wayland, Windows und macOS brauchen eigene Anbindung.

## P05 Block 1/20 – echter Vulkan-WSI-Probe am SDL-Viewerfenster (04.11.2026)

P05/1 abgeschlossen. Bestehende Dateien oglrtxhost.cpp,
ogl_rtx_module.cpp und ogl_rtx_api.h erweitert: Das zuvor
herstellerneutral erzeugte VkInstance/VkDevice mit Grafik-VkQueue
kann mit ZWEI expliziten Entwicklerflags
OGL_VULKAN_WORLD_BOOTSTRAP=1 und OGL_VULKAN_WINDOW_SURFACE=1
die Instanz-Erweiterungen VK_KHR_surface und VK_KHR_xlib_surface
prüfen und aktivieren. Der Host übergibt die ECHTE
SDL/X11-Display-/Window-ID an das Modul, kein zweites Fenster.
Das Modul erzeugt VkSurfaceKHR, validiert über
vkGetPhysicalDeviceSurfaceSupportKHR dieselbe Grafik-Queue
als Presentqueue, prüft Surface-Formate, FIFO-Modus,
Größen, Bildanzahl und Transform/Alpha/Usage-Grenzen,
erstellt VkSwapchainKHR, prüft deren Bildanzahl und
zerstört Swapchain/Surface direkt vor dem ersten
normalen OpenGL-Frame. Vulkan-Bildübernahme und
vkQueuePresentKHR finden ausdrücklich NICHT statt.

Reale Linux-GUI-Abnahme am isolierten Xvfb-Display:
OGL Viewer Development 7.2.5.219,
Log "P05 real SDL/X11 Vulkan surface and swapchain probe passed;
handles released before OpenGL rendering, no Vulkan present".
Das Log bestätigt weiterhin "World backend: OpenGL (actual)",
Login-Wait und Exit 0. Der WSI-Erfolg wurde auf
isolierter Mesa-/Xvfb-Laufzeit geprüft, NICHT als RTX-
4070-SUPER- oder authentifizierter OpenSim-Welt-Test.
Weitere GUI-Läufe: kein Flag -> kein Vulkan-WSI;
absichtlich falscher Vulkan-ICD -> explizite
WSI-unavailable-Meldung, OpenGL Login/Exit weiterhin OK.
323 Python-Tests und 41 native CTests grün,
inklusive nativer Negativ-Tests für Null-Fenster und
Module-Stop. Der Linux-Viewer wurde vollständig kompiliert
und gelinkt. Kein Benutzerprofil/Cache verändert,
vorhandener User-Viewer nicht beendet.

**Abnahmegrenze P05/2:** Bisher ist dies eine absichtlich
KURZLEBIGE Vulkan-Surface/Swapchain-Probe am realen
SDL/OpenGL-Fenster. Eine dauerhafte Vulkan-Framepipeline
erfordert eigenen Vulkan-Fenster-/Present-Lebenszyklus,
vkAcquireNextImageKHR, vkQueuePresentKHR, Resize,
Synchronisation und später Terrain/Mesh/Avatare.
Diese Funktionen sind nicht implementiert; die auswählbare
Vulkan-Welt-Engine bleibt gesperrt. Nur Linux/X11 ist
als WSI-Probe vorhanden; Wayland, Windows und macOS
brauchen eigene WSI-Pfade. Formal **P05 1/20**,
nächster Block 2: dauerhafte Surface/Swapchain-
Integration mit kontrolliertem OpenGL-Fallback.

## P05 Vulkan-Grafikgerätebootstrap – historischer Vor-WSI-Stand 0/20 (04.11.2026)

Vorarbeit im vorhandenen ogl_rtx_module.cpp sowie ogl_rtx_api.h:
Das explizite Entwickler-Opt-in OGL_VULKAN_WORLD_BOOTSTRAP=1
erstellt eine echte VkInstance, ein persistentes VkDevice
mit VK_QUEUE_GRAPHICS_BIT und eine über vkGetDeviceProcAddr
ermittelte VkQueue. Herstellerneutrale Geräte-/Treiberprüfung,
VK_KHR_swapchain-Fähigkeit und vollständige Freigabe bei
Fehler, wiederholtem Start oder Modul-Shutdown sind enthalten.
Die C-ABI-Funktion ogl_vulkan_world_device_bootstrapped()
meldet nur diese Gerätebereitstellung, niemals Vulkan-Weltframes
oder Raytracing. Ohne Opt-in bleibt die Funktion abgeschaltet.

Echte Linux-Nachweise: VkDevice/VkQueue erstellt, zweimal
neu initialisiert und sauber freigegeben; absichtlich
ungültiges Vulkan-Treiber-ICD korrekt abgefangen.
Vollständiger Linux-Viewer gebaut und gelinkt,
321/321 Python-Tests und 41/41 native CTests grün.
Zwei isolierte 1280x720-Xvfb-Prelogin-Viewerstarts,
Opt-in aus und an: jeweils OpenGL aktive Weltengine,
Login-Wait erreicht und sauberer Exit 0.

P05 Block 2 vorgeprüft: Der bestehende LLWindowSDL
erstellt SDL_WINDOW_OPENGL und einen GL-Kontext.
Es fehlen SDL-Vulkan-Window-Handoff, VkSurfaceKHR,
ein für die konkrete Surface bestätigtes Present-Queue-Family,
VkSwapchainKHR und Vulkan-Welt-Frames. Eine bloße
VK_KHR_swapchain-Extension gilt nicht als Present-Fähigkeit.
Der nächste echte technische Schritt ist der abgesicherte
alternative Vulkan-Window-/Context-Lebenszyklus mit GL-Fallback,
nicht ein zweites Testfenster oder eine Schein-Engine.
Es wurde kein Login-, Cache-, Benutzer- oder Secret-Profil
verändert. Windows/macOS-Vollviewer und Installer nicht
bestätigt.

**Phasengate unverändert:** P05 0/20; Block 1 ist
eine geprüfte Teilimplementierung, nicht ein fertiger
Vulkan-Welt-Renderer. Block 2 mit realer Surface/Swapchain
steht aus. Keine 20 P05-Blöcke als erledigt ausgeben.

## P05 Blöcke 1–2/20 – historischer Befund vor realem SDL/X11-WSI-Test (04.11.2026)

Historischer Befund vor dem neuen Vulkan-Bootstrap: Das optionale Vulkan-/RTX-Diagnosemodul in
ogl/modules/rtx/ogl_rtx_module.cpp erstellt nur eine kurzlebige
VkInstance, enumeriert Physical Devices und RT-Extensions und
zerstört diese Instanz sofort. Es besitzt KEIN persistentes,
dem realen Viewer-World-Renderer zugeordnetes VkDevice/VkQueue.
Der separate vulkan_blas_smoke.cpp hat zwar ein nachweisbares
vkCreateInstance/vkCreateDevice/vkGetDeviceQueue und räumt wieder
auf, ist aber Testhardwarediagnose und kein Weltbackend.

P05/2 analysiert: Kein VkSurfaceKHR, kein VkSwapchainKHR und kein
SDL_Vulkan_CreateSurface in der produktiven Vulkan-Modul-/Host-
Integration. Die bestehende SDL-Viewer-Window-Implementierung
besitzt SDL_Window und einen OpenGL-Kontext. Eine echte Vulkan-
Engine benötigt eigene, sichere Instance/PhysicalDevice/LogicalDevice/
PresentQueue-Initialisierung, Window-Surface-Übergabe, Swapchain-
Erstellung und klar geregelte Kontext-/Thread-/Resize-/Shutdown-
Übergänge auf allen Zielplattformen. Bis dahin bleibt Vulkan
im Grafiktab deaktiviert. KEINE P05-Fortschrittsblöcke als
implementiert bestätigt; P05 weiterhin 0/20.

## P04 Blöcke 18–20/20 – Portable CI, Cleanup und Phasenabschluss (04.11.2026)

**18:** GitHub Actions OGL portable graphics Run 41901011126
für Code-Commit ec52fd45 ist komplett SUCCESS:
Linux ubuntu-24.04 x64 und ubuntu-24.04-arm,
Windows windows-2022 x64 und windows-11-arm,
macOS macos-15-intel x64 und macos-15 ARM64.
Die sechs Jobs testen portable Module und Quellverträge,
KEINEN vollständigen Firestorm-GUI-Viewer.
Windows-x64-Vollviewer-Installer Job war SKIPPED, also
kein erfolgreiches Windows-Setup vorliegend.

**19:** Source-Referenzen geprüft: OGL_RTX_DEBUG_OVERLAY,
OGL_RTX_DEPTH_GUIDED_PROBE und OGL_RTX_SHADOW_VISUALIZE
werden real in llviewerdisplay.cpp, pipeline.cpp und
dem RTX-Modul verwendet. Keine unbewiesene Löschung.

**20:** Linux-Viewer 7.2.5.219, 319/319 Python-Tests,
41/41 native CTests, 7 C++-Fallback-Szenarien und
Vulkan-Request-zu-OpenGL-Laufzeittest bestanden;
Code-Commit ec52fd45 veröffentlicht. Dieser Status,
Handoff und Hauptroadmap synchronisiert. P04 operativ
als GL-Fallback-/Backend-Abstraktionsphase 20/20 geschlossen.
P05 beginnt ohne manuelle Nutzerabnahme.

**Klare Grenzen:** Kein Vulkan-Swapchain-/World-Renderer
oder Live-Enginewechsel vorhanden. Vulkan-Auswahl bleibt
absichtlich deaktiviert bis echte Vulkan-Weltpipeline/
Present-Device-Sicherheit implementiert sind. Keine
Windows/macOS-Full-Viewer-GUI-/Install-Abnahme. Keine
vorgegebene echte Raytracing-Weltbeleuchtung.

## P04 Blöcke 3–17/20 – OpenGL-Weltpfad, Backendauswahl und Fallback (04.11.2026)

Code ec52fd45 auf origin/ogl-develop. Bestehende llviewerdisplay,
oglrtxhost, llappviewer, llfloaterpreference, settings.xml und
Grafik-XUI EN/DE erweitert; keine parallelen Rendererdateien.

3: Echte display()-Weltframe-Funktion an OpenGL-Backendvertrag
gebunden; alle realen Welt-/UI-/Deferred-Pipeline-Aufrufe bleiben.
4: GL-World-/Present-Besitz bei Firestorm, Vulkan-Diagnose beim
optionalen Modul, Lifecycle-Logs bei Viewerstart und -shutdown.
5: Plattformneutrales GL/Vulkan-Welt-Capability-Snapshot.
6: GL-Treiber, erkannte/effektive VRAM-Menge, Texture-Budget
und konkrete Fallback-Ursache getrennt protokolliert.
7: OpenGL bleibt Standard und Kompatibilitätsmodus.
8: Neuer Renderer-Reiter im vorhandenen EN/DE-Grafikpanel.
9: Vulkan-Eintrag sichtbar, aber mangels kompletter Weltpipeline
ausdrücklich nicht auswählbar; keine GPU-Marken-Whitelist.
10: Gewünschte und zuletzt nach echter GL-Presentation erfolgreiche
Engine getrennt persistent, letzter Erfolg erst nach swapBuffers.
11: Neustart-Anforderung für spätere Engine-Umschaltung vermerkt;
kein Live-Wechsel möglich, solange Vulkan unvollständig ist.
12: Im realen Linux-Viewer eine manuell angeforderte Vulkan-Engine
getestet: OpenGL bleibt aktiv, Log nennt fehlende Weltpipeline.
13: Bestehende GL-Grafikprofile, Quality- und User-Einstellungen
nicht überschrieben.
14: Einmalige, klare Backend-/Fallback-Logs statt Frame-/Login-Spam.
15: 319/319 Python-Tests plus 7 C++20-Hardware-/Fallback-Mocks.
16: Intel/AMD/NVIDIA sowie x86/ARM nicht fest verdrahtet.
17: Linux OpenSim Vollviewer 7.2.5.219 gebaut/gelinkt,
41/41 native CTests, EN/DE-XUI und settings.xml paketidentisch.
Deutscher Prelogin-GUI-Start mit Renderer-Reiter Exit 0.
GPU-Diagnose nach realem Screenshot zweizeilig/korrigiert.
Kein authentifizierter Live-3D-GUI-Vulkan-Test behauptet.

Block 18: Portable GitHub-Grafikmatrix läuft; Modul-Jobs
sind nicht gleich Windows/macOS-Vollviewer-Installer.
Block 19: Alt-Diagnosehooks OGL_RTX_DEBUG_OVERLAY,
OGL_RTX_DEPTH_GUIDED_PROBE und OGL_RTX_SHADOW_VISUALIZE
nach Abhängigkeitsprüfung real verwendet, nicht blind gelöscht.
Block 20: Erst nach CI-/Cleanup-Gate schließen und P05 beginnen.
Windows-Vollviewer-Setup weiter nicht bestätigt.

## P04 Block 2/20 – Backend-Schnittstelle (04.11.2026)

In `indra/newview/oglrtxhost.h` ist ein gemeinsamer, constexpr-fähiger Entscheidungsvertrag hinzugekommen: `OGLWorldBackend`, `OGLWorldFallback` und `ogl_choose_world_backend()`. Vulkan als Welt-Renderer benötigt gleichzeitig eine vollständige Welt-/Present-Pipeline und ein nutzbares Gerät. Die vorhandene RTX/GPU-Diagnose allein reicht ausdrücklich nicht aus. `ogl_current_world_backend()` bleibt OpenGL; es wird weder eine Engine umgeschaltet noch ein Nutzersetting geschrieben. GL/LLPipeline behalten Welt, Fenster, Swap und UI; RTX besitzt nur Diagnoseressourcen. Keine zusätzlichen Rendererdateien angelegt. C++20-Vertragsprobe (5 Fälle), Linux-Vollviewer 7.2.5.218 seriell kompiliert/gelinkt, 315 Python- und 41 native Tests erfolgreich. Ein erster paralleler Build traf auf X11-Makrokollision `None`, danach auf Objektdateikollision durch überlappende Builds; `NoFallback` und serieller Rebuild beheben beides. Ein isolierter Prelogin-Smoke betraf noch die zuvor paketierte 7.2.5.214 und ist kein neuer 7.2.5.218-GUI-Nachweis. Windows/macOS-Vollviewer und echter Vulkan-Weltpfad weiterhin offen. **P04 2/20, nächster Block 3:** vorhandene Frame-Schnittstellen anbinden, OpenGL erhalten.

## P04 Block 1/20 – OpenGL-/Vulkan-Renderer-Inventur (04.11.2026)

**Commit c2aade42 auf origin/ogl-develop.**
Die eigentliche 3D-Darstellung
liegt weiterhin bei
indra/newview/llviewerdisplay.cpp
und LLPipeline in
indra/newview/pipeline.cpp.
Wichtige reale Renderaufrufe:
gPipeline.renderGeomDeferred,
gPipeline.renderDeferredLighting,
gPipeline.renderFinalize. OpenGL-
Status, Shader und Vertexbuffer
bleiben in indra/llrender/llgl.cpp,
llvertexbuffer.cpp,
llviewershadermgr.cpp und
dem bisherigen Pipeline-Code.
Grafiksettings enthalten
RenderMaxOpenGLVersion;
keine auswählbare Engine
in den bisherigen Preferences.

Optionale Vulkan-/RTX-Diagnose:
indra/newview/llappviewer.cpp
ruft ogl_init_rtx_module bei
Start und ogl_shutdown_rtx_module
beim Ende. oglrtxhost.cpp lädt
nur Linux/Windows gebündeltes
ogl_rtx.so bzw. ogl_rtx.dll
und prüft ID, ABI und Version.
Mac-Host aktuell Stub.
ogl/modules/rtx/ogl_rtx_module.cpp
prüft Vulkan-Systembibliothek,
GPU-Fähigkeiten, sammelt
Kamera-/Szene-/Shadowdaten;
das zweite Modul
ogl_gpu_probe liefert
Diagnosedaten. CMake staged
beide Module. Vulkan-
Hardwareerkennung ist
NICHT ein aktivierter
vollwertiger Vulkan-
3D-Weltrenderer:
OpenGL bleibt Standard
und aktiver Weltpfad.
Experimentelle Vulkan-
Schattenmasken können nur
bei explizitem opt-in
in einzelne OpenGL-
Targets eingeblendet
werden. Sie ersetzen
nicht die vollständige
Renderer-Pipeline.

Neue Baseline-Regression
in vorhandenem
ogl/tests/test_public_graphics.py:
prüft GL-Frame-Aufrufe,
Modulstart und -shutdown,
Module-Bundle/ABI,
Diagnose-Flags und
nicht vorhandene Engine-
Umschaltung. **314/314
Python-Tests bestanden**.
P03-Vollviewer 7.2.5.214
und 41/41 CTests
waren zuvor erfolgreich;
P04/1 hat keinen
neuen C++-Renderer
erstellt. Windows/macOS
Full-Viewer-Installer
weiter offen.

**P04 jetzt 1/20;
weiter Block 2/20:**
Backend-Vertrag,
Lebenszyklus und
sicheres OpenGL-
Fallback spezifizieren,
ohne Debugmodule
als Engine auszugeben.

## P03 Blöcke 19–20/20 – Linux-Build, portable Theme-Sicherheit und Übergang P04 (04.11.2026)

**Block 19 erfolgreich:**
Vollständiger Linux-
Viewer-Build 7.2.5.214
mit OpenSim bestanden.
313/313 Python-Tests,
41/41 native CTests
grün. Theme-CLI-
Sicherheitsuite 12/12
und Theme-/Viewer-
Quellvertrag 19/19.
XUI-EN/DE-Daten
im Viewer-Paket
bytegleich. Echte
Linux-GUI-Smokes:
DE 1440x900 bei
100 % UI-Skalierung,
EN 1920x1080 bei
140 %, Preferences
und OGL Designs
sichtbar, jeweils
sauberer Exit 0.
Native Theme-Import-/
Remove-/Negativ-E2E-
Tests aus P03/17
waren zuvor mit
Version 7.2.5.210
erfolgreich.

Die OGL-portable-
graphics-GitHub-CI
run 41891918909
für Commit 6ff4cbbc
wurde als Erfolg
bestätigt: sechs
native Grafikjobs
Linux x64/ARM,
Windows x64/ARM
und macOS x64/ARM
grün. Der separate
Windows-x64-
Vollviewer-Installer-
Job wurde SKIPPED,
also kein gebauter
oder installierter
Windows-Viewer
nachgewiesen.
Auch macOS-
Vollviewer-GUI/
Grid-Login wurde
nicht geprüft.

**Block 20:** Relevante
GitHub-Commits,
Handoff, Hauptroadmap
und Sicherheitsstatus
synchronisiert und
Folgephase P04
ohne manuelle
Abnahme gestartet.

**P03 operativ 20/20
als Linux-Code- und
Theme-Sicherheits-
Phase abgeschlossen.**
Unterstützter
nativer Theme-Import
bleibt absichtlich
lokal, unsigned und
auf ein geprüftes
Farbmanifest
theme.json beschränkt.
Kein Code/Plugins,
keine Bild-/Skin-
Dateien, keine
Signatur-Vertrauens-
infrastruktur.
Diese Funktionen
sind NICHT als
vollständig umgesetzt
oder releasefreigegeben
zu betrachten.
Plattformweite
Vollviewer-/
Installer- und
In-World-Gates
bleiben ausdrücklich
im P13-Release-
und späteren
Härtungsplan.

**Aktiver Einstieg:
P04 Block 1/20.**

## P03 Block 18/20 – Theme-Duplikate und Skin-Overrides (04.11.2026, abgeschlossen)

**Commit 854e8a2d auf origin/ogl-develop.**
Acht getrackte Viewer-Skins,
22596 getrackte Skin-Dateien
und OGL-/Upstream-Theme-
Referenzen inventarisiert.
Für das OGL-Design-Panel
existieren genau zwei
notwendige Quelldateien:
default/xui/en/
panel_preferences_ogl_themes.xml
(Hauptlayout) und
default/xui/de/
panel_preferences_ogl_themes.xml
(Sprachüberlagerung).
In den anderen sieben
Skins existiert kein
paralleles OGL-Theme-
Panel. Im EN-Preferences-
XML eine zentrale
Theme-Panel-Injektion,
im DE-XML genau eine
OGL-Lokalisierung.
Zwei Theme-Beispiele
mit unterschiedlichen IDs.
Kein überflüssiger
OGL-Theme-Override
oder mehrfaches Template
nachgewiesen.

Es wurden 303 Gruppen
bytegleicher XML-/Plugin-
Daten gefunden. Dies
sind überwiegend
Upstream-Sprachdateien
und eigenständige
Kompatibilitäts-Skins.
Dateinamen wie
script_copy.png,
Badge_Background_New.png
und client_list_v2.xml
sind keine belegten
OGL-Template-Backups.
Ohne Verwendungsnachweis
wurden diese Dateien
nicht blind gelöscht.

Der vorhandene Test
ogl/tests/test_viewer_theme_goal.py
enthält jetzt einen
Vertrag für genau
EN-Hauptlayout,
DE-Sprachüberlagerung,
einmalige Preferences-
Referenzen, einzigartige
Example-IDs und das
Verbot paralleler
OGL-Theme-Panels in
anderen Skins.
313/313 Python-Tests
bestanden, EN/DE-XUI
mit Buildpaket
byteidentisch.

**P03 Stand 18/20.**
Nächster Block 19:
vollständiger Linux-
Viewer-Build,  native
CTest-Regressionen
und CI-/Theme-Security-
Gates. Windows/macOS-
Release-Abnahmen
weiter offen.

## P03 Block 17/20 – Native ZIP-Installation, Entfernen und Negativpakete (04.11.2026, ABGESCHLOSSEN)

**Code:** Commit 59bc92d3
auf origin/ogl-develop.
**Root Cause:** Die tatsächlich
verlinkte minizip-ng-Bibliothek
liefert bei normalen ZIP-Dateien
für entry->linkname einen
nicht-null Zeiger auf die
leere Zeichenkette. Die
bisherige Prüfung
entry->linkname || hat
auch gültige ZIPs abgewiesen.
Die korrigierte Prüfung
unterscheidet leere von
nichtleeren Link-Zielen;
der zusätzliche Unix-
Modus-Symlink-/Sonderdatei-
Guard bleibt aktiv. Mit
einem isolierten C++-Probe-
Programm gegen exakt die
im Viewer verlinkte
libminizip.a nachgewiesen.

**Echter Linux-Viewer GUI-E2E:**
7.2.5.210, Xvfb 1440x900,
separate XDG_DATA_HOME und
XDG_STATE_HOME, echter
GTK-LLFilePicker mit
Dateisystem-/Ort-Eingabe,
expliziter Installations-
Button und anschließender
Dateisystemprüfung. Keine
produktiven Nutzerdateien
berührt. Der valide
Ocean-Blue-Test (exakt
theme.json) wurde im
Benutzer-Themes-Verzeichnis
unter de.nexverse.theme.ocean-blue
atomisch installiert,
der kanonische Manifest-ID-
Wert geprüft, ohne das
Theme zu aktivieren.
Anschließend per echter
Entfernen-Schaltfläche
gelöscht; Ordner/Manifest
danach nicht mehr vorhanden.
Viewer sauber mit Exit 0.

**Native Negativfälle über
denselben GUI-Pfad:** Jedes
Paket wurde zuerst im
Dateidialog erfolgreich
ausgewählt (Status-Screenshots
verifiziert), dann durch den
Installations-Button abgelehnt:
(1) Zusatzdatei im ZIP,
(2) theme.json als
symbolischer Link markiert,
(3) ../theme.json als
Pfadtraversal,
(4) doppelte JSON-
Property type. In allen
vier Fällen nach dem
Ablehnungsstatus:
keine installierte
theme.json, keine
.ogltheme-Staging-
Verzeichnisse, Exit 0.
Fünf Szenarien insgesamt
(1 erfolgreich, 4
absichtlich abgewiesen).

**Weitere Gates:** Vollständiger
Linux-C++-Build erfolgreich,
312/312 Python-Tests,
41/41 native CTests.
ZIP-Permissions bleiben
lokale, manuell gewählte,
unsignierte Farbmanifeste;
keine Plugins, ausführbaren
Assets oder Netzrechte.
Signaturprüfung und
Windows/macOS-GUI/Full-
Viewer-Release-Abnahmen
weiter offen. Auf GitHub
ist ein Windows-Installer-
Erfolg nicht bestätigt:
Installer-Job bei letzter
CI-Prüfung übersprungen,
macos-15-Grafikjob
abgebrochen. Dies ist
keine Vollplattform-
Freigabe.

**P03 Fortschritt:** 17/20,
nächster Block 18:
nicht benötigte
Skin-Overrides und
Theme-Duplikate im
Bestand inventarisieren
und nur nach belegter
Unbenutztheit bereinigen.
Keine zusätzlichen
parallelen Templates.

## P03 Block 16/20 – Sprache, HiDPI, Farbvorschau und Kompatibilität (04.11.2026)

**Code 1f174bc7 auf origin/ogl-develop.**
Der Built-in-Theme-Name in
refreshOglThemes wird jetzt
über den lokalisierten
XUI-Schlüssel
ogl_theme_default_name
bezogen. EN: "OGL Standard
(default)", DE: "OGL Standard
(Voreinstellung)".
Sechs Textbereiche im
bestehenden EN-XUI folgen
links/rechts, statt auf
starrer Breite zu bleiben;
rechte Action-Schaltflächen
sind rechts verankert.
Die bisher deaktivierten
Farbvorschau-Buttons waren
ausgegraut und kaum lesbar.
Sie wurden in bestehendem
Theme-XUI durch drei nicht
anklickbare Panels mit
tatsächlichem
LLPanel::setBackgroundColor
und eigenständigen
übersetzten Beschriftungen
ersetzt. Die Vorschau
ändert nicht die
LLUIColorTable.
Chat-/Menü-Farb-Tokens
bleiben via bestehender
TOKEN_COLORS und
colors.xml/ChatHeader
abgebildet; vorhandene
Skin-/IM-Fenster wurden
nicht dupliziert.

**Nachweise:** Linux-Vollviewer
7.2.5.207 gebaut; 311/311
Python- und 41/41 native
CTest erfolgreich. EN/DE
Theme-XUI bytegleich im
Viewer-Paket.
Echter GUI-Test auf
separatem Xvfb mit EN
1920x1080 bei UI-Faktor
1,4 sowie DE 1440x900
bei Faktor 1,0. In beiden
Fällen OGL-Designs-Panel
tatsächlich geöffnet,
Aktionselemente
vollständig sichtbar,
Farbfelder lesbar,
lokalisierte Default-
Anzeige korrekt.
Beide Viewer sauber mit
Exit 0 geschlossen; keine
Anmeldung in Live-3D-Welt
oder echte Chatfarben in
einer laufenden
IM-Sitzung geprüft.
Keine alten User-Dateien
verändert, keine neuen
Projektdateien erzeugt.

**P03 jetzt 16/20,
nächster Block 17:**
native ZIP-Import- und
Manipulations-End-to-End-
Tests. Erfolgreicher
Installer-/Windows-GUI-
Test weiterhin offen.

## P03 Blöcke 9–15/20 – Native sichere Farbdesign-Verwaltung (04.11.2026)

**Code:** 73565590 direkt auf
origin/ogl-develop.

**Blocknachweise:**
9. **Paketvertrauen/Berechtigungen:**
   explizite Zustimmung über separate
   Schaltfläche "Installieren";
   ausschließlich lokale, unsigned,
   deklarative Farbdesigns ohne
   XUI, Plugin-/Skriptcode,
   Netzwerk-/OS-Rechte. In EN/DE
   sichtbar als unsigniert
   gekennzeichnet. Keine
   kryptografische Signatur-
   Authentifizierung behaupten:
   deren Einführung/Vertrauens-
   Infrastruktur bleibt offen.
10. **Native atomische Installation:**
    Bereits vorhandene minizip-ng-
    Bibliothek im C++-Viewer,
    ohne Python-/externes Tool.
    VOR Benutzer-Verzeichnisänderung
    das komplette ZIP vorprüfen:
    maximal 16 MiB Gesamtarchiv,
    exakt eine Datei namens
    theme.json (maximal 16 KiB),
    nur Store/Deflate,
    keine Verschlüsselung,
    symlinks, Sonderdateien,
    ZIP-Zusatzdateien, hohe
    Kompressionsrate,
    falsche CRC, doppelte
    JSON-Properties oder
    ungültige Manifestdaten.
    Manifest wird kanonisiert,
    in zufälligem neu angelegtem
    Staging-Verzeichnis geschrieben,
    danach per fs::rename auf
    endgültiges ID-Verzeichnis
    veröffentlicht. Keine
    vorhandene Installation
    überschreiben; temporäre
    Dateien bei Fehler entfernen.
    Nur reine Farb-Manifeste
    derzeit nativ importierbar.
    PNG/WebP-Assets müssen
    später gesondert sicher
    nativ unterstützt werden.
11. **Installierte Themes:**
    bestehende Listendarstellung
    refreshOglThemes() und
    Validierung der installierten
    Manifeste wiederverwendet.
    Liste wird nach Import erneuert.
12. **Vorschau:** Bestehendes
    Farb-Swatch-Panel nutzt
    previewOglTheme(), welche
    ausschließlich Darstellung
    ändert, nicht die
    LLUIColorTable speichert.
    Neu importiertes Theme
    wird ausgewählt und als
    Vorschau angezeigt.
13. **Status und Neustart:**
    bestehende Hinweise zur
    Farb-Aktivierung/Neustart;
    EN/DE zusätzliche eindeutige
    Status für Auswahl,
    Installation, Ablehnung
    und Entfernung.
14. **Aktivierung/Zurücksetzen:**
    bisherige onOglThemeActivate
    und onOglThemeReset,
    snapshot_original und
    Restore des Nutzer-Farbprofils
    erhalten. Installation
    aktiviert NICHT automatisch.
15. **Sicheres Entfernen:**
    neuer "Entfernen"-Button
    in EN/DE, nur für inaktive
    geprüfte manifest-only
    Pakete. Aktive oder
    veränderte/zusätzliche
    Dateien werden abgewiesen.
    Kein rekursives Entfernen
    und kein Löschen fremder
    Nutzerinhalte.

**Nachweise:** Linux-Vollviewer
7.2.5.204 in C++20 gebaut;
308/308 Python-Tests,
41/41 native CTests bestanden.
Neue EN/DE-XUI-Dateien
bytegleich im Viewer-Paket,
jeweils Auswahl/Installieren/
Entfernen/Aktivieren/Reset
in XUI einzeln geprüft.
DE-Xvfb-GUI 1440×900:
Preferences öffnet, OGL-
Designs-Reiter und alle
neuen Schaltflächen
tatsächlich sichtbar,
nativer GTK-Dateiauswahl-
Dialog öffnet, Viewer
stabil mit sauberem Exit.
DE 1280×720 und
EN 1920×1080: Prelogin-
GUI-Smokes Exit 0.
Paketsicherheitsvertrags-
Tests prüfen C++-Quellcode;
existierende Python-
Theme-Import-/Rollback-/
Manipulationsprüfungen
bestanden. Ein tatsächlicher
Import/Entfernen-Klick mit
gültigem ZIP innerhalb
des Viewer-Dialogs wurde
NICHT end-to-end verifiziert.
Kein bestehendes reales
User-Theme oder Login
verändert.

**P03-Fortschritt:** 15/20
als umgesetzte/verifizierte
Linux-Codepfade. Nächster
Block 16: DE/EN, HiDPI,
Chat-/Menü-Integration.
17: nativer Manipulations-
und erfolgreicher Import-
E2E-Test. 18: Overrides/
Duplikate geprüft bereinigen.
19: plattformneutrale
Sicherheitstests und
Linux-/CI-Gates. 20:
Phasenabschluss erst
bei erfüllten Gates.
Windows-/macOS-Vollviewer
und signierte Release-
Verteilung weiterhin
nicht bestätigt.

## P03 Blöcke 1–8/20 – OGL-Standardfarben und sichere Theme-Auswahl (04.11.2026)

Code c4a615ba auf origin/ogl-develop.
Acht einzeln bearbeitete oder überprüfte
technische Schritte ohne neue Paralleltemplates:

1. Bestehende Theme-Funktionen in
   llfloaterpreference.cpp/h, EN/DE-
   XUI, validate_theme.py und
   manage_themes.py inventarisiert.
2. OGL Standard als sichtbaren Ausgangspunkt
   im Theme-Panel bezeichnet; fünf
   zentrale Farbwerte in bestehendem
   colors.xml für OGL-NexVerse
   ergänzt und die vorhandenen
   Emphasis-/Scheme-/Selection-
   Palettenfarben referenziert.
3. Vorhandenes original-palette.json,
   Sicherung der User-Farben,
   Reset und optionales Classic-
   Design als Fallback bewahrt.
4. C++-Manifestprüfung durch
   strengen SemVer-Vertrag
   major.minor.patch ergänzt;
   Versionslängenlimit in Python
   identisch auf 32 gesetzt.
5. Bisherige zentrale Farbtabelle
   TOKEN_COLORS in der Preview
   statt mehrfach hartcodierter
   Farbnamen verwendet.
6. Theme-Layout nur in bestehenden
   EN-Basis- und DE-Overlay-XUI
   angepasst; originale Skins-Seite
   bleibt erhalten. Keine Kopien.
7. Nativen LLFilePicker für
   .ogltheme-Auswahl hinzugefügt:
   C++20 UTF-8-Dateiname, nur
   reguläre lokale Dateien,
   keine Symlinks, maximal
   16 MiB. Ein ausgewähltes Paket
   wird ausdrücklich **nicht**
   installiert oder ausgeführt;
   native Installation bleibt
   P03/9–10 und späteren
   Sicherheitsschritten vorbehalten.
8. Bestehenden Python-ZIP-Validator
   gegen FIFOs, Sockets, Links,
   andere Spezialdateien, unbekannte
   Kompression, doppelte JSON-
   Eigenschaften, komprimierte
   ZIP-Übergrößen, Case-Kollisionen,
   Traversal und SemVer-Überlängen
   weiter gehärtet. Tests ergänzt.

**Abnahmen:** Linux-Vollviewer
7.2.5.202 einschließlich C++20-
Theme-Dialog erfolgreich gebaut.
306/306 Python-Tests, 41/41
native CTests bestanden. colors.xml,
EN/DE-Theme-XUI (3/3) bytegleich
im Viewer-Paket. DE-/EN-GUI-
Prelogin-Smokes mit Exit 0.
OGL-Ocean-Blue-Beispielpaket
im isolierten /tmp-Nutzerprofil
validiert, installiert, gelistet,
aktiviert, zurückgesetzt und
entfernt (kein echtes Userprofil
berührt). Eine frühere isolierte
CLI-Test-ID war zunächst falsch,
der gesamte Lauf mit der
Manifest-ID wurde danach
erfolgreich wiederholt.

**Abnahmegrenzen:** Native
Dateiauswahl ist noch keine
native ZIP-Validierung/
Installation. Keine echte
In-World-Theme-Vorschau
oder Windows-/macOS-
Viewer-Tests durchgeführt.
P01 Linux 20/20, P02 Linux-
Code/Regression 20/20,
P03 8/20. Nächster Schritt
P03/9: Paketvertrauen/
Berechtigungsmodell und
Sicherheitsgrenze vor
atomischem Native-Import.
Windows-Installer noch kein
downloadbar bestätigtes Setup.

## P02 Blöcke 9–20/20 – Chat-Fokus, UI-Gleichlauf, Prüfung und Abschluss (04.11.2026)

**Code-Commit 68e50e2b** auf origin/ogl-develop.
In FSFloaterIM::onFocusLost wird der aktive
Sitzungszustand nur zurückgesetzt, wenn
die aktuell aktive Session tatsächlich die
fokusverlierende Session ist. onFocusReceived
übernimmt aktive Session, Chiclet und
Mention-Picker für gehostete IMs nur bei
tatsächlich sichtbarem, nicht minimiertem,
ausgewähltem Fenster. Für losgelöste
Einzel-/Gruppenfenster bleibt die
Standalone-Nutzung erhalten.
Das asynchrone P2P-Namens-Callback
ignoriert fremde Empfänger-IDs.
Das bereits verwendete IM-Toast-XUI
panel_instant_message.xml hat keinen
fest codierten Demobenutzernamen mehr,
und der Avatar wird rund gerendert.

**12 Einzelblock-Nachweise, ohne
jeweils neue Parallel-Dateien:**
- 09: IM-Fokus, aktive Sitzung,
  Nachrichteneingabe/Unread geprüft
  und in C++ tatsächlich korrigiert.
- 10: Gruppen- und Einzelchat
  verwenden dieselbe FSChatHistory-
  Nachrichtenstruktur (inkl.
  is_p2p-Kontext) – im Bestand geprüft.
- 11: Reale runde Avatar-Icons
  in Chat-Header und IM-Toast
  geprüft, Toast-Icon korrigiert.
- 12: Absendername und Uhrzeit
  im Chat-Header zur Laufzeit
  gebunden, keine Demotexte.
- 13: Nachrichtenfeld/-bereich
  mit wrap, is_expandable und
  rechter/unterer Layoutbindung
  als responsive Basis geprüft.
- 14: Kontakte und aktive
  Sitzungsnavigation inklusive
  Tab-Reihenfolge und Unread-
  Markierungen geprüft.
- 15: RLVa-Avatar-Namensschutz
  und fremde Avatar-Cache-IDs
  geprüft, letzteres korrigiert.
- 16: Voice-Status/Call-Buttons
  und Gruppen-Sprecherliste
  im Bestand erhalten/geprüft.
- 17: DE/EN-IM-Texte,
  Nachrichteneingabe und
  vorhandene Tastatur-/Fokus-
  Bedienung geprüft.
- 18: Aktiven, sichtbaren
  Test-Placeholder aus IM-Toast
  entfernt, weitere historische
  XUI-/script_copy-Dateien
  ohne Abhängigkeitsprüfung
  ausdrücklich nicht gelöscht.
- 19: Linux-Vollviewer
  Version 7.2.5.200 gebaut;
  299/299 Python-Tests, davon
  145 Branding-/UI-Tests,
  und 41/41 native CTests
  bestanden. Zwei DE-/EN-
  Xvfb-Prelogin-GUI-Starts
  mit Exit 0, gepackte
  IM-Toast-Datei bytegleich
  mit Quelle.
- 20: Code auf ogl-develop
  veröffentlicht; Dokumentation
  aktualisiert und Übergang zu
  P03 ohne Nutzerabnahmestopp.

**P02-Abschluss:** 20/20 als
technische Linux-Code-/Quellen-
Regressionsphase nachgewiesen;
**nicht** als vollständige,
authentifizierte In-World-
Funktions-/Voice-/DPI-Abnahme.
Reale Live-GUI, Windows/macOS-
Vollviewer-Installer und
Windows/macOS-GUI gehören zu
P13. Diese externen Gates
werden nicht fingiert.

**P03 eröffnet:** existierender
Theme-Manager in
llfloaterpreference.cpp,
panel_preferences_ogl_themes.xml,
validate_theme.py und manage_themes.py
wird gemäß P03/1–20
weiterentwickelt. Erst die
Sicherheits- und Aktivierungsprüfungen
entscheiden über P03-Blockabschluss.

## P02 Block 8/20 – IM-Fensterwechsel und Sitzungs-/Voice-Status (04.11.2026)

**Code-Commit:** 8116671a direkt auf origin/ogl-develop.

Im vorhandenen FSFloaterIMContainer (C++-Quelldatei
und Header, ohne neue Templates) wurden
mehrere reale Zustandsprobleme behoben:
- Das Öffnen des Sammelfensters
  löscht nicht länger pauschal alle
  mFlashingSessions. Nur eine wirklich
  sichtbare, nicht minimierte,
  aktuell ausgewählte Sitzung gilt
  beim Anzeigen als gelesen.
- Der Tab-Wechsel (tabOpen) löscht
  nur die Markierung der tatsächlich
  sichtbaren Sitzung; andere Einzel-,
  Gruppen- und Ad-hoc-Chats bleiben
  in ihrer ungelesenen Statusmarkierung.
  Der sichtbare Tab-Button-Blinkzustand
  wird gezielt beendet. Automatische
  Tab-Auswahl im versteckten oder
  minimierten Sammelfenster darf keine
  Nachrichten als gelesen markieren.
- Bei einem Session-ID-Update, wie es
  insbesondere für Ad-hoc-Chats
  vorkommen kann, wird neben mSessions
  auch eine ausstehende Blinkmarkierung
  auf die endgültige ID migriert,
  ohne doppelte Einträge anzulegen.
- Der Session-Close-Callback wird
  an das wirkliche Floater-Objekt
  statt eine zum Erstellungszeitpunkt
  eingefrorene Session-ID gebunden.
  Beim Schließen ermittelt er den
  aktuellen Session-Schlüssel über
  die Floater-Identität, bereinigt
  Blinkstatus und Map-Eintrag und
  vermeidet einen unbeabsichtigten
  Eintrag durch mSessions[id].
- Beim Entfernen eines Tabs wird
  ein zugehöriger alter Voice-
  Status-Floater-Zeiger verworfen
  und die nächste Aktualisierung
  gezielt neu angefordert.
- checkFlashing prüft, ob die
  globale Toolbar tatsächlich
  existiert, bevor die Blinkanzeige
  synchronisiert wird.
- Vier neue Regressionstests im
  vorhandenen test_branding_contract.py
  prüfen die geschilderten Übergänge
  in C++-Quelltext und Header.
  Vorhandene P2P-Avatarbilder, Voice-
  Kanalzustände und Gruppen-Chiclets
  bleiben unverändert.

**Prüfnachweise:** Linux-Vollviewer
7.2.5.198 vollständig gebaut,
295/295 Python-Tests, davon
141/141 Branding-/UI-Tests auch
unter LC_ALL=C/PYTHONUTF8=0,
und 41/41 native CTests bestanden.
Isolierte GUI-Smokes DE 1280x720,
EN 1920x1080 bis STATE_LOGIN_WAIT,
beide Viewer mit Exit 0.
Es wurden keine XUI-Dateien
verändert; kein unnötiger
Paketvergleich. Kein automatisierter
Live-Login, kein echter In-World-
IM-Tabwechsel und keine
Gruppen-/Voice-Interaktion
durchgeführt. Nutzer-/Cache-/
Credential-Daten unverändert.
Keine neuen Projekt-/Quell-
oder Backupdateien erzeugt.

**Phasenstand:** P01 Linux 20/20;
P02 8/20. Nächster Einstieg
P02 Block 9/20: Fokus- und
Eingabeverhalten beim Wechsel
zwischen IM-Sitzungen sowie
Unread-Counter prüfen.

**Windows:** Actions-Run 41510587323
hat laut Build-Log NSIS-Paketierung
durchlaufen, aber die anschließende
Setup-Suche/Artefaktbereitstellung
scheiterte. Weiter kein bestätigter
downloadbarer Windows-Installer,
kein Windows-GUI-Test. Release-
Abnahmen für Windows/macOS
bleiben unabhängig in P13 offen.

## P02 Block 7/20 – IM-/Gruppenfenster, Namen und Avatarprofil (04.11.2026)

**Code:** b04c547c, direkt auf origin/ogl-develop.

**Bestandsfehler und Änderungen:**
- Aktive IM-Gesprächsliste panel_fs_activeim_row.xml:
  Der sichtbare Text-Platzhalter "TestString
  PleaseIgnore" wurde entfernt. Der bestehende
  RowPanel-Konstruktor setzt den tatsächlichen
  Sitzungsnamen sofort – unabhängig davon, ob
  es sich um P2P-, Gruppen- oder Ad-hoc-Chats
  handelt. Ein echter Name kann anschließend
  bei P2P-Chat asynchron aus dem Avatar-Cache
  ergänzt werden. Der Tooltip übernimmt
  ebenfalls den realen Namen.
- LLIMWellWindow::RowPanel: Nur für einen
  gültigen Teilnehmer-UUID wird der P2P-
  Avatar-Lookup gestartet. Die ursprüngliche
  Callback-Verbindung wird gespeichert und
  im Zeilen-Destruktor getrennt. Verspätete
  Cache-Antworten mit anderer Teilnehmer-ID
  dürfen die Beschriftung nicht überschreiben.
  Gruppen-/Mehrpersonen-Symbole,
  Unread-Counter und die echte
  Sitzungs-Schließen-Funktion bleiben
  in bestehenden C++-Pfaden erhalten.
- Der bestehende Einzelchat-Kontrollbereich
  panel_fs_im_control_panel.xml verwendet
  ein rundes, quadratisches Avatarbild
  (104x104) statt eines verzerrbaren
  114x105-Bildes. Die Gruppen-
  Sprecherliste in panel_fs_group_control_panel
  bleibt unverändert.
- Drei neue Python-Regressionsprüfungen
  im bestehenden test_branding_contract.py:
  kein Design-Demotext, echter Sitzungsname
  und kontrollierte Cache-Callback-Lebensdauer,
  runder Einzelchat-Avatar bei unveränderter
  Gruppenliste.
- Keine neuen Projektdateien/Paralleldialoge,
  keine Login-/Cache-Nutzerdaten verändert.
  Sieben bekannte _v2/script_copy-Altbestände
  nicht ohne Nutzungsanalyse gelöscht.

**Testnachweise:** Linux-Vollviewer
7.2.5.196 erfolgreich gebaut. 291/291
Python-Tests, darunter 137/137
Branding-/UI-Tests (auch mit
LC_ALL=C/PYTHONUTF8=0), und 41/41
native CTests bestanden. Beide
geänderten EN-XUI-Dateien bytegleich
im Linux-Buildpaket. IM-/Gruppen-
Kontrollpanels und Floater-XUI
erfolgreich als XML validiert.
Isolierte DE-1280x720- und
EN-1920x1080-Login-GUI-Smokes
mit STATE_LOGIN_WAIT ohne AUTH_INIT,
sauberer Exit 0 in beiden Fällen.
Keine authentifizierte IM-/Gruppen-
chat-Interaktion in einer Live-Welt
behaupten.

**Phasenstand:** P01 Linux 20/20;
P02 7/20. Nächster Block:
P02 Block 8/20 – Fensterwechsel,
aktive Sitzung und Statusindikatoren.

**Windows-Installer:** Windows-Vollviewer-
Buildversuch 41510587323 scheiterte
beim Artefakt-/Installer-Verifikations-
schritt. Keine herunterladbare
Windows-Setup.exe bestätigt.
Separate Windows-/macOS-Release-
Gates in P13 bleiben offen.

## P02 Block 6/20 – Lokalisierte Avatar- und Chat-Kopfbereiche (04.11.2026)

**Code:** 8960ecec auf origin/ogl-develop.

Im bestehenden FSChatHistoryHeader und den
bestehenden Sprach-XUI-Dateien wurden
konkrete Inkonsistenzen korrigiert:
- Japanisch: Den im Header-Overlay
  fest hinterlegten Test-Benutzernamen
  entfernt. Der Benutzername wird weiterhin
  ausschließlich durch die bestehende
  dynamische Chat-/Avatar-Namensauflösung
  angezeigt.
- Spanisch: Internen Chat-Header-Label
  soy_titulo auf den kanonischen
  Wert im_header zurückgestellt.
- Erweiterte Chatansicht: Die feste,
  rechtsbündige Zeitspalte folgt jetzt
  der vorhandenen Einstellung show_time,
  analog zum kompakten Chat. Bei
  Teleport-Trennern bleibt sie unsichtbar.
  Avatar-/Namensbereich wird nicht
  dynamisch neu skaliert. Der vollständige
  Zeitstempel bleibt im Tooltip hinterlegt.
- Avatar-Cache-Callback: Ein verspätetes
  Namens-Cache-Ergebnis mit einem
  anderen agent_id darf weder Anzeigename
  noch Username-Tooltip des aktuellen
  Header-Absenders überschreiben.
  RLVa-geschützte Avatar-Namen und
  Unbekannt-Icon bleiben erhalten.
- Drei neue Regressionstests im
  bestehenden test_branding_contract.py
  sichern eindeutige Header-Daten,
  die show_time-Sichtbarkeit und
  Absenderidentität bei asynchronen
  Cache-Antworten ab.

**Prüfnachweise:** Vollständiger Linux-
Viewer 7.2.5.194 erfolgreich gebaut;
288/288 Python-Tests, davon 134/134
Branding-/UI-Tests auch unter
LC_ALL=C/PYTHONUTF8=0, sowie
41/41 native CTests bestanden.
Alle sieben vorhandenen Chat-Header-
Sprachlayouts als XML validiert.
Zwei geänderte Header-XUI-Dateien
JA/ES bytegleich im Viewer-Paket.
Isolierte GUI-Starts JA 1280x720,
ES 1280x720, DE 1920x1080:
Login-Wait erreicht, ohne
Authentifizierung, Exit 0.
Kein authentifizierter In-World-
Chat-/IM-Interaktionstest erfolgt.
Sieben bekannte _v2/script_copy-
Dateien bleiben unverändert:
keine blinde Altdatei-Löschung,
keine neuen Quelldateien,
keine Benutzer-/Secret-Daten geändert.

**Formal:** P01 Linux 20/20;
P02 6/20. Nächster Block 7/20:
bestehende IM-/Gruppenfenster-
Kopfzeilen, Status und Navigation
konsistent gestalten.

**Windows:** Der bislang letzte
Windows-Vollviewer-CI-Versuch
Run 41510587323 produzierte laut
NSIS-Log ein Setup, aber der
Artefaktsuch-/Upload-Schritt
scheiterte. Noch kein verifiziert
downloadbarer Windows-Installer
und keine Windows-GUI-Abnahme.
Die aktuelle sechsfache
Modulmatrix ersetzt keinen
Windows-Vollviewer-Test.

## P02 Block 5/20 – OGL-Infofenster, Symbol und Kopieren-Schaltfläche (04.11.2026)

**Code:** 8c274734 auf origin/ogl-develop.

Im bestehenden "Über den Viewer"-Fenster wurde
der sichtbare Infobereich für den OGL-Kanal
modernisiert, ohne das Panel zu duplizieren:
- In llfloaterabout.cpp ersetzt eine
  APP_NAME == "OGL"-Verzweigung den alten
  Firestorm-Supporttext durch einen
  OGL-GitHub-Support-/Updatehinweis.
  Andere Kanäle behalten ihren Alttext.
- Im bereits vorhandenen EN-floater_about.xml
  ist ein zunächst verstecktes OGL-Icon
  am rechten Rand des Infokopfs ergänzt.
  Zur Laufzeit ist es nur für OGL sichtbar;
  Bildressource OGL_System_Icon verweist
  auf die bereits vorhandene OGL-Grafik.
- Die vorhandene Schaltfläche copy_btn
  erhält im OGL-Kanal eine passende
  Beschriftung; sie behält ihre echte
  onClickCopyToClipboard-Implementierung.
- OGLAboutSupport und OGLAboutCopy sind
  in allen 13 bereits bestehenden
  strings.xml-Sprachressourcen übersetzt
  (AZ, DA, DE, EN, ES, FR, IT, JA,
  PL, PT, RU, TR, ZH).
- Fenstertitel "About [APP_NAME]" und
  separate Credits für Linden/Firestorm,
  Lizenzhinweise und Contributor-Angaben
  bleiben erhalten; rechtlich erforderliche
  Herkunftsnachweise werden nicht verdeckt.
- Drei neue Regressionstests im vorhandenen
  ogl/tests/test_branding_contract.py
  prüfen kanalabhängige C++-Verdrahtung,
  existierende Icon-Ressource, 13
  Sprachtexte und unveränderte Credits.

**Testnachweise:** Linux-Vollviewer
7.2.5.192 erfolgreich kompiliert,
285/285 Python-Tests, darunter
131/131 Branding-Tests auch unter
LC_ALL=C/PYTHONUTF8=0, und
41/41 native CTests bestanden.
13/13 bearbeitete strings.xml-Dateien
und EN-floater_about.xml bytegleich
im Linux-Viewer-Paket; vorhandene
OGL_System_Icon-Textur registriert.
Isolierte Prelogin-GUI-Smokes
DE 1280×720 und EN 1920×1080 bis
STATE_LOGIN_WAIT, jeweils Exit 0.
Nicht durchgeführt: authentifizierte
In-World-Öffnung und visuelle
Prüfung des Infofensters selbst.
Keine neuen Projektdateien,
Benutzerprofile oder Secrets geändert.

**Formal:** P01 Linux 20/20,
P02 5/20. Nächster /goal-Einstieg:
P02 Block 6/20 – Avatar-/Chat-Kopfbereiche
und bestehende Fensterfunktionen
weiter vereinheitlichen.

**Windows-Installer:** Run 41510587323
blieb ohne herunterladbares Installer-
Artefakt. NSIS meldete eine Setup.exe,
aber die anschließende Artefaktsuche
scheiterte; getrennt nachzuholen.
Windows/macOS-Vollviewer-Freigabe
nicht erteilt.

## P02 Block 4/20 – Entwickler-Untermenüs und Toolbar-Overlays konsolidiert (04.11.2026)

**Code:** fc097c18 direkt auf origin/ogl-develop.

- Spanisch: Im Entwicklerbereich war DevelopWorld als
  doppeltes direktes Untermenü vorhanden; das kürzere
  Duplikat entfernt, vollständiges Welt-Menü mit
  Simulator-Diagnose DumpSimFeaturesToChat erhalten.
  Die zweite, leere Set-Logging-Level-Kopie gelöscht;
  Debug, Info, Warning, Error und None bleiben erhalten
  und erben ihre funktionalen englischen Basis-Callbacks.
- DE/PT/ZH-Toolbar-Übersetzungen: drei zusätzliche
  leere toolbar_panel-Layoutknoten entfernt.
  Die echte Toolbar bleibt ausschließlich im
  englischen Basis-XUI; lokale Chatbutton-Labels,
  Tooltips und ursprüngliche Breiten bleiben erhalten.
- Drei Regressionstests im bestehenden
  test_branding_contract.py ergänzen Submenü-
  Eindeutigkeit pro Elternmenü, spanische Welt-/
  Logging-Aktionen und nur eine Toolbar-Basis.
  Gleichnamige Untermenüs in unterschiedlichen
  Kontexten (etwa RLVa Debug) wurden nicht
  fälschlich gelöscht.

**Tests und Build:** Linux-Vollviewer 7.2.5.190
erfolgreich, 282/282 Python-Tests,
128/128 Brandingtests auch mit
LC_ALL=C/PYTHONUTF8=0 und
41/41 native CTests bestanden.
13/13 Menü-XUI und 11/11 Toolbar-XUI
XML-validiert. Vier bearbeitete
XUI-Dateien zum Linux-Build-Paket
byteweise identisch. Vier isolierte
Prelogin-GUI-Smokes (ES 1280×720,
DE 1920×1080, PT 1280×720,
ZH 1280×720) bis STATE_LOGIN_WAIT,
ohne Authentifizierung; alle Exit 0.
Keine echten In-World-Menüklicks
auf den vier Sprachen getestet.
Keine neuen Projekt-/Backup-Dateien
oder Nutzerdaten angelegt/verändert.

**Formal:** P01 Linux 20/20,
P02 4/20. Nächster Block 5/20:
Fenstertitel, Icons und Buttons in den
vorhandenen Dateien an OGL/NexVerse anpassen.

**Windows Installer:** Actions-Lauf 41510587323
hatte den Windows-x64-Viewer kompiliert und
per NSIS eine Setup.exe laut Log generiert.
Der nachgelagerte Artefakt-Suchschritt
scheiterte, also keine herunterladbare
Setup-Datei oder Windows-GUI-Abnahme.
Außerdem wurde noch der alte
Firestorm-OGL-Paketkanal verwendet;
vor Windows-Testerfreigabe müssen
OGL-Buildidentität und Upload-Pfad
korrekt und explizit geprüft werden.

## P02 Block 3/20 – Admin-Untermenüs dedupliziert, Toolbar-Basis geprüft (02.11.2026)

**Code-Commit 4a2f34cc, origin/ogl-develop.**
Die bestehenden OGL-Hauptmenüs haben
unter Avatar (Me) und Entwickler (Develop)
denselben Admin-Status-Befehl geführt.
Im englischen XUI war der Avatar-Eintrag
zwar unsichtbar, aber zusätzlich mit
Strg+Alt+G belegt. Für OGL werden
Request Admin Options und Leave Admin
Options jetzt nur noch unter Develop
definiert; die Admin- und Diagnosefunktionen
sowie ihre XUI-Callbacks bleiben nutzbar.

Konkret zwei redundante Avatar-Einträge
in elf bestehenden Menü-Sprachlayouts
(az, de, en, es, fr, it, ja, pl,
pt, ru, zh) entfernt. Dänisch und
Türkisch hatten diese Doppelnavigation
schon vorher nicht. Kein zweites
Menüsystem, keine neuen Quelldateien.
Einmalige Login-/Moderator- und
Grid-Sicherheitsfreigaben wurden nicht
verändert. Das Bewegungspaar Sit/Stand
nutzt bewusst denselben Shortcut
Alt+Shift+S mit gegenläufigen
Sichtbarkeits-Callbacks und wurde
nicht fälschlich als Duplikat entfernt.

Toolbar-Inventur: Ein echtes EN-Basislayout
mit toolbar_left, toolbar_right und
toolbar_bottom; 10 zusätzliche
Sprach-Overlays ohne weitere
Toolbar-Grunddefinitionen. Bestehende
Struktur- und Layout-Namen bleiben
unverändert, damit Nutzeranpassungen
und Drag-and-Drop-Belegungen weiter
funktionieren. Drei Regressionstests
im bestehenden test_branding_contract.py
sichern die Admin-Eindeutigkeit,
Callbacks/Shortcuts und Toolbar-Vererbung.

**Prüfnachweise:** Linux-Vollviewer
7.2.5.188 erfolgreich gebaut;
279/279 Python-Tests und 41/41 native
CTests bestanden, davon 125/125
Branding-/Navigationstests auch unter
LC_ALL=C/PYTHONUTF8=0. Alle 13
menü_viewer.xml-Sprachdateien
XML-validiert und zum gepackten
Viewer byteweise identisch.
GUI-Voranmelde-Smokes: DE 1280×720,
EN 1920×1080, ES 1280×720,
STATE_LOGIN_WAIT ohne AUTH_INIT,
Exit 0 in allen drei Läufen.
Kein echter In-World-Menüklick in
einer Live-Welt getestet.

**Phasenstand:** P01 Linux 20/20,
P02 3/20. Nächster /goal:
P02 Block 4/20 – Untermenüs und
Toolbar konsistent gestalten;
keine Paralleltemplates erzeugen.

**Windows-Installer-CI:** Getrennte
Windows-x64-Open-Source-Konfiguration
aus Run 41510587323 erfolgreich;
Windows-Vollviewer-Kompilierung
zuletzt in_progress, aber noch
kein Installer-Artefakt bestätigt.
Ältere Windows-Konfigurationsfehler
und Wiederholungsversuche bleiben
historisch dokumentiert; erst
fertiges .exe und Nutzer-Test
sind Windows-Freigabenachweise.

## P02 Block 2/20 – Kanonische OGL-Navigationshierarchie (02.11.2026)

**Code-Commit 9c10de4e auf origin/ogl-develop.**
Feste funktionale Hauptmenü-Reihenfolge für
alle vorhandenen 13 Sprach-Layouts:
1. Me / Avatar (Profil, Aussehen, Inventar)
2. Communicate / Chat & Kontakte (Nahchat,
   Direktnachrichten, Gruppen und Freunde)
3. World / Welt (Navigation, Regionen, Karte)
4. BuildTools / Bauen (Objekt- und Bauwerkzeuge)
5. Content / Entdecken (Suche, Skriptbibliothek,
   NexVerse-Nachrichten)
6. Help / Hilfe & Support (Portal, Fehlerbericht,
   Downloads, rechtlich erforderliche Hinweise)

RLVa, Advanced, Develop und Admin bleiben
als vorhandene separate Spezialmenüs erhalten;
die bestehenden technischen Namen und
Viewer-Event-Callbacks werden nicht geändert.
Die Menüdefinition liegt weiterhin in den
vorhandenen menu_viewer.xml-Dateien und
deren englischer Vererbungsbasis; kein neues
Menü-, Skin- oder C++-Parallelsystem.

**Tatsächliche Änderungen:**
- Spanisch: acht leere bzw. unbenutzte
  zusätzliche V1-Hauptmenü-Overlays inklusive
  altem V1-Menus-Schalter entfernt; die
  funktionale Hauptmenü-Struktur bleibt erhalten.
- Dänisch: Content-Overlay mit dem Label Opdag
  ergänzt; englische Funktionsmenü-Unterelemente
  werden weiter geerbt.
- Türkisch: Content-Overlay mit Keşfet ergänzt;
  englische Funktionsmenü-Unterelemente werden
  weiter geerbt. Französisch hatte bereits
  ein korrektes Contenu-Menü und blieb unverändert.
- Drei Regressionstests im bestehenden
  test_branding_contract.py hinzugefügt.

**Nachweise:** Linux-Vollviewer 7.2.5.186
kompiliert; 276/276 Python-Tests,
41/41 native CTests bestanden. Alle
13/13 vorhandenen Hauptmenüs als XML
geparst; Reihenfolge, Eindeutigkeit
und Abwesenheit von V1-Hauptmenüs
überprüft. Die drei geänderten XUI-Dateien
wurden mit dem im Linux-Viewer-Paket
enthaltenen Stand byteweise verglichen
(3/3 identisch). Isolierte GUI-Starts
ES/DA/TR bei 1280×720 bis Login-Wait,
ohne Authentifizierung, jeweils Exit 0.
Kein In-World-Interaktionstest mit
diesen drei Sprachen behauptet.
Es wurden keine neuen Projektdateien
oder Benutzereinstellungen erzeugt;
die bekannten sieben historischen
_v2-/script_copy-Pfade bleiben bis zur
geprüften Nutzungs-/Lizenzanalyse erhalten.

**Formaler Stand:** P01 Linux 20/20;
P02 2/20. Nächster /goal-Schritt:
**P02 Block 3/20**: Bestand der bereits
vorhandenen Hauptnavigation/Untermenüs
und Toolbar gezielt konsolidieren,
ohne zusätzliche Template-Dateien
oder doppelte Navigationswege.

## Windows-Testinstaller – isolierter GitHub-Actions-Build (02.11.2026)

Anforderung: Auch unter Windows den vollständigen OGL Viewer
testen können. Das frühere portable Grafik-CI-Ergebnis
enthält keinen Installer. Vor der ersten Windows-Abnahme
existiert noch kein nachweislich installierbares .exe-Artefakt.

Vorbereitet in bestehenden Dateien:
- .github/workflows/ogl-portable-graphics.yml:
  schwergewichtiger vollständiger Windows-2022/x64-
  OpenSim-Viewer-Build nur bei bewusstem Push-Commit
  mit [windows-installer] im Committext. Er installiert
  öffentliche Autobuild-Werkzeuge und NSIS,
  schaltet proprietäre Audio-/KDU-/Signier-
  Komponenten für diesen Testlauf ab, soll
  ein Setup.exe-Artefakt 14 Tage bereitstellen
  und löscht/überschreibt keine Releases.
  Die bisherige sechsfache Modulmatrix bleibt erhalten.
- indra/newview/viewer_manifest.py:
  alte NSIS-Pfad-Fehlzuordnung korrigiert;
  OGL-Installationsname, Programmnamen und
  GitHub-Downloadlink von Firestorm getrennt.
- indra/newview/fs_viewer_manifest.py:
  OGL-spezifischer Windows-Installerdateiname;
  Originalverhalten anderer Kanäle bleibt erhalten.
- ogl/tests/test_branding_contract.py:
  CI-Testgate, NSIS-Compilerpfad und getrennte
  Installeridentität durch neue Regressionstests.

**Erster Windows-Buildversuch (GitHub Actions Run 41509897607):**
Checkout, Python, Autobuild, NSIS und öffentliche
Firestorm-Buildvariablen erfolgreich. Der eigentliche
Konfigurationsschritt scheiterte an einem relativen
AUTOBUILD_VARIABLES_FILE-Pfad, weil Autobuild aus
build-vc170-64 arbeitet. Fehler bei ogg_vorbis war
ein Folgefehler. Der Workflow verwendet jetzt
einen absoluten Workspace-Pfad. Kein Installer
aus dem ersten Lauf erzeugt.

**Zweiter Windows-Buildversuch (Run 41510216092):**
Der absolute Variablenpfad funktioniert. Danach wurde
die ursprüngliche ReleaseFS-Konfiguration als
Problem sichtbar: Sie übergibt intern --fmodstudio
und --kdu und verlangt nicht-öffentliche Archive
aus einem lokalen Cygwin-Verzeichnis.
Der bereits in autobuild.xml vorhandene
Buildmodus ReleaseFS_open benötigt diese
proprietären Pakete nicht. Der Windows-Job
verwendet ihn jetzt für Configure und Build.

**Wichtig:** Dies ist zunächst ein Buildversuch, kein
bestätigter Windows-Installer. Erst ein erfolgreiches
Windows-CI-Artefakt und Installation/Viewer-Login
auf einem realen Windows-Rechner erlauben die
Windows-Vollviewer-Abnahme. Testsetup ohne
Windows-Codesignatur erzeugt ggf. SmartScreen-
Warnungen. Verifizierbarer Build-/Artefaktstatus
ist auf GitHub Actions maßgeblich.
P01 Linux 20/20 und P02 1/20 bleiben unverändert.
Plattformfreigaben werden weiterhin separat
in P13 bewertet.

## Phasenwechsel: P01 Linux-Abnahme / P02 Block 1 (02.11.2026)

P01 unter Linux 20/20 abgeschlossen nach
expliziter Rückmeldung des Nutzers:
Viewer-Login ins Live-Grid erfolgreich,
3D-Welt funktioniert, Viewer-Shutdown
sauber, gespeicherte Zugangsdaten
werden zuverlässig wiederhergestellt.
Vom Assistenten separat nachgewiesen:
Linux-Vollbuild 7.2.5.180,
270 Python-/41 native Tests
und isolierte Voranmelde-GUI-Smokes.
Kein eigenständiger Login mit Nutzerkonto
und keine Windows-/macOS-Vollviewer-GUI-
Abnahme behaupten. Diese Plattformtests
sind Pflicht vor OS-spezifischen Releases
und in P13 verblieben. Beta-TLS-Fehler
wird in P08 bearbeitet; kein Bypass.

P02 Block 1/20: Inventar der bestehenden
Menü-/Fenster-/Chat-Dateien erstellt:
13 menu_viewer.xml-Übersetzungen,
11 panel_toolbar_view.xml-Locales,
13 IM-Container-Locale-Overlays,
sieben panel_chat_header.xml-Locales,
zwei Nearby-Chat-Overlays,
zehn moderne Login-Overrides zur Abgrenzung.
258 englische floater_*.xml-Dateien
vorhanden; alle untersuchten XML-Dateien
erfolgreich geparst. Aktive Quellen:
fschathistory.cpp, fsfloaternearbychat.cpp,
fsfloaterim.cpp, llviewerwindow.cpp.
Sieben bekannte _v2-/script_copy-Dateien
inventarisiert, nicht blind entfernt.
Keine neuen Quelldateien.
P02 nächster Block 2/20: einheitliche
OGL-Navigationshierarchie festlegen.
Die folgenden älteren Berichte dokumentieren
historische damalige Stände, nicht den
aktuellen Phasenstatus.

## P01 Block 19/20 – Transport-/CI-/Credential-Prüfung (02.11.2026)

Code-Commit 263a224e auf origin/ogl-develop.
20/20 technische Prüfpositionen abgeschlossen;
das **formale P01-End-to-End-Gate bleibt offen**.
Linux-Vollviewer 7.2.5.180 gebaut,
270/270 Python-Tests und 41/41 native CTests
bestanden; 116/116 Branding-Tests und 51/51
Gridtests unter LC_ALL=C/PYTHONUTF8=0
bestanden. DE-720p-/EN-1080p-Xvfb-GUI-Starts
bis STATE_LOGIN_WAIT ohne AUTH_INIT, sauberer Exit 0.

Das reale Live-Grid-GET
http://hg.stadt-nexverse.de:80/get_grid_info
lieferte HTTP 200 und application/xml.
Ein HEAD auf diesen Pfad lieferte dagegen
405 MethodNotAllowed; dies ist kein
erfolgreicher Live-Avatar-Login.
Beta-GET unter
https://beta.stadt-nexverse.de/get_grid_info
scheiterte weiterhin an der regulären
TLS-Hostnamevalidierung (curl Fehler 60).
Keine Login-Credentials übertragen; keine
TLS-Prüfung deaktiviert.

Die Authenticator-Reuse-Prüfung in
indra/newview/oglnexversegrid.h verwirft
nun mehrdeutige Credential-Keys mit mehr
als einem @-Trennzeichen, auch wenn der
letzte Gridteil dem gepinnten Live- oder
Beta-Grid entspricht. Sie lässt korrekt
gebildete Einwort-/Zweitwort-OpenSim-
Konten weiter zu. Zehn native Prüfvarianten
in der bestehenden Datei
ogl/tests/test_grid_http_policy.cpp
ergänzt, darunter nested, doppelte und
nachgestellte Trenner sowie gültige Konten.
Zwei Python-Vertragstests in der bestehenden
test_nexverse_grid.py ergänzen die
Quellcode- und CI-Triggerprüfung.

Die bestehende sechsfach CI-Matrix
.github/workflows/ogl-portable-graphics.yml
startet bei Push und Pull Request jetzt auch
bei Änderungen an oglnexversegrid.h,
fspanellogin.cpp/h, llstartup.cpp und
llgridmanager.cpp/h. Das ist ein portabler
nativer Testlauf, NICHT ein kompletter
Windows-/macOS-Vollviewer-Build.
YAML geparst, 17 Filter je Event
und alle sechs Runner verifiziert.

20 tatsächlich ausgeführte Prüfpunkte:
01 Branch/Remote sauber;
02 letzte Code-CI geprüft;
03 P01-Roadmap/Projektstatus/Handoff gelesen;
04 zentralen authenticator-Reuse-Vertrag untersucht;
05 Main-Grid-GET HTTP 200/XML geprüft;
06 abweichendes HEAD-Verhalten 405 separat geprüft;
07 Beta-TLS-Fehler 60 bestätigt, kein Bypass;
08 CI-Triggerlücke beim Grid-Header festgestellt;
09 Push-Pfade für P01-Sicherheitsdateien erweitert;
10 Pull-Request-Pfade analog erweitert;
11 sechs Windows-/Linux-/macOS-Architekturen geprüft;
12 Credential-Parsing auf genau einen Trenner beschränkt;
13 nested/doppelte/angehängte Trenner abgewehrt;
14 Einwort-/Zweitwort- sowie gültige Beta-
   Authenticator-Prüfung unverändert positiv;
15 zehn zusätzliche native Testvarianten ergänzt;
16 zwei Python-Regressionstests,
   51 Grid-/116 Brandingtests auch unter ASCII grün;
17 CI-YAML mit Parser und Event-/Runner-Matrix geprüft;
18 Linux-Vollbuild 7.2.5.180,
   270 Python-/41 native Tests bestanden;
19 GUI-Starts DE 720p und EN 1080p
   bis Login, beide Exit 0;
20 Code unmittelbar auf origin/ogl-develop
   gepusht, diff-check und Altbestand geprüft.
Sieben bekannte _v2-/script_copy-Pfade nicht
unüberlegt gelöscht, keine neuen Projektdateien
und keine Profile/Secrets/Caches bearbeitet.

**Formal P01 18/20, P02 0/20.** P01 Block 19
braucht weiter einen authentifizierten Live-Login,
echte Credential-Wiederherstellung, In-World-
Shutdown, volle Windows-/macOS-Vollviewer-
Build-/GUI-Nachweise. Beta-TLS bleibt blockiert.
Die portable CI ersetzt diese End-to-End-Gates
nicht. Nächster /goal-Einstieg P01 Block 19/20.

## P01 unabhängige Vorarbeit – Benutzernamenwechsel setzt auch leeren Passwortstatus zurück (02.11.2026)

Code bfd5e2d9 auf origin/ogl-develop.
20/20 technische Prüfpunkte dieser unabhängigen
P01-Vorarbeit, keine formale Phasenabnahme.
Linux-Vollviewer 7.2.5.178 erfolgreich,
268/268 Python-Tests, 41/41 native CTests,
116/116 Branding-Tests und 49/49 Gridtests
bestanden. Gezielte Tests auch mit
LC_ALL=C/PYTHONUTF8=0. DE-720p- und EN-1080p-
Xvfb-GUI-Starts bis STATE_LOGIN_WAIT,
kein AUTH_INIT, sauberer Exit 0.

Korrektur in der bestehenden Datei
indra/newview/fspanellogin.cpp:
Im aktiven onUsernameTextChanged den zusätzlichen
Guard mPasswordLength != 0 entfernt. Sobald der
Benutzername von mPreviousUsername abweicht,
werden auch bei leerem sichtbarem Passwortfeld
der alte Retry-Zwischenspeicher sPassword,
die Passwortlänge, das Änderungskennzeichen,
die Augenknöpfe und die frühere Löschfreigabe
zurückgesetzt. Die einmalige HTTP-Zustimmung
wird weiterhin frühzeitig invalidiert.
Bei der Auswahl eines unbekannten Kontos
werden die Augenknöpfe nach dem Reset
ausdrücklich synchronisiert. Normale
gespeicherte Konten und Nicht-OGL-Kanäle
behalten ihr bisheriges Verhalten.
Drei neue Regressionstests im bestehenden
test_branding_contract.py, zwei bestehende
Tests in test_nexverse_grid.py aktualisiert.
Keine zusätzlichen Dateien angelegt.

20 tatsächlich durchgeführte Prüfpunkte:
01 Repository und ogl-develop geprüft;
02 saubere Ausgangsbasis bestätigt;
03 vorherige Code-CI erfolgreich kontrolliert;
04 Haupt-Roadmap, Status und Handoff gelesen;
05 aktiven onUsernameTextChanged untersucht;
06 alten Passwortlängen-Guard als Fehler isoliert;
07 Namensänderung unabhängig vom Passwortwert abgesichert;
08 Passwortfeld beim Identitätswechsel geleert;
09 temporären Retry-Speicher gelöscht;
10 Passwortlänge auf null gesetzt;
11 Passwort-Bearbeitungsstatus zurückgesetzt;
12 Passwort-Anzeige-Schaltflächen synchronisiert;
13 vorherige Konto-Löschfreigabe aufgehoben;
14 Login-Button nach Zustandwechsel aktualisiert;
15 One-Time-HTTP-Zustimmung weiterhin vor Reset verworfen;
16 unbekannten Konto-Auswahlpfad ebenfalls korrigiert;
17 drei neue und zwei aktualisierte Tests:
   116 Branding-/49 Gridtests auch unter ASCII grün;
18 Linux-Vollbuild 7.2.5.178,
   268 Python-/41 native Tests grün;
19 DE-720p-/EN-1080p-GUI ohne Authentifizierung,
   mit sauberem Exit-Code 0;
20 Code direkt auf GitHub gepusht, diff geprüft,
   keine neuen Dateien; sieben bekannte
   _v2-/script_copy-Pfade nicht blind gelöscht.
Keine Nutzerprofile, Secrets, Caches,
Credentials oder Serverdaten verändert.

Formal P01 18/20, P02 0/20.
Block 19/20 erfordert weiterhin autorisierten
Live-Login, echte gespeicherte Credential-
Wiederherstellung, In-World-Shutdown und
vollständige Windows-/macOS-Vollviewer-
Builds und GUI-Tests. Beta-TLS-Hostname-
abweichung bleibt offen; keine Umgehung.
Portabler Modultest ersetzt keine
plattformübergreifende End-to-End-Abnahme.

## P01 unabhängige Vorarbeit – Retry-Passwort strikt von gespeichertem Platzhalter trennen (02.11.2026)

Code-Commit be18b791 auf origin/ogl-develop.
20/20 technische Prüfschritte dieser unabhängigen
P01-Vorarbeit, KEINE formale P01-Abnahme.
Linux-Vollviewer 7.2.5.176 gebaut;
265/265 Python-Tests, 41/41 native CTests,
113/113 Branding- und 49/49 Gridtests
bestanden (gezielte Tests auch unter
LC_ALL=C und PYTHONUTF8=0).
Isolierter DE-GUI-Start bei 1280x720 und
EN-GUI-Start bei 1920x1080 jeweils
STATE_LOGIN_WAIT, ohne AUTH_INIT, sauberer
Exit-Code 0. Es wurde kein reales
Konto authentifiziert, kein Secure-Store
oder echtes Benutzerprofil verändert.

Sicherheits- und Funktionsergänzung nur in
zwei vorhandenen Projektdateien:
- indra/newview/fspanellogin.cpp:
  getFields kopiert bei OGL nur den
  manuell eingegebenen Wert in den statischen
  Retry-Zwischenspeicher. Ein bloßes
  gespeichertes Authenticator-Token mit
  Maskierungs-/Anzeigefülltext leert diesen
  Zwischenspeicher statt den Fülltext zu
  übernehmen. Die geschützte Konto-Credential
  bleibt über den alten Secure-Store nutzbar.
  Beim ausdrücklich gewählten neuen
  gespeicherten Konto wird der vorherige
  Klartext-Retry eines anderen Kontos
  verworfen, ausgenommen der Startup-
  Wiederherstellungsweg, der ein tatsächlich
  manuell eingegebenes Passwort benötigen
  kann. Ein fehlgeschlagener Startversuch
  ohne gespeichertes echtes Retry-Passwort
  leert das Eingabefeld und setzt Länge,
  Änderungsmerkmal und Login-Button
  zurück. Ein tatsächlich manuell eingegebenes
  Passwort darf nach einer fehlgeschlagenen
  Anmeldung weiter restauriert werden.
  Nicht-OGL-Kanäle behalten bisheriges
  Verhalten.
- ogl/tests/test_branding_contract.py:
  drei zusätzliche gezielte Source-Contract-
  Tests zum manuell-only Retry-Speicher,
  Fehlversuchs-Rückfall und Kontowechsel;
  unveränderter sicherer Speicherpfad
  geprüft. Diese statischen Tests ersetzen
  keine tatsächliche Secure-Store-
  Integration mit realen Credentials.

20 nachweislich abgearbeitete Prüfpositionen:
01 Repository und Branch ogl-develop geprüft;
02 Arbeitsverzeichnis vor Änderungen sauber;
03 jüngste CI-Ergebnisse erfolgreich geprüft;
04 Hand-off und P01-Detailplan gelesen;
05 OGL-Loginformular-Credential-Lebenszyklus geprüft;
06 getFields zwischen gespeicherter
   und manuell bearbeiteter Eingabe unterschieden;
07 gespeicherter Authenticator wird nicht
   als sichtbares Passwort interpretiert;
08 vorherige Übernahme des Fülltextes in
   den statischen Retry-Speicher nachvollzogen;
09 Cache-Schreibpfad auf manuelle Eingabe
   beschränkt;
10 Cache für geschützte gespeicherte
   Kontoauswahl explizit geleert;
11 manuelles Passwort bleibt für
   fehlgeschlagene Login-Retries verfügbar;
12 Kontoauswahl leert früheres
   Klartext-Retry außerhalb des Startuprücklaufs;
13 leerer Startup-Retry löscht das Eingabefeld;
14 leerer Startup-Retry setzt Passwortlänge
   und Bearbeitungsstatus zurück;
15 gültige gespeicherte Authenticator-
   Auswahl und Non-OGL-Pfade erhalten;
16 drei neue Source-Contract-Tests ergänzt,
   113/113 Branding- und 49/49 Gridtests
   auch unter ASCII-Systemkodierung bestanden;
17 Linux-Vollviewer-Build 7.2.5.176,
   265/265 Python- und 41/41 native Tests bestanden;
18 isolierter DE-720p-Login-GUI-Start
   und sauberer Exit 0;
19 isolierter EN-1080p-Login-GUI-Start
   und sauberer Exit 0;
20 Code direkt auf origin/ogl-develop gepusht,
   Änderungen auf Whitespace geprüft,
   bestehende Datei- und Altbestandsregel beachtet.
Sieben bekannte _v2-/script_copy-Pfade wurden
nicht ohne Nutzungs-/Lizenzprüfung gelöscht.
Keine neuen Quell- oder Testdateien angelegt.

**Formaler Stand unverändert P01 18/20,
P02 0/20.** Nächster operativer /goal-
Block P01 19/20: autorisierter Live-Login,
tatsächliche gespeicherte Passwort-
Wiederherstellung, In-World-Shutdown und
vollständige Windows-/macOS-Vollviewer-
Build-/GUI-Nachweise stehen noch aus.
Beta-TLS-Hostnameabweichung weiter offen,
keine Umgehung der TLS-Validierung.
Portable Grafikmatrix ist kein Ersatz
für Plattform-Vollviewer-Abnahmen.

## P01 unabhängige Vorarbeit – Legacy-Grid-Callbacks absichern (02.11.2026)

Code-Commit 8e9e773e auf origin/ogl-develop;
20/20 nachgewiesene technische Prüfpunkte, KEINE formale
P01-Abnahme. Der OGL-Viewer 7.2.5.174 wurde unter Linux
vollständig gebaut, 262/262 Python-Tests, 41/41 CTests,
110/110 Branding- und 49/49 Grid-Tests bestanden
(gezielte Tests auch mit LC_ALL=C/PYTHONUTF8=0).
Isolierter DE-Loginstart 1280x720 sowie EN-Loginstart
1920x1080 jeweils STATE_LOGIN_WAIT, kein AUTH_INIT,
sauberer Exit-Code 0. Keine reale gespeicherte Anmeldung.

**Änderungen nur an zwei bestehenden Dateien:**
- indra/newview/fspanellogin.cpp: Der alte Grid-
  Manager-Hilfe-Callback öffnet im OGL-Kanal ausschließlich
  das offizielle NexVerse-Portal statt eines Second-Life-
  Hilfe-Topics. Der alte Grid-Builder-Callback bricht
  unter OGL vor dem Fremd-URL-Aufruf ab, auch falls
  ein altes UI oder Skin den Callback nachträglich auslöst.
  Für andere Viewer-Kanäle bleiben die Legacy-Funktionen.
- ogl/tests/test_branding_contract.py: drei weitere
  Tests für die zwei direkten Callback-Sicherungen,
  nicht verdrahtete OGL-Hilfe-/Builder-Callbacks,
  ursprüngliche versteckte Builder-Komponenten
  und den unveränderten Zwei-Grid-Selektor.

**20 technische Prüfpositionen:**
01 Repo/Branch/saubere Ausgangsbasis geprüft;
02 Projekt-Handoff/Status/Phasenplan gelesen;
03 jüngste GitHub-CI (vorheriger Code erfolgreich);
04 zwei fest hinterlegte NexVerse-Grids überprüft;
05 Callback-Verdrahtung im aktiven OGL-Login untersucht;
06 Legacy-Grid-Hilfe-Direktfunktion identifiziert;
07 offiziellen Portal-Fallback eingebaut;
08 Rücksprung vor upstream Help-Resolver geprüft;
09 Legacy-Grid-Builder-Direktfunktion identifiziert;
10 OGL-Rücksprung vor Upstream-Fremd-URL eingebaut;
11 übrige Viewer-Kanäle unverändert belassen;
12 13 bestehende Legacy-Login-Sprachlayouts überprüft;
13 10 moderne Login-Overlay-Sprachen überprüft;
14 Builder nirgends neu sichtbar und
   Zwei-Grid-Eingabe bleibt schreibgeschützt;
15 drei Regressionstests ergänzt, 110/110 grün;
16 49/49 Gridtests auch unter ASCII grün;
17 vollständiger Linux-Build, 262/262 Python-
   und 41/41 native Tests grün;
18 DE 720p Voranmelde-GUI bis Login und Exit 0;
19 EN 1080p Voranmelde-GUI bis Login und Exit 0;
20 Quellcode direkt gepusht, git diff --check,
   Altbestand und keine neuen Dateien kontrolliert.
Sieben bekannte _v2-/script_copy-Pfade bleiben
bis zur sicheren Nutzungs-/Lizenzprüfung erhalten.
Keine Nutzerdaten, Secure-Store-Einträge,
Caches, Secrets oder Serverdateien bearbeitet.

**Formal weiterhin P01 18/20, P02 0/20.**
P01-Block 19 (autorisierter Live-Login, tatsächliche
Credential-Restore- und plattformübergreifende
Viewer-GUI-Abnahme) bleibt offen, ebenso Beta-
TLS-Hostnamefehler, In-World-Shutdown und
vollständige Windows-/macOS-Vollviewer-Tests.
Die portable Grafikmodulmatrix ersetzt diese
End-to-End-Gates nicht. Weiterer /goal-Einstieg
P01 Block 19/20; unabhängige Vorarbeiten möglich.

## P01 unabhängige Vorarbeit – verwaiste Start-Credentials nicht als Passwort übernehmen (02.11.2026)

Code-Commit 4ff7e7cb direkt auf origin/ogl-develop.
20/20 technische Prüfpositionen dieser unabhängigen
P01-Sicherheitsrunde; keine formale P01-Abnahme.
Linux-Vollviewer 7.2.5.172 gebaut:
259/259 Python-Tests, 41/41 native CTests,
107/107 Branding-Tests und 49/49 Gridtests
auch unter LC_ALL=C/PYTHONUTF8=0 bestanden.
Isolierte DE-/EN-Xvfb-GUI-Starts 1280x720
bis STATE_LOGIN_WAIT ohne AUTH_INIT, Exit-Code 0.
Kein Test mit echten gespeicherten Credentials.

Konkreter Fehler in FSPanelLogin::setFields:
Wenn eine beim Startup gelieferte Credential auf
dem aktuellen festen NexVerse-Grid liegt, aber
nicht in der gespeicherten Kontenauswahl vorkommt,
wurde sie dennoch mit einem Passwort-Fülltext
angezeigt und als manuelle Passwortänderung
markiert. Dadurch konnte der Platzhalter im
späteren Loginversuch irrtümlich wie ein echtes
Passwort verwendet werden.

Implementierung in bestehenden Dateien:
- indra/newview/fspanellogin.cpp: Fehlende
  Kontoauswahl verzweigt unter OGL vor dem
  Passwort-Fülltext in einen sicheren Rückfall.
  Passwortfeld und sPassword leeren, Status
  sCredentialSet zurücknehmen, Passwortlänge
  und Änderungskennzeichen zurücksetzen,
  Vorname/Accountname und Länge zur
  freiwilligen Neueingabe korrekt führen,
  Augenknöpfe und Login-Schaltfläche
  synchronisieren, kein automatisches
  Credential-Reuse. One-part OpenSim-
  Kontoidentifikatoren werden angezeigt,
  statt leer zu erscheinen. Normale
  gespeicherte Kontoauswahl und Nicht-OGL-
  Kanäle behalten ihren bisherigen Ablauf.
- ogl/tests/test_branding_contract.py:
  drei zusätzliche Source-Contract-Prüfungen
  zum sicheren Rückfall, Kontoanzeigenamen
  und weiterhin vorhandenen sicheren
  Credential-Reuse des gültigen Listeneintrags.
- ogl/tests/test_nexverse_grid.py:
  bestehenden Regressionstest konkretisiert:
  frühzeitiges Maskieren beim verworfenen
  Konto sowie abschließende Aktualisierung
  beim gültigen gesicherten Konto müssen
  jeweils unabhängig passieren.
Keine neuen Projektdateien; keine Änderungen
an Secure-Store, Einstellungen, Nutzerdaten,
Serverdaten, Cache oder Secrets.

20 technische Prüfpositionen:
01 korrektes Repository und ogl-develop;
02 saubere Ausgangsbasis;
03 P01-Blockade und Roadmap gelesen;
04 vorherige GitHub-CI ausgewertet;
05 setFields-Startuprichtung überprüft;
06 feste NexVerse-Gridprüfung erhalten;
07 Unterscheidung gültiger/fehlender
   Dropdown-Credential nachgezeichnet;
08 problematischen Fülltextpfad isoliert;
09 OpenSim-Einwort-Kontonamen zurückgegeben;
10 bei fehlender gespeicherter Auswahl
   echte Passworteingabe erzwungen;
11 Passwortfeld und statischen Passwort-
   Zwischenspeicher im Rückfall geleert;
12 sCredentialSet deaktiviert;
13 Passwortlänge und geändertes-Passwort-
   Kennzeichen zurückgesetzt;
14 Username, Löschen-Button und
   Eingabelängen zur Neueingabe abgeglichen;
15 Augenknöpfe und Login-Button beim
   Rückfall synchronisiert;
16 unveränderten gültigen Credential-
   Auswahl-/Secure-Store-Weg geprüft;
17 drei neue Branding-Tests sowie
   bestehenden Gridtest auf beide
   Wiederherstellungsfälle angepasst;
18 107 Branding- und 49 Gridtests unter
   ASCII, kompletter Linux-Build 259
   Python-/41 native Tests bestanden;
19 isolierter DE- und EN-GUI-Start und
   sauberer Exit ohne Authentifizierung;
20 Code gepusht; Altdateien, keine
   untracked Dateien, Projektstatus geprüft.
Sieben bekannte _v2-/script_copy-Pfade
nicht ohne Lizenz-/Nutzungsprüfung gelöscht.

Offen bleiben: reale validierte Credential-
Wiederherstellung im autorisierten
Hauptgrid, Live-Login und echter In-World-
Shutdown, Beta-TLS-Zertifikat sowie
vollständige Windows-/macOS-Vollviewer-
Builds und GUI-Tests. Die portable
Grafikmodul-CI ist kein Vollviewer-Nachweis.
Formal weiterhin P01 18/20, P02 0/20;
nächster operativer /goal-Block P01 19/20.
Diese Vorarbeit zählt nicht als Abschluss
des fehlenden End-to-End-Gates.

## P02 unabhängige Vorarbeit – lokalisierte OGL-/Grid-Identität (02.11.2026)

Code-Commit 72c8beed direkt auf origin/ogl-develop.
20/20 technische Prüfpositionen dieser unabhängigen
P02-Vorarbeit abgeschlossen, ohne formale P02-
Phasenabnahme. Linux-Vollviewer 7.2.5.170
erfolgreich: 256/256 Python-Tests, 41/41 native
CTests, 104/104 gezielte Branding-Regressionstests
auch bei LC_ALL=C/PYTHONUTF8=0 bestanden.
Alle 13 strings.xml-Sprachen bytegenau und
wohlgeformt in der Viewer-Paketstruktur.
Vier getrennte 1280x720-Xvfb-GUI-Starts
ES/RU/TR/ZH bis STATE_LOGIN_WAIT,
ohne AUTH_INIT, mit sauberem Exit-Code 0.

Die ursprünglichen Sprach-Overlays hatten
in ES/RU/ZH den Viewer-Namen Firestorm,
in TR Second Life überschrieben.
Chinesisches APP_NAME_ABBR war FS;
TR-Systemchat-Label zeigte Second Life,
ZH verwendete einen nicht aufgelösten
lokalisierten Platzhalter; neun Sprachen
hatten SECOND_LIFE_GRID fest auf Second
Life gesetzt. Die existierenden XML-Dateien
AZ/DE/EN/ES/FR/PT/RU/TR/ZH direkt
geändert: APP_NAME und ausgeschriebene
Variante mit OGL-Identität, Kurzname
OGL und die beiden System-/Gridbezeichner
verwenden [CURRENT_GRID] statt alter
Second-Life-Texte. Die übrigen vier
Sprachen erben die englischen Basiswerte.
Es wurden keine neuen Assets, Dateien
oder ersetzenden Locale-Overlays angelegt.
Die Quelltext-Credits/Lizenzen bleiben erhalten.

In ogl/tests/test_branding_contract.py
drei zusätzliche Tests für alle 13
Sprach-XMLs, dynamisches Grid-Label
und die bestehenden zentralen
LLTrans-Substitutionen aufgenommen.

20 technische Arbeitsschritte/Nachweise:
01 korrekter Branch/Remote und vorhandene CI
   kontrolliert;
02 systematische Inventur der 13 Sprachdateien
   und sechs dynamischen OGL-Identitätswerte;
03 AZ-Gridlabel bereinigt;
04 DE-Gridlabel bereinigt;
05 EN-Basis-Gridlabel bereinigt;
06 ES-Vieweridentität und Gridlabel bereinigt;
07 FR-Gridlabel bereinigt;
08 PT-Gridlabel bereinigt;
09 RU-Vieweridentität und Gridlabel bereinigt;
10 TR-Vieweridentität, System- und Gridlabel;
11 ZH-Vieweridentität, Grid-Platzhalter
   und Kurztitel korrigiert;
12 DA/IT/JA/PL auf Vererbung und
   fehlende abweichende Werte geprüft;
13 drei neue Locale-Regressionstests ergänzt;
14 104/104 Tests mit erzwungener
   ASCII-Systemkodierung bestanden;
15 vollständiger Linux-Vollbuild und
   256/256 Python-/41/41 native Tests bestanden;
16 13/13 originale strings.xml-Quelldateien
   bytegenau im Buildstaging nachgewiesen;
17 isolierter ES-Login-GUI-Start/Exit 0;
18 isolierter RU-Login-GUI-Start/Exit 0;
19 isolierte TR- und ZH-Login-GUI-Starts/Exit 0;
20 No-untracked/diff-check/Altbestand
   kontrolliert, Code auf GitHub gepusht,
   vorhandenen Projektplan aktualisiert.
Sieben zuvor bekannte _v2-/script_copy-Pfade
inventarisiert, nicht ungeprüft entfernt.
Keine Nutzerprofile, Secrets, Passwörter,
Cachedateien oder Serverkonfiguration verändert.

Die GUI-Smokes bestätigen den Viewerstart,
nicht eine authentifizierte Systemchat-
Darstellung in beiden NexVerse-Welten.
Offene P01-Gates: autorisierter Live-Login,
gespeicherte Credential-Wiederherstellung,
In-World-Shutdown und vollständige
Windows-/macOS-Vollviewer-Build-/GUI-
Prüfungen. Beta-TLS-Zertifikatsproblem
weiter offen; nicht umgehen.
Portable OS-Grafikmodultests sind keine
komplette Viewer-Abnahme.
Formal P01 18/20, P02 0/20.
Nächster /goal-Einstieg P01 Block 19/20.

## P02 unabhängige Vorarbeit – Chatkopf ohne doppelte Breitenänderung (02.11.2026)

Code-Commit a462330e direkt auf origin/ogl-develop.
20/20 technische Prüfschritte dieser unabhängigen
P02-Vorarbeit abgeschlossen; **keine formale
P02-Phasenabnahme**. Linux-Vollviewer 7.2.5.168
erfolgreich gebaut: 253/253 Python-Tests,
41/41 native CTests, 101/101 Branding-
/Chatlayout-Regressionsprüfungen auch bei
LC_ALL=C/PYTHONUTF8=0 bestanden. DE-/EN-
1280x720-Xvfb-Starts jeweils STATE_LOGIN_WAIT,
kein AUTH_INIT, sauberer Exit-Code 0.
Keine reale In-World-Chat-GUI abgenommen.

Nur zwei bestehende Quelldateien aktualisiert:
- indra/newview/fschathistory.cpp: alten
  FSChatHistoryHeader::draw()-Sonderweg
  entfernt, der die schon vom XUI-Layout
  begrenzte Usernamenspalte ein zweites Mal
  um die Zeitstempelbreite verkleinerte.
  Die veralteten Zustände mNeedsTimeBox,
  mMinUserNameWidth und sechs obsolete
  updateMinUserNameWidth-Aufrufstellen
  entfernt; die in panel_chat_header.xml
  fest eingeteilte rechte Zeitspalte bleibt
  maßgeblich. Die echte Zeitsetzung und
  Tooltips, Avatarname-Cache, RLVa-Filter
  und separate Teleport-Zeitunterdrückung
  bleiben erhalten.
- ogl/tests/test_branding_contract.py: drei
  neue Regressionstests für keine doppelte
  Größensteuerung, behaltene Namens-/RLVa-
  und Timestamp-Funktionen, sowie XUI-
  Spaltentrennung. Einen veralteten
  bestehenden Test auf den neuen
  korrekten Layoutvertrag umgestellt.
Keine Änderungen an Benutzerprofilen,
Authentifizierung, Netzwerken, Caches
oder privaten Daten; keine neuen Dateien.

20 tatsächlich geprüfte Arbeitsschritte:
01 Git-Branch und Remote geprüft;
02 bestehende CI-Stände kontrolliert;
03 Hauptprojektplan und P01-Gates gelesen;
04 aktive Chatklasse statt deaktivierter
    LLChatHistory ermittelt;
05 englisches XUI-Spaltenlayout untersucht;
06 Zeitspalte 72px mit reserviertem rechtem
    Layoutabstand nachgewiesen;
07 ehemalige doppelte Namenverkleinerung
    im Draw-Pfad isoliert;
08 Draw-Sonderweg entfernt;
09 überflüssige Zeit-Boolvariable entfernt;
10 alten Namensbreitenzustand entfernt;
11 sechs veraltete Breitenupdate-Aufrufe entfernt;
12 Teleport-Trennzeile und ausgeblendete
    Teleport-Zeit geprüft;
13 Avatar-Namens-Cache/Tooltip erhalten;
14 RLVa-Anonymisierung und Unbekannt-Icon geprüft;
15 Zeittext/-Tooltip und alle sieben Chat-Header-
    Sprachlayouts auf richtige Vererbung geprüft;
16 101/101 Branding-/Layouttests auch unter
    ASCII-Systemkodierung bestanden;
17 Linux-Vollbuild und 253/253 Python-/41/41
    native Tests bestanden;
18 isolierter DE-GUI-Start bis Login und Exit 0;
19 isolierter EN-GUI-Start bis Login und Exit 0;
20 Code direkt gepusht; Bestands- und
    Dokumentationsstand geprüft.
Sieben bekannte _v2-/script_copy-Pfade
nicht ohne Nutzungs-/Lizenzprüfung gelöscht.

Offene Gates: authentifizierter Hauptgrid-
Login, gespeicherte Credential-Wiederherstellung,
In-World-Chat-/Avatar- und Shutdown-Prüfungen,
vollständige Windows-/macOS-Vollviewer-Build-
und GUI-Nachweise. Beta-TLS-Hostnameproblem
weiter blockierend; Portable-Grafikmodul-CI
ersetzt diese Nachweise nicht.
**Formal P01 18/20, P02 0/20;
operativer /goal-Einstieg bleibt P01 19/20.**

## P02 unabhängige Vorarbeit – ungelesene Chatnachrichten zuverlässig zählen (02.11.2026)

Code-Commit 8deebedc direkt auf origin/ogl-develop.
20/20 technische Prüfpositionen dieser unabhängigen
P02-Runde; keine formale P02-Phasenfreigabe.
Linux-Vollviewer 7.2.5.166 erfolgreich:
250/250 Python-Tests, 41/41 native CTests und
98/98 gezielte Branding-/Chat-Regressionstests
(auch mit LC_ALL=C/PYTHONUTF8=0) bestanden.
Isolierte DE-/EN-Xvfb-GUI-Starts 1280x720
bis STATE_LOGIN_WAIT ohne AUTH_INIT,
beide mit sauberem Exit-Code 0.

Änderungen nur in drei bestehenden Dateien:
- fsfloaternearbychat.cpp: negative Eingangs-
  zähler vor dem Speichern in getrennten regulären
  und stummgeschalteten History-Caches auf null
  begrenzt. Vorhandene History-Auswahl,
  User-Präferenzen und [NUM]-Ersetzung erhalten.
- fsfloaterim.cpp: gleiche defensive Korrektur
  vor der IM-Anzeige. Nullwert blendet die
  Benachrichtigung weiterhin aus.
- ogl/tests/test_branding_contract.py: fünf
  Regressionstests für beide C++-Pfadprüfungen,
  korrekt verborgenen Startzustand und
  Sprach-Overlays der Nearby-Chat-Fenster.
  Die Ausgangsprüfung ergab: Das vorhandene
  englische XUI startet bereits unsichtbar
  mit 20 Pixeln Höhe – daher keine unnötige
  Änderung an XUI-Dateien.

20 tatsächlich durchgeführte Prüfpositionen:
01 Branch/Remote-Synchronität; 02 CI-Zustände
der Vorarbeiten geprüft; 03 Hauptprojektplan
und P01-Blocker überprüft; 04 aktiven
Nearby-Chat-Quellpfad untersucht;
05 IM-Quellpfad untersucht;
06 EN-Startzustand Nearby-Zähler geprüft;
07 EN-Startzustand IM-Zähler geprüft;
08 elf Nearby-Sprach-Layouts geprüft;
09 dreizehn IM-Sprach-Layouts geprüft;
10 negativen Nearby-Eingangszähler abgefangen;
11 regulären Nearby-History-Zähler erhalten;
12 separaten Muted-History-Zähler erhalten;
13 Chat-Präferenzschalter erhalten;
14 negativen IM-Eingangszähler abgefangen;
15 IM-Präferenz und Nullwert-Ausblendung erhalten;
16 fünf Regressionstests ergänzt, 98/98
unter ASCII-Systemkodierung bestanden;
17 Linux-Vollbuild und 250 Python-/41 native
Tests bestanden; 18 DE-GUI bis Login/sauberer Exit;
19 EN-GUI bis Login/sauberer Exit;
20 Code gepusht, Altbestand und Roadmap geprüft.
Sieben bekannte _v2-/script_copy-Pfade weiter
inventarisiert, ohne ungesicherte Löschungen.
Keine neuen Projektdateien, Nutzerprofile,
Caches oder Passwörter verändert.

Zusätzlicher externer Befund: TLS zu
https://beta.stadt-nexverse.de/get_grid_info
schlug am 02.11.2026 auf Linux Mint erneut
wegen unpassendem Zertifikats-Hostnamen fehl
(curl Fehler 60). Beta bleibt vor Anmeldung
gesperrt; TLS-Prüfung nicht abgeschwächt.
Main-Grid HTTPS-Port 443 antwortete beim
lokalen Test nicht; kein Eingriff am
bestehenden Main-Grid-HTTP-Vertrag.

Nicht abgenommen: Authentifizierter Live-Login,
Passwort-Persistenz, reale In-World-Chat-
Zählung und Shutdown, vollständige
Windows-/macOS-Vollviewer-Build-/GUI-Nachweise.
Portable Grafikmatrix ersetzt diese Gates nicht.
Formal P01 18/20, P02 0/20;
nächster /goal-Einstieg P01 Block 19/20.

## P02 unabhängige Vorarbeit – alte Firestorm-IM-Datenweitergabe gesperrt (02.11.2026)

**Code-Commit cfa671e4** direkt auf origin/ogl-develop.
**20/20 technische Prüfpositionen dieser unabhängigen
P02-Runde**, keine formale P02-Phasenabnahme.
Linux-Vollviewer 7.2.5.164 erfolgreich gebaut:
245/245 Python-Tests, 41/41 native CTests,
93/93 Branding-/Datenschutz-Regressionstests
(einschließlich LC_ALL=C/PYTHONUTF8=0) bestanden.
DE-/EN-Xvfb-GUI-Starts jeweils bis
STATE_LOGIN_WAIT ohne AUTH_INIT, sauberer
Fenster-Close, Exit-Code 0.
Neue plattformübergreifende GitHub-CI wurde
durch den Push angestoßen, ihr Abschluss ist
noch separat zu prüfen. Native Portabilitäts-
Modultests ersetzen keine Windows-/macOS-
Vollviewer-Builds.

**Änderungen nur an zwei bestehenden Dateien:**
- indra/newview/fsfloaterim.cpp:
  Die Firestorm-Supportgruppenpräfixe dürfen im
  OGL-Kanal auch bei migrierten aktivierten
  Einstellungen weder Viewer-Metadaten, OS,
  Skin noch RLVa-Status an die IM-Nachricht
  anhängen. Ursprüngliche Sendelogik für
  normale Nachrichten bleibt unverändert.
  Die Firestorm-Support-/Testgruppen-Schalter
  und ihre Beitritts-Popups erscheinen in OGL
  nicht. Das alte Sysinfo-Sendeelement ist
  unabhängig von migrierten Einstellungen
  in OGL ausgeblendet; direkte Klick-
  und verzögerte Bestätigungs-Callbacks
  sind ebenfalls blockiert. Auch ein später
  Firestorm-Versionsdialog-Callback darf die
  alten Einstellungen in OGL nicht wieder
  aktivieren. Non-OGL-Kanäle behalten die
  vorherigen Funktionen.
- ogl/tests/test_branding_contract.py:
  fünf zusätzliche source-contract Regressionstests
  für Präfix, Gruppen-UI, verstecktes Sysinfo-
  Element, Klick-/Bestätigungs-Callbacks
  und verspäteten Versionsdialog.

**20 konkrete Prüfpositionen:**
01 Branch/Remote und saubere Ausgangsbasis;
02 vorherige Matrix-CI erfolgreich;
03 aktiven FSFloaterIM-Sendepfad gelesen;
04 migrierte Support-Flags im Quellcode geprüft;
05 kein Firestorm-Präfix im OGL-Sendepfad;
06 normaler LLIMModel-Nachrichtenversand erhalten;
07 Supportgruppen-Unterscheidung in OGL unterdrückt;
08 Testgruppen-Unterscheidung in OGL unterdrückt;
09 Support-/Test-Schalter im OGL verborgen;
10 Legacy-Beitrittsdialog im OGL nicht mehr erreichbar;
11 Sysinfo-Button trotz Nutzer-Altwert verborgen;
12 direkter Sysinfo-Klick in OGL gesperrt;
13 verzögerter Sysinfo-Sende-Callback gesperrt;
14 späterer Versions-Prompt-Callback gesperrt;
15 Einstellungen in settings.xml nicht überschrieben
und Nicht-OGL-Verhalten erhalten;
16 fünf neue Tests, 93/93 Branding-Tests
auch unter ASCII-Systemkodierung erfolgreich;
17 Linux-Vollbuild und 245/245 Python-/41/41
native Tests erfolgreich;
18 isolierter DE-Login-GUI-Start/Exit 0;
19 isolierter EN-Login-GUI-Start/Exit 0;
20 Commit gepusht, Status/Altbestand kontrolliert.
Sieben vorhandene _v2-/script_copy-Pfade
inventarisiert, ohne ungesicherte Löschungen.
Keine neuen Projektdateien, keine echten Nutzer-
Credentials, Serverdaten, Caches oder Secrets verändert.

**Offene Gates:** Autorisierter Live-Login,
Passwort-Persistenzprüfung, In-World-Chat-Test
der Datenschutzfunktion, Viewer-Shutdown und
vollständige Windows-/macOS-Viewer-Build-/GUI-
Prüfungen; Beta-TLS-Hostnameabweichung weiter
ungeklärt, Beta-Login gesperrt.
**Formal P01 18/20, P02 0/20; nächster operativer
/goal-Einstieg P01 Block 19/20.** Unabhängige
P02-Vorarbeiten dürfen weiterlaufen.

## P02 unabhängige Vorarbeit – Chat-Zeitstempel stabilisieren (02.11.2026)

**Code-Commit bde46c18** auf origin/ogl-develop.
**20/20 technische Prüfpositionen**; ausdrücklich
keine formale P02-Phasenabnahme. Linux-Vollviewer
7.2.5.162 erfolgreich gebaut; **240/240 Python-
Tests und 41/41 native CTests** bestanden.
88/88 gezielte Branding-Tests bestanden, zusätzlich
bei ASCII-Systemkodierung (LC_ALL=C,
PYTHONUTF8=0). Isolierte deutsche und englische
GUI-Starts bei 1280x720 bis STATE_LOGIN_WAIT,
kein AUTH_INIT; beide Fenster sauber geschlossen,
Exit-Code 0. Kein authentifizierter Chat-Test.

**Bestehende Dateien direkt überarbeitet (sieben):**
- indra/newview/fschathistory.cpp: Im aktiven
  FSChatHistoryHeader dehnt die Timestamp-Zeile
  den Text und die horizontale Namens-Layoutgruppe
  nicht mehr dynamisch aus. Der vollständige
  Zeitwert steht als Tooltip bereit. Ungenutzten
  mHeaderLayoutStack-C++-Zeiger entfernt.
  Anonymisierung, Radar-Bild, Namen und die
  bestehende Zeit-Sichtbarkeitslogik bleiben erhalten.
- panel_chat_header.xml (englische Basis):
  rechts ausgerichteter Zeitbereich bleibt 72
  Layout-Pixel breit und verwendet Ellipsen.
- Existierende DE-, JA-, PT- und TR-Sprach-
  Overlays: feste Demo-Uhrzeiten entfernt;
  alle anderen Sprachen erben die englische
  Basis oder enthalten keinen Zeit-Override.
- ogl/tests/test_branding_contract.py: drei
  zusätzliche Regressionstests für keine
  Chatzeit-Layoutverschiebung, fixe/ellipsierte
  Time-Box, unveränderte runde Avatare und
  alle lokalisierten Zeitplatzhalter.

**20 technische Prüfpunkte dieser Runde:**
01 Branch und Ursprung kontrolliert;
02 P01-/P02-Blocker erneut geprüft;
03 aktive statt deaktivierter Chatklasse geprüft;
04 Resize-Problem isoliert;
05 unveränderte Zeitwert-Zuweisung;
06 vollständiger Zeitstempel als Tooltip;
07 Zeitfeld wächst nicht dynamisch;
08 kein Verschieben der Avatar-/Namens-Layoutgruppe;
09 toten C++-Layoutzeiger entfernt;
10 feste EN-Zeitbox mit 72px geprüft;
11 Ellipsenmodus geprüft;
12 runde Avatar- und Namensdarstellung erhalten;
13 DE-Demowert entfernt;
14 JA-Demowert entfernt;
15 PT-Demowert entfernt;
16 TR-Demowert entfernt;
17 88/88 Branding-Tests auch unter ASCII erfolgreich;
18 Linux-Vollbuild und 240 Python-/41 native
Tests sowie 7/7 bytegleiche XUI-Staging-Dateien;
19 DE-/EN-Voranmelde-GUI bis STATE_LOGIN_WAIT
und Exit-Code 0;
20 geprüften Code auf GitHub gepusht,
Dokumentation und Altbestand aktualisiert.
Sieben vorhandene _v2-/script_copy-Pfade
nicht ohne Abhängigkeits-/Lizenznachweis gelöscht;
keine neuen Projektdateien, keine Nutzerdaten,
Caches oder gespeicherte Zugangsdaten verändert.

**Offene Gates:** Authentifizierter
Hauptgrid-Login, echte gespeicherte
Credential-Wiederherstellung, reale
In-World-Chatdarstellung und Shutdown,
kompletter Windows-/macOS-Viewer-Build
und GUI-Test, Beta-TLS-Hostnameproblem.
Portable Grafikmodule-CI ist kein vollständiger
Windows-/macOS-Viewer-Test.
**Formal P01 weiterhin 18/20, P02 0/20**;
nächster Einstieg /goal P01 Block 19/20;
unabhängige P02-Vorarbeiten bleiben möglich.

## P02 unabhängige Vorarbeit – OGL-Systemicons und Chatköpfe (02.11.2026)

Code-Commit 6b6779fc direkt auf origin/ogl-develop.
Unabhängige P02-20er-Runde; keine formale Freigabe der
gesamten Phase. Linux-Vollviewer 7.2.5.160 erfolgreich.
237/237 Python- und 41/41 native CTests bestanden;
85/85 Branding-Tests zusätzlich mit LC_ALL=C und
PYTHONUTF8=0 bestanden. Isolierte DE-/EN-1280x720-
Xvfb-Viewerstarts bis STATE_LOGIN_WAIT und saubere
WM-Close-/Exit-Code-0-Beendigungen geprüft.
Keine Anmeldung, kein In-World-Test durchgeführt.

Änderungen ausschließlich in sechs bestehenden Dateien:
- textures.xml verweist OGL_System_Icon auf das
  bereits vorhandene windows/login_ogl_logo.png;
  kein neuer Asset oder Systemicon-Fallback gelöscht.
- fschathistory.cpp (aktiver FS-Chat-History-Pfad):
  normale System-/Regionmeldungen tragen im OGL-Kanal
  OGL_System_Icon. Radar-Avatarbilder und RLVa-
  Namensfilter-Zweige sind unverändert.
  Die alte llchathistory.cpp ist mit #if 0 deaktiviert
  und wird ausdrücklich nicht als aktiver Pfad gewertet.
- llchatitemscontainerctrl.cpp (Nearby-Chat-Toast):
  Systemmeldungen verwenden das OGL-Icon,
  echte Avatare weiterhin ihre IDs/Bilder.
- llnotificationlistitem.cpp:
  eingeklappte/ausgeklappte systembezogene
  Benachrichtigungsicons tragen dieselbe OGL-Marke.
- panel_chat_header.xml: Test-Name und fiktive
  Anzeigeuhrzeit aus der XUI-Basis entfernt, bevor
  echte Chatmetadaten geliefert werden.
- test_branding_contract.py: fünf neue Regressionstests
  für Icon-Registry, aktive Chat-/Toast-Pfade,
  beide Benachrichtigungsansichten und XUI-Metadaten.
  Nicht-OGL-Kanäle behalten die vorherigen Icon-Namen.

20 technische Prüfpositionen dieser Runde:
01 korrekter Branch; 02 keine fremden lokalen Änderungen;
03 aktive FSChatHistory statt deaktivierter LLChatHistory
ermittelt; 04 bestehenden OGL-Asset verifiziert;
05 Icon-Name in existierender Registry eingetragen;
06 FS-Chat-System-/Region-Nachrichten umgestellt;
07 Radar-Avatar-Fall unverändert; 08 RLVa-
Anonymisierungspfad unverändert; 09 Nicht-OGL-
Icon-Fallback erhalten; 10 Nearby-Toast-Systemicon;
11 Nearby-Toast-Avatar-ID erhalten; 12 kompaktes
Benachrichtigungsicon; 13 erweitertes Benachrichtigungsicon;
14 Demo-Chatname aus XUI entfernt; 15 Demo-Zeit
aus XUI entfernt; 16 fünf neue Regressionstests
und 85 Branding-Tests auch unter ASCII;
17 Linux-Vollbuild plus 237 Python-/41 native Tests;
18 drei Ressourcen (Registry, OGL-PNG,
Chat-XUI) bytegleich im paketierten Build;
19 DE- und EN-GUI-Voranmelde-Starts Exit-Code 0;
20 Code direkt gepusht, Status/Altbestand inventarisiert.
Sieben bestehende _v2-/script_copy-Dateipfade
nicht ohne Abhängigkeits-/Lizenznachweis gelöscht.
Keine Nutzerprofile, Caches, Credentials oder Secrets
verändert, keine Projektdateien neu angelegt.

Offen bleiben: Hauptgrid-Live-Login mit autorisiertem
Testkonto, echte Credential-Wiederherstellung,
In-World-Chat-/Benachrichtigungs-Sichtprüfung
und Shutdown, vollständige Windows-/macOS-
Viewer-Build-/GUI-Nachweise. Beta-TLS-Fehler
ist weiterhin ungeklärt, Beta-Login blockiert.
Die portable Grafikmodule-CI ersetzt keine
Vollviewer-Tests. Formal weiterhin P01 18/20,
P02 0/20, nächster operativer Block
/goal P01 Block 19/20. Unabhängige P02-
Vorarbeiten können parallel fortgesetzt werden.

## P02 unabhängige Vorarbeit – 20 Blöcke Menübereinigung und Windows-CI (02.11.2026)

Code-Commit ef3dc0e5 direkt auf origin/ogl-develop.
20/20 technische Arbeitsschritte abgeschlossen;
keine formale P02-Fertigstellung. Linux-Vollviewer
7.2.5.158 gebaut; 232/232 Python-Tests, 41/41 native
CTests und 80/80 Branding-Tests bestanden. Zusätzlich
80 Branding-Tests mit LC_ALL=C und PYTHONUTF8=0
bestanden, um die Windows-Defaultkodierung unabhängig
von der Linux-UTF-8-Standardeinstellung zu prüfen.

Implementierung nur in 13 bestehenden Projektdateien:
- EN-Basismenü und elf bestehende Sprach-Overlays
  AZ, DE, ES, FR, IT, JA, PL, PT, RU, TR, ZH:
  das alte DeprecatedHelp-Untermenü mit alten
  Second-Life-Issue-Tracker- und Hilfeseiten entfernt.
  Dänisch definiert keinen eigenen DeprecatedHelp-
  Eintrag und erbt die englische Menüstruktur.
- Die aktuellen NexVerse-Hilfemenüs, Report Bug,
  Script Library, Release-Links und technischen
  Advanced-/Admin-Menüs bleiben bestehen.
- Der vorhandene englische Advanced-Webbrowser-Test
  öffnet statt einer alten Second-Life-Webseite
  https://stadt-nexverse.de/.
- Zwei neue Menüregressionstests und fünf bestehende
  Textleser im vorhandenen Branding-Test ausdrücklich
  auf UTF-8 umgestellt. Vorher war GitHub-Lauf
  41491396250 auf Windows-2022 und Windows-11-ARM
  wegen eines cp1252-UnicodeDecodeError gescheitert;
  die vier übrigen Matrixjobs waren erfolgreich.
  Der korrigierte GitHub-CI-Lauf steht noch aus.

20 tatsächlich geprüfte technische Arbeitsschritte:
01 Branch/Arbeitsverzeichnis sauber;
02 Windows-CI-Fehlerlog ausgewertet;
03 Python-Textdekodierung korrigiert;
04 alle 13 Menü-Overlays und Callback-Abhängigkeiten geprüft;
05 englische Menü-Basis korrigiert; 06 AZ-Overlay;
07 DE; 08 ES; 09 FR; 10 IT; 11 JA; 12 PL;
13 PT; 14 RU; 15 TR; 16 ZH;
17 Advanced-Browser-Smoke auf NexVerse korrigiert;
18 80 Branding-Tests auch mit ASCII-Systemkodierung bestanden;
19 Linux-Vollbuild, 232 Python-Tests und 41 native
CTests, 13/13 XUI-Menüs bytegleich im Build;
DE-/EN-Xvfb-Voranmelde-GUI jeweils mit
STATE_LOGIN_WAIT, kein AUTH_INIT, Exit-Code 0;
20 Code auf GitHub gepusht, Status/Altbestand geprüft.
Keine neuen Projektdateien; sieben bekannte alte
_v2-/script_copy-Dateipfade ohne Nutzungs-/Lizenznachweis
nicht gelöscht. Keine Nutzerdaten, Credentials,
User-Caches, Secrets verändert.

Nicht abgenommen: authentifizierter Hauptgrid-Live-
Login, echte Passwortwiederherstellung und In-World-
Shutdown, vollständige Windows-/macOS-Vollviewer-
Build-/GUI-Tests. Beta-TLS-Hostnameproblem offen,
Beta-Login weiterhin gesperrt. Portable Matrix-CI
ersetzt diese Viewer-Nachweise nicht.
Formaler Stand P01 18/20 und P02 0/20.
Nächster /goal-Einstieg P01 Block 19/20;
unabhängige P02-Vorarbeiten bleiben möglich.

## P02 unabhängige Vorarbeit – externe Altlinks in Einstellungen (02.11.2026)

**Code-Commit bf031d29** auf origin/ogl-develop;
Linux-Vollviewer 7.2.5.156 erfolgreich gebaut.
**230/230 Python-Tests, 41/41 native CTests, 78/78 gezielte
Branding-Regressionsprüfungen erfolgreich.** Echte isolierte
DE-/EN-Xvfb-Login-GUI-Smokes jeweils mit STATE_LOGIN_WAIT,
ohne STATE_LOGIN_AUTH_INIT, per Fenster geschlossen, Exit-Code 0.

**Geändert nur an 20 bestehenden Projektdateien:**
- 13 bestehende panel_preferences_setup.xml-Sprachvarianten:
  alte Verweise auf die externe Firestorm-Wiki bei
  Bandbreite/Cache entfernt, lokale Überschriften,
  Einstellungen und Layoutanker erhalten. Türkische
  Overlay-Datei definiert nur die Bandbreitenüberschrift.
- EN/DE/JA/PL/ZH panel_preferences_chat.xml:
  toten Firestorm-Wiki-Link für den vorhandenen
  Chat-Befehlsbereich durch kurze, nicht klickbare
  Überschrift ersetzt. Command-Checkbox und bestehende
  Befehlsbeispiele bleiben erhalten.
- llfloaterpreference.cpp: Die alte Second-Life-Konto-
  E-Mail-Einstellungsseite wird in OGL auch bei
  unerwarteter Second-Life-Grideinstellung nicht wieder
  sichtbar. Der vorhandene OpenSim-Checkboxpfad für
  Offline-IM-Mail bei unterstützten Grids bleibt erhalten;
  sonstige Viewer-Kanäle behalten ihr bisheriges Verhalten.
- ogl/tests/test_branding_contract.py: drei zusätzliche
  Regressionstests, alle 13 Setup-Sprachen und fünf
  Chat-Overlays per XML auf Funktionserhalt geprüft.

**20 technische Prüfpositionen (keine formalen P02-Blöcke):**
01 korrekter Branch, 02 origin-Synchronität, 03 keine
untracked Projektdateien, 04 ursprüngliche UI-Controls
erhalten, 05 Netz-Bandbreite-Label, 06 Cache-Label,
07 alte Wiki-Links nicht mehr verfügbar, 08 alle 13
Setup-XUI gültig, 09 fünf Chat-Doku-XUI gültig,
10 Chat-Command-Schalter vorhanden, 11 alte
Account-Seite im OGL-Kanal versteckt, 12 OpenSim-
E-Mail-Checkbox bleibt vorhanden, 13 Upstream-
Nicht-OGL-Pfad erhalten, 14 Quelltextregressionen,
15 78 Branding-Tests, 16 230 Python-Tests, 17
41 native CTests, 18 voller Linux-Build und 18
geänderte XUI-Quellen bytegleich im Paket, 19 echter
DE-Xvfb-Login-Smoke/Exit 0, 20 echter EN-Xvfb-
Login-Smoke/Exit 0 sowie Altbestandkontrolle.
Sieben vorhandene _v2-/script_copy-Pfade inventarisiert,
aber ohne sichere Abhängigkeits-/Lizenzprüfung nicht
gelöscht. Nutzerprofile, Secrets und Caches nicht geändert.

**Nicht abgenommen:** authentifizierter Hauptgrid-Live-
Login, Restore realer gespeicherter Passwörter,
In-World-Shutdown, Windows-/macOS-Vollviewer-Builds
und GUI-Tests. Die portable Grafikmodul-CI ersetzt
diese Nachweise nicht; Beta-TLS-Hostnamenproblem nach
vorigem Befund weiter offen, Beta-Anmeldung gesperrt.
**P01 weiterhin 18/20; P02 formal 0/20.**
Nächster operativer Einstieg /goal P01 Block 19/20,
unabhängige P02-Vorarbeit bleibt möglich.

## P02 unabhängige Vorarbeit – OGL-Datenschutz bei Absturzberichten (02.11.2026)

**Code-Commit: f1552bb1** (origin/ogl-develop).
**Linux-Vollbuild: erfolgreich, 227/227 Python- und 41/41 native
CTests bestanden.** Isolierte 1280×720-EN-/DE-Xvfb-
Login-GUI-Läufe jeweils bis STATE_LOGIN_WAIT,
kein STATE_LOGIN_AUTH_INIT und sauberer Fenster-Exit 0.
**Formale Phasen weiter P01 18/20 und P02 0/20.**

**Implementierung ausschließlich in sechs vorhandenen Quelldateien:**
- indra/newview/llappviewerlinux.cpp: OGL darf den
  bisherigen Linux-Breakpad-/Fremdserver-Uploadpfad auch
  bei übernommenen Einstellungen nicht initialisieren.
- indra/newview/llappviewerwin32.cpp: Der bisherige
  BugSplat-Sender wird unter OGL nicht konstruiert.
  Die reguläre Viewer-Initialisierung läuft weiter.
- indra/newview/llappdelegate-objc.mm: macOS startet
  den vorhandenen BugSplat-Dienst im OGL-Kanal nicht;
  normale App-Initialisierung bleibt unberührt.
- indra/llcrashlogger/llcrashlogger.cpp: Auch ein
  separat gestarteter alter Crash-Logger verweigert unter
  OGL die Initialisierung bereits vor HTTP-Start.
- indra/newview/llfloaterpreference.cpp: Der nicht mehr
  passende Firestorm-Crash-Upload-Tab wird im OGL
  unabhängig von der Compile-Time-Reporting-Option
  entfernt. Fremde Kanäle bleiben kompatibel.
- ogl/tests/test_branding_contract.py: fünf neue
  plattformspezifische Quellcode-Regressionsprüfungen.
  75/75 Branding-Tests bestanden.

**20 technische Prüfpositionen (keine formalen Phasenblöcke):**
01 korrekter Branch; 02 origin synchron; 03 keine neuen
Projektdateien; 04 Linux-Sender-Gate vorhanden; 05
übernommene Einstellung erzwingt keine Freigabe;
06 Nicht-OGL-Linux-Verhalten erhalten; 07 Windows-
Bugsplat-Gate; 08 Windows-Basisinitialisierung erhalten;
09 macOS-Sender-Gate; 10 macOS-Viewer-Start unverändert;
11 direkte Crash-Logger-Sperre; 12 Sperre vor
HTTP-Initialisierung; 13 OGL-Crash-Tab verborgen;
14 Reporting-Compile-Option weiterhin berücksichtigt;
15 bestehende Nutzer-Konfigurationsdateien unverändert;
16 fünf zusätzliche Regressionstests; 17 75 Branding-
Tests; 18 227 Python-/41 native Tests und Linux-Vollbuild;
19 echter DE-Xvfb-Voranmeldelauf/sauberer Exit;
20 echter EN-Xvfb-Voranmeldelauf/sauberer Exit und
Altbestand-Inventur (sieben vorhandene _v2-/script_copy-
Pfade, ohne Abhängigkeits-/Lizenznachweis nicht gelöscht).

**Testgrenzen:** Der Linux-Build war ohne BugSplat
konfiguriert. Die Windows-/macOS-Codepfade sind durch
statische Regressionstests abgedeckt, aber noch **nicht**
durch vollständige native Viewer-Builds oder Laufzeittests
bestätigt. Ein zukünftiger freiwilliger OGL-Crashupload
benötigt einen geprüften eigenen Dienst mit eigener
Datenschutzerklärung und ausdrücklicher Einwilligung.
Alle aktuellen lokalen Benutzer-/Credential-Daten
bleiben unberührt.

**Weiter offen:** autorisierter Live-Login des Hauptgrids,
echte gespeicherte Credential-Wiederherstellung, In-World-
Shutdown, vollständige Windows-/macOS-Viewer-Tests und
das zuvor festgestellte Beta-TLS-Zertifikatsproblem.
Nächster operativer Einstieg: /goal P01 Block 19/20.
Unabhängige P02-Vorarbeiten können fortgesetzt werden.

## P02 unabhängige Vorarbeit – eingebetteter NexVerse-Webbrowser (02.11.2026)

**Code-Commit:** `912a7320` auf `origin/ogl-develop`.
**Vollständiger Linux-Vollviewer-Build erfolgreich; 222/222 Python-
und 41/41 native CTests grün.** Formale Phasenstände bleiben
**P01 18/20, P02 0/20**: Diese einzelne, vollständig getestete
P02-Vorarbeit ist kein Ersatz für eine Welt-/Plattformabnahme.

**Implementiert, ausschließlich in 12 vorhandenen Quelldateien:**
- `indra/newview/llweb.cpp`: Die vorhandene Browser-Präferenz
  `PreferredBrowserBehavior=1` öffnet im OGL-Kanal nur noch
  explizite HTTPS-Adressen der exakt zugelassenen Hosts
  `stadt-nexverse.de` und `marketplace.stadt-nexverse.de`
  im internen Webbrowser (Standard-HTTPS-Port oder 443).
  Ungültige URLs, HTTP, fremde Hosts, Subdomain-Täuschungen und
  andere Ports fallen auf den externen Systembrowser zurück.
  Die bisherige Second-Life-Linkpolitik für Nicht-OGL-Kanäle
  und die Präferenzwerte bleiben kompatibel.
  Diese Linkauswahl erteilt Webseiten **keine** erhöhten
  Browserrechte; die vorhandene Web-Floater-Vorgabe bleibt
  `trusted_content=false`.
- In 10 bestehenden `panel_preferences_setup.xml`-Sprachdateien
  sind Beschriftung und Tooltip der vorhandenen Option auf
  NexVerse aktualisiert. Die Overlays `da`, `es` und `tr`
  definieren keinen eigenen `new_internal`-Eintrag und
  übernehmen daher unverändert die englische Basis.
- Zwei neue Regressionstests in der **bestehenden**
  `ogl/tests/test_branding_contract.py`; insgesamt
  70/70 Branding-Tests bestanden.

**20 überprüfte technische QA-Positionen (keine formalen
P02-Phasenblöcke):** 01 richtiger Branch; 02 Remote synchron
vor Änderung; 03 nur Bestandsdateien; 04 URI-Parser-Prüfung;
05 ungültige URI extern; 06 OGL-Kanal getrennt; 07 HTTPS nötig;
08 exakte Portal-Domain; 09 exakte Marketplace-Domain;
10 keine beliebigen Unterdomains; 11 Port 443/Standard;
12 sonst externe Browserwahl; 13 Upstream-Policy intakt;
14 unveränderte Preference-ID; 15 zehn lokalisierte
Beschriftungen; 16 drei sprachliche Vererbungen; 17 gültiges
XUI und Bytegleichheit aller zehn gestagten XML-Dateien;
18 222 Python-Tests und 41 native CTests; 19 isolierter
1280×720-EN-Xvfb-Viewerstart bis `STATE_LOGIN_WAIT`;
20 Fenster per WM-Close beendet, Exit-Code 0.
Einzelfälle der Hostprüfung sind bislang durch Quelltext-
Regression und Vollkompilierung, **nicht** als native
Browser-Klick-Matrix geprüft. Inventur fand sieben alte
`_v2`-/`script_copy`-Dateipfade; keine davon ohne
Abhängigkeits- und Lizenzprüfung gelöscht.

**Offene Blocker unverändert:** Hauptgrid-Live-Login mit
autorisiertem Konto, Wiederherstellung echter gespeicherter
Credentials, In-World-Shutdown, vollständige Windows-/
macOS-Viewer-Builds und GUI-Abnahmen; Beta-TLS nach
vorherigem Befund ungeklärt und Anmeldung dort gesperrt.
**Nächster operativer Block: `/goal P01 Block 19/20`.**
Unabhängige P02-Vorarbeit darf parallel weitergehen.

## P01 Block 19 – gespeicherter Passwortplatzhalter bleibt verborgen (20 Prüfpunkte)

**Code-Commit `b2520078`** auf `ogl-develop`.
**20/20 technische Prüfpositionen bestanden.**
Die formale P01-Phase bleibt **18/20**,
P02 bleibt **0/20**.

**Implementiert ausschließlich in bestehenden Dateien:**

- `indra/newview/fspanellogin.cpp`:
  Bei einem gespeicherten Credential
  steht im Passwortfeld nur der
  maskierte, nicht entschlüsselbare
  Anzeigetext `Enter a password`.
  Der OGL-Augenknopf darf diesen
  Platzhalter nicht als echtes
  Passwort offenlegen: Beide
  Augenknöpfe bleiben deaktiviert,
  solange keine eigene neue
  Passworteingabe erfolgt ist.
  Auch ein programmgesteuerter
  Klick auf `onShowHidePasswordClick`
  wird in diesem Zustand abgewiesen.
- Manuell eingegebene Passwörter
  dürfen weiterhin über das Auge
  angezeigt und wieder verborgen
  werden. Nach dem vollständigen
  Löschen des Passworts wird
  `mShowPassword` wieder auf
  `false` gesetzt, sodass die
  nächste Eingabe maskiert beginnt.
  Kontoauswahl, `setFields`,
  Gridwechsel und direkte
  Benutzernamenbearbeitung
  synchronisieren Sichtbarkeit
  und Aktivierungszustand.
  Nicht-OGL-Kanäle behalten
  ihre ursprüngliche
  Augenknopf-Aktivierungslogik.
- Fünf weitere Regressionstests
  in der bereits vorhandenen
  `ogl/tests/test_nexverse_grid.py`.
  **Keine neuen Projektdateien,
  keine vorhandenen Credentials
  oder Nutzerdaten überschrieben.**
  Vorhandene Altdateien mit
  `_v2`/`script_copy` nur
  inventarisiert, nicht blind
  entfernt.

**20 bestandene Prüfpositionen:**
01 Branch; 02 GitHub-Synchronität;
03 keine neuen Dateien;
04 zwei feste Grids;
05 öffentliche Hauptgrid-
Auskunft bestätigt exakten
OpenSim-HTTP-Endpunkt;
06 Beta-Authentifizierung im
Quellcode weiter gesperrt;
07 gespeicherter Platzhalter
nicht aufdeckbar;
08 Abbruch vor Eye-Toggle;
09 kein Credential-Store-
Zugriff im Eye-Handler;
10 beide Augenknöpfe
bis zur manuellen Eingabe
deaktiviert; 11 OGL-spezifische
Logik ohne Upstream-
Nebenwirkungen; 12
Maskierung bei unerlaubter
Freigabe; 13 Aktivierung
nach Passworteingabe;
14 Reset nach Löschen;
15 UI-Sync nach `setFields`;
16 Sync nach gespeicherter
Kontoauswahl; 17 Sync bei
Grid-/Benutzernamenwechsel;
18 Linux-Vollbuild und
220 Python-/41 native Tests;
19 echter DE-Xvfb-GUI-Start,
kein Login, regulärer Quit;
20 visueller EN-Xvfb-
Login-Screenshot.
**20/20 bestanden.**

**Netz-/Plattformnachweise:**
`http://hg.stadt-nexverse.de:80/get_grid_info`
lieferte HTTP 200, XML
`platform=OpenSim`,
`login=http://hg.stadt-nexverse.de:80/`.
Bei `https://beta.stadt-nexverse.de/get_grid_info`
schlug die Zertifikatsprüfung
wegen Hostnamenabweichung
fehl. Beta **nicht** freigeben.

`bash ogl/scripts/build-test.sh`
vollständig erfolgreich:
**220/220 Python-Tests und
41/41 native CTests**,
Linux-Vollviewer kompiliert.
Im isolierten 1280×720-Xvfb-
Lauf **OGL Viewer Development
7.2.5.149**, `STATE_LOGIN_WAIT`,
kein `STATE_LOGIN_AUTH_INIT`,
DE-Exit-Code **0**;
EN-Login als Bild visuell
geprüft. Der zusätzliche
Beta-GUI-Interaktionslauf
konnte nicht abgeschlossen
werden und zählt ausdrücklich
**nicht** als bestanden.
Ebenso wurde kein gespeichertes
echtes Passwort im angemeldeten
Client getestet; die
Augensymbol-Sonderfälle sind
durch Code-/Regressionstests
und Kompilierung belegt.

**Weiter offen:** autorisierter
Hauptgrid-Live-Login,
tatsächliches Restore
gespeicherter Credentials,
In-World-Shutdown sowie
vollständige Windows-/macOS-
Viewer-Build-/GUI-Abnahmen.
Nächster Einstieg bleibt
`/goal P01 Block 19/20`,
unabhängige P02-Vorarbeit
parallel zulässig.
Keine manuelle Zwischenabnahme.

## P01 Block 19 – Passwortsichtbarkeit bei Konto-/Gridwechsel zurücksetzen

**Code-Commit `c3db484e`** (`ogl-develop`).
**20/20 technische Arbeits- und Einzelprüfpositionen bestanden.**
Die formalen P01-Phasenblöcke bleiben **18/20**,
P02 bleibt **0/20** bis zu echten Live-/
Plattform-Viewer-Abnahmen.

**Entwicklung ausschließlich in vorhandenen Dateien:**
- In `indra/newview/fspanellogin.cpp` und
  `fspanellogin.h` ist
  `resetOGLPasswordVisibility()` ergänzt.
  Das zuvor per Auge eingeblendete
  Klartext-Passwort wird beim Wechsel
  des gespeicherten Kontos, beim
  direkten Bearbeiten des Benutzernamens,
  beim Laden eines anderen Credentials
  und beim Gridwechsel automatisch
  wieder maskiert. Nach dem Wechsel
  muss die Sichtbarkeit bewusst erneut
  aktiviert werden.
- Der vorhandene Augenknopf bleibt
  für die **aktuell bearbeitete**
  Eingabe funktionsfähig. Das Reset
  wird ausschließlich im OGL-Kanal
  und nur bei zuvor aktiviertem
  Klartextmodus durchgeführt.
  Passwortinhalt, gesicherte
  Credentials und benutzerdefinierte
  Speicherung bleiben unangetastet.
  Bestehende HTTP-Einmalfreigaben
  werden bei Grid-/Kontoänderung
  weiterhin invalidiert.
- Fünf neue Regressionstests in
  `ogl/tests/test_nexverse_grid.py`.
  Keine weiteren Projektdateien
  erzeugt. Alte `_v2`-/`script_copy`
  Ressourcen auf Namen geprüft,
  nicht ohne Abhängigkeitsbeweis
  entfernt.

**20 bestandene Prüfblöcke:** 01
Branch; 02 origin-Sync; 03
keine neuen Dateien; 04
Maskierungshelper; 05
nur OGL und zuvor aktivierte
Sichtbarkeit; 06 synchroner
Augenknopf-Reset; 07 kein
Löschen von Secrets; 08
manuelles Anzeigen bleibt
verfügbar; 09 beide Augenknöpfe;
10 Textmaskierung;
11 Laden gespeicherter
Konten; 12 Gridwechsel
vor Feldlöschung; 13
HTTP-Freigabe nach Gridwechsel
ungültig; 14 andere
Kontoauswahl; 15 direkte
Benutzernamenbearbeitung;
16 gleiche Kontoauswahl
behält bewusst aktivierten
Sichtbarkeitsmodus; 17
Nicht-OGL-Kompatibilität;
18 Linux-Vollbuild und Tests;
19 deutscher GUI-Start;
20 französischer GUI-Start.
**20/20 erfolgreich.**

**Nachweise:** vollständiger
`bash ogl/scripts/build-test.sh`
mit **215/215 Python-Tests
und 41/41 nativen CTests**,
Linux-Mint-Vollviewer
erfolgreich gebaut.
Zwei isolierte 1280×720-
Xvfb-GUI-Starts des
**OGL Viewer Development
7.2.5.147** in DE/FR:
`STATE_LOGIN_WAIT`, keine
`STATE_LOGIN_AUTH_INIT`,
Strg+Q mit **Exit-Code 0**.
Ein interaktiver End-to-End-
Klicktest der Augentasten nach
Kontowechsel steht weiter aus;
die konkrete Zustandslogik
ist durch Regressionstests,
Kompilierung und Quelltext-
Prüfungen abgesichert.

**P01 weiterhin formal
blockiert:** autorisierter
Hauptgrid-Live-Login,
echte Wiederherstellung
gespeicherter Passwörter,
In-World-Shutdown und
vollständige Windows-/
macOS-Viewer-Build-/GUI-
Abnahmen. Operativer Einstieg
bleibt `/goal P01 Block 19/20`;
unabhängige P02-Vorarbeit
ist zulässig. Keine manuelle
Zwischenabnahme.

## CI-Nachprüfung – sechs portable Plattformjobs erfolgreich

GitHub Actions `ogl-portable-graphics.yml`:
**Lauf 41479072688**, Code und
Dokumentation bis `511f889c`,
**6/6 Jobs erfolgreich**
(Ubuntu x64/ARM64, Windows
x64/ARM64, macOS Intel/ARM64).
Bestätigt wurden Python-
Quelltextverträge und native
Grafikmodule auf allen sechs
Plattformzielen. Dies ist kein
vollständiger Windows-/macOS-
Viewer-Build und keine
In-World-Laufzeitabnahme.
**P01 bleibt 18/20, P02 0/20.**

## P01 Block 19 – externe Welcome-Seite nicht mehr privilegiert (20 Prüfpunkte)

**Code-Commit `3a655add` auf `ogl-develop`.**
**20/20 einzeln nachgewiesene technische Prüfpositionen bestanden.**
Formaler P01-Stand bleibt **18/20**, P02 **0/20**.

**Implementiert in bereits vorhandenen Dateien:**
- Das reguläre Loginlayout
  `skins/default/xui/en/panel_fs_nui_login.xml`
  und die bereits vorhandenen Varianten
  `starlight` und `starlightcui`
  setzen `trusted_content="false"`
  für den externen Welcome-Webbrowser.
  OGL-Logo, Splash-Seite, Eingabefelder
  und Schaltflächen bleiben erhalten.
- `indra/newview/fspanellogin.cpp`
  erzwingt beim Laden der OGL-
  Welcome-Seite zusätzlich
  `web_browser->setTrustedContent(false)`
  **vor** jeder Navigation. Damit kann
  selbst eine künftige Skin-Überlagerung
  den Fremdinhalt nicht erneut als
  privilegiert markieren. Die
  unveränderte `LLMediaCtrl`-Basis
  gibt den Trustwert an die Media-
  Engine und den SLURL-Dispatcher
  weiter. Die fest eingetragene
  HTTPS-Welcome-Adresse, der
  `ForceLoginURL`-Schutz und die
  vorhandene sparsame
  Browsertelemetrie bleiben erhalten.
- Drei neue Python-Regressionstests
  in der bestehenden Datei
  `ogl/tests/test_branding_contract.py`.
  **Keine neuen Projektdateien.**
  Alte `_v2`-/`script_copy`-Namen
  wurden geprüft, aber ohne sichere
  Abhängigkeitsprüfung nicht gelöscht.

**20 erfolgreiche Kontrollen:**
01 Branch, 02 Remote-Sync,
03 keine neuen Dateien,
04–06 Standard-, Starlight-
und Starlight-CUI-Browser ohne
Privilegien, 07 fehlender
Tastaturfokus, 08 responsive
Fensteranbindung, 09 feste
Welcome-URL, 10 gesperrter
URL-Override, 11 beschränkte
Browsertelemetrie, 12 C++-
Enforcement, 13 Enforcement
vor Navigation, 14 kein
Regrant, 15 Trustweitergabe
an Engine, 16 Trustweitergabe
an SLURL-Dispatcher,
17 bytegleiches Linux-Staging
aller drei Skins, 18 Vollbuild
mit Python-/Native-Tests,
19 englischer GUI-Start
und 20 deutscher GUI-Start.
**20/20 bestanden.**

**Nachweise:** Vollständiger Linux-
Viewer-Build über
`bash ogl/scripts/build-test.sh`,
**210/210 Python-Tests,
41/41 native CTests** bestanden.
Zwei echte isolierte 1280×720-
Xvfb-GUI-Starts: **OGL Viewer
Development 7.2.5.144**,
`STATE_LOGIN_WAIT`, keine
`STATE_LOGIN_AUTH_INIT`,
sauberer Strg+Q-Abschluss mit
**Exit-Code 0** (EN und DE).
Der Vertrauensentzug wurde durch
Quellcode, XML, Paketinhalte und
Regressionstests geprüft. Es wurde
**kein** externer Angriff simuliert
und keine Live-Grid-Anmeldung
durchgeführt.

**Weiterer Einstieg:** P01 Block 19/20,
parallel unabhängige P02-Vorarbeit.
Für formalen P01-Abschluss fehlen
weiterhin ein autorisierter
Hauptgrid-Live-Login, echte
Passwortwiederherstellung und
In-World-Shutdown sowie vollständige
Windows-/macOS-Viewer-Build- und
GUI-Tests. Keine manuelle
Zwischenabnahme erforderlich.

## Plattformnachprüfung der aktiven Login-XUI – 6/6 grün

GitHub-Actions-Workflow `ogl-portable-graphics.yml`
auf dem gepushten Stand `c8359daa`,
Lauf **41476349998**, **6/6 Jobs
erfolgreich** (Ubuntu x64/ARM64,
Windows x64/ARM64, macOS Intel
und Apple Silicon). Die
sprachübergreifenden Quelltext-
und nativen Grafikmodule wurden
geprüft; dies ist **kein**
vollständiger Build der Windows-/
macOS-Viewer oder eine Live-
Anmelde-/In-World-Abnahme.
**P01 bleibt 18/20, P02 0/20.**

## P01 Block 19 – aktive Login-XUI und Lokalisierungen korrigiert (20 Prüfblöcke)

**Code-Commit `f76ad6f1`** (`ogl-develop`).
**20/20 unabhängige Entwicklungs- und
Prüfpositionen bestanden.** P01 bleibt
**formal 18/20**, P02 **formal 0/20**,
da reale autorisierte Anmeldung und
volle Windows-/macOS-Viewer-Abnahme
noch ausstehen.

**Umgesetzt, nur vorhandene Dateien bearbeitet:**

- Die wirklich verwendete
  `skins/default/xui/en/panel_fs_nui_login.xml`
  hatte `max_length_chars="16"` für das
  Passwort; auf `255` erhöht. Dies entspricht
  dem bestehenden OpenSim-Laufzeitwert
  `MAX_PASSWORD_OPENSIM=255`.
  Maskierung, Auge-Buttons,
  Fokussteuerung und grafisches
  OGL-Welcome-Fenster bleiben erhalten.
- Die zehn vorhandenen aktiven
  Login-Sprachen AZ, DE, EN, FR, IT,
  JA, PL, PT, RU und ZH werben
  nicht mehr für beliebige
  Grid-URIs, sondern erläutern
  die feste Auswahl **Stadt NexVerse**
  und **Stadt NexVerse Beta** und
  die noch gesperrte Beta-Anmeldung.
  Die alten Second-Life-
  Passwortlinks in FR/PT/JA
  wurden auf das offizielle
  `https://stadt-nexverse.de/`
  zurückgeführt. Ein eigener
  bestätigter Passwort-Reset-
  Endpoint ist weiterhin nicht
  angegeben und wurde
  deshalb **nicht** erfunden.
- Die zwei bereits existierenden
  vollständigen Login-XUI-
  Varianten `starlight` und
  `starlightcui` auf denselben
  EN-Stand gebracht; sie werden
  **nicht** als neue Skins
  angelegt oder ersetzt.
  Vier weitere Regressionstests
  in der vorhandenen
  `ogl/tests/test_branding_contract.py`.
  Keine neuen Quell-/Projektdateien.
  Alte `_v2`-/`script_copy`-
  Ressourcen lediglich überprüft,
  nicht ohne Nachweis gelöscht.

**20 bestandene Prüfblöcke:**
01 Branch; 02 Remote-Sync; 03 keine
neuen Projektdateien; 04
valide OGL-Login-Basis;
05 255-stelliges Passwortfeld;
06 Maskierung/Auge;
07 OpenSim-Laufzeitlimit;
08 alle zehn Lokalisierungen valide;
09 beide Grids genannt;
10 keine frei wählbaren URI-Grids
beworben; 11 Beta-Einschränkung
erkennbar; 12 DE/EN-Hilfe erhalten;
13 vorhandene Passwort-Links
zur offiziellen Domain;
14 FR/PT/JA ohne Second Life;
15 Bedienelemente/Callbacks
vorhanden; 16 Starlight/
Starlight-CUI identisch zur Basis;
17 elf geänderte XUI-Dateien
bytegleich im Staging;
18 Linux-Vollbuild, 207 Python
und 41 native Tests;
19 deutscher GUI-Start;
20 französischer GUI-Start.
**20/20 bestanden.**

**Nachweis:** `bash ogl/scripts/build-test.sh`
vollständig erfolgreich, **207/207
Python-Tests, 41/41 native CTests**,
Linux-Vollviewer. Zwei isolierte
1280×720-Xvfb-GUI-Starts in
DE und FR: Fenster **OGL Viewer
Development 7.2.5.141**, Status
`STATE_LOGIN_WAIT`, kein
`STATE_LOGIN_AUTH_INIT`, sauberes
Strg+Q, Exit-Code **0**.
Die neue Eingabegrenze und alle
Lokalisierungswerte wurden
durch XML-/Build-Tests geprüft;
ein interaktiver 255-Zeichen-
Passwort-Login mit realem Account
wurde **nicht** durchgeführt.

**Offene formale P01-Gates:**
autorisierter Hauptgrid-Live-Login,
Passwortwiederherstellung/Persistenz
nach erfolgreicher Anmeldung,
In-World-Shutdown und vollständige
Windows-/macOS-Viewer-Builds/GUI-
Tests. Nächster operativer Einstieg
`/goal P01 Block 19/20`,
parallel unabhängige P02-Vorarbeiten.
Keine manuelle Zwischenabnahme.

## Nachprüfung – plattformübergreifende Grafikmodul-CI

GitHub Actions `ogl-portable-graphics.yml`,
**Lauf 41340630759**, HEAD
`54d4cd7a`: **6/6 Plattformjobs
erfolgreich**. Die plattformneutrale
Credential-Policy im bereits vorhandenen
Grid-HTTP-Nativtest und die insgesamt
203 Python-Verträge bestanden auf
Ubuntu x64 und ARM64, Windows x64
und ARM64 sowie macOS Intel und
Apple Silicon. Der lokale
Linux-Vollviewer-Build mit
41 nativen CTests bestand ebenfalls.

**Abgrenzung:** Das ist eine portable
Grafikmodul- und Quelltext-Testmatrix,
keine vollständige Windows-/macOS-
Viewer-Build-/GUI-Abnahme; die
formalen P01/P02-Stände bleiben
**18/20 und 0/20**.

## P01 Block 19 – gespeicherte Zugangsdaten an echte Kontoauswahl gebunden

**Code-Commit:** `7ab16aaf` auf `ogl-develop`.
**20/20 technische Entwicklungs-/QA-Positionen bestanden.**
Formale P01-Abnahme weiterhin **18/20**;
P02 bleibt **0/20**, bis die fehlenden
End-to-End- und Plattformnachweise
tatsächlich vorhanden sind.

**Änderungen – ausschließlich an vier bestehenden Quelldateien:**

- `indra/newview/fspanellogin.cpp`: In
  `FSPanelLogin::getFields` wird ein
  bestehender verschlüsselter
  Authenticator im OGL-Kanal nur dann
  gelesen, wenn die ausdrücklich
  aus der Kontenliste ausgewählte
  Identität exakt mit dem aktuellen
  Formular und dem ausgewählten
  NexVerse-Grid übereinstimmt und
  das Passwort nicht bearbeitet wurde.
  Das bloße Eintippen eines bereits
  gespeicherten Benutzernamens
  reicht ausdrücklich nicht aus.
  Manuelle Passworteingaben bleiben
  nutzbar und werden von früheren
  Credential-Pointern getrennt.
- Fehlende oder zwischenzeitlich
  verschwundene gespeicherte
  Credentials werden ohne
  Authentifizierungsversuch
  zurückgewiesen. Ein überholter
  maskierter Passwort-Platzhalter
  darf keinen Login auslösen.
  Die eigentliche HTTP-Warnung
  und die Beta-Protokollsperre
  bleiben unangetastet.
- `indra/newview/oglnexversegrid.h`:
  die reine, plattformneutrale
  Policy `canReuseSavedAuthenticator`
  prüft exakte Kontoidentität,
  kanonische Haupt-/Beta-Gridkeys,
  echtes Auswahlereignis, unverändertes
  Passwort und nicht leeren
  Benutzernamen.
- `ogl/tests/test_grid_http_policy.cpp`:
  **16 ausführbare native Testfälle**
  für erlaubte und abgelehnte
  Kombinationen, auch in
  Release-Builds mit aktiven
  Assertions. Fünf zusätzliche
  Python-Regressionstests in
  `ogl/tests/test_nexverse_grid.py`.
  Keine neuen Projektdateien,
  Accounts, Backups oder Secrets;
  keine Änderungen an der
  allgemeinen Nicht-OGL-
  Authentifizierung.

**20 bestandene Prüfblöcke:**
01 Branch; 02 Upstream-Sync; 03
keine neuen Projektdateien;
04 zwei feste Gridprofile;
05 verwendete OGL-Credential-Policy;
06 echte gespeicherte Kontoauswahl;
07 exakte Formular-/Kontogleichheit;
08 Passwortänderung;
09 nur kanonische Gridkeys;
10 exakter Grid-Suffix;
11 nicht leerer Benutzername;
12 Auswahlprüfung vor Secret-Zugriff;
13 veralteter Passwort-Platzhalter
fail-closed; 14 fehlendes gespeichertes
Credential fail-closed;
15 keine alten Credential-Pointer
bei manueller Eingabe;
16 Nicht-OGL-Kompatibilität;
17 16 echte native Positiv-/
Negativassertions;
18 Linux-Vollbuild, Python- und CTests;
19 Hauptgrid-HTTP-Warnungs-GUI;
20 Beta-Protokollsperr-GUI.
**20/20 bestanden.**

**Prüfnachweis:** Vollständiger
`bash ogl/scripts/build-test.sh`
erfolgreich: **203/203 Python-Tests,
41/41 native CTests**, kompilierter
Linux-Mint-Viewer. Isolierte
1280×720-Xvfb-GUI-Negativläufe
mit synthetischen Eingaben:
Hauptgrid-HTTP-Warnung und
Beta-Anmeldesperre bestätigt,
kein `STATE_LOGIN_AUTH_INIT`,
beide Strg+Q-Exit-Code **0**.
Die native Auswahlpolicy wurde
ausgeführt; ein realer
Secure-Store-Re-Login mit
gültigem Benutzerkonto ist
weiterhin **nicht** nachgewiesen.

**Wiedereinstieg:** `/goal P01
Block 19/20`; unabhängig
testbare P02-Vorarbeiten bleiben
zulässig. Für P01 weiterhin offen:
autorisierter Live-Login,
echte Passwortpersistenz im
Hauptgrid, In-World-Shutdown und
vollständige Windows-/macOS-
Viewer-Build-/GUI-Tests.
Keine Nutzerzwischenabnahme nötig.

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

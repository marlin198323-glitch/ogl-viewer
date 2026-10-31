# OGL Firestorm – P32: echter NexVerse Chat-Umbau

Stand 31.10.2026. Linux Mint, GitHub ogl-viewer, Branch ogl-develop.

## Auftrag und visuelle Vorlage

Der alte Firestorm-Dialog "Gespräche – Antonia Porta" soll zu einem
modernen NexVerse-Chat mit dunkler Navy-/Cyan-Oberfläche werden.
Wichtigste Anforderungen: tatsächliche OpenSim-Avatarbilder,
Kontakte links, Chat rechts, pro Nachricht erst Avatar und
Name/Uhrzeit, danach Nachrichtentext. Nicht nur ein Bild;
die bestehende OpenSim-Kommunikation muss unverändert funktionieren.

## Technischer Befund

Die alten Linden-Labs-Fenster floater_im_container.xml und
floater_im_session.xml sind in dieser Firestorm-Version deaktiviert.
Die tatsächlich aktiven Floater sind:
- floater_fs_im_container.xml -> FSFloaterIMContainer
- floater_fs_im_session.xml -> FSFloaterIM
- FSChatHistory / panel_chat_header.xml
- Firestorms eigene, registrierte Chat-Optionen.

Änderungen an den alten LL-Fenstern wären wirkungslos gewesen.
Diese Runde aktualisiert nur die vorhandenen aktiven Dateien.

## 20 Umsetzungsblöcke

1. Ausgangsscreens ausgewertet und bestehendes Layout verglichen.
2. Richtiges GitHub-Repository auf Linux Mint überprüft.
3. Veraltete LL-Chatfenster und aktive FS-Chatfenster unterschieden.
4. Echten Nachrichten-Widget- und Avatar-Bildcode identifiziert.
5. Hauptfenster auf 1080×650 als neuen Standard vergrößert;
   untere responsive Grenzen bleiben erhalten.
6. Gesprächsliste links auf 252 px und Tabhöhe auf 42 px erweitert.
7. Firestorms vorhandene benutzerdefinierte Tab-Icons aktiviert.
8. Echte OpenSim-Avatar-UUIDs an Foto-Controls der privaten
   Gespräche gebunden; Avatare stammen aus den Profilbildern.
9. Gruppen zeigen reale Gruppen-Icons und keine Fantasie-Avatare.
10. Einen ausschließlich für OGL genutzten dunkelblauen UI-Farbsatz
    mit Cyan-Akzent hinzugefügt, statt andere Viewerfenster
    oder die globale Firestorm-Farbgebung umzuschreiben.
11. IM-Hauptbereich und Eingabe vergrößert; Text bleibt skalierbar.
12. Vorhandenen, bereits mit sendMsgFromInputEditor verknüpften
    Senden-Button sichtbar gemacht, statt einen Fake-Button zu bauen.
13. Neue Nachrichtenvorlage mit 34×34-Avatar, hervorgehobenem
    Namen und realer Chat-Uhrzeit eingefügt.
14. Nachrichtenkörper im selben Chatverlauf unter dem Kopf
    ausgerichtet, bei weiterhin funktionierender Textselektion,
    URLs, Links, IM-Emotes, Moderatoraktionen und Chatprotokollen.
15. Wiederholte Nachrichten derselben Person werden im
    modernen Modus nicht mehr unter einen alten Header gruppiert:
    jede Nachricht bekommt eigenen Avatar/Name/Uhrzeit.
16. In IMs von standardmäßig reinem Textverlauf auf
    Rich-Chat-Ansicht umgeschaltet; Nearby-Chat behält
    seine bisherigen Nutzereinstellungen.
17. Persistenten Schalter OGLModernConversationCards ergänzt;
    Legacy-Darstellung bleibt jederzeit verfügbar.
18. Schalter in vorhandene Firestorm-Chat-Optionen eingebaut,
    deutsch und englisch benannt, mit echten ToggleControl-
    und CheckControl-Callbacks.
19. Automatische Tests für aktive Fensternamen, XUI-XML,
    Avatarbindung, Nachrichtenmetadaten, Eingabe/Versand,
    Datenschutz (RLVa), Sprachdateien und Rückschalter erstellt.
20. Linux-C++-Build, Python-Tests, alle GPU-/Modultests,
    XUI-Paketverteilung und GitHub-Synchronisierung ausgeführt.

## Nutzungsanleitung

Den bestehenden Viewer vollständig schließen. Den aktuellen
lokalen, frisch gebauten Viewer starten:

    cd "/home/jam/Schreibtisch/OGL-Viewer/ogl-firestorm"
    bash ogl/scripts/run-viewer-test.sh

In OpenSim anmelden und "Gespräche" öffnen. Eine echte IM oder
Gruppenunterhaltung auswählen. In der großen linken Liste erscheint
bei einem registrierten Avatar dessen Profilbild, rechts stehen Name,
Uhrzeit und tatsächliche empfangene Nachrichten.

NexVerse-Karten zurückschalten unter:
Chat-Optionen > NexVerse Chat: Avatar, Name und Uhrzeit je Nachricht

Bei ausgeschaltetem Schalter gilt der bisherige Firestorm-Textstil
und die vorherige zeitliche Gruppierung für neue Nachrichten.

## Ehrliche Grenzen

Die optische Vorlage ist ein Design-Mockup, keine unmittelbar
vorhandene Firestorm-Komponente. Dieser Stand modernisiert die
wirklich vorhandenen Gesprächsfenster und zeigt echte Profile und
Metadaten. Es sind noch nicht alle Mockup-Funktionen realisiert:
eine neue "In der Nähe / Region / Gruppen / Direkt"-Filterleiste,
Chatkarten mit komplexen Chat-Bubble-Hintergründen,
Live-Freunde-Onlinezählung, Social-Kontaktvorschauen und
Sprachpegel-Dashboard existieren noch nicht als neue Funktionen.
Die realen Firestorm-Navigationstabs, Voice- und Emoji-Funktionen
bleiben bewusst erhalten. Weder fingierte Nachrichten noch
erfundene Profilbilder werden angezeigt.

Eine manuelle grafische Abnahme nach OpenSim-Login
ist weiterhin nötig. Der Build/Test allein beweist nicht, dass
jede UI-Anordnung bei allen Displaygrößen exakt der Vorlage
entspricht. Noch eventuell nötige Nachjustierungen anhand des
nächsten echten Viewer-Screenshots vornehmen.

# OGL Firestorm – P33: Einheitliches Design aller aktiven Chatansichten

31.10.2026 | Linux Mint | ogl-develop

## Auftrag

Avatarbild links, Name und Uhrzeit oben, Nachrichtentext darunter.
Gemeinsames modernes Navy-/Cyan-Design und abgerundete
Nachrichtenelemente für alle aktiven Chatbereiche.
Änderungen befinden sich in den bestehenden Firestorm-Dateien,
nicht in einem losgelösten Mockup.

## Bearbeitete aktive Oberflächen

1. Direkte IMs und Konferenzen in FSFloaterIM nutzen seit P32
   echte OpenSim-Profile und Einzelkopf pro Nachricht.
2. Gruppenchat verwendet dieselbe Nachrichtenkomponente
   mit der tatsächlichen Absender-ID jedes Gruppenmitglieds.
3. Der Chat in der Nähe nutzt jetzt ebenfalls die gemeinsame
   Einstellung OGLModernConversationCards.
4. Für jeden lokalen Chat wird im Kartenmodus die echte Zeit
   ausgegeben; auch wiederholte Nachrichten desselben Kontakts
   erhalten einen eigenen Kopf.
5. Die stummgeschaltete Nachrichtenansicht verwendet
   exakt denselben FSChatHistory-Nachrichtenstil.
6. Gespeicherte Verläufe setzen nun pro Nachricht das gleiche
   Karten-Argument, den Absender und die gespeicherte Uhrzeit.
7. Fenster der lokalen Chats wurden vergrößert und
   mit Navy-Hintergrund und Cyan-Akzent versehen.
8. Eingabe- und Chat-Layout bleiben über die vorhandenen
   Firestorm-Steuerungen skalierbar.
9. Das bestehende globale FSChatHistory-XUI-Widget definiert
   konsistente Einzüge, Abstände und Farben für die Unterbereiche.
10. Die Header verwenden echte OpenSim-Avatarbilder aus der Cache-API.
11. Eine vorhandene native Rounded_Rect-Textur gestaltet
    die Nachrichtenköpfe als gerundete Karten; keine neuen Assets.
12. Der lokale 3D-Toast zeigt Avatar, Namen und tatsächliche
    Eingangszeit, dann den Chattext in der folgenden Zeile.
13. Im modernen Modus werden aufeinanderfolgende lokale
    Toast-Nachrichten nicht länger zusammengefasst.
14. Die alte Zusammenfassung bleibt im klassischen Modus erhalten.
15. System- und Objektnachrichten behalten vorhandene
    System-/Objektsymbole statt falscher Avatarfotos.
16. Bei gesperrter Namensanzeige (RLVa) werden echte
    Avatarbilder im Toast nicht mehr offengelegt.
17. Die Chatverlauf-Übersicht zeigt größer formatierte
    echte Avatar- und Gruppensymbole sowie vorhandene Zeitwerte.
18. Die Verlaufsübersicht erhält keine erfundenen Nachrichten-
    Vorschautexte; echte Texte stehen im geöffneten Protokoll.
19. Bestehende Funktionen für Senden, Emoji, Gruppen, Voice,
    Kanal, Flüstern/Sprechen/Rufen, Suche und Mute bleiben erhalten.
20. Python-/native-/Vulkan-Tests, Linux-Paketdateien und GitHub-
    Freigabe werden für den aktuellen Stand geprüft.

## Optische Abnahme

Die technische Umsetzung bildet die Vorlage nach, aber eine
pixelgenaue 1:1-Übereinstimmung auf allen Fenstergrößen ist ohne
visuelle Prüfung des laufenden Viewers nicht nachgewiesen.
Die 3D-Toast-Uhrzeit ist eine reale Eingangszeit, gespeicherte
Verläufe erhalten ihre tatsächlichen Zeitstrings.

Viewer-Start auf Linux Mint:

    cd "/home/jam/Schreibtisch/OGL-Viewer/ogl-firestorm"
    bash ogl/scripts/run-viewer-test.sh

Abnahme: Direktnachrichten, Gruppenchat, Chat in der Nähe,
stummgeschaltete Nachrichten, historische Verläufe und 3D-
Einblendungen anhand echter OpenSim-Nachrichten öffnen.

Unter Chat-Optionen kann der Schalter
"NexVerse Chat: Avatar, Name und Uhrzeit je Nachricht"
für den bisherigen Firestorm-Darstellungsmodus deaktiviert werden.

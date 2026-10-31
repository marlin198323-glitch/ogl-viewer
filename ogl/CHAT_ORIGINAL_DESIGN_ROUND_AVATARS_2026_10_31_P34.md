# OGL Firestorm – ursprünglicher Chat mit runden Avataren (P34)

Datum 31.10.2026, Linux Mint, Branch ogl-develop.

## Auftrag

Die vorherige Navy-/Cyan-Neugestaltung des Chats vollständig
zurücknehmen. Firestorms originale Fenstergrößen, Farben, Schriften,
Abstände, Nachrichtengruppierung, Textverlauf, Eingabebereiche,
Menüs und Send-Button-Verhalten sollen wieder gelten. Nur echte
OpenSim-Avatarbilder im Chat dürfen bleiben und rund erscheinen.

## Implementiert

- Den Git-Zustand a2942513 vor dem Chatdesign für 19 aktive
  Chat-Layout-, Farb-, Eingabe- und Nachrichtenquellen wiederhergestellt.
- Das normale 396x390 große Firestorm-Gesprächsfenster mit
  ursprünglichen 115px-Gesprächstabs ist zurück.
- Die ursprüngliche Darstellung für Gruppenchat, Chat in der
  Nähe, Direktnachrichten, stummgeschalteten Chat, gespeicherte
  Nachrichten und 3D-Chat-Einblendungen ist zurück.
- Die navyblauen OGLChat-Farben, vergrößerten Felder,
  modernen Karten, erzwungenen Zeitstempel und neue
  Chat-Optionen sind entfernt.
- Originale Firestorm-Einstellungen (PlainTextChatHistory,
  FSShowTimestampsIM etc.) bleiben wieder maßgeblich.
- Das vorhandene Profilfoto für die P2P-Konversation in
  FSFloaterIMContainer bleibt über die tatsächliche
  OpenSim-Avatar-ID gebunden. Die originale Größe bleibt.
- LLAvatarIconCtrl hat ein optionales circular-Flag (default false).
  Nur die tatsächlichen Chat-Avataranzeigen setzen es auf true.
  Ein kreisförmiger Texturpolygon-Fan mit 64 Segmenten
  zeichnet ausschließlich den runden Bereich des echten
  Profilbilds – kein viereckiges Bild mit darüberliegendem Fake-Rahmen.
- Für noch nicht geladene Fotos wird das alte generische
  Platzhaltersymbol weiterhin in Firestorm angezeigt.
- Avatare an anderen Stellen des Viewers werden nicht verändert.
- Aktuelle Tests prüfen ausdrücklich Originalfarben/-maße und
  den echten runden Avatar-Zeichenpfad.
- Keine neue oder duplizierte Skin-Datei erstellt.

## Test auf dem Linux-Mint-PC

Bisherigen Viewer schließen, danach:

    cd "/home/jam/Schreibtisch/OGL-Viewer/ogl-firestorm"
    bash ogl/scripts/run-viewer-test.sh

Im Viewer Chat öffnen, ein geladenes Bewohnerprofil prüfen.
Es muss das originale Firestorm-Chatdesign mit rundem
echtem Avatarfoto anzeigen. Bei fehlendem Profilfoto
erscheint vorübergehend das originale Symbol.
Eine visuelle Abnahme anhand eines Screenshots bleibt erforderlich.

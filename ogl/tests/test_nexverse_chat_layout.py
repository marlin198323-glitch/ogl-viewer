"""Static active-Firestorm UI regression: real avatar/message metadata.

Keep native OpenSim IM callbacks and profile image fetches intact. This
test checks the REGISTERED FS floaters, NOT unused legacy LL IM floaters.
"""
from pathlib import Path
import xml.etree.ElementTree as ET
import unittest

ROOT=Path(__file__).resolve().parents[2]
SKIN=ROOT/"indra/newview/skins/default"
XUI=SKIN/"xui/en"
SOURCE=ROOT/"indra/newview"

def xui(file):
    return ET.parse(XUI/file).getroot()


class NexVerseChatLayoutTests(unittest.TestCase):
    def test_live_firestorm_floater_registration(self):
        source=(SOURCE/"llviewerfloaterreg.cpp").read_text()
        self.assertIn('LLFloaterReg::add("fs_im_container", "floater_fs_im_container.xml"',source)
        self.assertIn('LLFloaterReg::add("fs_impanel", "floater_fs_im_session.xml"',source)

    def test_scoped_neon_palette_exists(self):
        xml=ET.parse(SKIN/"colors.xml").getroot()
        names={c.get("name") for c in xml.findall("color")}
        for color in ("OGLChatCanvas","OGLChatPanel","OGLChatMessage",
                      "OGLChatAccent","OGLChatTimestamp","OGLChatInput"):
            self.assertIn(color,names)

    def test_real_conversations_sidebar_and_responsive_layout(self):
        root=xui("floater_fs_im_container.xml")
        self.assertGreaterEqual(int(root.get("width")),960)
        self.assertGreaterEqual(int(root.get("height")),560)
        self.assertLessEqual(int(root.get("min_width")),760)
        tab=root.find(".//tab_container[@name='im_box_tab_container']")
        self.assertIsNotNone(tab)
        self.assertEqual(tab.get("tab_position"),"left")
        self.assertEqual(tab.get("use_custom_icon_ctrl"),"true")
        self.assertGreaterEqual(int(tab.get("tab_height")),34)
        self.assertGreaterEqual(int(tab.get("tab_width")),220)

    def test_each_message_renders_real_profile_and_metadata(self):
        x=xui("panel_chat_header.xml")
        self.assertEqual(x.get("bg_alpha_color"),"OGLChatMessage")
        self.assertGreaterEqual(int(x.get("height")),38)
        image=x.find(".//avatar_icon[@name='avatar_icon']")
        self.assertIsNotNone(image)
        self.assertGreaterEqual(int(image.get("height")),32)
        self.assertIsNone(image.get("visibility_control"))
        self.assertIsNotNone(x.find(".//text[@name='user_name']"))
        time=x.find(".//text[@name='time_box']")
        self.assertIsNotNone(time)
        self.assertEqual(time.get("text_color"),"OGLChatTimestamp")
        self.assertEqual(time.get("value"),"") # no fake messages

        source=(SOURCE/"fschathistory.cpp").read_text()
        self.assertIn('!args["ogl_message_cards"].asBoolean()',source)
        self.assertIn('args["ogl_message_cards"].asBoolean() ?',source)
        self.assertIn('icon->setValue(chat.mFromID);',source)

    def test_real_im_history_and_bound_send_button(self):
        root=xui("floater_fs_im_session.xml")
        self.assertGreaterEqual(int(root.get("min_height")),280)
        history=root.find(".//fs_chat_history[@name='chat_history']")
        self.assertIsNotNone(history)
        self.assertEqual(history.get("bg_readonly_color"),"OGLChatCanvas")
        self.assertGreaterEqual(int(history.get("left_text_pad")),48)
        entry=root.find(".//chat_editor[@name='chat_editor']")
        self.assertIsNotNone(entry)
        self.assertEqual(entry.get("bg_writeable_color"),"OGLChatInput")
        button=root.find(".//button[@name='send_chat']")
        self.assertIsNotNone(button)
        # The button is visible and is already connected to genuine IM sending.
        parent=root.find(".//layout_panel[@name='input_button_layout_panel']")
        self.assertIsNotNone(parent)
        self.assertIsNone(parent.get("visibility_control"))
        code=(SOURCE/"fsfloaterim.cpp").read_text()
        self.assertIn('getChild<LLButton>("send_chat")->setCommitCallback',code)
        self.assertIn('sendMsgFromInputEditor',code)

    def test_avatar_photos_from_open_sim_not_mockups(self):
        code=(SOURCE/"fsfloaterimcontainer.cpp").read_text()
        self.assertIn("LLAvatarIconCtrl::Params params;",code)
        self.assertIn("params.avatar_id = session->mOtherParticipantID;",code)
        self.assertIn("mTabContainer->setTabImage(floaterp, portrait)",code)
        self.assertIn("LLGroupIconCtrl::Params params;",code)
        # An actual session, not locally fabricated contacts, supplies IDs.
        self.assertIn("findIMSession(floaterp->getKey())",code)

    def test_legacy_mode_is_recoverable_and_privacy_preserved(self):
        root=ET.parse(SOURCE/"app_settings/settings.xml").getroot()
        # The main LLSD settings XML stores keys in alternating <key>/<map>.
        self.assertIn("OGLModernConversationCards",
                      (SOURCE/"app_settings/settings.xml").read_text())
        code=(SOURCE/"fsfloaterim.cpp").read_text()
        self.assertIn('gSavedSettings.getBOOL("OGLModernConversationCards")',code)
        history=(SOURCE/"fschathistory.cpp").read_text()
        self.assertIn("chat.mRlvNamesFiltered",history)
        self.assertIn("CHAT_SOURCE_SYSTEM",history)

    def test_existing_chat_options_menu_controls_real_mode(self):
        root=xui("menu_fs_chat_options.xml")
        switch=root.find(".//menu_item_check[@name='ogl_modern_conversation_cards']")
        self.assertIsNotNone(switch)
        self.assertEqual(
            switch.find("menu_item_check.on_click").get("parameter"),
            "OGLModernConversationCards")
        self.assertEqual(
            switch.find("menu_item_check.on_check").get("parameter"),
            "OGLModernConversationCards")
        de=ET.parse(SKIN/"xui/de/menu_fs_chat_options.xml").getroot()
        translated=de.find(
            ".//menu_item_check[@name='ogl_modern_conversation_cards']")
        self.assertIsNotNone(translated)
        self.assertIn("Avatar",translated.get("label"))

    def test_nearby_and_muted_nearby_use_same_avatar_message_cards(self):
        cpp=(SOURCE/"fsfloaternearbychat.cpp").read_text()
        self.assertIn('gSavedSettings.getBOOL("OGLModernConversationCards")',cpp)
        self.assertIn('chat_args["ogl_message_cards"] = ogl_cards;',cpp)
        self.assertIn('mChatHistoryMuted->appendMessage(chat, chat_args',cpp)
        self.assertIn('mChatHistory->appendMessage(chat, chat_args',cpp)
        self.assertIn('ogl_cards || gSavedSettings.getBOOL("FSShowTimestampsNearbyChat")',cpp)
        root=xui("floater_fs_nearby_chat.xml")
        self.assertEqual(root.get("bg_opaque_color"),"OGLChatCanvas")
        self.assertGreaterEqual(int(root.get("width")),600)
        self.assertGreaterEqual(int(root.get("height")),540)
        for name in ("chat_history","chat_history_muted"):
            history=root.find(".//fs_chat_history[@name='%s']" % name)
            self.assertIsNotNone(history)
            self.assertGreaterEqual(int(history.get("left_text_pad")),48)
            self.assertEqual(history.get("bg_readonly_color"),"OGLChatCanvas")
        self.assertIsNotNone(root.find(".//chat_editor[@name='chat_box']"))
        self.assertIsNotNone(root.find(".//button[@name='send_chat']"))
        self.assertIsNotNone(root.find(".//combo_box[@name='chat_type']"))
        self.assertIsNotNone(root.find(".//spinner[@name='ChatChannel']"))

    def test_saved_conversation_preview_has_consistent_metadata(self):
        code=(SOURCE/"llfloaterconversationpreview.cpp").read_text()
        self.assertIn('chat_args["ogl_message_cards"] = ogl_cards;',code)
        self.assertIn('chat_args["show_time"] =',code)
        self.assertIn('mChatHistory->appendMessage(chat,chat_args);',code)
        self.assertIn('chat.mFromID = from_id;',code)
        self.assertIn('chat.mTimeStr = time;',code)
        root=xui("floater_conversation_preview.xml")
        self.assertGreaterEqual(int(root.get("width")),700)
        history=root.find(".//fs_chat_history[@name='chat_history']")
        self.assertEqual(history.get("bg_readonly_color"),"OGLChatCanvas")
        self.assertGreaterEqual(int(history.get("left_text_pad")),48)
        self.assertIsNotNone(root.find(".//button[@name='search_btn']"))
        self.assertIsNotNone(root.find(".//button[@name='open_external_btn']"))

    def test_shared_avatar_header_spacing_and_real_system_icons(self):
        widget=xui("widgets/fs_chat_history.xml")
        self.assertGreaterEqual(int(widget.get("left_text_pad")),48)
        self.assertEqual(widget.get("message_header"),"panel_chat_header.xml")
        source=(SOURCE/"fschathistory.cpp").read_text()
        self.assertIn('icon->setValue(chat.mFromID);',source)
        self.assertIn('icon->setValue(LLSD("OBJECT_Icon"));',source)
        self.assertIn('icon->setValue(LLSD("SL_Logo"));',source)
        self.assertIn('icon->setValue(LLSD("Unknown_Icon"));',source)

    def test_toast_uses_sender_and_real_arrival_time_above_body(self):
        toast=(SOURCE/"llchatitemscontainerctrl.cpp").read_text()
        handler=(SOURCE/"llfloaterimnearbychathandler.cpp").read_text()
        self.assertIn("static std::string oglToastArrivalTime()",toast)
        self.assertIn('gSavedSettings.getBOOL("OGLModernConversationCards")',toast)
        self.assertIn('fromName + "   " + oglToastArrivalTime() + "\\n"',toast)
        self.assertIn("mMsgText->appendText(str_sender, false",toast)
        self.assertIn("mMsgText->appendText(messageText, false",toast)
        self.assertIn('icon->setValue(mFromID)',toast)
        self.assertIn('icon->setValue(LLSD("Unknown_Icon"))',toast)
        self.assertIn("!gSavedSettings.getBOOL(\"OGLModernConversationCards\")",handler)
        x=xui("panel_chat_item.xml")
        image=x.find(".//avatar_icon[@name='avatar_icon']")
        self.assertGreaterEqual(int(image.get("width")),30)
        self.assertIsNotNone(x.find(".//text_chat[@name='msg_text']"))

    def test_conversation_log_lists_real_contact_and_time_not_fake_message(self):
        source=(SOURCE/"llconversationloglistitem.cpp").read_text()
        self.assertIn('avatar_icon->setValue(mConversation->getParticipantID());',source)
        self.assertIn('group_icon->setValue(mConversation->getSessionID());',source)
        self.assertIn('mConversationDate->setValue(mConversation->getTimestamp());',source)
        x=xui("panel_conversation_log_list_item.xml")
        self.assertGreaterEqual(int(x.get("height")),32)
        self.assertEqual(
            x.find(".//text[@name='date_time']").get("text_color"),
            "OGLChatTimestamp")
        self.assertIsNotNone(x.find(".//text[@name='conversation_name']"))
        self.assertIsNone(x.find(".//text[@name='fake_message_preview']"))

    def test_reversible_cards_setting_affects_live_and_history(self):
        for cpp in ("fsfloaterim.cpp","fsfloaternearbychat.cpp",
                    "llfloaterconversationpreview.cpp",
                    "llchatitemscontainerctrl.cpp",
                    "llfloaterimnearbychathandler.cpp"):
            self.assertIn("OGLModernConversationCards",
                          (SOURCE/cpp).read_text())

    def test_localized_german_inheritance_and_no_duplicate_files(self):
        self.assertEqual(xui("floater_fs_im_session.xml").get("name"),"panel_im")
        de=ET.parse(SKIN/"xui/de/floater_fs_im_session.xml").getroot()
        self.assertIsNotNone(de.find(".//button[@name='send_chat']"))
        self.assertEqual(de.find(".//button[@name='send_chat']").get("label"),
                         "Senden")


if __name__=="__main__":
    unittest.main()

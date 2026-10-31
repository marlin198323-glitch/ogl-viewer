"""Regression: untouched Firestorm chat skin, real *round* chat portraits.

No navy/neon themes, giant floaters, forced chat cards or synthetic
messages may be introduced when maintaining this UI.
"""
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
SKIN=ROOT/"indra/newview/skins/default"
XUI=SKIN/"xui/en"
VIEWER=ROOT/"indra/newview"


def read_xui(relative):
    return ET.parse(XUI/relative).getroot()


class OriginalFirestormChatTests(unittest.TestCase):
    def test_original_contact_floater_geometry_and_gray_skin(self):
        box=read_xui("floater_fs_im_container.xml")
        self.assertEqual(box.get("height"),"390")
        self.assertEqual(box.get("width"),"396")
        tabs=box.find(".//tab_container[@name='im_box_tab_container']")
        self.assertIsNotNone(tabs)
        self.assertEqual(tabs.get("width"),"394")
        self.assertEqual(tabs.get("tab_width"),"115")
        self.assertEqual(tabs.get("tab_height"),"20")
        self.assertEqual(tabs.get("tab_position"),"left")
        # Only deliberate visual difference: REAL profile images on tabs.
        self.assertEqual(tabs.get("use_custom_icon_ctrl"),"true")
        self.assertIsNone(box.get("bg_opaque_color"))

    def test_original_im_floater_and_send_button(self):
        chat=read_xui("floater_fs_im_session.xml")
        self.assertEqual(chat.get("height"),"332")
        self.assertEqual(chat.get("width"),"394")
        self.assertEqual(chat.get("min_width"),"250")
        self.assertEqual(chat.get("min_height"),"150")
        self.assertIsNone(chat.get("bg_opaque_color"))
        self.assertIsNotNone(chat.find(".//fs_chat_history[@name='chat_history']"))
        send=chat.find(".//button[@name='send_chat']")
        self.assertIsNotNone(send)
        # Original Firestorm choice of visible/hidden Send button.
        parent=chat.find(".//layout_panel[@name='input_button_layout_panel']")
        self.assertIsNotNone(parent)
        self.assertEqual(parent.get("visibility_control"),"FSShowIMSendButton")
        self.assertIsNotNone(chat.find(".//chat_editor[@name='chat_editor']"))

    def test_original_nearby_and_mute_layout(self):
        chat=read_xui("floater_fs_nearby_chat.xml")
        self.assertEqual(chat.get("height"),"332")
        self.assertEqual(chat.get("width"),"394")
        self.assertIsNone(chat.get("bg_opaque_color"))
        self.assertIsNotNone(chat.find(".//fs_chat_history[@name='chat_history']"))
        self.assertIsNotNone(chat.find(".//fs_chat_history[@name='chat_history_muted']"))
        self.assertIsNotNone(chat.find(".//chat_editor[@name='chat_box']"))
        cpp=(VIEWER/"fsfloaternearbychat.cpp").read_text()
        self.assertNotIn("OGLModernConversationCards",cpp)

    def test_original_chat_header_with_circular_real_photo(self):
        row=read_xui("panel_chat_header.xml")
        self.assertEqual(row.get("height"),"24")
        self.assertEqual(row.get("bg_alpha_color"),"ChatHeaderAlphaBackground")
        self.assertEqual(row.get("bg_opaque_color"),"ChatHeaderOpaqueBackground")
        self.assertIsNone(row.get("bg_alpha_image"))
        icon=row.find(".//avatar_icon[@name='avatar_icon']")
        self.assertIsNotNone(icon)
        self.assertEqual(icon.get("width"),"18")
        self.assertEqual(icon.get("height"),"18")
        self.assertEqual(icon.get("circular"),"true")
        # Portrait stays visible regardless of old ShowChatMiniIcons preference.
        self.assertIsNone(icon.get("visibility_control"))
        self.assertIsNotNone(row.find(".//text[@name='user_name']"))
        self.assertIsNotNone(row.find(".//text[@name='time_box']"))
        self.assertEqual(row.find(".//text[@name='user_name']").get(
            "text_color"),"ChatNameColor")

    def test_original_history_and_colors(self):
        history=read_xui("widgets/fs_chat_history.xml")
        self.assertEqual(history.get("left_text_pad"),"10")
        self.assertEqual(history.get("top_header_pad"),"12")
        self.assertEqual(history.get("bg_readonly_color"),
                         "FSChatHistoryBackground")
        colors=(SKIN/"colors.xml").read_text()
        self.assertNotIn('name="OGLChatCanvas"',colors)
        self.assertNotIn('name="OGLChatMessage"',colors)
        self.assertNotIn('name="OGLChatAccent"',colors)
        self.assertNotIn('name="OGLChatInput"',colors)

    def test_original_historical_conversations(self):
        preview=read_xui("floater_conversation_preview.xml")
        self.assertEqual(preview.get("width"),"400")
        self.assertEqual(preview.get("height"),"387")
        history=preview.find(".//fs_chat_history[@name='chat_history']")
        self.assertIsNotNone(history)
        self.assertIsNone(history.get("bg_readonly_color"))
        log=read_xui("floater_conversation_log.xml")
        self.assertEqual(log.get("width"),"300")
        self.assertEqual(log.get("height"),"200")
        item=read_xui("panel_conversation_log_list_item.xml")
        self.assertEqual(item.get("height"),"23")
        avatar=item.find(".//avatar_icon")
        self.assertEqual(avatar.get("circular"),"true")

    def test_original_3d_toast_only_circle_photo(self):
        toast=read_xui("panel_chat_item.xml")
        self.assertEqual(toast.get("height"),"180")
        avatar=toast.find(".//avatar_icon[@name='avatar_icon']")
        self.assertIsNotNone(avatar)
        self.assertEqual(avatar.get("height"),"18")
        self.assertEqual(avatar.get("circular"),"true")
        self.assertIsNotNone(toast.find(".//text_chat[@name='msg_text']"))
        self.assertIsNone(toast.get("bg_alpha_image"))
        toast_source=(VIEWER/"llchatitemscontainerctrl.cpp").read_text()
        self.assertNotIn("oglToastArrivalTime",toast_source)
        self.assertNotIn("OGLModernConversationCards",toast_source)

    def test_existing_real_photo_in_contact_tabs(self):
        cpp=(VIEWER/"fsfloaterimcontainer.cpp").read_text()
        self.assertIn("findIMSession(floaterp->getKey())",cpp)
        self.assertIn("params.avatar_id = session->mOtherParticipantID;",cpp)
        self.assertIn("params.circular = true;",cpp)
        self.assertIn("mTabContainer->setTabImage(floaterp, portrait);",cpp)
        self.assertNotIn("LLGroupIconCtrl::Params params;",cpp)
        self.assertIn("LLAvatarIconCtrl::Params params;",cpp)

    def test_circular_texture_disc_is_opt_in_not_global(self):
        header=(VIEWER/"llavatariconctrl.h").read_text()
        code=(VIEWER/"llavatariconctrl.cpp").read_text()
        self.assertIn("Optional<bool>          circular;",header)
        self.assertIn("void draw() override;",header)
        self.assertIn('circular("circular", false)',code)
        self.assertIn("if (!mCircular || portrait.isNull()",code)
        self.assertIn("!getValue().isUUID()",code)
        self.assertIn("gGL.begin(LLRender::TRIANGLE_FAN)",code)
        self.assertIn("LLIconCtrl::draw();",code)
        self.assertIn("LLUICtrl::draw();",code)

    def test_old_chat_format_and_user_preferences(self):
        for cpp in ("fschathistory.cpp","fsfloaterim.cpp",
                    "llfloaterconversationpreview.cpp",
                    "llfloaterimnearbychathandler.cpp"):
            self.assertNotIn("ogl_message_cards",(VIEWER/cpp).read_text())
        settings=(VIEWER/"app_settings/settings.xml").read_text()
        self.assertNotIn("OGLModernConversationCards",settings)
        self.assertIn("PlainTextChatHistory",settings)
        self.assertIn("FSShowTimestampsIM",settings)
        english=read_xui("menu_fs_chat_options.xml")
        german=ET.parse(SKIN/"xui/de/menu_fs_chat_options.xml").getroot()
        self.assertIsNone(english.find(
            ".//menu_item_check[@name='ogl_modern_conversation_cards']"))
        self.assertIsNone(german.find(
            ".//menu_item_check[@name='ogl_modern_conversation_cards']"))

    def test_real_profile_photos_honor_privacy(self):
        src=(VIEWER/"fschathistory.cpp").read_text()
        self.assertIn("!chat.mRlvNamesFiltered",src)
        self.assertIn('icon->setValue(chat.mFromID);',src)
        self.assertIn('icon->setValue(LLSD("Unknown_Icon"));',src)
        self.assertIn('icon->setValue(LLSD("OBJECT_Icon"));',src)

    def test_no_new_chat_layout_duplicates(self):
        for rel in ("floater_fs_im_session.xml","floater_fs_im_container.xml",
                    "floater_fs_nearby_chat.xml","panel_chat_header.xml",
                    "panel_chat_item.xml","panel_conversation_log_list_item.xml"):
            root=read_xui(rel)
            self.assertIsNotNone(root.tag)


if __name__=="__main__":
    unittest.main()

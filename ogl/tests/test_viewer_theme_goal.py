"""Goal phase 1: assert Firestorm integrates portable safe theme controls."""
from pathlib import Path
import json
import xml.etree.ElementTree as ET
import unittest

ROOT=Path(__file__).resolve().parents[2]
CPP=(ROOT/"indra/newview/llfloaterpreference.cpp").read_text()
HDR=(ROOT/"indra/newview/llfloaterpreference.h").read_text()
XUI=ROOT/"indra/newview/skins/default/xui/en/panel_preferences_ogl_themes.xml"
PALETTE=ET.parse(ROOT/"indra/newview/skins/default/colors.xml")
EXAMPLE=json.loads((ROOT/"ogl/plugins/examples/classic-firestorm/theme.json").read_text())
TOOLS=(ROOT/"ogl/plugins/tools/manage_themes.py").read_text()

class ViewerThemeGoalPhaseOne(unittest.TestCase):
    def test_skins_panel_extension_without_replacing_existing_layout(self):
        panel=ET.parse(XUI).getroot()
        names=[v.get("name") for v in panel.iter()]
        original=(ROOT/"indra/newview/skins/default/xui/en/panel_preferences_skins.xml").read_text()
        for old in ("skin_preview","skin_combobox","theme_combobox",
                    "reset_toolbar_settings","FSToolbarsResetOnModeChange"):
            self.assertIn(old,original)
        for required in (
            "ogl_theme_combo","ogl_theme_folder","ogl_theme_color_bg",
            "ogl_theme_color_text","ogl_theme_color_chat",
            "ogl_theme_activate","ogl_theme_reset","ogl_theme_status"):
            self.assertIn(required,names)
        self.assertIn('filename="panel_preferences_skins.xml"',
                      (ROOT/"indra/newview/skins/default/xui/en/floater_preferences.xml").read_text())
        self.assertIn('filename="panel_preferences_ogl_themes.xml"',
                      (ROOT/"indra/newview/skins/default/xui/en/floater_preferences.xml").read_text())
        self.assertIn('class="panel_preference_skins"',
                      (ROOT/"indra/newview/skins/default/xui/en/floater_preferences.xml").read_text())

    def test_every_advertised_color_maps_to_real_firestorm_color(self):
        import re
        mapping=dict(re.findall(r'\{"([a-z.]+)", "([A-Za-z0-9]+)"\}',CPP))
        supported={name for name in (
            "window.background", "window.text", "window.border",
            "accent", "accent.text", "chat.background", "chat.text",
            "chat.header", "chat.timestamp", "button.background",
            "button.text", "button.border")}
        self.assertEqual(set(mapping),supported)
        actual={item.get("name") for item in PALETTE.iter("color")}
        for color in mapping.values():
            self.assertIn(color,actual)
        self.assertTrue(set(EXAMPLE["colors"]).issubset(supported))

    def test_localized_theme_panel_same_controls(self):
        base=ROOT/"indra/newview/skins/default/xui"
        en=ET.parse(base/"en/panel_preferences_ogl_themes.xml").getroot()
        de=ET.parse(base/"de/panel_preferences_ogl_themes.xml").getroot()
        en_controls={node.get("name") for node in en}
        de_controls={node.get("name") for node in de}
        self.assertTrue(de_controls.issubset(en_controls))
        self.assertIn('name="ogl_themes"',
                      (base/"de/floater_preferences.xml").read_text())
        self.assertIn("OGL-Designs",(base/"de/floater_preferences.xml").read_text())

    def test_ocean_blue_is_valid_and_distinct_from_original(self):
        from importlib.util import spec_from_file_location,module_from_spec
        s=spec_from_file_location("ogl_theme_validate",
                                  ROOT/"ogl/plugins/tools/validate_theme.py")
        mod=module_from_spec(s);s.loader.exec_module(mod)
        ocean=json.loads((ROOT/"ogl/plugins/examples/ocean-blue/theme.json").read_text())
        mod.validate_manifest(ocean)
        self.assertNotEqual(EXAMPLE["colors"]["window.background"],
                            ocean["colors"]["window.background"])
        self.assertEqual(set(ocean["platforms"]),{"linux","windows","macos"})

    def test_preview_never_mutates_live_palette(self):
        beginning=CPP.index("void LLPanelPreferenceOglThemes::previewOglTheme()")
        end=CPP.index("void LLPanelPreferenceOglThemes::onOglThemeActivate()",beginning)
        preview=CPP[beginning:end]
        self.assertIn("setImageColor",preview)
        self.assertNotIn("setColor(",preview)
        self.assertNotIn("saveUserSettings",preview)
        self.assertNotIn("write_json",preview)

    def test_activation_and_reset_preserve_existing_user_colors(self):
        self.assertIn('original-palette.json',CPP)
        self.assertIn('active-theme.json',CPP)
        self.assertIn("snapshot_original();",CPP)
        self.assertIn("restore_original();",CPP)
        self.assertIn("table.saveUserSettings()",CPP)
        self.assertIn("table.resetToDefault(key)",CPP)
        self.assertIn("checked_backup()",CPP)
        self.assertIn("backed_up_color(",CPP)
        self.assertIn('entry.at("custom").as_bool()',CPP)
        self.assertIn('setSelectedByValue("",true)',CPP)

    def test_declared_theme_manifest_checked_before_live_application(self):
        self.assertIn('const auto doc=manifest(id);',CPP)
        self.assertIn('doc.size()!=7',CPP)
        self.assertIn('doc.at("schema_version")!=1',CPP)
        self.assertIn('platforms!=std::set<std::string>',CPP)
        self.assertIn("valid_hex",CPP)
        self.assertIn('fs::is_symlink',CPP)
        self.assertIn('fs::file_size(file)>16384',CPP)
        self.assertIn('std::filesystem::is_symlink(themes)',CPP)
        self.assertIn("TOKEN_COLORS.count",CPP)

    def test_same_package_directory_as_portable_installer(self):
        self.assertIn('std::getenv("XDG_DATA_HOME")',CPP)
        self.assertIn('os.environ.get("XDG_DATA_HOME")',TOOLS)
        self.assertIn('APPDATA',CPP)
        self.assertIn('APPDATA',TOOLS)
        self.assertIn('"Application Support" / "OGL Viewer"',CPP)
        self.assertIn('original-palette.json',CPP)
        self.assertNotIn("system(",CPP[CPP.index("namespace ogl_pref_themes"):CPP.index("} // namespace ogl_pref_themes")])

    def test_original_skin_callbacks_and_default_kept(self):
        self.assertIn("LLPanelPreferenceSkins::onSkinChanged()",CPP)
        self.assertIn("LLPanelPreferenceSkins::onSkinThemeChanged()",CPP)
        self.assertIn("Original Firestorm (default)",CPP)
        for name in ("onOglThemeChanged","onOglThemeActivate",
                     "onOglThemeReset","onOglThemeFolder","refreshOglThemes"):
            self.assertIn(name+"();",HDR) if name.startswith("on") else self.assertIn(name+"();",HDR)
            self.assertIn("LLPanelPreferenceOglThemes::"+name,CPP)

if __name__=="__main__":
    unittest.main()

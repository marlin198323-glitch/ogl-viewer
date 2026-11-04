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

    def test_ogl_default_palette_is_shared_and_compatible_fallback_exists(self):
        palette=ET.parse(ROOT/"indra/newview/skins/default/colors.xml").getroot()
        colors={n.get("name"):n for n in palette.iter("color")}
        references={
            "EmphasisColor":"OGLThemeAccent",
            "EmphasisColorLight":"OGLThemeAccentLight",
            "SelectionColor":"OGLThemeSelection",
            "SchemeDark":"OGLThemeSchemeDark",
            "SchemeLightest":"OGLThemeSchemeLightest",
        }
        for name,target in references.items():
            self.assertEqual(colors[name].get("reference"),target,name)
            self.assertIn("value",colors[target].attrib,target)
        self.assertIn("OGL Standard (default)",CPP)
        self.assertIn("Original user palette",XUI.read_text())
        self.assertIn("snapshot_original();",CPP)
        self.assertIn("restore_original();",CPP)
        self.assertTrue((ROOT/"ogl/plugins/examples/classic-firestorm/theme.json").exists())

    def test_native_file_chooser_is_safe_selection_only(self):
        ui=ET.parse(XUI).getroot()
        controls=[n for n in ui if n.get("name")=="ogl_theme_select_file"]
        self.assertEqual(len(controls),1)
        self.assertIn("onOglThemeSelectPackage();",HDR)
        method=CPP.split("void LLPanelPreferenceOglThemes::onOglThemeSelectPackage()",1)[1].split(
            "void LLPanelPreferenceOglThemes::onOglThemeFolder()",1)[0]
        for value in ('LLFilePicker::instance()', 'getOpenFile(LLFilePicker::FFLOAD_ALL)',
                      'package.extension() != ".ogltheme"',
                      'std::filesystem::is_symlink(package)',
                      'std::filesystem::is_regular_file(package)',
                      'std::filesystem::file_size(package)',
                      'ogl_status_selected'):
            self.assertIn(value,method)
        self.assertIn('reinterpret_cast<const char8_t*>(filename_utf8.c_str())',method)
        self.assertNotIn("std::filesystem::u8path(",method)
        self.assertNotIn("std::system(",method)
        self.assertNotIn("ogl_pref_themes::activate(",method)
        self.assertNotIn("std::filesystem::copy",method)
        self.assertNotIn("extract(",method)
        de=ET.parse(ROOT/"indra/newview/skins/default/xui/de/panel_preferences_ogl_themes.xml").getroot()
        self.assertTrue(any(n.get("name")=="ogl_theme_select_file" for n in de))
        self.assertIn("Selected but not installed",XUI.read_text())

    def test_native_theme_package_import_is_explicit_restricted_and_atomic(self):
        cpp=CPP
        hdr=HDR
        en=ET.parse(XUI).getroot()
        de=ET.parse(ROOT/"indra/newview/skins/default/xui/de/panel_preferences_ogl_themes.xml").getroot()
        for panel in (en,de):
            self.assertEqual(sum(node.get("name")=="ogl_theme_select_file" for node in panel),1)
            self.assertEqual(sum(node.get("name")=="ogl_theme_install" for node in panel),1)
            self.assertEqual(sum(node.get("name")=="ogl_theme_activate" for node in panel),1)
        self.assertIn('void onOglThemeInstallPackage();',hdr)
        self.assertIn('std::string mOglPendingPackage;',hdr)
        select=cpp.split("void LLPanelPreferenceOglThemes::onOglThemeSelectPackage()",1)[1].split(
            "void LLPanelPreferenceOglThemes::onOglThemeInstallPackage()",1)[0]
        install=cpp.split("void LLPanelPreferenceOglThemes::onOglThemeInstallPackage()",1)[1].split(
            "void LLPanelPreferenceOglThemes::onOglThemeFolder()",1)[0]
        self.assertNotIn("ogl_pref_themes::install_local_package(",select)
        self.assertIn("mOglPendingPackage=filename_utf8;",select)
        self.assertIn("ogl_pref_themes::install_local_package(package);",install)
        self.assertIn("refreshOglThemes();",install)
        self.assertIn("previewOglTheme();",install)
        self.assertNotIn("ogl_pref_themes::activate(",install)
        self.assertIn("getString(\"ogl_status_installed\")",install)
        reader=cpp.split("static bj::object inspect_local_package(",1)[1].split(
            "static std::string install_local_package(",1)[0]
        for text in ('mz_zip_reader_open_file(', 'mz_zip_reader_entry_get_info(',
                     'mz_zip_reader_entry_read(', 'mz_zip_reader_entry_close(',
                     'mz_zip_reader_goto_next_entry(', 'MZ_END_OF_LIST',
                     'std::strcmp(entry->filename,"theme.json")!=0',
                     'MZ_ZIP_FLAG_ENCRYPTED','MZ_COMPRESS_METHOD_DEFLATE',
                     'entry->uncompressed_size>16384','entry->external_fa'):
            self.assertIn(text,reader)
        self.assertIn("reject_duplicate_json_keys(payload);",reader)
        self.assertIn('static void reject_duplicate_json_keys(',cpp)
        self.assertIn('Duplicate theme JSON key',cpp)
        installer=cpp.split("static std::string install_local_package(",1)[1].split(
            "static std::string active()",1)[0]
        self.assertLess(installer.index("inspect_local_package(package)"),
                        installer.index("fs::create_directories(themes);"))
        self.assertIn('fs::rename(staging,final);',installer)
        self.assertIn('fs::remove_all(staging,ignored);',installer)
        self.assertIn('fs::is_symlink(themes)',installer)
        self.assertNotIn('system(',installer)
        self.assertNotIn('unzip ',installer)
        self.assertNotIn('exec(',installer)

    def test_native_theme_remove_rejects_active_modified_or_other_folders(self):
        import_source = CPP
        for panel in (
            ET.parse(XUI).getroot(),
            ET.parse(ROOT/"indra/newview/skins/default/xui/de/panel_preferences_ogl_themes.xml").getroot()):
            self.assertEqual(sum(x.get("name")=="ogl_theme_remove" for x in panel),1)
        self.assertIn("void onOglThemeRemovePackage();",HDR)
        remove=import_source.split("static void remove_local_theme(",1)[1].split(
            "static bj::object checked_backup();",1)[0]
        self.assertIn("active()==id",remove)
        self.assertIn("installed_folder(id)",remove)
        self.assertIn("manifest(id);",remove)
        self.assertIn("fs::is_symlink(file)",remove)
        self.assertIn("fs::directory_iterator(folder)",remove)
        self.assertIn("fs::remove(file)",remove)
        self.assertIn("fs::remove(folder)",remove)
        self.assertNotIn("fs::remove_all",remove)
        callback=import_source.split("void LLPanelPreferenceOglThemes::onOglThemeRemovePackage()",1)[1].split(
            "void LLPanelPreferenceOglThemes::onOglThemeFolder()",1)[0]
        self.assertIn("remove_local_theme(mOglSelected)",callback)
        self.assertIn("refreshOglThemes();",callback)
        self.assertIn('getString("ogl_status_removed")',callback)

    def test_strict_cpp_semver_matches_portable_manifest_policy(self):
        self.assertIn("static bool valid_semver(",CPP)
        self.assertIn("!valid_semver(std::string(doc.at",CPP)
        from importlib.util import spec_from_file_location,module_from_spec
        source=ROOT/"ogl/plugins/tools/validate_theme.py"
        spec=spec_from_file_location("ogl_validator_p03",source)
        mod=module_from_spec(spec);spec.loader.exec_module(mod)
        for version in ("0.0.1","1.2.3","10.0.200"):
            self.assertTrue(mod.SEMVER.fullmatch(version))
        for version in ("01.2.3","1.02.3","1.2.03","1.2","1.2.3-rc1"):
            self.assertFalse(bool(mod.SEMVER.fullmatch(version)))
        self.assertIn("TOKEN_COLORS.at",CPP)

    def test_original_skin_callbacks_and_default_kept(self):
        self.assertIn("LLPanelPreferenceSkins::onSkinChanged()",CPP)
        self.assertIn("LLPanelPreferenceSkins::onSkinThemeChanged()",CPP)
        self.assertIn("OGL Standard (default)",CPP)
        for name in ("onOglThemeChanged","onOglThemeActivate",
                     "onOglThemeReset","onOglThemeFolder","refreshOglThemes"):
            self.assertIn(name+"();",HDR) if name.startswith("on") else self.assertIn(name+"();",HDR)
            self.assertIn("LLPanelPreferenceOglThemes::"+name,CPP)

if __name__=="__main__":
    unittest.main()

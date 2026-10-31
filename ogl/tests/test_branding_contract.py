#!/usr/bin/env python3
"""Static OGL fork smoke tests; runtime acceptance still requires a live GUI."""
import unittest
from pathlib import Path
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]


class OGLBrandingContract(unittest.TestCase):
    def source(self, path):
        return (ROOT / path).read_text()

    def test_source_brand_and_translations(self):
        self.assertIn('const std::string APP_NAME = "OGL";',
                      self.source("indra/llcommon/indra_constants.h"))
        for language in ("en", "de"):
            tree = ET.parse(ROOT / f"indra/newview/skins/default/xui/{language}/strings.xml")
            keys = {s.get("name"): (s.text or "").strip() for s in tree.getroot()}
            self.assertIn("OGLInsecureGridLogin", keys)
            self.assertIn("HTTP", keys["OGLInsecureGridLogin"])
        base = ET.parse(ROOT / "indra/newview/skins/default/xui/en/strings.xml")
        labels = {x.get("name"): (x.text or "").strip() for x in base.getroot()}
        self.assertEqual(labels.get("APP_NAME"), "OGL Viewer")

    def test_login_logo_and_asset(self):
        textures = ET.parse(ROOT / "indra/newview/skins/default/textures/textures.xml")
        mapping = {x.get("name"): x.get("file_name") for x in textures.getroot()}
        self.assertEqual(mapping["login_ogl_logo"], "windows/login_ogl_logo.png")
        png = ROOT / "indra/newview/skins/default/textures/windows/login_ogl_logo.png"
        self.assertTrue(png.is_file())
        self.assertEqual(png.read_bytes()[:8], b"\x89PNG\r\n\x1a\n")
        panel = self.source("indra/newview/skins/default/xui/en/panel_fs_nui_login.xml")
        self.assertIn('image_name="login_ogl_logo"', panel)

    def test_nexverse_background_and_https_auth_guard(self):
        source = self.source("indra/newview/fspanellogin.cpp")
        self.assertIn("OGLNexVerseGrid::kWelcomeURI", source)
        self.assertIn('parsed.scheme() != "https"', source)
        self.assertIn("getLoginURIs(login_uris)", source)
        self.assertIn("OGLInsecureGridLogin", source)
        self.assertIn("OGLHTTPLoginWarning", source)
        self.assertIn("OGLNexVerseGrid::isExactPinnedHTTPLogin", source)
        self.assertIn("User approved HTTP login for pinned Stadt NexVerse", source)

    def test_nexverse_login_bar_readable_and_graphics_status_truthful(self):
        from struct import unpack
        path = ROOT / "indra/newview/skins/default/xui/en/panel_fs_nui_login.xml"
        root = ET.parse(path).getroot()
        elements = {x.get("name"): x for x in root.iter() if x.get("name")}
        for name in ("ogl_brand_caption", "ogl_username_label",
                     "ogl_password_label", "ogl_start_label", "ogl_graphics_status"):
            self.assertIn(name, elements)
        self.assertEqual(elements["connect_btn"].get("label"), "Welt betreten")
        german = ET.parse(ROOT / "indra/newview/skins/default/xui/de/panel_fs_nui_login.xml")
        glabels = [x for x in german.getroot().iter() if x.get("name") == "connect_btn"]
        self.assertEqual(len(glabels), 1)
        self.assertEqual(glabels[0].get("label"), "Welt betreten")
        self.assertEqual(elements["connect_btn"].get("image_unselected"), "OGL_Login")
        self.assertIn("0.022", elements["panel_login"].get("bg_opaque_color"))
        textures = ET.parse(ROOT / "indra/newview/skins/default/textures/textures.xml")
        names = {x.get("name") for x in textures.getroot()}
        for name in ("OGL_Login", "OGL_Login_Over", "OGL_Login_Pressed"):
            self.assertIn(name, names)
            data = (ROOT / f"indra/newview/skins/default/textures/widgets/{name}.png").read_bytes()
            self.assertEqual(data[:8], bytes.fromhex("89504e470d0a1a0a"))
            self.assertEqual(unpack("!II", data[16:24]), (299, 64))
        login_cpp = self.source("indra/newview/fspanellogin.cpp")
        self.assertIn('ogl_rtx_rendering_active()', login_cpp)
        self.assertIn('Vulkan RT bereit | Effekte aus', login_cpp)
        self.assertIn('APP_NAME == "OGL" || !gSavedSettings.getBOOL', login_cpp)

    def test_rtx_world_frame_bridge_never_claims_live_rendering(self):
        pipeline = self.source("indra/newview/pipeline.cpp")
        module = self.source("ogl/modules/rtx/ogl_rtx_module.cpp")
        self.assertIn('ogl_rtx_submit_camera_sun(', pipeline)
        self.assertIn('getSunDirection()', pipeline)
        self.assertIn('camera->getOrigin().mV', pipeline)
        self.assertIn('ogl_rtx_submit_frame', module)
        self.assertIn('ogl_rtx_submit_geometry', module)
        self.assertIn('OGL RTX CPU scene snapshot: triangles=', module)
        self.assertIn('rendering_active=0', module)
        self.assertIn('OGL_RTX_MAX_SCENE_TRIANGLES', pipeline)
        self.assertIn('gObjectList.getObject(', pipeline)
        self.assertIn('getVolumeFace(', pipeline)
        self.assertIn('rendering_active=0', module)

    def test_launcher_keeps_test_profile_isolated(self):
        launcher = self.source("ogl/scripts/run-viewer-test.sh")
        self.assertIn('export OGL_X64_USER_DIR="$PROFILE"', launcher)
        self.assertIn('register_hopprotocol', launcher)
        shortcut = self.source("ogl/scripts/install-test-shortcut.sh")
        self.assertIn("login_ogl_logo.png", shortcut)

    def test_linux_icons_match_ogl_sizes(self):
        from struct import unpack
        icons = {
            "indra/newview/res/firestorm_icon.png": 512,
            "indra/newview/res-sdl/firestorm_icon.png": 512,
            "indra/newview/res-sdl/firestorm_icon128.png": 128,
            "indra/newview/skins/default/textures/firestorm_icon.png": 128,
        }
        for size in (16, 32, 48, 128, 256, 512):
            icons[f"indra/newview/icons/private-os/firestorm_{size}.png"] = size
        for filename, size in icons.items():
            data = (ROOT / filename).read_bytes()
            self.assertEqual(data[:8], bytes.fromhex("89504e470d0a1a0a"), filename)
            self.assertEqual(unpack("!II", data[16:24]), (size, size), filename)

    def test_upstream_copyright_still_present(self):
        self.assertIn("Linden Research, Inc.",
                      self.source("indra/llcommon/indra_constants.h"))
        self.assertIn("FirestormViewer/phoenix-firestorm",
                      self.source("ogl/README.md"))


if __name__ == "__main__":
    unittest.main(verbosity=2)

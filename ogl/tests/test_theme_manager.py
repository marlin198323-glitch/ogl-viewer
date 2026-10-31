"""Cross-platform theme installation in isolated user-data only."""
import importlib.util
import io
import json
from pathlib import Path
import stat
import sys
import tempfile
import unittest
import zipfile

ROOT=Path(__file__).resolve().parents[2]
TOOLS=ROOT/"ogl/plugins/tools"
if str(TOOLS) not in sys.path:
    sys.path.insert(0,str(TOOLS))
import manage_themes as manager
from validate_theme import UnsafeTheme

EXAMPLE=json.loads((ROOT/"ogl/plugins/examples/classic-firestorm/theme.json").read_text())

def archive(folder,doc=EXAMPLE,files=()):
    output=Path(folder)/"test.ogltheme"
    with zipfile.ZipFile(output,"w",zipfile.ZIP_DEFLATED) as f:
        f.writestr("theme.json",json.dumps(doc))
        for name,data in files:
            f.writestr(name,data)
    return output

class ThemeManagementTests(unittest.TestCase):
    def test_install_list_activate_reset_remove_without_touching_system(self):
        with tempfile.TemporaryDirectory() as t:
            base=Path(t)/"my-data"
            package=archive(t)
            theme_id=manager.install(base,package)
            self.assertEqual(theme_id,EXAMPLE["id"])
            self.assertEqual(manager.list_installed(base)[0]["active"],False)
            self.assertIsNone(manager.active_id(base))
            manager.activate(base,theme_id)
            self.assertEqual(manager.active_id(base),theme_id)
            self.assertTrue(manager.list_installed(base)[0]["active"])
            manager.write_active(base,None)
            self.assertIsNone(manager.active_id(base))
            manager.activate(base,theme_id)
            manager.remove(base,theme_id)
            self.assertIsNone(manager.active_id(base))
            self.assertFalse(manager.list_installed(base))

    def test_disallow_overwriting_installed_themes_and_traversal(self):
        with tempfile.TemporaryDirectory() as t:
            package=archive(t)
            base=Path(t)/"data"
            manager.install(base,package)
            with self.assertRaises(UnsafeTheme):
                manager.install(base,package)
            for name in ("..","../../etc","C:\\evil","", "foo/bar"):
                with self.subTest(id=name),self.assertRaises(UnsafeTheme):
                    manager.activate(base,name)
            with self.assertRaises(UnsafeTheme):
                manager.remove(base,"missing.theme")

    def test_reject_symlinked_theme_root(self):
        with tempfile.TemporaryDirectory() as t:
            base=Path(t)/"root"
            target=Path(t)/"other"
            target.mkdir()
            base.symlink_to(target,target_is_directory=True)
            with self.assertRaises(UnsafeTheme):
                manager.install(base,archive(t))

    def test_reject_invalid_images_and_cleanup_staging_directory(self):
        with tempfile.TemporaryDirectory() as t:
            base=Path(t)/"data"
            package=archive(t,files=[("assets/face.png",b"not an image")])
            with self.assertRaises(UnsafeTheme):
                manager.install(base,package)
            self.assertEqual(list((base/"themes").iterdir()),[])

    def test_valid_bounded_image_header(self):
        valid=(b"\x89PNG\r\n\x1a\n"+b"\x00\x00\x00\x0dIHDR"+
               (32).to_bytes(4,"big")+(48).to_bytes(4,"big"))
        with tempfile.TemporaryDirectory() as t:
            base=Path(t)/"data"
            p=archive(t,files=[("assets/portrait.png",valid)])
            name=manager.install(base,p)
            self.assertEqual((base/"themes"/name/"assets"/"portrait.png").read_bytes(),valid)

    def test_reject_extreme_image_dimensions(self):
        sig=b"\x89PNG\r\n\x1a\n"+b"\x00\x00\x00\x0dIHDR"
        invalid=sig+(65535).to_bytes(4,"big")+(2).to_bytes(4,"big")
        with tempfile.TemporaryDirectory() as t:
            with self.assertRaises(UnsafeTheme):
                manager.install(Path(t)/"data",archive(t,files=[("assets/a.png",invalid)]))

    def test_missing_or_invalid_active_state_fails_closed(self):
        with tempfile.TemporaryDirectory() as t:
            root=Path(t)
            self.assertIsNone(manager.active_id(root))
            state=root/"active-theme.json"
            for content in ('{"id":"../wrong"}','{"other":"x"}','not-json'):
                state.write_text(content)
                with self.assertRaises(UnsafeTheme):
                    manager.active_id(root)

    def test_paths_are_user_scoped_on_each_os(self):
        import unittest.mock
        with unittest.mock.patch.object(manager.sys,"platform","darwin"):
            self.assertIn("Application Support",str(manager.default_data_dir()))
        with unittest.mock.patch.object(manager.sys,"platform","linux"):
            self.assertIn("ogl-viewer",str(manager.default_data_dir()))
        with unittest.mock.patch.object(manager.sys,"platform","win32"):
            self.assertIn("OGL Viewer",str(manager.default_data_dir()))

if __name__=="__main__":
    unittest.main()

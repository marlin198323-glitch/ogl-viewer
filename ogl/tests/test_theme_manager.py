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

    def test_reject_nonregular_unix_fifo_and_symlink_entries(self):
        from validate_theme import validate_theme
        for kind in (stat.S_IFIFO, stat.S_IFLNK, stat.S_IFSOCK):
            with self.subTest(kind=kind), tempfile.TemporaryDirectory() as t:
                p=Path(t)/"nonregular.ogltheme"
                with zipfile.ZipFile(p,"w") as package:
                    package.writestr("theme.json",json.dumps(EXAMPLE))
                    info=zipfile.ZipInfo("assets/bad.png")
                    info.create_system=3
                    info.external_attr=(kind | 0o644)<<16
                    package.writestr(info,b"invalid")
                with self.assertRaises(UnsafeTheme):
                    validate_theme(p)

    def test_reject_duplicate_json_properties_and_semver_overflow(self):
        from validate_theme import validate_theme
        with tempfile.TemporaryDirectory() as t:
            p=Path(t)/"duplicate.ogltheme"
            text=json.dumps(EXAMPLE)
            duplicate=text.replace('"schema_version": 1',
                                   '"schema_version": 1, "schema_version": 1',1)
            with zipfile.ZipFile(p,"w") as package:
                package.writestr("theme.json",duplicate)
            with self.assertRaises(UnsafeTheme):
                validate_theme(p)
            huge=dict(EXAMPLE,version="1."+"1"*35+".0")
            with self.assertRaises(UnsafeTheme):
                validate_theme(archive(t,doc=huge))

    def test_reject_unsupported_zip_method_and_extra_oversize_archive_bytes(self):
        from validate_theme import validate_theme
        with tempfile.TemporaryDirectory() as t:
            p=Path(t)/"bzip.ogltheme"
            with zipfile.ZipFile(p,"w") as package:
                info=zipfile.ZipInfo("theme.json")
                info.compress_type=zipfile.ZIP_BZIP2
                package.writestr(info,json.dumps(EXAMPLE))
            with self.assertRaises(UnsafeTheme):
                validate_theme(p)
            p=archive(t)
            with p.open("ab") as f:
                f.write(b"x"*(16*1024*1024))
            with self.assertRaises(UnsafeTheme):
                validate_theme(p)

    def test_reject_case_collisions_and_parent_traversal(self):
        from validate_theme import validate_theme
        with tempfile.TemporaryDirectory() as t:
            p=archive(t,files=[
                ("assets/Name.png", b""),
                ("assets/name.PNG", b"")])
            with self.assertRaises(UnsafeTheme):
                validate_theme(p)
            p=archive(t,files=[("assets/../evil.png",b"")])
            with self.assertRaises(UnsafeTheme):
                validate_theme(p)

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

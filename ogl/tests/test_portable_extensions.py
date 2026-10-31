"""Portable OGL C-ABI contract and *non-executable* theme metadata tests."""
import importlib.util
import json
from pathlib import Path
import stat
import tempfile
import unittest
import zipfile

ROOT=Path(__file__).resolve().parents[2]
SCRIPT=ROOT/"ogl/plugins/tools/validate_theme.py"
spec=importlib.util.spec_from_file_location("ogl_theme_checker",SCRIPT)
validator=importlib.util.module_from_spec(spec)
spec.loader.exec_module(validator)

EXAMPLE=json.loads((ROOT/"ogl/plugins/examples/classic-firestorm/theme.json").read_text())

class PortablePluginFoundationTests(unittest.TestCase):
    def archive(self,directory, entries):
        p=Path(directory)/"example.ogltheme"
        with zipfile.ZipFile(p,"w",compression=zipfile.ZIP_DEFLATED) as z:
            for name,value in entries:
                # ZipInfo(name) normalizes backslashes to '/' when the
                # test runs on Windows. Override the raw stored filename
                # AFTER construction so ZIP traversal checks actually
                # receive identical hostile names on all three OSes.
                info=zipfile.ZipInfo("placeholder")
                info.filename=name
                info.compress_type=zipfile.ZIP_DEFLATED
                z.writestr(info,value)
        # Python/Windows also normalizes ZipInfo.filename at ZIP write
        # time. To test the actual security boundary, patch the raw ZIP
        # filenames in the local AND central directory records. A slash
        # and backslash are both one ASCII byte, so lengths stay valid.
        bad_names=[name for name,_ in entries if "\\" in name]
        if bad_names:
            blob=p.read_bytes()
            for dangerous in bad_names:
                normalized=dangerous.replace("\\","/").encode("ascii")
                raw_name=dangerous.encode("ascii")
                assert len(normalized)==len(raw_name)
                if blob.count(raw_name)>=2:
                    continue  # POSIX already writes raw backslashes.
                assert blob.count(normalized)>=2, (dangerous,blob.count(normalized))
                blob=blob.replace(normalized,raw_name)
            p.write_bytes(blob)
        with zipfile.ZipFile(p) as z:
            assert z.namelist()==[name for name,_ in entries], (z.namelist(),entries)
        return p

    def test_example_theme_supports_all_three_os(self):
        doc=validator.validate_manifest(EXAMPLE)
        self.assertEqual(set(doc["platforms"]),{"linux","windows","macos"})
        with tempfile.TemporaryDirectory() as t:
            p=self.archive(t,[("theme.json",json.dumps(doc))])
            self.assertEqual(validator.validate_theme(p)["id"],doc["id"])

    def test_absolute_and_traversal_paths_rejected(self):
        for path in ("../evil.png","/tmp/evil.png","C:/temp/evil.png",
                     "assets/../evil.png","assets\\evil.png",
                     "assets/.hidden.png","assets/folder//item.png"):
            with self.subTest(path=path),tempfile.TemporaryDirectory() as t:
                doc=json.dumps(EXAMPLE)
                p=self.archive(t,[("theme.json",doc),(path,b"pixel")])
                with self.assertRaises(validator.UnsafeTheme):
                    validator.validate_theme(p)

    def test_scripts_native_code_and_xml_rejected(self):
        for path in ("hello.dll","hello.so","hello.dylib","assets/chat.xml",
                     "assets/payload.js","assets/fake.svg","assets/../login.png"):
            with self.subTest(path=path),tempfile.TemporaryDirectory() as t:
                p=self.archive(t,[("theme.json",json.dumps(EXAMPLE)),(path,b"data")])
                with self.assertRaises(validator.UnsafeTheme):
                    validator.validate_theme(p)

    def test_unknown_tokens_or_invalid_colors_rejected(self):
        for token,color in (("login.password","#000000"),("chat.text","red"),
                            ("chat.background","#XYZXYZ"),("scripts","#111111")):
            with self.subTest(token=token):
                doc=dict(EXAMPLE,colors={token:color})
                with self.assertRaises(validator.UnsafeTheme):
                    validator.validate_manifest(doc)

    def test_wrong_schema_type_platforms_and_id_rejected(self):
        invalid=(
            dict(EXAMPLE,type="native"),
            dict(EXAMPLE,platforms=["linux"]),
            dict(EXAMPLE,platforms=["linux","windows",{}]),
            dict(EXAMPLE,id="../oops"),
            dict(EXAMPLE,version="1.0.0-beta"),
            dict(EXAMPLE,schema_version=True),
            dict(EXAMPLE,command="rm -rf /"),
        )
        for doc in invalid:
            with self.subTest(doc=list(doc.keys())):
                with self.assertRaises(validator.UnsafeTheme):
                    validator.validate_manifest(doc)

    def test_oversized_and_case_duplicate_entry_rejected(self):
        for entries in (
            [("theme.json",json.dumps(EXAMPLE)),("assets/large.png",b"x"*(validator.MAX_FILE_SIZE+1))],
            [("theme.json",json.dumps(EXAMPLE)),("assets/X.png",b"x"),
             ("assets/x.png",b"y")],
        ):
            with tempfile.TemporaryDirectory() as t:
                p=self.archive(t,entries)
                with self.assertRaises(validator.UnsafeTheme):
                    validator.validate_theme(p)

    def test_symlink_entry_rejected(self):
        with tempfile.TemporaryDirectory() as t:
            p=Path(t)/"evil.ogltheme"
            link=zipfile.ZipInfo("assets/link.png")
            link.create_system=3
            link.external_attr=(stat.S_IFLNK|0o777)<<16
            with zipfile.ZipFile(p,"w") as z:
                z.writestr("theme.json",json.dumps(EXAMPLE))
                z.writestr(link,"../../sensitive")
            with self.assertRaises(validator.UnsafeTheme):
                validator.validate_theme(p)

    def test_module_abi_loader_has_3_os_paths_and_no_auto_install(self):
        cpp=(ROOT/"ogl/modules/tools/module_probe.cpp").read_text()
        cmake=(ROOT/"ogl/modules/CMakeLists.txt").read_text()
        self.assertIn("#if defined(_WIN32)",cpp)
        self.assertIn("MultiByteToWideChar(CP_UTF8",cpp)
        self.assertIn("LoadLibraryExW(",cpp)
        self.assertIn("LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR",cpp)
        self.assertIn("dlopen(path, RTLD_NOW | RTLD_LOCAL)",cpp)
        self.assertIn("ogl_module_get_info",cpp)
        self.assertIn("close_module(handle)",cpp)
        self.assertIn("ogl_module_handshake",cmake)
        self.assertNotIn('if(UNIX)\n    add_test(NAME ogl_module_handshake',cmake)

    def test_cpu_dispatch_requires_instruction_and_os_support(self):
        header=(ROOT/"ogl/modules/include/ogl/ogl_cpu_features.h").read_text()
        dispatch=(ROOT/"ogl/modules/cpu/buffer_dispatch.cpp").read_text()
        cmake=(ROOT/"ogl/modules/CMakeLists.txt").read_text()
        self.assertIn("CP_UTF8", (ROOT/"ogl/modules/tools/module_probe.cpp").read_text())
        self.assertIn("XGETBV",header)
        self.assertIn("osxsave",header)
        self.assertIn("features.avx2",header)
        self.assertIn("features.neon=true",header)
        self.assertIn("CpuPath::scalar",header)
        self.assertIn("OGL_WITH_AVX2",dispatch)
        self.assertIn("OGL_WITH_NEON",dispatch)
        shared=(ROOT/"ogl/modules/cpu/ogl_cpu_buffer_targets.cmake").read_text()
        self.assertIn("OGL_CPU_SINGLE_ARCH",shared)
        self.assertIn("OGL_HAVE_ARM_NEON_HEADER",shared)
        self.assertIn("ogl_cpu_buffer_dispatch_correctness",cmake)

    def test_mac_ci_arm64_intel_windows_linux(self):
        workflow=(ROOT/".github/workflows/ogl-portable-graphics.yml").read_text()
        for runner in ("ubuntu-24.04","ubuntu-24.04-arm",
                       "windows-2022","windows-11-arm",
                       "macos-15","macos-15-intel"):
            self.assertIn(runner,workflow)
        self.assertIn("ctest --test-dir build/ogl-graphics",workflow)
        self.assertIn("Configure macOS (Intel and Apple Silicon)",workflow)


if __name__=="__main__":
    unittest.main()

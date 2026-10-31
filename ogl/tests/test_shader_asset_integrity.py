"""Prevent a GLSL-only edit from accidentally shipping stale Vulkan SPIR-V."""
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[2]
SCRIPT=ROOT/"ogl/scripts/ogl-check-shaders.py"
spec=importlib.util.spec_from_file_location("ogl_shader_check",SCRIPT)
module=importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class ShaderSyncTests(unittest.TestCase):
    def test_locked_pairs_match_checkout(self):
        manifest=json.loads((ROOT/module.MANIFEST).read_text())
        self.assertEqual(module.verify_pairs(ROOT,manifest),[])

    def test_detects_changed_glsl_source_even_if_spv_is_unchanged(self):
        original=json.loads((ROOT/module.MANIFEST).read_text())
        with tempfile.TemporaryDirectory() as folder:
            scratch=Path(folder)
            for pair in original["pairs"]:
                for field in ("source","compiled"):
                    dest=scratch/pair[field]
                    dest.parent.mkdir(parents=True,exist_ok=True)
                    dest.write_bytes((ROOT/pair[field]).read_bytes())
            self.assertEqual(module.verify_pairs(scratch,original),[])
            target=scratch/original["pairs"][2]["source"]
            target.write_bytes(target.read_bytes()+b"\n// stale shader\n")
            failure=module.verify_pairs(scratch,original)
            self.assertEqual(len(failure),1)
            self.assertIn("SHA256 mismatch",failure[0])

    def test_rejects_mismatched_spv_and_traversal(self):
        manifest=json.loads((ROOT/module.MANIFEST).read_text())
        with tempfile.TemporaryDirectory() as folder:
            scratch=Path(folder)
            for pair in manifest["pairs"]:
                for field in ("source","compiled"):
                    dest=scratch/pair[field]
                    dest.parent.mkdir(parents=True,exist_ok=True)
                    dest.write_bytes((ROOT/pair[field]).read_bytes())
            target=scratch/manifest["pairs"][2]["compiled"]
            target.write_bytes(b"invalid old SPV")
            self.assertTrue(module.verify_pairs(scratch,manifest))
            bad=json.loads(json.dumps(manifest))
            bad["pairs"][1]["compiled"]="../bad.spv"
            self.assertTrue(module.verify_pairs(scratch,bad))

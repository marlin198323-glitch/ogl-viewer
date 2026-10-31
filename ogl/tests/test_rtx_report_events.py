"""CPU scene diagnostics must not report stale geometry after teleport."""
import importlib.util
from pathlib import Path
import unittest

script = Path(__file__).resolve().parents[1] / "scripts/ogl-test-report.py"
spec = importlib.util.spec_from_file_location("ogl_rt_report", script)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class RTXSceneLogTests(unittest.TestCase):
    def test_no_scene(self):
        self.assertIsNone(module.latest_scene_event("STATE_LOGIN_WAIT"))

    def test_first_scene(self):
        self.assertEqual(module.latest_scene_event(
            "OGL RTX CPU scene snapshot: triangles=64 revision=1"),
            ("sample", 64, 1))

    def test_duplicate_sample_revisions(self):
        self.assertEqual(module.latest_scene_event(
            "OGL RTX CPU scene snapshot: triangles=64 revision=1\n"
            "OGL RTX CPU scene snapshot: triangles=1024 revision=30"),
            ("sample", 1024, 30))

    def test_teleport_clears_latest_scene(self):
        self.assertEqual(module.latest_scene_event(
            "OGL RTX CPU scene snapshot: triangles=64 revision=1\n"
            "OGL RTX CPU scene cleared revision=2"),
            ("cleared", 0, 2))

    def test_new_scene_after_teleport(self):
        self.assertEqual(module.latest_scene_event(
            "OGL RTX CPU scene snapshot: triangles=64 revision=1\n"
            "OGL RTX CPU scene cleared revision=2\n"
            "OGL RTX CPU scene snapshot: triangles=200 revision=3"),
            ("sample", 200, 3))

    def test_no_account_names_in_extracted_information(self):
        result = module.latest_scene_event(
            "username=private\n"
            "OGL RTX CPU scene snapshot: triangles=8 revision=17\n")
        self.assertEqual(result, ("sample", 8, 17))


if __name__ == "__main__":
    unittest.main()

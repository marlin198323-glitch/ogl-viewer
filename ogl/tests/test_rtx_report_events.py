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

    def test_depth_report_distinguishes_queued_and_completed(self):
        source = script.read_text()
        self.assertIn('Depth-to-Vulkan GPU processing: queued', source)
        self.assertIn('Depth-to-Vulkan GPU processing: confirmed', source)
        self.assertNotIn('Depth samples are NOT yet fed', source)
        self.assertIn('developer probe configured: worker=', source)

    def test_depth_coverage_report_separates_sky_from_surfaces(self):
        source = script.read_text()
        self.assertIn('RTX last depth-grid coverage:', source)
        self.assertIn('OGL RTX depth coverage: surface=', source)
        self.assertIn('not automatically against 64 valid surfaces', source)

    def test_primary_ray_geometry_report(self):
        source = script.read_text()
        self.assertIn("RTX primary-ray geometry coverage:",source)
        self.assertIn("RTX OpenGL/Vulkan depth agreement:",source)
        self.assertIn("primary_hits=",source)
        self.assertIn("depth_matches=",source)
        self.assertIn("RTX depth disagreement:",source)
        self.assertIn("primary_farther=",source)

    def test_scene_capacity_exposed(self):
        source = script.read_text()
        self.assertIn("RTX geometry capacity:", source)
        self.assertIn("eligible candidate objects;", source)

    def test_warning_indicators_not_sensitive_details(self):
        source = script.read_text()
        self.assertIn("OpenSim/texture warning indicators:", source)
        self.assertIn("Low system memory detected",source)
        self.assertIn("bounds_recovered=",source)

    def test_latest_probe_revision_freshness(self):
        log = ("OGL RTX CPU scene snapshot: triangles=8192 revision=1\n"
               "OGL RTX GPU probe: triangles=8192 rays=64 occluded=0 revision=1\n"
               "OGL RTX CPU scene snapshot: triangles=8192 revision=30\n")
        self.assertEqual(module.latest_gpu_probe_revision(log),1)
        self.assertEqual(module.latest_scene_event(log),("sample",8192,30))
        log += ("OGL RTX GPU probe: triangles=8192 rays=64 "
                "occluded=0 revision=30\n")
        self.assertEqual(module.latest_gpu_probe_revision(log),30)
        self.assertIsNone(module.latest_gpu_probe_revision("no GPU jobs yet"))
        source = script.read_text()
        self.assertIn("RTX probe freshness:",source)
        self.assertIn("older than latest",source)
        self.assertIn("older viewers log",source)

    def test_camera_ray_anchor_report(self):
        source = script.read_text()
        self.assertIn("RTX exact camera-ray geometry:",source)
        self.assertIn("exact_ray_anchors=",source)
        self.assertIn("prepass_triangles=",source)

    def test_no_account_names_in_extracted_information(self):
        result = module.latest_scene_event(
            "username=private\n"
            "OGL RTX CPU scene snapshot: triangles=8 revision=17\n")
        self.assertEqual(result, ("sample", 8, 17))


if __name__ == "__main__":
    unittest.main()

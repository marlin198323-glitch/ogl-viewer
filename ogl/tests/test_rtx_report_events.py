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
        self.assertIn("RTX scan geometry capacity:", source)
        self.assertIn("RTX scene scan:", source)
        self.assertIn("partial_deferrals=", source)
        self.assertIn("anchor_deferrals=", source)
        self.assertIn("RTX dropped-anchor snapshot protection:",source)
        self.assertIn("exact_ray_samples=", source)
        self.assertIn("RTX exact camera-ray coverage:", source)
        self.assertIn("RTX validated diagnostic shadow rays:", source)
        self.assertIn("eligible candidate objects;", source)

    def test_warning_indicators_not_sensitive_details(self):
        source = script.read_text()
        self.assertIn("OpenSim/texture warning indicators:", source)
        self.assertIn("Low system memory detected",source)
        self.assertIn("bounds_recovered=",source)

    def test_independent_cpu_reference_log_parser(self):
        result = module.latest_cpu_depth_reference(
            "OGL RTX CPU depth reference: revision=18 triangles=8192 "
            "surface=51 background=13 primary_hits=42 depth_matches=36 "
            "primary_closer=6 primary_farther=0 ray_samples=50 cpu_us=2400")
        self.assertEqual(result, (18,8192,51,13,42,36,6,0,50,2400))
        self.assertIsNone(module.latest_cpu_depth_reference("no cpu probe"))
        source = script.read_text()
        self.assertIn("RTX independent CPU depth reference:",source)
        self.assertIn("RTX CPU/Vulkan comparison:",source)
        self.assertIn("DIFFERENT primary",source)
        self.assertIn("sample synchronization must be",source)
        self.assertIn("RTX CPU scene scan cost:",source)
        self.assertIn("RTX camera broad-phase coverage:",source)
        self.assertIn("RTX CPU depth range",source)
        self.assertIn("depth_near=",source)

    def test_completed_gpu_depth_match_history(self):
        first = ("OGL RTX GPU probe: triangles=8192 rays=64 occluded=7 "
                 "revision=8 device_inits=1 surface_samples=43 "
                 "background_samples=21 primary_hits=32 depth_matches=21")
        second = ("OGL RTX GPU probe: triangles=8192 rays=64 occluded=9 "
                  "revision=10 device_inits=1 surface_samples=46 "
                  "background_samples=18 primary_hits=37 depth_matches=22")
        self.assertEqual(module.gpu_depth_history(first+"\n"+second),
                         [(8,43,32,21),(10,46,37,22)])
        self.assertEqual(module.gpu_depth_history(first+"\n"+second,1),
                         [(10,46,37,22)])
        self.assertEqual(module.gpu_depth_history("no gpu result"),[])
        source=script.read_text()
        self.assertIn("RTX Vulkan geometry coverage gap:",source)
        self.assertIn("RTX completed depth-match history",source)

    def test_world_shadow_bridge_is_explicitly_not_rendering(self):
        source=script.read_text()
        self.assertIn("RTX world-shadow bridge:",source)
        self.assertIn("world-shadow input eligible: sample=",source)
        self.assertIn("not composited; still 8x8",source)
        self.assertIn('Ray-traced world rendering:',source)

    def test_gpu_source_scan_resolves_older_revision(self):
        text = (
            "OGL RTX scene selection: exact_ray_anchors=30 "
            "exact_ray_samples=31 prepass_triangles=32768 scene_publish=1\n"
            "OGL RTX CPU scene snapshot: triangles=8192 revision=1\n"
            "OGL RTX scene selection: exact_ray_anchors=2 "
            "exact_ray_samples=2 prepass_triangles=32768 scene_publish=1\n"
            "OGL RTX CPU scene snapshot: triangles=8192 revision=2\n"
            "OGL RTX GPU probe: triangles=8192 rays=64 occluded=7 revision=1\n")
        self.assertIn("exact_ray_samples=31",
            module.scene_selection_at_revision(text,1))
        self.assertIn("exact_ray_samples=2",
            module.scene_selection_at_revision(text,2))
        self.assertIsNone(module.scene_selection_at_revision(text,3))

    def test_gpu_source_scan_skips_deferred_and_teleported(self):
        text = (
            "OGL RTX scene selection: exact_ray_samples=30 scene_publish=1\n"
            "OGL RTX CPU scene snapshot: triangles=8192 revision=4\n"
            "OGL RTX scene selection: exact_ray_samples=0 scene_publish=0\n"
            "OGL RTX CPU scene snapshot: triangles=8192 revision=5\n")
        self.assertIsNone(module.scene_selection_at_revision(text,5))
        self.assertIn("exact_ray_samples=30",
            module.scene_selection_at_revision(text,4))
        text += "OGL RTX CPU scene cleared revision=6\n"
        self.assertIsNone(module.scene_selection_at_revision(text,4))

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

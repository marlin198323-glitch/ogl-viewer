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

    def test_startup_low_fps_report(self):
        source=script.read_text()
        self.assertIn("RTX startup scan:",source)
        self.assertIn("RTX scene startup:",source)
        self.assertIn("frame-count-only ",source)
        self.assertIn("scanning can be delayed",source)
        self.assertIn("startup scene scan: cause=",source)
        self.assertIn("FPS: ([0-9.]+)",source)

    def test_dense_gpu_in_world_compositing_report(self):
        source=script.read_text()
        for expected in ("RTX dense Vulkan sunlight:",
                         "RTX experimental dense GL depth capture:",
                         "RTX experimental in-world sunlight compositor:",
                         "dense GPU probe: triangles=",
                         "dense world sunlight composed:"):
            self.assertIn(expected,source)

    def test_dense_report_recognizes_completed_without_legacy_8x8(self):
        source=script.read_text()
        self.assertIn("elif dense_results:",source)
        self.assertIn("legacy 8x8 diagnostic intentionally inactive",source)
        self.assertIn("RTX dense GPU freshness:",source)
        self.assertIn("older than latest scene revision",source)
        self.assertIn("RTX experimental in-world sunlight compositor:",source)
        self.assertIn("OpenGL deferred depth readback:",source)
        self.assertIn("full raytracing/path ",source)
        self.assertIn("tracing is NOT implemented",source)

    def test_world_shadow_fragment_coverage_accountability(self):
        source=script.read_text()
        self.assertIn("RTX OpenGL fragment coverage:",source)
        self.assertIn("GL shadow pixel coverage: samples=",source)
        self.assertIn("RTX visible-shadow blocker:",source)
        self.assertIn("NOT final ",source)
        self.assertIn("screen brightness",source)

    def test_dense_worker_telemetry(self):
        source=script.read_text()
        self.assertIn("RTX dense GPU worker duration:",source)
        self.assertIn("RTX dense scheduler:",source)
        self.assertIn("busy_depth_skips=",source)
        self.assertIn("worker_wall_ms=",source)

    def test_gpu_failure_reason_logging(self):
        source=script.read_text()
        self.assertIn("RTX GPU failure classification:",source)
        self.assertIn("world-mask mismatches",source)
        self.assertIn("gpu_rc=",source)

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

    # P05 real deferred-source GPU evidence, using anonymized sample log
    # lines ONLY. These are parser fixtures, not real grid login evidence.
    def p05_example(self):
        return (
            "P05 deferred evidence session: begin=1\n"
            "STATE_STARTED\n"
            "P05 deferred region boundary: epoch=1\n"
            "P05 deferred capture receipt: epoch=1 capture=1 "
            "triangles=400 mesh_generation=9\n"
            "P05 deferred GPU receipt: epoch=1 capture=1 "
            "mesh_generation=9 source=mesh-rebind frames=1 "
            "camera_frames=0 nonclear_pixels=144 status=completed\n"
        )

    def test_p05_source_receipts_need_session_and_world(self):
        self.assertEqual(module.p05_live_gpu_evidence("STATE_STARTED")["status"],
                         "no_deferred_evidence_session")
        self.assertEqual(module.p05_live_gpu_evidence(
            self.p05_example().replace("STATE_STARTED\n","")
        )["status"],"world_entry_not_observed")
        self.assertEqual(module.p05_live_gpu_evidence(
            "P05 deferred evidence session: begin=1\nSTATE_STARTED\n"
        )["status"],"no_current_region_capture")

    def test_p05_real_deferred_gpu_receipt_chain(self):
        result=module.p05_live_gpu_evidence(self.p05_example())
        self.assertEqual(result["status"],"deferred_source_gpu_completed")
        self.assertEqual(result["current_region_epoch"],1)
        self.assertEqual(result["accepted_capture_events"],1)
        self.assertEqual(result["completed_gpu_frames"],1)
        self.assertEqual(result["camera_gpu_frames"],0)
        self.assertEqual(result["last_nonclear_pixels"],144)
        self.assertEqual(result["rejected_receipt_events"],0)

    def test_p05_missing_capture_never_uses_raw_gpu_result(self):
        log=self.p05_example()
        no_capture="\n".join(line for line in log.splitlines()
                              if "deferred capture receipt:" not in line)
        result=module.p05_live_gpu_evidence(no_capture)
        self.assertEqual(result["status"],"no_current_region_capture")
        self.assertEqual(result["completed_gpu_frames"],0)
        self.assertEqual(result["rejected_receipt_events"],1)

    def test_p05_mismatched_gpu_mesh_generation_rejected(self):
        log=self.p05_example().replace(
            "mesh_generation=9 source=mesh-rebind",
            "mesh_generation=8 source=mesh-rebind")
        result=module.p05_live_gpu_evidence(log)
        self.assertEqual(result["status"],"capture_accepted_gpu_pending")
        self.assertEqual(result["rejected_receipt_events"],1)

    def test_p05_mismatched_gpu_capture_id_rejected(self):
        log=self.p05_example().replace(
            "capture=1 mesh_generation=9 source=",
            "capture=2 mesh_generation=9 source=")
        result=module.p05_live_gpu_evidence(log)
        self.assertEqual(result["status"],"capture_accepted_gpu_pending")
        self.assertEqual(result["rejected_receipt_events"],1)

    def test_p05_region_change_invalidates_old_gpu_evidence(self):
        log=self.p05_example()+"P05 deferred region boundary: epoch=2\n"
        result=module.p05_live_gpu_evidence(log)
        self.assertEqual(result["status"],"no_current_region_capture")
        self.assertEqual(result["current_region_epoch"],2)
        self.assertEqual(result["completed_gpu_frames"],0)

    def test_p05_new_capture_supersedes_old_gpu_evidence(self):
        log=self.p05_example()+(
            "P05 deferred capture receipt: epoch=1 capture=2 "
            "triangles=300 mesh_generation=10\n"
        )
        result=module.p05_live_gpu_evidence(log)
        self.assertEqual(result["status"],"capture_accepted_gpu_pending")
        self.assertEqual(result["completed_gpu_frames"],0)
        self.assertEqual(result["accepted_capture_events"],2)

    def test_p05_camera_only_gpu_frames_use_latest_capture(self):
        log=self.p05_example()+(
            "P05 deferred GPU receipt: epoch=1 capture=1 "
            "mesh_generation=9 source=camera-tick frames=2 "
            "camera_frames=1 nonclear_pixels=0 status=completed\n"
            "P05 deferred GPU receipt: epoch=1 capture=1 "
            "mesh_generation=9 source=camera-tick frames=3 "
            "camera_frames=2 nonclear_pixels=192 status=completed\n"
        )
        result=module.p05_live_gpu_evidence(log)
        self.assertEqual(result["status"],"deferred_source_gpu_completed")
        self.assertEqual(result["completed_gpu_frames"],3)
        self.assertEqual(result["camera_gpu_frames"],2)
        self.assertEqual(result["last_nonclear_pixels"],192)

    def test_p05_prelogin_gpu_test_or_old_session_not_proven(self):
        log=self.p05_example()+(
            "P05 deferred evidence session: begin=1\n"
            "STATE_LOGIN_WAIT\n"
        )
        result=module.p05_live_gpu_evidence(log)
        self.assertEqual(result["status"],"world_entry_not_observed")
        self.assertEqual(result["completed_gpu_frames"],0)

    def test_p05_forget_stale_gpu_after_new_region_capture(self):
        log=self.p05_example()+(
            "P05 deferred region boundary: epoch=2\n"
            "P05 deferred capture receipt: epoch=2 capture=2 "
            "triangles=100 mesh_generation=11\n"
            "P05 deferred GPU receipt: epoch=1 capture=1 "
            "mesh_generation=9 source=camera-tick frames=2 "
            "camera_frames=1 nonclear_pixels=200 status=completed\n"
        )
        result=module.p05_live_gpu_evidence(log)
        self.assertEqual(result["status"],"capture_accepted_gpu_pending")
        self.assertEqual(result["rejected_receipt_events"],1)
        self.assertEqual(result["completed_gpu_frames"],0)

    def test_p05_duplicate_out_of_order_and_bad_pixel_receipts(self):
        log=self.p05_example()+(
            "P05 deferred GPU receipt: epoch=1 capture=1 "
            "mesh_generation=9 source=camera-tick frames=1 "
            "camera_frames=1 nonclear_pixels=20 status=completed\n"
            "P05 deferred GPU receipt: epoch=1 capture=1 "
            "mesh_generation=9 source=camera-tick frames=2 "
            "camera_frames=1 nonclear_pixels=1025 status=completed\n"
        )
        result=module.p05_live_gpu_evidence(log)
        self.assertEqual(result["completed_gpu_frames"],1)
        self.assertEqual(result["rejected_receipt_events"],2)

    def test_p05_capture_bounds_and_monotonic_id(self):
        log=self.p05_example()+(
            "P05 deferred capture receipt: epoch=1 capture=1 "
            "triangles=12 mesh_generation=9\n"
            "P05 deferred capture receipt: epoch=1 capture=2 "
            "triangles=9000 mesh_generation=10\n"
        )
        result=module.p05_live_gpu_evidence(log)
        self.assertEqual(result["accepted_capture_events"],1)
        self.assertEqual(result["rejected_receipt_events"],2)

    def test_p05_receipts_exclude_private_identity(self):
        log=self.p05_example() + (
            "user=PRIVATE-AVATAR grid=PRIVATE-REGION account=PRIVATE\n"
        )
        result=module.p05_live_gpu_evidence(log)
        self.assertEqual(set(result),{
            "status","current_region_epoch","accepted_capture_events",
            "completed_gpu_frames","camera_gpu_frames",
            "last_nonclear_pixels","rejected_receipt_events",
        })
        self.assertNotIn("PRIVATE",str(result))
        self.assertIn("not independent proof of authenticated grid login",
                      script.read_text())

    def test_no_account_names_in_extracted_information(self):
        result = module.latest_scene_event(
            "username=private\n"
            "OGL RTX CPU scene snapshot: triangles=8 revision=17\n")
        self.assertEqual(result, ("sample", 8, 17))


if __name__ == "__main__":
    unittest.main()

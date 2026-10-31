#!/usr/bin/env python3
"""Local OGL test health summary (no credentials or full log lines printed)."""
from pathlib import Path
import os
import re
import shutil
import subprocess

HOME = Path.home()
ROOT = Path(__file__).resolve().parents[2]
PROFILE = Path(os.environ.get("XDG_DATA_HOME", str(HOME / ".local/share"))) / "ogl-firestorm-test"
LOG = PROFILE / "logs/OGL.log"  # after the fork APP_NAME changes
BINARY = ROOT / "build-linux-x86_64/newview/packaged/bin/do-not-directly-run-firestorm-bin"
CACHE = ROOT / "build-linux-x86_64/CMakeCache.txt"


def disk_size(path: Path) -> str:
    if not path.exists():
        return "not yet created"
    try:
        result = subprocess.run(
            ["du", "-sh", "--", str(path)],
            check=True, capture_output=True, text=True, timeout=8
        )
        return result.stdout.split()[0]
    except (OSError, subprocess.SubprocessError, IndexError):
        return "unavailable"


# Use chronological log order; a region teleport invalidates the last
# geometry even if an earlier snapshot still appears in this log.
SCENE_EVENT = re.compile(
    r"OGL RTX CPU scene (?:snapshot: triangles=(\d+) revision=(\d+)"
    r"|cleared revision=(\d+))"
)


def latest_scene_event(log_text: str):
    matches = list(SCENE_EVENT.finditer(log_text))
    if not matches:
        return None
    match = matches[-1]
    if match.group(3) is not None:
        return ("cleared", 0, int(match.group(3)))
    return ("sample", int(match.group(1)), int(match.group(2)))


GPU_RESULT_REVISION = re.compile(
    r"OGL RTX GPU probe: triangles=\d+ rays=\d+ "
    r"occluded=\d+ revision=(\d+)"
)


def latest_gpu_probe_revision(log_text: str):
    records = list(GPU_RESULT_REVISION.finditer(log_text))
    return int(records[-1].group(1)) if records else None


def report():
    print("OGL Viewer – local Linux Mint test")
    print("Binary:", "present" if BINARY.is_file() else "missing")
    print("Test profile:", PROFILE)
    if CACHE.exists():
        cmake = CACHE.read_text(errors="replace")
        for key in ("VIEWER_CHANNEL", "OPENSIM"):
            value = re.search(rf"^{key}:[^=]+=([^\n]*)", cmake, re.M)
            print(f"{key}:", value.group(1) if value else "unknown")
    if LOG.exists():
        # Numeric counts only: avoid leaking account names, URLs or credentials.
        text = LOG.read_text(errors="replace")
        print("Log file:", LOG)
        print("Error entries:", len(re.findall(r"\b(?:ERROR|FATAL)\b", text)))
        print("Warning entries:", len(re.findall(r"\bWARNING\b", text)))
        print("Login screen ready:", "STATE_LOGIN_WAIT" in text)
        world_started = "STATE_STARTED" in text
        print("3D world entered in this log:", world_started)
        print("Viewer stopped cleanly:", (
            "yes" if "cleanup : Goodbye!" in text
            else "not recorded (viewer may still be running)"))
        rtx_lines = re.findall(
            r"OGLRTX[^\n]*GPU [^\n]*hardware_ready=(\d+) rendering_active=(\d+)",
            text
        )
        last_scene = latest_scene_event(text)
        scene_batches = re.findall(r"OGL RTX captured real OpenSim triangles=(\d+)", text)
        if last_scene:
            kind, count, revision = last_scene
            if kind == "cleared":
                print("RTX world geometry sample: cleared (region change/empty scene)")
            else:
                print("RTX world geometry sample:", count, "triangles received")
            print("RTX geometry revision:", revision)
            last_completed_revision = latest_gpu_probe_revision(text)
            if last_completed_revision is not None:
                if last_completed_revision < revision:
                    print(f"RTX probe freshness: GPU revision "
                          f"{last_completed_revision} is older than latest "
                          f"logged CPU scene revision {revision} (stale)")
                elif last_completed_revision > revision:
                    print(f"RTX probe freshness: GPU revision "
                          f"{last_completed_revision} is newer than last logged "
                          f"CPU scene revision {revision}; older viewers log "
                          "CPU revisions sparsely")
                else:
                    print("RTX probe freshness: completed GPU result matches "
                          "latest logged CPU scene revision")
        elif scene_batches:
            print("RTX world geometry sample:", scene_batches[-1], "triangles received")
        else:
            reason = ("no triangle snapshot recorded yet" if world_started else
                      "last log never reached the 3D world")
            print("RTX world geometry sample: not observed (" + reason + ")")
        selected = re.findall(
            r"OGL RTX scene selection: candidates=(\d+) sampled_objects=(\d+) "
            r"mesh_triangles=(\d+) terrain_triangles=(\d+) cap=(\d+)",text)
        if selected:
            candidates, sampled_objects, mesh_tris, terrain_tris, budget = selected[-1]
            print(f"RTX scene selection: {sampled_objects}/{candidates} "
                  f"eligible candidate objects; {mesh_tris} mesh triangles, "
                  f"{terrain_tris} terrain triangles; budget {budget}")
            unused = int(budget) - int(mesh_tris) - int(terrain_tris)
            print(f"RTX geometry capacity: {unused}/{budget} triangle slots unused "
                  "(partial geometry still possible)")
            anchor_events = re.findall(
                r"OGL RTX scene selection: [^\n]*candidate_limit=(\d+) "
                r"exact_ray_anchors=(\d+) prepass_triangles=(\d+)",text)
            if anchor_events:
                cap, anchors, inspected = anchor_events[-1]
                print(f"RTX exact camera-ray geometry: {anchors}/64 anchor "
                      f"triangles, {inspected} full-mesh triangles inspected; "
                      f"candidate limit {cap} (last periodic scan)")
        developer = re.findall(
            r"OGL RTX developer probe configured: worker=(\d) "
            r"camera=(\d) depth_guided=(\d)", text)
        queued = text.count("OGL RTX depth-guided GPU snapshot queued")
        probes = re.findall(
            r"OGL RTX GPU probe: triangles=(\d+) rays=(\d+) "
            r"occluded=(\d+) revision=(\d+)", text)
        if probes:
            count, rays, hits, revision = probes[-1]
            print(f"RTX GPU diagnostic: {count} scene triangles; "
                  f"{hits}/{rays} ray hits; revision {revision}")
            counters = re.findall(
                r"OGL RTX GPU probe: [^\n]*device_inits=(\d+) "
                r"blas_builds=(\d+)(?: blas_refits=(\d+))?"
                r"(?: tlas_refits=(\d+))? cache_hits=(\d+)", text)
            if counters:
                device_inits, blas_builds, blas_refits, tlas_refits, cache_hits = counters[-1]
                print(f"RTX GPU reuse: {device_inits} device init(s), "
                      f"{blas_builds} BLAS build(s), "
                      f"{blas_refits or '0'} BLAS refit(s), "
                      f"{tlas_refits or '0'} TLAS refit(s), "
                      f"{cache_hits} cache hit(s)")
            print("RTX GPU shadow probe: 8x8 validated debug mask"
                  " (not a rendered shadow map)")
            print("RTX GPU diagnostic is NOT visual raytracing in the world.")
            completed_surface = re.findall(
                r"OGL RTX GPU probe: [^\n]*surface_samples=(\d+) "
                r"background_samples=(\d+)", text)
            if completed_surface:
                visible, background = completed_surface[-1]
                print(f"RTX completed GPU sample coverage: {visible}/64 "
                      f"reconstructed surfaces, {background}/64 sky/background")
                primary = re.findall(
                    r"OGL RTX GPU probe: [^\n]*surface_samples=\d+ "
                    r"background_samples=\d+ primary_hits=(\d+) "
                    r"depth_matches=(\d+)", text)
                if primary:
                    primary_hits, matched = map(int, primary[-1])
                    surfaces = int(visible)
                    print(f"RTX primary-ray geometry coverage: "
                          f"{primary_hits}/{surfaces} surface samples hit "
                          f"the incomplete Vulkan scene")
                    print(f"RTX OpenGL/Vulkan depth agreement: "
                          f"{matched}/{surfaces} surface samples "
                          "(within 2% or 0.25 m)")
                    comparison = re.findall(
                        r"OGL RTX GPU probe: [^\n]*primary_hits=\d+ "
                        r"depth_matches=\d+ primary_closer=(\d+) "
                        r"primary_farther=(\d+)", text)
                    if comparison:
                        closer, farther = map(int, comparison[-1])
                        print(f"RTX depth disagreement: {closer} Vulkan "
                              f"surfaces closer than GL, {farther} farther")
                        if primary_hits != matched + closer + farther:
                            print("RTX warning: depth relation counts disagree.")
                        elif primary_hits > 0 and matched == 0:
                            if farther > closer:
                                print("RTX diagnostic: GPU scene mostly behind "
                                      "visible GL surfaces; missing near objects "
                                      "or camera/depth projection mismatch.")
                            elif closer > farther:
                                print("RTX diagnostic: GPU scene mostly in front "
                                      "of visible GL surfaces; check transforms, "
                                      "occlusion and stale object snapshots.")
                    if surfaces > 0 and primary_hits == 0:
                        print("RTX diagnostic: no primary ray hits; check "
                              "incomplete mesh capture and coordinate transforms.")
                    elif surfaces > 0 and matched == 0:
                        print("RTX diagnostic: no matching GL depth; check "
                              "projection and occluding foreground geometry.")
                print("Only reconstructed surfaces are eligible for shadows; "
                      "0/64 hits does not by itself indicate a GPU error.")
        elif ("OGL RTX GPU scene probe failed;" in text or
              "OGL RTX async GPU probe failed;" in text):
            print("RTX GPU diagnostic: failed; OpenGL fallback active")
        elif developer:
            worker, camera, guided = developer[-1]
            print(f"RTX GPU diagnostic: configured, but no completed probe "
                  f"(worker={worker}, camera={camera}, depth_guided={guided}, "
                  f"queued={queued})")
            print("Check current session for worker failure or stale GPU snapshots.")
        else:
            print("RTX GPU diagnostic: no completed probe recorded "
                  "(developer mode may be off, or older build lacks job logs)")
        samples = re.findall(
            r"Deferred depth full-view grid captured (\d+)x(\d+)", text)
        if samples:
            w, h = samples[-1]
            print(f"OpenGL deferred depth readback: whole-view 8x8 grid "
                  f"sampled from {w}x{h} render target (developer mode only)")
            if probes and developer and developer[-1][2] == "1":
                print("Depth-to-Vulkan GPU processing: confirmed by completed probe.")
            elif queued:
                print("Depth-to-Vulkan GPU processing: queued, no successful result logged.")
            else:
                print("Depth-to-Vulkan GPU processing: not confirmed by this log.")
        else:
            print("OpenGL deferred depth readback: not observed "
                  "(requires logged-in world and OGL_RTX_DEPTH_PROBE=1)")
        coverage = re.findall(
            r"OGL RTX depth coverage: surface=(\d+) background=(\d+) "
            r"view_m_min=([0-9.]+) view_m_max=([0-9.]+)", text)
        if coverage:
            surface, background, near_m, far_m = coverage[-1]
            print(f"RTX last depth-grid coverage: {surface}/64 visible "
                  f"surface samples; {background}/64 background/sky")
            print(f"RTX reconstructed view depths: {near_m}–{far_m} m "
                  "(perspective assumption; no world-shadow validation)")
            if probes:
                print("RTX shadow hits must be interpreted against surface "
                      "coverage, not automatically against 64 valid surfaces.")
        elif samples:
            print("RTX depth-grid coverage: not recorded by this build.")
        if rtx_lines:
            ready, active = rtx_lines[-1]
            print("RTX Vulkan hardware:", "supported" if ready == "1" else "not supported")
            print("Ray-traced world rendering:", "active" if active == "1" else "not implemented")
        else:
            print("RTX module status: not reported")
    else:
        print("Viewer log: not yet created")
    print("Application data size:", disk_size(PROFILE))
    for name in ("cache", "texturecache", "objectcache", "logs"):
        path = PROFILE / name
        if path.exists():
            print(f"  {name}: {disk_size(path)}")
    if shutil.which("nvidia-smi"):
        try:
            gpu = subprocess.run(
                ["nvidia-smi", "--query-gpu=name,driver_version",
                 "--format=csv,noheader"],
                capture_output=True, text=True, timeout=3
            )
            if gpu.returncode == 0:
                print("GPU:", gpu.stdout.strip().splitlines()[0])
        except (OSError, subprocess.SubprocessError, IndexError):
            pass
    print("Note: disk size and logs do not measure actual texture/mesh cache hit rate.")


if __name__ == "__main__":
    report()

#include "ogl/ogl_rtx_api.h"
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <limits>
#include <vector>
#include <thread>
#include <chrono>
#include <dlfcn.h>
#if defined(OGL_TEST_X11_WSI)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cstring>
#endif

void log_cb(int, const char*) {}

int main(int argc, char** argv) {
    assert(argc == 2);
    void* library = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    assert(library);
    const auto start = reinterpret_cast<ogl_module_start_fn>(
        dlsym(library, "ogl_module_start"));
    const auto stop = reinterpret_cast<ogl_module_stop_fn>(
        dlsym(library, "ogl_module_stop"));
    const auto status = reinterpret_cast<const ogl_rtx_status*(*)()>(
        dlsym(library, "ogl_rtx_get_status"));
    const auto submit = reinterpret_cast<int(*)(const ogl_rtx_frame_state*)>(
        dlsym(library, "ogl_rtx_submit_frame"));
    const auto count = reinterpret_cast<uint64_t(*)()>(
        dlsym(library, "ogl_rtx_received_frames"));
    const auto submit_geometry = reinterpret_cast<int(*)(const ogl_rtx_triangle*, uint32_t)>(
        dlsym(library, "ogl_rtx_submit_geometry"));
    const auto triangle_count = reinterpret_cast<uint32_t(*)()>(
        dlsym(library, "ogl_rtx_received_triangles"));
    const auto geometry_revision = reinterpret_cast<uint64_t(*)()>(
        dlsym(library, "ogl_rtx_geometry_revision"));
    const auto copy_geometry = reinterpret_cast<uint32_t(*)(ogl_rtx_triangle*,uint32_t)>(
        dlsym(library, "ogl_rtx_copy_geometry"));
    const auto submit_depth = reinterpret_cast<int(*)(const ogl_rtx_depth_debug*)>(
        dlsym(library, "ogl_rtx_submit_depth_debug"));
    const auto copy_depth = reinterpret_cast<int(*)(ogl_rtx_depth_debug*)>(
        dlsym(library, "ogl_rtx_copy_depth_debug"));
    const auto world_device = reinterpret_cast<int(*)()>(
        dlsym(library, "ogl_vulkan_world_device_bootstrapped"));
    const auto exclusive_present = reinterpret_cast<uint32_t(*)(uint32_t)>(
        dlsym(library,"ogl_vulkan_world_present_exclusive_test_frame"));
    const auto exclusive_count = reinterpret_cast<uint32_t(*)()>(
        dlsym(library,"ogl_vulkan_world_exclusive_present_count"));
    const auto persistent_clear = reinterpret_cast<uint32_t(*)(uint32_t)>(
        dlsym(library,"ogl_vulkan_world_persistent_clear_frame"));
    const auto persistent_count = reinterpret_cast<uint32_t(*)()>(
        dlsym(library,"ogl_vulkan_world_persistent_frame_count"));
    const auto persistent_builds = reinterpret_cast<uint32_t(*)()>(
        dlsym(library,"ogl_vulkan_world_persistent_build_count"));
    const auto persistent_releases = reinterpret_cast<uint32_t(*)()>(
        dlsym(library,"ogl_vulkan_world_persistent_release_count"));
    const auto persistent_idles = reinterpret_cast<uint32_t(*)()>(
        dlsym(library,"ogl_vulkan_world_persistent_teardown_idles"));
    const auto world_sync = reinterpret_cast<uint32_t(*)()>(
        dlsym(library, "ogl_vulkan_world_submit_sync_probe"));
    const auto sync_max_outstanding = reinterpret_cast<uint32_t(*)()>(
        dlsym(library,"ogl_vulkan_world_sync_max_outstanding"));
    const auto sync_last_submitted = reinterpret_cast<uint32_t(*)()>(
        dlsym(library,"ogl_vulkan_world_sync_last_submitted"));
    const auto sync_last_completed = reinterpret_cast<uint32_t(*)()>(
        dlsym(library,"ogl_vulkan_world_sync_last_completed"));
    const auto offscreen_frame = reinterpret_cast<uint32_t(*)()>(
        dlsym(library, "ogl_vulkan_world_render_offscreen_probe"));
    const auto offscreen_world = reinterpret_cast<uint32_t(*)()>(
        dlsym(library,"ogl_vulkan_world_render_offscreen_scene"));
    const auto offscreen_world_frames = reinterpret_cast<uint64_t(*)()>(
        dlsym(library,"ogl_vulkan_world_offscreen_scene_frames"));
    const auto offscreen_world_pixels = reinterpret_cast<uint32_t(*)()>(
        dlsym(library,"ogl_vulkan_world_offscreen_scene_pixels"));
    const auto offscreen_cache_builds = reinterpret_cast<uint32_t(*)()>(
        dlsym(library,"ogl_vulkan_world_offscreen_cache_builds"));
    const auto offscreen_cache_reuses = reinterpret_cast<uint32_t(*)()>(
        dlsym(library,"ogl_vulkan_world_offscreen_cache_reuses"));
    const auto offscreen_cache_releases = reinterpret_cast<uint32_t(*)()>(
        dlsym(library,"ogl_vulkan_world_offscreen_cache_releases"));
    const auto offscreen_cache_release = reinterpret_cast<uint32_t(*)()>(
        dlsym(library,"ogl_vulkan_world_offscreen_cache_release"));
    assert(offscreen_world && offscreen_world_frames && offscreen_world_pixels &&
           offscreen_cache_builds && offscreen_cache_reuses &&
           offscreen_cache_releases && offscreen_cache_release);
    assert(offscreen_world()==0 && offscreen_world_frames()==0 &&
           offscreen_world_pixels()==0 &&
           offscreen_cache_builds()==0 && offscreen_cache_reuses()==0 &&
           offscreen_cache_releases()==0 && offscreen_cache_release()==0);
    const auto world_upload = reinterpret_cast<int(*)(const ogl_rtx_triangle*,uint32_t)>(
        dlsym(library, "ogl_vulkan_world_upload_triangles"));
    const auto world_gpu_count = reinterpret_cast<uint32_t(*)()>(
        dlsym(library, "ogl_vulkan_world_gpu_triangle_count"));
    const auto world_camera = reinterpret_cast<int(*)(const ogl_rtx_frame_state*)>(
        dlsym(library,"ogl_vulkan_world_set_projection_camera"));
    const auto camera_ready = reinterpret_cast<uint32_t(*)()>(
        dlsym(library,"ogl_vulkan_world_projection_camera_ready"));
    const auto aspect_matches = reinterpret_cast<uint32_t(*)()>(
        dlsym(library,"ogl_vulkan_world_camera_matches_swapchain"));
    const auto live_inputs = reinterpret_cast<uint32_t(*)()>(
        dlsym(library,"ogl_vulkan_world_live_inputs_ready"));
    const auto camera_updates = reinterpret_cast<uint64_t(*)()>(
        dlsym(library,"ogl_vulkan_world_camera_updates"));
    const auto mesh_uploads = reinterpret_cast<uint64_t(*)()>(
        dlsym(library,"ogl_vulkan_world_mesh_uploads"));
    const auto mesh_generation = reinterpret_cast<uint64_t(*)()>(
        dlsym(library,"ogl_vulkan_world_mesh_generation"));
    const auto region_barrier = reinterpret_cast<int(*)()>(
        dlsym(library,"ogl_vulkan_world_invalidate_region"));
    const auto region_barrier_accepts = reinterpret_cast<uint64_t(*)()>(
        dlsym(library,"ogl_vulkan_world_region_barrier_accepts"));
    const auto region_barrier_failures = reinterpret_cast<uint64_t(*)()>(
        dlsym(library,"ogl_vulkan_world_region_barrier_failures"));
    const auto mesh_dedup_skips = reinterpret_cast<uint64_t(*)()>(
        dlsym(library,"ogl_vulkan_world_mesh_dedup_skips"));
    const auto camera_mesh_generation = reinterpret_cast<uint64_t(*)()>(
        dlsym(library,"ogl_vulkan_world_camera_mesh_generation"));
    const auto mesh_retire_waits = reinterpret_cast<uint32_t(*)()>(
        dlsym(library,"ogl_vulkan_world_mesh_retire_waits"));
    const auto mesh_retire_failures = reinterpret_cast<uint32_t(*)()>(
        dlsym(library,"ogl_vulkan_world_mesh_retire_failures"));
    const auto window_attach = reinterpret_cast<int(*)(void*,uint64_t,uint32_t,uint32_t)>(
        dlsym(library, "ogl_vulkan_world_attach_x11_window"));
    const auto window_ready = reinterpret_cast<int(*)()>(
        dlsym(library, "ogl_vulkan_world_swapchain_probe_ready"));
    const auto window_detach = reinterpret_cast<void(*)()>(
        dlsym(library, "ogl_vulkan_world_detach_window"));
    assert(world_camera && camera_ready && aspect_matches && live_inputs &&
           camera_updates && mesh_uploads && mesh_generation &&
           camera_mesh_generation && mesh_dedup_skips &&
           mesh_retire_waits && mesh_retire_failures &&
           region_barrier && region_barrier_accepts && region_barrier_failures);
    assert(region_barrier()==0 && region_barrier_accepts()==0 &&
           region_barrier_failures()==0);
    assert(mesh_dedup_skips()==0 && camera_mesh_generation()==0);
    assert(mesh_generation()==0 && mesh_retire_waits()==0 &&
           mesh_retire_failures()==0);
    assert(camera_ready()==0 && live_inputs()==0 &&
           aspect_matches()==0 && camera_updates()==0 && mesh_uploads()==0);
    assert(world_camera(nullptr)==0);
    assert(exclusive_present && exclusive_count &&
           exclusive_count()==0 && exclusive_present(1)==0);
    assert(persistent_clear && persistent_count && persistent_builds &&
           persistent_releases && persistent_idles);
    assert(persistent_clear(1)==0 && persistent_count()==0 &&
           persistent_builds()==0 && persistent_releases()==0 &&
           persistent_idles()==0);
    assert(window_attach && window_ready && window_detach &&
           world_sync && world_upload && world_gpu_count && offscreen_frame);
    assert(start && stop && status && submit && count && world_device &&
           submit_geometry && triangle_count &&
           geometry_revision && copy_geometry && submit_depth && copy_depth);
    ogl_host_api host{sizeof(host), OGL_MODULE_ABI_VERSION, log_cb};
    // A Vulkan world-device bootstrap never runs without explicit developer
    // consent. The full world/swapchain renderer is still unavailable.
    unsetenv("OGL_VULKAN_WORLD_BOOTSTRAP");
    unsetenv("OGL_VULKAN_FRAME_SYNC_PROBE");
    unsetenv("OGL_VULKAN_WORLD_MESH_UPLOAD");
    unsetenv("OGL_VULKAN_OFFSCREEN_FRAME_PROBE");
    unsetenv("OGL_VULKAN_EXCLUSIVE_FRAME_LOOP");
    assert(start(&host) == 0);
    assert(offscreen_frame()==0);
    assert(world_sync && sync_max_outstanding && sync_last_submitted &&
           sync_last_completed);
    assert(sync_max_outstanding()==0 && sync_last_submitted()==0 &&
           sync_last_completed()==0);
    assert(world_sync()==0);
    assert(world_gpu_count()==0);
    assert(world_device() == 0);
    assert(window_ready() == 0);
    assert(window_attach(nullptr,0,0,0)==0); // never fabricate a surface
    stop();
    assert(world_device() == 0);
    assert(window_ready() == 0);
    assert(setenv("OGL_VULKAN_WORLD_BOOTSTRAP", "1", 1) == 0);
    assert(start(&host) == 0);
    const int ready = world_device(); // 0 on OpenGL-only/CI adapters, 1 when available
    assert(ready == 0 || ready == 1);
    // Two genuine GPU queue submits and completion fences, when supported;
    // otherwise explicitly no-op on Vulkan-less CI machines.
    assert(world_sync()==0);
    assert(setenv("OGL_VULKAN_FRAME_SYNC_PROBE", "1", 1)==0);
    const uint32_t actual_sync=world_sync();
    assert(actual_sync==(ready ? 2u : 0u));
    assert(sync_max_outstanding()==(ready ? 2u : 0u));
    assert(sync_last_submitted()==(ready ? 2u : 0u));
    assert(sync_last_completed()==(ready ? 2u : 0u));
    // Repeating the probe must not re-use signaled fences or reset the
    // historical high-water mark. It submits two FRESH GPU command buffers.
    assert(world_sync()==(ready ? 2u : 0u));
    assert(sync_max_outstanding()==(ready ? 2u : 0u));
    assert(sync_last_submitted()==(ready ? 2u : 0u));
    assert(sync_last_completed()==(ready ? 2u : 0u));
    unsetenv("OGL_VULKAN_FRAME_SYNC_PROBE");
    assert(world_sync()==0);
    assert(sync_max_outstanding()==(ready ? 2u : 0u));
    // A genuine graphics VkRenderPass that clears 32x32 RGBA and verifies
    // multiple GPU-written pixels via VkBuffer image readback and fences.
    // Do not require a GPU on CI or any ray tracing hardware.
    assert(offscreen_frame()==0);
    assert(setenv("OGL_VULKAN_OFFSCREEN_FRAME_PROBE","1",1)==0);
    const uint32_t actual_frame=offscreen_frame();
    assert(actual_frame==0 || actual_frame==1);
    // Portable CI may lack a Vulkan GPU, but a hardware-qualified developer
    // run must fail if no real GPU frame was produced and verified.
    const char* require_frame=getenv("OGL_REQUIRE_VULKAN_OFFSCREEN_FRAME");
    if (require_frame && require_frame[0]=='1' && require_frame[1]=='\0' &&
        (ready!=1 || actual_frame!=1)) {
        std::fprintf(stderr,"Required Vulkan offscreen GPU frame not verified: device=%d frame=%u\n",
                     ready,actual_frame);
        stop();
        return 11;
    }
    if (actual_frame) assert(offscreen_frame()==1);
    unsetenv("OGL_VULKAN_OFFSCREEN_FRAME_PROBE");
    assert(offscreen_frame()==0);
    // Vertex/index GPU buffer ownership is independent of RTX. A missing
    // Vulkan GPU or missing host-coherent memory must fail closed.
    ogl_rtx_triangle world_triangle{};
    world_triangle.position[3]=1.f;
    world_triangle.position[7]=1.f;
    assert(world_upload(&world_triangle,1)==0); // opt-in is required
    assert(setenv("OGL_VULKAN_WORLD_MESH_UPLOAD","1",1)==0);
    const int uploaded=world_upload(&world_triangle,1);
    assert(uploaded==0 || uploaded==1);
    if (uploaded) {
        assert(world_gpu_count()==1);
        assert(live_inputs()==0 && camera_ready()==0 &&
               camera_mesh_generation()==0 && mesh_generation()>0 &&
               camera_updates()==0 && mesh_uploads()==1);
        ogl_rtx_frame_state live_frame{};
        live_frame.struct_size=sizeof(live_frame);
        live_frame.frame_version=OGL_RTX_FRAME_VERSION;
        live_frame.width=800;
        live_frame.height=600;
        live_frame.camera_forward[0]=1.f;
        live_frame.camera_right[1]=1.f;
        live_frame.camera_up[2]=1.f;
        live_frame.vertical_fov_radians=1.1f;
        live_frame.aspect_ratio=4.f/3.f;
        live_frame.near_clip_m=.1f;
        live_frame.far_clip_m=128.f;
        assert(world_camera(&live_frame)==0); // no camera opt-in
        assert(setenv("OGL_VULKAN_WORLD_CAMERA_PROBE","1",1)==0);
        assert(world_camera(&live_frame)==1);
        assert(camera_ready()==1 && live_inputs()==1 &&
               camera_mesh_generation()==mesh_generation() &&
               camera_updates()==1);
        live_frame.aspect_ratio=2.f;
        assert(world_camera(&live_frame)==0); // invalid input clears camera
        assert(camera_ready()==0 && live_inputs()==0 &&
               camera_mesh_generation()==0 &&
               camera_updates()==1);
        live_frame.aspect_ratio=4.f/3.f;
        assert(world_camera(&live_frame)==1 && live_inputs()==1 &&
               camera_updates()==2);
        const uint64_t old_mesh=mesh_generation();
        assert(world_upload(&world_triangle,1)==1); // new scene invalidates pose
        assert(mesh_generation()==old_mesh+1 &&
               camera_mesh_generation()==0 &&
               camera_ready()==0 && live_inputs()==0 && mesh_uploads()==2);
        assert(world_camera(&live_frame)==1 && live_inputs()==1 &&
               camera_mesh_generation()==mesh_generation() &&
               camera_updates()==3);
        auto bad=world_triangle;
        bad.position[0]=std::numeric_limits<float>::quiet_NaN();
        assert(world_upload(&bad,1)==0);
        assert(world_gpu_count()==1); // rejected upload never clears old scene
        assert(live_inputs()==1 && mesh_uploads()==2); // atomic rejection
        std::vector<ogl_rtx_triangle> all(8192,world_triangle);
        assert(world_upload(all.data(),8192)==1);
        assert(world_gpu_count()==8192);
        assert(camera_ready()==0 && live_inputs()==0 &&
               camera_mesh_generation()==0 &&
               mesh_uploads()==3); // no stale camera across mesh revisions
        assert(world_upload(all.data(),8193)==0);
        assert(world_gpu_count()==8192);
        assert(world_upload(nullptr,0)==1);
        assert(world_gpu_count()==0); // region-clear frees GPU buffers
        assert(camera_ready()==0 && live_inputs()==0 &&
               camera_mesh_generation()==0 &&
               mesh_uploads()==3 && camera_updates()==3);
        unsetenv("OGL_VULKAN_WORLD_CAMERA_PROBE");
    } else {
        assert(world_gpu_count()==0);
    }
    unsetenv("OGL_VULKAN_WORLD_MESH_UPLOAD");
    assert(world_upload(&world_triangle,1)==0);
    assert(world_gpu_count()==0);
    assert(window_ready() == 0);
    assert(window_attach(nullptr,0,1920,1080)==0);
    assert(window_ready() == 0);
    window_detach();
    assert(window_ready() == 0);
    assert(status()->rendering_active == 0);
    // Multiple starts and stop must never retain a stale logical device.
    assert(start(&host) == 0);
    assert(world_device() == ready || world_device() == 0);
    assert(window_attach(nullptr,0,1920,1080)==0);
    assert(window_ready()==0);
    stop();
    assert(world_device() == 0);
    assert(window_ready()==0);
    assert(offscreen_frame()==0);
    assert(offscreen_cache_builds()==0 && offscreen_cache_reuses()==0 &&
           offscreen_cache_releases()==0 && offscreen_cache_release()==0);
    assert(camera_ready()==0 && live_inputs()==0 &&
           aspect_matches()==0 &&
           camera_updates()==0 && mesh_uploads()==0);
    assert(sync_max_outstanding()==0 && sync_last_submitted()==0 &&
           sync_last_completed()==0);
    unsetenv("OGL_VULKAN_WORLD_BOOTSTRAP");
    assert(start(&host) == 0);
    assert(world_device() == 0);
    assert(count() == 0);

    ogl_rtx_frame_state valid{};
    valid.struct_size = sizeof(valid);
    valid.frame_version = OGL_RTX_FRAME_VERSION;
    valid.width = 3440;
    valid.height = 1440;
    valid.camera_forward[0] = 1.0f;
    valid.camera_right[1] = 1.0f;
    valid.camera_up[2] = 1.0f;
    valid.vertical_fov_radians = 1.0f;
    valid.aspect_ratio = float(valid.width) / valid.height;
    valid.near_clip_m = 0.1f;
    valid.far_clip_m = 256.f;
    valid.sun_direction[2] = 1.0f;

    ogl_rtx_frame_state invalid = valid;
    invalid.width = 0;
    assert(submit(&invalid) == 0);
    assert(submit(nullptr) == 0);
    assert(count() == 0);
    if (status()->hardware_ready) {
        assert(submit(&valid) == 1);
        assert(submit(&valid) == 1);
        assert(count() == 2);
        ogl_rtx_depth_debug depth{};
        depth.struct_size = sizeof(depth);
        depth.version = OGL_RTX_DEPTH_DEBUG_VERSION;
        depth.viewport_width = 1720; // half-resolution deferred target
        depth.viewport_height = 720;
        depth.grid_width = 8; // exact whole-viewport ray grid
        depth.grid_height = 8;
        for (float& pixel : depth.normalized_depth) pixel = 0.75f;
        ogl_rtx_depth_debug output{};
        output.struct_size = sizeof(output);
        output.version = OGL_RTX_DEPTH_DEBUG_VERSION;
        assert(copy_depth(&output) == 0); // no depth yet
        assert(submit_depth(&depth) == 1);
        assert(copy_depth(&output) == 1);
        assert(output.normalized_depth[63] == 0.75f);
        depth.normalized_depth[0] = std::numeric_limits<float>::quiet_NaN();
        assert(submit_depth(&depth) == 0);
        depth.normalized_depth[0] = 0.75f;
        depth.grid_width = 7;
        assert(submit_depth(&depth) == 0);
        depth.grid_width = 8;
        depth.grid_height = 9;
        assert(submit_depth(&depth) == 0);
        depth.grid_height = 8;
        depth.viewport_width = 1600; // incompatible aspect
        assert(submit_depth(&depth) == 0);
        depth.viewport_width = 1720;
        depth.version = 999;
        assert(submit_depth(&depth) == 0);
        depth.version = OGL_RTX_DEPTH_DEBUG_VERSION;
        assert(submit(&valid) == 1); // new frame discards older tile
        assert(copy_depth(&output) == 0);
    } else {
        assert(submit(&valid) == 0);
        assert(count() == 0);
    }
    assert(triangle_count() == 0);
    assert(geometry_revision() == 0);
    assert(copy_geometry(nullptr, 0) == 0);
    ogl_rtx_triangle valid_triangle{};
    valid_triangle.position[3] = 1.0f;
    valid_triangle.position[7] = 1.0f;
    assert(submit_geometry(nullptr, 1) == 0);
    // On machines without RT hardware the module may reject even a clear
    // request, and must never crash or claim rendering is supported.
    assert(submit_geometry(&valid_triangle, 0) == (status()->hardware_ready ? 1 : 0));
    std::vector<ogl_rtx_triangle> oversized(OGL_RTX_MAX_SCENE_TRIANGLES + 1);
    assert(submit_geometry(oversized.data(), oversized.size()) == 0);
    ogl_rtx_triangle nan_triangle = valid_triangle;
    nan_triangle.position[0] = std::numeric_limits<float>::quiet_NaN();
    assert(submit_geometry(&nan_triangle, 1) == 0);
    ogl_rtx_triangle degenerate_triangle{};
    assert(submit_geometry(&degenerate_triangle, 1) == 0);
    ogl_rtx_triangle out_of_range = valid_triangle;
    out_of_range.position[0] = 1.1e6f;
    assert(submit_geometry(&out_of_range, 1) == 0);
    if (status()->hardware_ready) {
        assert(submit_geometry(&valid_triangle, 1) == 1);
        assert(triangle_count() == 1);
        assert(geometry_revision() == 1);
        ogl_rtx_triangle copied{};
        assert(copy_geometry(&copied, 0) == 0);
        assert(copy_geometry(&copied, 1) == 1);
        for (unsigned j = 0; j < 9; ++j)
            assert(copied.position[j] == valid_triangle.position[j]);
        assert(submit_geometry(&valid_triangle, 1) == 1);
        assert(geometry_revision() == 1); // byte-identical scene deduped
        ogl_rtx_triangle moved = valid_triangle;
        moved.position[0] = 0.2f;
        assert(submit_geometry(&moved, 1) == 1);
        assert(geometry_revision() == 2);
        assert(copy_geometry(&copied, 1) == 1);
        assert(copied.position[0] == 0.2f);
        assert(submit_geometry(&nan_triangle, 1) == 0);
        assert(triangle_count() == 1); // invalid updates cannot destroy last valid geometry
        assert(submit_geometry(nullptr, 0) == 1);
        assert(triangle_count() == 0); // teleport/region change invalidates stale geometry
        assert(geometry_revision() == 3);
        assert(copy_geometry(&copied, 1) == 0);
        assert(submit_geometry(nullptr, 0) == 1);
        assert(geometry_revision() == 3); // repeated clear is idempotent
        std::vector<ogl_rtx_triangle> bulk(OGL_RTX_MAX_SCENE_TRIANGLES);
        for (size_t i = 0; i < bulk.size(); ++i) {
            bulk[i] = valid_triangle;
            const float offset = float(i) * 0.25f;
            for (unsigned vertex = 0; vertex < 3; ++vertex)
                bulk[i].position[3 * vertex] += offset;
        }
        assert(submit_geometry(bulk.data(), bulk.size()) == 1);
        assert(triangle_count() == bulk.size());
        assert(geometry_revision() == 4);
        std::vector<ogl_rtx_triangle> buffer(bulk.size());
        assert(copy_geometry(buffer.data(), bulk.size() - 1) == 0);
        assert(copy_geometry(buffer.data(), bulk.size()) == bulk.size());
        assert(buffer[1023].position[0] == bulk[1023].position[0]);
        assert(submit_geometry(bulk.data(), bulk.size()) == 1);
        assert(geometry_revision() == 4);
        assert(submit_geometry(nullptr, 0) == 1);
        assert(triangle_count() == 0);
        assert(geometry_revision() == 5);
    } else {
        assert(submit_geometry(&valid_triangle, 1) == 0);
        assert(triangle_count() == 0);
        assert(geometry_revision() == 0);
    }
    assert(status()->rendering_active == 0);
    stop();
#if defined(OGL_TEST_X11_WSI)
    // Explicit opt-in: disposable X11 QA window, with NO OpenGL context.
    // This is not a parallel window of the interactive GL Viewer.
    const char* native_flag=std::getenv("OGL_VULKAN_PRESENT_TEST_X11");
    if (native_flag && std::strcmp(native_flag,"1")==0) {
        Display* display=XOpenDisplay(nullptr);
        if (!display) {
            std::fputs("No X11 display for required Vulkan present test\n",stderr);
            return 12;
        }
        const Window root=DefaultRootWindow(display);
        Window window=XCreateSimpleWindow(display,root,30,30,320,240,0,0,0);
        XMapWindow(display,window);
        XSync(display,False);
        setenv("OGL_VULKAN_WORLD_BOOTSTRAP","1",1);
        setenv("OGL_VULKAN_WINDOW_SURFACE","1",1);
        setenv("OGL_VULKAN_EXCLUSIVE_PRESENT_PROBE","1",1);
        const int started=start(&host);
        const int device=world_device();
        const int attached=window_attach(display,static_cast<uint64_t>(window),320,240);
        const int swapchain=window_ready();
        const uint32_t without_owner=exclusive_present(0);
        unsetenv("OGL_VULKAN_EXCLUSIVE_PRESENT_PROBE");
        const uint32_t without_flag=exclusive_present(1);
        setenv("OGL_VULKAN_EXCLUSIVE_PRESENT_PROBE","1",1);
        const uint32_t presented=exclusive_present(1);
        const uint32_t duplicate=exclusive_present(1);
        const uint32_t first_count=exclusive_count();
        window_detach();
        const uint32_t after_first_detach=exclusive_count();
        // A resized native X11 window needs a newly attached swapchain;
        // never reuse an image/view/extent from the old attachment.
        XResizeWindow(display,window,480,300);
        XSync(display,False);
        const int resized=window_attach(display,static_cast<uint64_t>(window),480,300);
        const int resized_ready=window_ready();
        const uint32_t resized_presented=exclusive_present(1);
        const uint32_t resized_duplicate=exclusive_present(1);
        const uint32_t second_count=exclusive_count();
        window_detach();
        const uint32_t after_second_detach=exclusive_count();

        // P05/3 multi-frame lifecycle: one X11 swapchain, sixteen distinct
        // consecutive acquired/presented images; no reattachment in between.
        // The 17th frame must fail closed rather than reusing pending memory.
        XResizeWindow(display,window,640,360);
        XSync(display,False);
        const int frame_loop_attached=window_attach(
            display,static_cast<uint64_t>(window),640,360);
        const int frame_loop_ready=window_ready();
        setenv("OGL_VULKAN_EXCLUSIVE_FRAME_LOOP","1",1);
        uint32_t completed=0;
        bool frame_sequence_valid=true;
        for (uint32_t i=0; i<16; ++i) {
            if (exclusive_present(1)!=1 || exclusive_count()!=i+1) {
                frame_sequence_valid=false;
                break;
            }
            ++completed;
            // Verify every presented pixel, not just the native API result.
            // Normalize each RGB component through the X11 Visual masks,
            // so 24/32-bit vs 16-bit visuals are interpreted correctly.
            XSync(display,False);
            XImage* image=XGetImage(display,window,320,180,1,1,
                                   AllPlanes,ZPixmap);
            if (image) {
                const unsigned long pixel=XGetPixel(image,0,0);
                auto channel=[&](unsigned long mask) -> int {
                    if (!mask) return -1;
                    unsigned shift=0;
                    while (!(mask&1UL)) { mask>>=1U; ++shift; }
                    const unsigned long value=(pixel>>shift)&mask;
                    return static_cast<int>((value*255UL+mask/2)/mask);
                };
                const int red=channel(image->red_mask);
                const int green=channel(image->green_mask);
                const int blue=channel(image->blue_mask);
                const bool warm=(i%2)==1;
                const bool valid=std::abs(red-(warm?166:20))<=5 &&
                                 std::abs(green-(warm?89:54))<=5 &&
                                 std::abs(blue-(warm?31:115))<=5;
                if (!valid) frame_sequence_valid=false;
                if (i==0 || i==1 || i==15)
                    std::fprintf(stdout,
                        "P05 verified X11 swapchain pixel frame %u: R=%d G=%d B=%d valid=%d\n",
                        i+1,red,green,blue,valid?1:0);
                XDestroyImage(image);
            } else {
                frame_sequence_valid=false;
            }
        }
        const uint32_t overflow=exclusive_present(1);
        const uint32_t frame_count=exclusive_count();
        unsetenv("OGL_VULKAN_EXCLUSIVE_FRAME_LOOP");
        const uint32_t without_loop=exclusive_present(1);
        window_detach();
        const uint32_t reset_count=exclusive_count();
        // After leaving multi-frame mode and recreating the swapchain,
        // the original single-shot policy must be restored exactly.
        XResizeWindow(display,window,256,192);
        XSync(display,False);
        const int restored_attach=window_attach(display,
            static_cast<uint64_t>(window),256,192);
        const uint32_t restored_frame=exclusive_present(1);
        const uint32_t restored_repeat=exclusive_present(1);
        const uint32_t restored_count=exclusive_count();
        window_detach();
        const uint32_t restored_reset=exclusive_count();
        // Strict developer-only raster QA. This reuses the existing real
        // OpenSim XYZ VkVertex/VkIndex upload ABI, with explicitly
        // CLIP-SPACE coordinates solely for the graphics pipeline test.
        // Normal QA still tests only swapchain clears, not scene draws.
        const char* raster_test_env=std::getenv("OGL_VULKAN_RASTER_TEST_X11");
        const bool raster_test=raster_test_env &&
                               std::strcmp(raster_test_env,"1")==0;
        int raster_attached=0,raster_uploaded=0;
        uint32_t raster_presented=0,raster_count=0,raster_without_mesh=0;
        bool raster_pixels=false,raster_cleared=false;
        if (raster_test) {
            XResizeWindow(display,window,512,384);
            XSync(display,False);
            const ogl_rtx_triangle clip_triangle{{
                -0.72f,-0.68f,0.25f,
                 0.72f,-0.68f,0.25f,
                 0.f,  0.72f,0.25f
            }};
            setenv("OGL_VULKAN_WORLD_MESH_UPLOAD","1",1);
            raster_attached=window_attach(display,
                static_cast<uint64_t>(window),512,384);
            setenv("OGL_VULKAN_EXCLUSIVE_RASTER_PROBE","1",1);
            // If no GPU mesh was uploaded, raster MUST decline without
            // consuming or submitting a Vulkan swapchain image.
            if (raster_attached==1)
                raster_without_mesh=exclusive_present(1);
            raster_uploaded=world_upload(&clip_triangle,1);
            raster_count=world_gpu_count();
            if (raster_uploaded==1 && raster_attached==1 &&
                raster_count==1 && raster_without_mesh==0)
                raster_presented=exclusive_present(1);
            if (raster_presented==1) {
                auto sample=[&](int x,int y,int& red,int& green,int& blue) {
                    XSync(display,False);
                    XImage* image=XGetImage(display,window,x,y,1,1,
                                           AllPlanes,ZPixmap);
                    if (!image) return false;
                    const unsigned long pixel=XGetPixel(image,0,0);
                    auto channel=[&](unsigned long mask) {
                        if (!mask) return -1;
                        unsigned shift=0;
                        while (!(mask&1UL)) { mask>>=1; ++shift; }
                        return static_cast<int>(
                            (((pixel>>shift)&mask)*255UL+mask/2)/mask);
                    };
                    red=channel(image->red_mask);
                    green=channel(image->green_mask);
                    blue=channel(image->blue_mask);
                    XDestroyImage(image);
                    return true;
                };
                int cr=0,cg=0,cb=0,br=0,bg=0,bb=0;
                const bool center=sample(256,192,cr,cg,cb);
                const bool background=sample(30,30,br,bg,bb);
                raster_pixels=center && background &&
                    std::abs(cr-237)<=5 &&
                    std::abs(cg-48)<=5 &&
                    std::abs(cb-38)<=5 &&
                    std::abs(br-20)<=5 &&
                    std::abs(bg-54)<=5 &&
                    std::abs(bb-115)<=5;
                std::fprintf(stdout,
                    "P05 Vulkan indexed raster X11 pixel: center=%d,%d,%d "
                    "background=%d,%d,%d accepted=%d\n",
                    cr,cg,cb,br,bg,bb,raster_pixels ? 1:0);
            }
            unsetenv("OGL_VULKAN_EXCLUSIVE_RASTER_PROBE");
            window_detach();
            raster_cleared=world_upload(nullptr,0)==1 &&
                           world_gpu_count()==0;
            unsetenv("OGL_VULKAN_WORLD_MESH_UPLOAD");
        }
        // P05 world-matrix acceptance: synthetic agent-local WORLD XYZ,
        // not pre-projected clip vertices. Real VkPushConstants/GLSL
        // projection must move the projected triangle when camera moves.
        const char* projection_test=std::getenv(
            "OGL_VULKAN_WORLD_PROJECTION_TEST_X11");
        const bool test_world=projection_test &&
                              std::strcmp(projection_test,"1")==0;
        bool world_positive=false,world_shifted=false,world_bad=false;
        if (test_world) {
            const ogl_rtx_triangle world_triangle{{
                13.f,17.2f,28.05f,
                13.f,22.8f,28.05f,
                13.f,20.f,31.95f
            }};
            ogl_rtx_frame_state camera{};
            camera.struct_size=sizeof(camera);
            camera.frame_version=OGL_RTX_FRAME_VERSION;
            camera.width=512;
            camera.height=384;
            camera.camera_position[0]=10.f;
            camera.camera_position[1]=20.f;
            camera.camera_position[2]=30.f;
            camera.camera_forward[0]=1.f;
            camera.camera_right[1]=1.f;
            camera.camera_up[2]=1.f;
            camera.sun_direction[2]=1.f;
            camera.vertical_fov_radians=1.57079632679f;
            camera.aspect_ratio=512.f/384.f;
            camera.near_clip_m=0.1f;
            camera.far_clip_m=100.f;
            setenv("OGL_VULKAN_WORLD_MESH_UPLOAD","1",1);
            setenv("OGL_VULKAN_WORLD_CAMERA_PROBE","1",1);
            setenv("OGL_VULKAN_EXCLUSIVE_RASTER_PROBE","1",1);
            setenv("OGL_VULKAN_EXCLUSIVE_WORLD_PROBE","1",1);
            auto bad_camera=camera;
            bad_camera.camera_forward[0]=0.f;
            const bool refused_bad_axis=world_camera(&bad_camera)==0 &&
                                        camera_ready()==0;
            bad_camera=camera;
            bad_camera.aspect_ratio=4.f;
            const bool refused_bad_aspect=world_camera(&bad_camera)==0 &&
                                          camera_ready()==0;
            const bool uploaded_world=world_upload(&world_triangle,1)==1 &&
                                      world_gpu_count()==1;
            XResizeWindow(display,window,512,384);
            XSync(display,False);
            const bool attach_world=window_attach(display,
                static_cast<uint64_t>(window),512,384)==1;
            const bool rejected_no_camera=exclusive_present(1)==0 &&
                                          exclusive_count()==0;
            const bool accepted_camera=world_camera(&camera)==1 &&
                                       camera_ready()==1;
            const bool drawn_world=uploaded_world && attach_world &&
                accepted_camera && exclusive_present(1)==1;
            auto pixel_is=[&](int x,int y,int r,int g,int b) {
                XSync(display,False);
                XImage* image=XGetImage(display,window,x,y,1,1,
                                       AllPlanes,ZPixmap);
                if (!image) return false;
                const unsigned long pixel=XGetPixel(image,0,0);
                auto channel=[&](unsigned long mask) {
                    if (!mask) return -1;
                    unsigned shift=0;
                    while (!(mask&1UL)) {mask>>=1; ++shift;}
                    return static_cast<int>(
                        (((pixel>>shift)&mask)*255UL+mask/2)/mask);
                };
                const bool equal=std::abs(channel(image->red_mask)-r)<=5 &&
                    std::abs(channel(image->green_mask)-g)<=5 &&
                    std::abs(channel(image->blue_mask)-b)<=5;
                XDestroyImage(image);
                return equal;
            };
            world_positive=refused_bad_axis && refused_bad_aspect &&
                rejected_no_camera && drawn_world &&
                pixel_is(256,192,237,48,38) &&
                pixel_is(30,30,20,54,115);
            window_detach();
            // Translate camera sideward while keeping the exact same
            // world-space triangle GPU buffers. Center MUST now be clear.
            camera.camera_position[1]=32.f;
            const bool moved_camera=world_camera(&camera)==1 &&
                                    camera_ready()==1;
            const bool reattached=window_attach(display,
                static_cast<uint64_t>(window),512,384)==1;
            const bool moved_draw=moved_camera && reattached &&
                                   exclusive_present(1)==1;
            world_shifted=moved_draw && pixel_is(256,192,20,54,115);
            window_detach();
            bad_camera=camera;
            bad_camera.vertical_fov_radians=0.f;
            world_bad=world_camera(&bad_camera)==0 && camera_ready()==0 &&
                world_upload(nullptr,0)==1 && world_gpu_count()==0;
            unsetenv("OGL_VULKAN_EXCLUSIVE_WORLD_PROBE");
            unsetenv("OGL_VULKAN_EXCLUSIVE_RASTER_PROBE");
            unsetenv("OGL_VULKAN_WORLD_CAMERA_PROBE");
            unsetenv("OGL_VULKAN_WORLD_MESH_UPLOAD");
            std::fprintf(stdout,
                "P05 Vulkan world-camera projection: visible=%d moved=%d invalid=%d\n",
                world_positive?1:0,world_shifted?1:0,world_bad?1:0);
        }
        // P05 depth: two perfectly overlapping indexed triangles.
        // Near red (z=.25) is submitted FIRST; far green (z=.75) is
        // submitted LAST. Depth off would incorrectly show green.
        const char* depth_test_env=std::getenv("OGL_VULKAN_DEPTH_TEST_X11");
        const bool test_depth=depth_test_env &&
                              std::strcmp(depth_test_env,"1")==0;
        bool depth_positive=false,depth_resized=false,depth_cleared=false;
        if (test_depth) {
            const ogl_rtx_triangle near_triangle{{
                -.72f,-.68f,.25f, .72f,-.68f,.25f, 0.f,.72f,.25f
            }};
            const ogl_rtx_triangle far_triangle{{
                -.72f,-.68f,.75f, .72f,-.68f,.75f, 0.f,.72f,.75f
            }};
            const ogl_rtx_triangle triangles[2]{near_triangle,far_triangle};
            setenv("OGL_VULKAN_WORLD_MESH_UPLOAD","1",1);
            setenv("OGL_VULKAN_EXCLUSIVE_DEPTH_PROBE","1",1);
            XResizeWindow(display,window,512,384);
            XSync(display,False);
            const bool uploaded=world_upload(triangles,2)==1 &&
                                world_gpu_count()==2;
            const bool depth_attach=window_attach(display,
                static_cast<uint64_t>(window),512,384)==1;
            // A depth-only request must be rejected BEFORE image acquire.
            const bool denied=exclusive_present(1)==0 &&
                              exclusive_count()==0;
            setenv("OGL_VULKAN_EXCLUSIVE_RASTER_PROBE","1",1);
            const bool presented_depth=uploaded && depth_attach && denied &&
                                       exclusive_present(1)==1;
            auto depth_sample=[&](int x,int y,int red,int green,int blue) {
                XSync(display,False);
                XImage* image=XGetImage(display,window,x,y,1,1,
                                       AllPlanes,ZPixmap);
                if (!image) return false;
                const unsigned long pixel=XGetPixel(image,0,0);
                auto channel=[&](unsigned long mask) {
                    if (!mask) return -1;
                    unsigned shift=0;
                    while (!(mask&1UL)) {mask>>=1; ++shift;}
                    return static_cast<int>(
                        (((pixel>>shift)&mask)*255UL+mask/2)/mask);
                };
                const bool correct=
                    std::abs(channel(image->red_mask)-red)<=5 &&
                    std::abs(channel(image->green_mask)-green)<=5 &&
                    std::abs(channel(image->blue_mask)-blue)<=5;
                XDestroyImage(image);
                return correct;
            };
            depth_positive=presented_depth &&
                depth_sample(256,192,237,48,38) &&
                depth_sample(26,26,20,54,115);
            window_detach();
            XResizeWindow(display,window,640,400);
            XSync(display,False);
            const bool resize_attach=window_attach(display,
                static_cast<uint64_t>(window),640,400)==1;
            depth_resized=resize_attach && exclusive_present(1)==1 &&
                depth_sample(320,200,237,48,38) &&
                depth_sample(30,30,20,54,115);
            window_detach();
            depth_cleared=world_upload(nullptr,0)==1 &&
                          world_gpu_count()==0;
            unsetenv("OGL_VULKAN_EXCLUSIVE_DEPTH_PROBE");
            unsetenv("OGL_VULKAN_EXCLUSIVE_RASTER_PROBE");
            unsetenv("OGL_VULKAN_WORLD_MESH_UPLOAD");
            std::fprintf(stdout,
                "P05 Vulkan depth GPU occlusion: near_over_far=%d resized=%d cleared=%d\n",
                depth_positive?1:0,depth_resized?1:0,depth_cleared?1:0);
        }
        // Synthetic OpenSim-style XYZ and a real Vulkan perspective matrix:
        // two planar mesh triangles project to the exact same screen area,
        // but one is z=.25 and the far-last triangle z=.75 in Vulkan NDC.
        // The test also runs without a depth attachment as a negative
        // control: its same GPU fragment shader MUST show far green.
        const char* world_depth_env=std::getenv(
            "OGL_VULKAN_WORLD_DEPTH_TEST_X11");
        const bool test_world_depth=world_depth_env &&
                                  std::strcmp(world_depth_env,"1")==0;
        bool world_depth_near=false,world_depth_control=false;
        bool world_depth_moved=false,world_depth_resized=false;
        bool world_depth_aspect=false;
        bool world_depth_invalid=false,world_depth_cleared=false;
        if (test_world_depth) {
            const ogl_rtx_triangle projected_near{{
                12.5f,18.f,28.4f,
                12.5f,22.f,28.4f,
                12.5f,20.f,31.6f
            }};
            const ogl_rtx_triangle projected_far{{
                15.f,16.f,26.8f,
                15.f,24.f,26.8f,
                15.f,20.f,33.2f
            }};
            const ogl_rtx_triangle paired[2]{
                projected_near,projected_far
            };
            ogl_rtx_frame_state projection{};
            projection.struct_size=sizeof(projection);
            projection.frame_version=OGL_RTX_FRAME_VERSION;
            projection.width=512;
            projection.height=384;
            projection.camera_position[0]=10.f;
            projection.camera_position[1]=20.f;
            projection.camera_position[2]=30.f;
            projection.camera_forward[0]=1.f;
            projection.camera_right[1]=1.f;
            projection.camera_up[2]=1.f;
            projection.vertical_fov_radians=1.57079632679f;
            projection.aspect_ratio=4.f/3.f;
            projection.near_clip_m=2.f;
            projection.far_clip_m=10.f;
            projection.sun_direction[2]=1.f;
            auto read_rgb=[&](int x,int y,int r,int g,int b) {
                XSync(display,False);
                XImage* img=XGetImage(display,window,x,y,1,1,
                                     AllPlanes,ZPixmap);
                if (!img) return false;
                const unsigned long pixel=XGetPixel(img,0,0);
                auto channel=[pixel](unsigned long mask) {
                    if (!mask) return -1;
                    unsigned shift=0;
                    while (!(mask&1UL)) {mask>>=1; ++shift;}
                    return static_cast<int>(
                        (((pixel>>shift)&mask)*255UL+mask/2)/mask);
                };
                const bool matches=
                    std::abs(channel(img->red_mask)-r)<=5 &&
                    std::abs(channel(img->green_mask)-g)<=5 &&
                    std::abs(channel(img->blue_mask)-b)<=5;
                XDestroyImage(img);
                return matches;
            };
            setenv("OGL_VULKAN_WORLD_MESH_UPLOAD","1",1);
            setenv("OGL_VULKAN_WORLD_CAMERA_PROBE","1",1);
            setenv("OGL_VULKAN_EXCLUSIVE_RASTER_PROBE","1",1);
            setenv("OGL_VULKAN_EXCLUSIVE_WORLD_PROBE","1",1);
            setenv("OGL_VULKAN_EXCLUSIVE_DEPTH_PROBE","1",1);
            const bool mesh_ok=world_upload(paired,2)==1 &&
                               world_gpu_count()==2;
            const bool invalidated=mesh_ok && camera_ready()==0 &&
                                   live_inputs()==0;
            const bool camera_ok=world_camera(&projection)==1 &&
                                 camera_ready()==1 &&
                                 live_inputs()==1;
            XResizeWindow(display,window,512,384);
            XSync(display,False);
            const bool attached_world=window_attach(
                display,static_cast<uint64_t>(window),512,384)==1;
            world_depth_near=mesh_ok && invalidated && camera_ok &&
                attached_world && exclusive_present(1)==1 &&
                read_rgb(256,192,237,48,38) &&
                read_rgb(26,26,20,54,115);
            window_detach();

            // Exact same shader, XYZ buffers and camera, but NO depth image.
            unsetenv("OGL_VULKAN_EXCLUSIVE_DEPTH_PROBE");
            setenv("OGL_VULKAN_EXCLUSIVE_DEPTH_COLOR_PROBE","1",1);
            const bool negative_attach=window_attach(
                display,static_cast<uint64_t>(window),512,384)==1;
            world_depth_control=negative_attach && exclusive_present(1)==1 &&
                read_rgb(256,192,20,199,41) &&
                read_rgb(26,26,20,54,115);
            window_detach();
            unsetenv("OGL_VULKAN_EXCLUSIVE_DEPTH_COLOR_PROBE");
            setenv("OGL_VULKAN_EXCLUSIVE_DEPTH_PROBE","1",1);

            // Live camera movement with GPU triangle data unchanged:
            // neither near nor far triangle may remain in the center.
            projection.camera_position[1]=32.f;
            const bool moved_camera=world_camera(&projection)==1 &&
                                    live_inputs()==1;
            const bool moved_attach=window_attach(
                display,static_cast<uint64_t>(window),512,384)==1;
            world_depth_moved=moved_camera && moved_attach &&
                exclusive_present(1)==1 &&
                read_rgb(256,192,20,54,115);
            window_detach();

            // GPU depth resources must be rebuilt at a new swapchain extent.
            projection.camera_position[1]=20.f;
            projection.width=640;
            projection.height=480;
            const bool resized_camera=world_camera(&projection)==1 &&
                                      live_inputs()==1;
            XResizeWindow(display,window,640,480);
            XSync(display,False);
            const bool resize_attach=window_attach(
                display,static_cast<uint64_t>(window),640,480)==1;
            world_depth_resized=resized_camera && resize_attach &&
                aspect_matches()==1 &&
                exclusive_present(1)==1 &&
                read_rgb(320,240,237,48,38) &&
                read_rgb(30,30,20,54,115);
            window_detach();

            // Real 4:3 -> 16:9 X11 resize: an old camera has the wrong
            // viewport and cannot be used until a matching update arrives.
            XResizeWindow(display,window,800,450);
            XSync(display,False);
            const bool changed_aspect=window_attach(
                display,static_cast<uint64_t>(window),800,450)==1;
            const bool rejected_stale=changed_aspect &&
                camera_ready()==0 && aspect_matches()==0 &&
                live_inputs()==0 && exclusive_present(1)==0 &&
                exclusive_count()==0;
            projection.width=800;
            projection.height=450;
            projection.aspect_ratio=800.f/450.f;
            const bool corrected_camera=world_camera(&projection)==1 &&
                camera_ready()==1 && live_inputs()==1 &&
                aspect_matches()==1;
            const bool corrected_present=corrected_camera &&
                exclusive_present(1)==1 &&
                read_rgb(400,225,237,48,38) &&
                read_rgb(30,30,20,54,115);
            world_depth_aspect=rejected_stale && corrected_present;
            window_detach();

            // A failed camera update kills the previous projection, even
            // though the GPU meshes and their memory still exist.
            projection.vertical_fov_radians=0.f;
            world_depth_invalid=world_camera(&projection)==0 &&
                                camera_ready()==0 && live_inputs()==0;
            const bool rejected_attach=window_attach(
                display,static_cast<uint64_t>(window),800,450)==1;
            world_depth_invalid=world_depth_invalid && rejected_attach &&
                exclusive_present(1)==0 && exclusive_count()==0;
            window_detach();
            world_depth_cleared=world_upload(nullptr,0)==1 &&
                                world_gpu_count()==0 &&
                                live_inputs()==0 &&
                                camera_ready()==0;
            unsetenv("OGL_VULKAN_EXCLUSIVE_DEPTH_PROBE");
            unsetenv("OGL_VULKAN_EXCLUSIVE_WORLD_PROBE");
            unsetenv("OGL_VULKAN_EXCLUSIVE_RASTER_PROBE");
            unsetenv("OGL_VULKAN_WORLD_CAMERA_PROBE");
            unsetenv("OGL_VULKAN_WORLD_MESH_UPLOAD");
            std::fprintf(stdout,
                "P05 Vulkan projected world depth: near=%d no_depth_far=%d "
                "moved=%d resized=%d aspect=%d invalid=%d clear=%d\n",
                world_depth_near?1:0,world_depth_control?1:0,
                world_depth_moved?1:0,world_depth_resized?1:0,
                world_depth_aspect?1:0,
                world_depth_invalid?1:0,world_depth_cleared?1:0);
        }
        // P05/3 persistent real X11 Vulkan WSI: keep all GPU objects across
        // 16 frames. The per-frame function MUST contain no queue idle;
        // release must perform at most ONE teardown queue idle.
        const char* persistent_env=std::getenv(
            "OGL_VULKAN_PERSISTENT_WSI_TEST_X11");
        const bool test_persistent=persistent_env &&
            std::strcmp(persistent_env,"1")==0;
        bool persistent_ok=true,persistent_resized=true;
        if (test_persistent) {
            setenv("OGL_VULKAN_EXCLUSIVE_PERSISTENT_CLEAR_PROBE","1",1);
            XResizeWindow(display,window,640,360);
            XSync(display,False);
            const bool attached_persistent=window_attach(
                display,static_cast<uint64_t>(window),640,360)==1;
            bool frames_ok=attached_persistent &&
                persistent_builds()==0 && persistent_idles()==0;
            const auto front_matches=[&](uint32_t i,int cx,int cy) {
                const bool warm=(i%2)!=0;
                for (unsigned attempt=0;attempt<120;++attempt) {
                    XSync(display,False);
                    XImage* img=XGetImage(display,window,cx,cy,1,1,
                                         AllPlanes,ZPixmap);
                    if (img) {
                        const unsigned long pixel=XGetPixel(img,0,0);
                        auto channel=[pixel](unsigned long mask) {
                            if (!mask) return -1;
                            unsigned shift=0;
                            while (!(mask&1UL)) {mask>>=1; ++shift;}
                            return static_cast<int>(
                                (((pixel>>shift)&mask)*255UL+mask/2)/mask);
                        };
                        const bool match=
                            std::abs(channel(img->red_mask)-(warm?166:20))<=5 &&
                            std::abs(channel(img->green_mask)-(warm?89:54))<=5 &&
                            std::abs(channel(img->blue_mask)-(warm?31:115))<=5;
                        XDestroyImage(img);
                        if (match) return true;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(3));
                }
                return false;
            };
            for (uint32_t i=0;i<16 && frames_ok;++i) {
                frames_ok=persistent_clear(1)==1 &&
                    persistent_count()==i+1 &&
                    persistent_builds()==1 &&
                    persistent_releases()==0 &&
                    persistent_idles()==0 &&
                    front_matches(i,320,180);
            }
            frames_ok=frames_ok && persistent_clear(1)==0 &&
                persistent_count()==16 && persistent_builds()==1 &&
                persistent_idles()==0;
            window_detach();
            persistent_ok=frames_ok && persistent_count()==0 &&
                persistent_releases()==1 && persistent_idles()==1;
            XResizeWindow(display,window,480,300);
            XSync(display,False);
            const bool resized_attach=window_attach(
                display,static_cast<uint64_t>(window),480,300)==1;
            const bool resized_draw=resized_attach &&
                persistent_clear(1)==1 &&
                persistent_count()==1 &&
                persistent_builds()==2 &&
                front_matches(0,240,150);
            window_detach();
            persistent_resized=resized_draw &&
                persistent_releases()==2 && persistent_idles()==2 &&
                persistent_count()==0;
            unsetenv("OGL_VULKAN_EXCLUSIVE_PERSISTENT_CLEAR_PROBE");
            std::fprintf(stdout,
                "P05 persistent X11 WSI: frames=%d resized=%d builds=%u releases=%u teardown_idle=%u\n",
                persistent_ok?1:0,persistent_resized?1:0,
                persistent_builds(),persistent_releases(),persistent_idles());
        }
        // Persistent indexed WORLD + Depth GPU QA: identical projected
        // foreground/background triangles with depth buffers PER IMAGE.
        // A real camera motion happens without destroying the Vulkan
        // pipeline or rebuilding the GPU mesh. No vkQueueWaitIdle per frame.
        const char* persistent_world_env=std::getenv(
            "OGL_VULKAN_PERSISTENT_WORLD_TEST_X11");
        const bool test_persistent_world=persistent_world_env &&
            std::strcmp(persistent_world_env,"1")==0;
        bool persistent_world_ok=true,persistent_world_moved=true;
        bool persistent_world_immutable=true,persistent_world_resize=true;
        if (test_persistent_world) {
            setenv("OGL_VULKAN_WORLD_MESH_UPLOAD","1",1);
            setenv("OGL_VULKAN_WORLD_CAMERA_PROBE","1",1);
            setenv("OGL_VULKAN_EXCLUSIVE_RASTER_PROBE","1",1);
            setenv("OGL_VULKAN_EXCLUSIVE_WORLD_PROBE","1",1);
            setenv("OGL_VULKAN_EXCLUSIVE_DEPTH_PROBE","1",1);
            setenv("OGL_VULKAN_EXCLUSIVE_PERSISTENT_CLEAR_PROBE","1",1);
            setenv("OGL_VULKAN_EXCLUSIVE_PERSISTENT_WORLD_PROBE","1",1);
            const ogl_rtx_triangle front{{12.5f,18.f,28.4f,
                12.5f,22.f,28.4f,12.5f,20.f,31.6f}};
            const ogl_rtx_triangle back{{15.f,16.f,26.8f,
                15.f,24.f,26.8f,15.f,20.f,33.2f}};
            const ogl_rtx_triangle mesh[2]{front,back};
            ogl_rtx_frame_state camera{};
            camera.struct_size=sizeof(camera);
            camera.frame_version=OGL_RTX_FRAME_VERSION;
            camera.width=512;camera.height=384;
            camera.camera_position[0]=10.f;
            camera.camera_position[1]=20.f;
            camera.camera_position[2]=30.f;
            camera.camera_forward[0]=1.f;
            camera.camera_right[1]=1.f;
            camera.camera_up[2]=1.f;
            camera.sun_direction[2]=1.f;
            camera.vertical_fov_radians=1.57079632679f;
            camera.aspect_ratio=4.f/3.f;
            camera.near_clip_m=2.f;camera.far_clip_m=10.f;
            const bool inputs=world_upload(mesh,2)==1 &&
                world_gpu_count()==2 && camera_ready()==0 &&
                world_camera(&camera)==1 && live_inputs()==1;
            XResizeWindow(display,window,512,384);
            XSync(display,False);
            const bool attach_world=window_attach(display,
                static_cast<uint64_t>(window),512,384)==1 &&
                aspect_matches()==1;
            const uint32_t starts=persistent_builds();
            const uint32_t idles_before=persistent_idles();
            auto matches=[&](int x,int y,int red,int green,int blue){
                for (unsigned attempt=0;attempt<120;++attempt) {
                    XSync(display,False);
                    XImage* img=XGetImage(display,window,x,y,1,1,
                                          AllPlanes,ZPixmap);
                    if (img) {
                        const unsigned long pixel=XGetPixel(img,0,0);
                        auto channel=[pixel](unsigned long mask) {
                            if (!mask) return -1;
                            unsigned shift=0;
                            while (!(mask&1UL)) {mask>>=1; ++shift;}
                            return static_cast<int>(
                                (((pixel>>shift)&mask)*255UL+mask/2)/mask);
                        };
                        const bool ok=
                            std::abs(channel(img->red_mask)-red)<=5 &&
                            std::abs(channel(img->green_mask)-green)<=5 &&
                            std::abs(channel(img->blue_mask)-blue)<=5;
                        XDestroyImage(img);
                        if (ok) return true;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(3));
                }
                return false;
            };
            bool frames=inputs && attach_world;
            for (uint32_t frame=0;frame<16 && frames;++frame) {
                if (frame==8) {
                    camera.camera_position[1]=32.f;
                    persistent_world_moved=world_camera(&camera)==1 &&
                        world_gpu_count()==2 && live_inputs()==1;
                    frames=frames && persistent_world_moved;
                }
                if (frame==12) {
                    camera.camera_position[1]=20.f;
                    frames=frames && world_camera(&camera)==1 &&
                        live_inputs()==1;
                }
                frames=frames && persistent_clear(1)==1 &&
                    persistent_count()==frame+1 &&
                    persistent_builds()==starts+1 &&
                    persistent_idles()==idles_before;
                if (!frames) break;
                const bool clear_warm=(frame%2)==1;
                const bool moved=frame>=8 && frame<12;
                const int r=moved ? (clear_warm?166:20) : 237;
                const int g=moved ? (clear_warm?89:54) : 48;
                const int b=moved ? (clear_warm?31:115) : 38;
                frames=matches(256,192,r,g,b) &&
                    matches(26,26,clear_warm?166:20,
                        clear_warm?89:54,clear_warm?31:115);
                if (frame==0) {
                    // Drawing submissions still refer to these VkBuffers.
                    // Both mutation and region-clear must be rejected.
                    persistent_world_immutable=world_upload(mesh,2)==0 &&
                        world_upload(nullptr,0)==0 && world_gpu_count()==2;
                    frames=frames && persistent_world_immutable;
                }
            }
            persistent_world_ok=frames && persistent_clear(1)==0 &&
                persistent_count()==16;
            window_detach();
            persistent_world_ok=persistent_world_ok &&
                persistent_idles()==idles_before+1 &&
                persistent_count()==0;
            const bool cleared=world_upload(nullptr,0)==1 &&
                world_gpu_count()==0 && camera_ready()==0;
            // Rebuild a different aspect only after matching new camera.
            XResizeWindow(display,window,640,480);
            XSync(display,False);
            const bool attach_again=window_attach(display,
                static_cast<uint64_t>(window),640,480)==1;
            camera.width=640;camera.height=480;
            const bool inputs_again=world_upload(mesh,2)==1 &&
                world_camera(&camera)==1 && aspect_matches()==1;
            const uint32_t build_before=persistent_builds();
            persistent_world_resize=cleared && attach_again && inputs_again &&
                persistent_clear(1)==1 &&
                persistent_builds()==build_before+1 &&
                matches(320,240,237,48,38);
            window_detach();
            persistent_world_resize=persistent_world_resize &&
                world_upload(nullptr,0)==1 && live_inputs()==0;
            unsetenv("OGL_VULKAN_EXCLUSIVE_PERSISTENT_WORLD_PROBE");
            unsetenv("OGL_VULKAN_EXCLUSIVE_PERSISTENT_CLEAR_PROBE");
            unsetenv("OGL_VULKAN_EXCLUSIVE_DEPTH_PROBE");
            unsetenv("OGL_VULKAN_EXCLUSIVE_WORLD_PROBE");
            unsetenv("OGL_VULKAN_EXCLUSIVE_RASTER_PROBE");
            unsetenv("OGL_VULKAN_WORLD_CAMERA_PROBE");
            unsetenv("OGL_VULKAN_WORLD_MESH_UPLOAD");
            std::fprintf(stdout,
                "P05 persistent Vulkan WORLD depth: frames=%d moved=%d guarded=%d resize=%d\n",
                persistent_world_ok?1:0,persistent_world_moved?1:0,
                persistent_world_immutable?1:0,persistent_world_resize?1:0);
        }
        // P05 scene revision acceptance: the GPU completes old mesh
        // submissions using TWO bounded VkFence waits, without queue-idle.
        // During a single persistent VkSwapchain lifetime the screen
        // transitions RED -> GREEN -> empty -> RED -> GREEN. The empty
        // region refuses a draw until a new mesh AND camera are submitted.
        const char* rotation_env=std::getenv(
            "OGL_VULKAN_PERSISTENT_MESH_ROTATION_TEST_X11");
        const bool test_rotation=rotation_env &&
            std::strcmp(rotation_env,"1")==0;
        bool rotation_ok=true,rotation_invalid=true;
        bool rotation_dedup=true;
        bool rotation_clear=true,rotation_no_idle=true;
        if (test_rotation) {
            for (const char* flag:{
                 "OGL_VULKAN_WORLD_MESH_UPLOAD",
                 "OGL_VULKAN_WORLD_CAMERA_PROBE",
                 "OGL_VULKAN_EXCLUSIVE_RASTER_PROBE",
                 "OGL_VULKAN_EXCLUSIVE_WORLD_PROBE",
                 "OGL_VULKAN_EXCLUSIVE_DEPTH_PROBE",
                 "OGL_VULKAN_EXCLUSIVE_PERSISTENT_CLEAR_PROBE",
                 "OGL_VULKAN_EXCLUSIVE_PERSISTENT_WORLD_PROBE"})
                setenv(flag,"1",1);
            const ogl_rtx_triangle near{{
                12.5f,18.f,28.4f,12.5f,22.f,28.4f,12.5f,20.f,31.6f
            }};
            const ogl_rtx_triangle far{{
                15.f,16.f,26.8f,15.f,24.f,26.8f,15.f,20.f,33.2f
            }};
            const ogl_rtx_triangle pair[2]{near,far};
            ogl_rtx_frame_state camera{};
            camera.struct_size=sizeof(camera);
            camera.frame_version=OGL_RTX_FRAME_VERSION;
            camera.width=512;camera.height=384;
            camera.camera_position[0]=10.f;
            camera.camera_position[1]=20.f;
            camera.camera_position[2]=30.f;
            camera.camera_forward[0]=1.f;
            camera.camera_right[1]=1.f;
            camera.camera_up[2]=1.f;
            camera.sun_direction[2]=1.f;
            camera.vertical_fov_radians=1.57079632679f;
            camera.aspect_ratio=4.f/3.f;
            camera.near_clip_m=2.f;camera.far_clip_m=10.f;
            XResizeWindow(display,window,512,384);
            XSync(display,False);
            setenv("OGL_VULKAN_WORLD_MESH_DEDUP","1",1);
            const uint64_t dedup0=mesh_dedup_skips();
            const uint64_t gen0=mesh_generation();
            const uint32_t wait0=mesh_retire_waits();
            const uint32_t idle0=persistent_idles();
            const uint32_t builds0=persistent_builds();
            rotation_ok=world_upload(pair,2)==1 &&
                mesh_generation()==gen0+1 &&
                world_camera(&camera)==1 &&
                window_attach(display,static_cast<uint64_t>(window),
                              512,384)==1 && aspect_matches()==1;
            auto pixel_is=[&](int red,int green,int blue) {
                for (unsigned tries=0;tries<120;++tries) {
                    XSync(display,False);
                    XImage* img=XGetImage(display,window,256,192,
                        1,1,AllPlanes,ZPixmap);
                    if (img) {
                        const unsigned long pixel=XGetPixel(img,0,0);
                        auto channel=[pixel](unsigned long mask) {
                            if (!mask) return -1;
                            unsigned shift=0;
                            while (!(mask&1UL)) {mask>>=1;++shift;}
                            return static_cast<int>(
                                (((pixel>>shift)&mask)*255UL+mask/2)/mask);
                        };
                        const bool ok=
                            std::abs(channel(img->red_mask)-red)<=5 &&
                            std::abs(channel(img->green_mask)-green)<=5 &&
                            std::abs(channel(img->blue_mask)-blue)<=5;
                        XDestroyImage(img);
                        if (ok) return true;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(3));
                }
                return false;
            };
            // Submit in BOTH persistent GPU frame slots before the mesh
            // transition. They can remain in-flight simultaneously.
            rotation_ok=rotation_ok && persistent_clear(1)==1 &&
                persistent_count()==1 && pixel_is(237,48,38) &&
                persistent_clear(1)==1 &&
                persistent_count()==2 && pixel_is(237,48,38);
            // A byte-for-byte duplicate region snapshot must not retire
            // any in-flight buffers, rebind a camera, or advance revision.
            rotation_dedup=world_upload(pair,2)==1 &&
                mesh_dedup_skips()==dedup0+1 &&
                mesh_generation()==gen0+1 &&
                mesh_retire_waits()==wait0 &&
                camera_mesh_generation()==mesh_generation() &&
                live_inputs()==1 && persistent_count()==2;
            rotation_ok=rotation_ok && rotation_dedup;
            // Old default behavior still rejects mutation when explicit
            // mesh-generation retirement permission is not supplied.
            rotation_ok=rotation_ok &&
                world_upload(&far,1)==0 && world_gpu_count()==2 &&
                mesh_generation()==gen0+1 &&
                mesh_retire_waits()==wait0;
            setenv("OGL_VULKAN_PERSISTENT_MESH_ROTATION_PROBE","1",1);
            // Rejected bad coordinates leave the old region and its
            // counters untouched, WITHOUT waiting on a GPU fence.
            auto invalid=far;
            invalid.position[0]=std::numeric_limits<float>::quiet_NaN();
            rotation_invalid=world_upload(&invalid,1)==0 &&
                mesh_dedup_skips()==dedup0+1 &&
                mesh_generation()==gen0+1 &&
                mesh_retire_waits()==wait0 &&
                world_gpu_count()==2;
            rotation_ok=rotation_ok && rotation_invalid &&
                world_upload(&far,1)==1 &&
                mesh_generation()==gen0+2 &&
                mesh_retire_waits()==wait0+1 &&
                mesh_retire_failures()==0 && camera_ready()==0 &&
                persistent_clear(1)==0 &&
                world_camera(&camera)==1 &&
                persistent_clear(1)==1 && persistent_count()==3 &&
                pixel_is(20,199,41);
            // Region loss: draw must stop until a new generation and pose.
            rotation_clear=world_upload(nullptr,0)==1 &&
                world_gpu_count()==0 &&
                mesh_generation()==gen0+3 &&
                mesh_retire_waits()==wait0+2 &&
                camera_ready()==0 &&
                persistent_clear(1)==0;
            rotation_clear=rotation_clear && world_upload(&near,1)==1 &&
                mesh_generation()==gen0+4 &&
                camera_ready()==0 &&
                world_camera(&camera)==1 &&
                persistent_clear(1)==1 && persistent_count()==4 &&
                pixel_is(237,48,38);
            rotation_ok=rotation_ok && rotation_clear;
            // Fourth generation was still submitted. Another bounded
            // wait-all retires its buffer before the next mesh appears.
            rotation_ok=rotation_ok && world_upload(&far,1)==1 &&
                mesh_generation()==gen0+5 &&
                mesh_retire_waits()==wait0+3 &&
                world_camera(&camera)==1 &&
                persistent_clear(1)==1 && persistent_count()==5 &&
                pixel_is(20,199,41);
            rotation_no_idle=persistent_builds()==builds0+1 &&
                persistent_idles()==idle0 &&
                mesh_retire_failures()==0;
            window_detach();
            rotation_no_idle=rotation_no_idle &&
                persistent_idles()==idle0+1 &&
                persistent_count()==0;
            rotation_ok=rotation_ok && rotation_no_idle &&
                world_upload(nullptr,0)==1 &&
                mesh_generation()==gen0+6 &&
                world_gpu_count()==0 &&
                camera_ready()==0;
            for (const char* flag:{
                 "OGL_VULKAN_WORLD_MESH_DEDUP",
                 "OGL_VULKAN_PERSISTENT_MESH_ROTATION_PROBE",
                 "OGL_VULKAN_EXCLUSIVE_PERSISTENT_WORLD_PROBE",
                 "OGL_VULKAN_EXCLUSIVE_PERSISTENT_CLEAR_PROBE",
                 "OGL_VULKAN_EXCLUSIVE_DEPTH_PROBE",
                 "OGL_VULKAN_EXCLUSIVE_WORLD_PROBE",
                 "OGL_VULKAN_EXCLUSIVE_RASTER_PROBE",
                 "OGL_VULKAN_WORLD_CAMERA_PROBE",
                 "OGL_VULKAN_WORLD_MESH_UPLOAD"})
                unsetenv(flag);
            std::fprintf(stdout,
                "P05 live GPU mesh generations: rotation=%d invalid=%d "
                "dedup=%d clear=%d no-frame-idle=%d generations=%llu waits=%u\n",
                rotation_ok?1:0,rotation_invalid?1:0,rotation_dedup?1:0,
                rotation_clear?1:0,rotation_no_idle?1:0,
                static_cast<unsigned long long>(mesh_generation()),
                mesh_retire_waits());
        }
        // P05: a GENUINE region change is distinct from a generic mesh
        // rotation. It must invalidate the old camera before retiring
        // submissions from BOTH frame slots, even when rotation permission
        // is absent. First new-region mesh may upload without that permission.
        const char* barrier_env=std::getenv(
            "OGL_VULKAN_LIVE_REGION_BARRIER_TEST_X11");
        const bool test_region_barrier=barrier_env &&
            std::strcmp(barrier_env,"1")==0;
        bool barrier_ok=true,barrier_fenced=true,barrier_new_region=true;
        if (test_region_barrier) {
            for (const char* flag:{
                 "OGL_VULKAN_WORLD_MESH_UPLOAD",
                 "OGL_VULKAN_WORLD_CAMERA_PROBE",
                 "OGL_VULKAN_EXCLUSIVE_RASTER_PROBE",
                 "OGL_VULKAN_EXCLUSIVE_WORLD_PROBE",
                 "OGL_VULKAN_EXCLUSIVE_DEPTH_PROBE",
                 "OGL_VULKAN_EXCLUSIVE_PERSISTENT_CLEAR_PROBE",
                 "OGL_VULKAN_EXCLUSIVE_PERSISTENT_WORLD_PROBE"})
                setenv(flag,"1",1);
            unsetenv("OGL_VULKAN_PERSISTENT_MESH_ROTATION_PROBE");
            const ogl_rtx_triangle near{{
                12.5f,18.f,28.4f,12.5f,22.f,28.4f,12.5f,20.f,31.6f
            }};
            const ogl_rtx_triangle far{{
                15.f,16.f,26.8f,15.f,24.f,26.8f,15.f,20.f,33.2f
            }};
            ogl_rtx_frame_state camera{};
            camera.struct_size=sizeof(camera);
            camera.frame_version=OGL_RTX_FRAME_VERSION;
            camera.width=512;camera.height=384;
            camera.camera_position[0]=10.f;
            camera.camera_position[1]=20.f;
            camera.camera_position[2]=30.f;
            camera.camera_forward[0]=1.f;
            camera.camera_right[1]=1.f;
            camera.camera_up[2]=1.f;
            camera.sun_direction[2]=1.f;
            camera.vertical_fov_radians=1.57079632679f;
            camera.aspect_ratio=4.f/3.f;
            camera.near_clip_m=2.f;camera.far_clip_m=10.f;
            XResizeWindow(display,window,512,384);
            XSync(display,False);
            setenv("OGL_VULKAN_OFFSCREEN_FRAME_PROBE","1",1);
            setenv("OGL_VULKAN_WORLD_OFFSCREEN_RASTER","1",1);
            const uint64_t offscreen_start=offscreen_world_frames();
            const uint32_t offscreen_build0=offscreen_cache_builds();
            const uint32_t offscreen_reuse0=offscreen_cache_reuses();
            const uint32_t offscreen_release0=offscreen_cache_releases();
            const uint32_t denied_no_mesh=offscreen_world();
            const uint64_t gen0=mesh_generation();
            const uint64_t accepts0=region_barrier_accepts();
            const uint32_t retire0=mesh_retire_waits();
            const uint32_t idle0=persistent_idles();
            barrier_ok=denied_no_mesh==0 &&
                world_upload(&near,1)==1 &&
                world_camera(&camera)==1 &&
                // Four explicit flags. No world GPU render on absent
                // permission; no metrics increment on rejection.
                (unsetenv("OGL_VULKAN_WORLD_OFFSCREEN_RASTER")==0) &&
                offscreen_world()==0 &&
                offscreen_world_frames()==offscreen_start &&
                (setenv("OGL_VULKAN_WORLD_OFFSCREEN_RASTER","1",1)==0) &&
                // Indexed 32x32 Vulkan World-XYZ draw + Z test BEFORE
                // creating or attaching any native X11 VkSwapchain.
                offscreen_world()==1 &&
                offscreen_world_frames()==offscreen_start+1 &&
                offscreen_cache_builds()==offscreen_build0+1 &&
                offscreen_cache_reuses()==offscreen_reuse0 &&
                offscreen_cache_releases()==offscreen_release0 &&
                offscreen_world_pixels()>0 &&
                offscreen_world_pixels()<1024 &&
                // The camera moves completely away from scene: the GPU
                // still renders, but observed visible pixels drop to ZERO.
                ((camera.camera_position[1]=50.f),true) &&
                world_camera(&camera)==1 &&
                offscreen_world()==1 &&
                offscreen_world_frames()==offscreen_start+2 &&
                offscreen_cache_builds()==offscreen_build0+1 &&
                offscreen_cache_reuses()==offscreen_reuse0+1 &&
                offscreen_world_pixels()==0 &&
                ((camera.camera_position[1]=20.f),true) &&
                world_camera(&camera)==1 &&
                window_attach(display,static_cast<uint64_t>(window),
                              512,384)==1 && live_inputs()==1 &&
                persistent_clear(1)==1 &&
                persistent_clear(1)==1 && persistent_count()==2;
            auto center_matches=[&](int red,int green,int blue) {
                for (unsigned attempt=0;attempt<120;++attempt) {
                    XSync(display,False);
                    XImage* img=XGetImage(display,window,256,192,
                        1,1,AllPlanes,ZPixmap);
                    if (img) {
                        const unsigned long pixel=XGetPixel(img,0,0);
                        auto channel=[pixel](unsigned long mask) {
                            if (!mask) return -1;
                            unsigned shift=0;
                            while (!(mask&1UL)) {mask>>=1;++shift;}
                            return static_cast<int>(
                                (((pixel>>shift)&mask)*255UL+mask/2)/mask);
                        };
                        const bool match=
                            std::abs(channel(img->red_mask)-red)<=5 &&
                            std::abs(channel(img->green_mask)-green)<=5 &&
                            std::abs(channel(img->blue_mask)-blue)<=5;
                        XDestroyImage(img);
                        if (match) return true;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(3));
                }
                return false;
            };
            barrier_ok=barrier_ok && center_matches(237,48,38) &&
                // Without opt-in there is NO region barrier and the
                // strict default running-mesh guard still applies.
                region_barrier()==0 &&
                world_upload(&far,1)==0 &&
                world_gpu_count()==1 && live_inputs()==1;
            setenv("OGL_VULKAN_LIVE_REGION_BARRIER","1",1);
            barrier_ok=barrier_ok && region_barrier()==1 &&
                // A cleared region must refuse an offscreen GPU draw,
                // just as it refuses a visible VkQueuePresentKHR frame.
                offscreen_world()==0 &&
                offscreen_world_frames()==offscreen_start+2 &&
                region_barrier_accepts()==accepts0+1 &&
                region_barrier_failures()==0 &&
                mesh_generation()==gen0+2 &&
                world_gpu_count()==0 &&
                camera_ready()==0 && live_inputs()==0 &&
                camera_mesh_generation()==0 &&
                persistent_clear(1)==0 &&
                persistent_count()==2;
            barrier_fenced=mesh_retire_waits()==retire0+1 &&
                persistent_idles()==idle0 &&
                mesh_retire_failures()==0;
            barrier_ok=barrier_ok && barrier_fenced;
            // A new camera ALONE cannot make an empty region renderable.
            barrier_new_region=world_camera(&camera)==1 &&
                live_inputs()==0 && persistent_clear(1)==0 &&
                // New REGION mesh may upload even without general
                // in-flight MESH ROTATION permission.
                world_upload(&far,1)==1 &&
                mesh_generation()==gen0+3 &&
                camera_ready()==0 && live_inputs()==0 &&
                world_camera(&camera)==1 && live_inputs()==1 &&
                offscreen_world()==1 &&
                offscreen_world_frames()==offscreen_start+3 &&
                offscreen_cache_builds()==offscreen_build0+1 &&
                offscreen_cache_reuses()==offscreen_reuse0+2 &&
                offscreen_cache_releases()==offscreen_release0 &&
                offscreen_world_pixels()>0 &&
                persistent_clear(1)==1 &&
                persistent_count()==3 &&
                center_matches(20,199,41);
            barrier_new_region=barrier_new_region &&
                region_barrier()==1 &&
                region_barrier_accepts()==accepts0+2 &&
                mesh_generation()==gen0+4 &&
                world_gpu_count()==0 && live_inputs()==0 &&
                persistent_clear(1)==0 &&
                mesh_retire_waits()==retire0+2 &&
                persistent_idles()==idle0 &&
                region_barrier_failures()==0;
            barrier_ok=barrier_ok && barrier_new_region &&
                offscreen_world()==0 &&
                offscreen_world_frames()==offscreen_start+3;
            window_detach();
            barrier_ok=barrier_ok && persistent_count()==0 &&
                persistent_idles()==idle0+1;
            barrier_ok=barrier_ok &&
                offscreen_cache_release()==1 &&
                offscreen_cache_releases()==offscreen_release0+1 &&
                offscreen_cache_release()==0 &&
                offscreen_cache_builds()==offscreen_build0+1 &&
                offscreen_cache_reuses()==offscreen_reuse0+2 &&
                offscreen_world_frames()==offscreen_start+3;
            // Rebuild after explicit release, using the newly valid
            // scene/camera; no stale pipeline or stale GPU image survives.
            barrier_ok=barrier_ok && world_upload(&near,1)==1 &&
                world_camera(&camera)==1 && offscreen_world()==1 &&
                offscreen_world_frames()==offscreen_start+4 &&
                offscreen_world_pixels()>0 &&
                offscreen_cache_builds()==offscreen_build0+2 &&
                offscreen_cache_reuses()==offscreen_reuse0+2 &&
                offscreen_cache_release()==1 &&
                offscreen_cache_releases()==offscreen_release0+2 &&
                offscreen_cache_release()==0;
            for (const char* flag:{
                 "OGL_VULKAN_OFFSCREEN_FRAME_PROBE",
                 "OGL_VULKAN_WORLD_OFFSCREEN_RASTER",
                 "OGL_VULKAN_LIVE_REGION_BARRIER",
                 "OGL_VULKAN_EXCLUSIVE_PERSISTENT_WORLD_PROBE",
                 "OGL_VULKAN_EXCLUSIVE_PERSISTENT_CLEAR_PROBE",
                 "OGL_VULKAN_EXCLUSIVE_DEPTH_PROBE",
                 "OGL_VULKAN_EXCLUSIVE_WORLD_PROBE",
                 "OGL_VULKAN_EXCLUSIVE_RASTER_PROBE",
                 "OGL_VULKAN_WORLD_CAMERA_PROBE",
                 "OGL_VULKAN_WORLD_MESH_UPLOAD"})
                unsetenv(flag);
            std::fprintf(stdout,
                "P05 private Vulkan WORLD offscreen indexed/depth: "
                "frames=%llu visible_pixels=%u builds=%u reuses=%u releases=%u "
                "no_swapchain=1\n",
                static_cast<unsigned long long>(offscreen_world_frames()),
                offscreen_world_pixels(),
                offscreen_cache_builds(),offscreen_cache_reuses(),
                offscreen_cache_releases());
            std::fprintf(stdout,
                "P05 live region barrier GPU: valid=%d fenced=%d "
                "new_region=%d accepts=%llu waits=%u failures=%llu\n",
                barrier_ok?1:0,barrier_fenced?1:0,
                barrier_new_region?1:0,
                static_cast<unsigned long long>(region_barrier_accepts()),
                mesh_retire_waits(),
                static_cast<unsigned long long>(region_barrier_failures()));
        }
        stop();
        const uint32_t stopped_count=exclusive_count();
        if (test_world && camera_ready()!=0) world_bad=false;
        XDestroyWindow(display,window);
        XCloseDisplay(display);
        if (started!=0 || device!=1 || attached!=1 || swapchain!=1 ||
            without_owner!=0 || without_flag!=0 ||
            presented!=1 || duplicate!=0 ||
            first_count!=1 || after_first_detach!=0 ||
            resized!=1 || resized_ready!=1 ||
            resized_presented!=1 || resized_duplicate!=0 ||
            second_count!=1 || after_second_detach!=0 ||
            frame_loop_attached!=1 || frame_loop_ready!=1 ||
            !frame_sequence_valid || completed!=16 ||
            overflow!=0 || frame_count!=16 || without_loop!=0 ||
            reset_count!=0 || restored_attach!=1 ||
            restored_frame!=1 || restored_repeat!=0 ||
            restored_count!=1 || restored_reset!=0 ||
            stopped_count!=0 ||
            (test_region_barrier && (!barrier_ok || !barrier_fenced ||
                !barrier_new_region)) ||
            (test_rotation && (!rotation_ok || !rotation_invalid ||
                !rotation_dedup || !rotation_clear || !rotation_no_idle)) ||
            (test_persistent_world && (!persistent_world_ok ||
                !persistent_world_moved || !persistent_world_immutable ||
                !persistent_world_resize)) ||
            (test_persistent && (!persistent_ok || !persistent_resized)) ||
            (test_world_depth && (!world_depth_near ||
                                  !world_depth_control ||
                                  !world_depth_moved ||
                                  !world_depth_resized ||
                                  !world_depth_aspect ||
                                  !world_depth_invalid ||
                                  !world_depth_cleared)) ||
            (test_depth && (!depth_positive || !depth_resized ||
                            !depth_cleared)) ||
            (test_world && (!world_positive || !world_shifted || !world_bad)) ||
            (raster_test && (raster_attached!=1 || raster_uploaded!=1 ||
                             raster_count!=1 || raster_without_mesh!=0 ||
                             raster_presented!=1 || !raster_pixels ||
                             !raster_cleared))) {
            std::fprintf(stderr,
                "Native Vulkan present FAILED: start=%d gpu=%d attach=%d "
                "swapchain=%d no-owner=%u no-flag=%u present=%u repeat=%u "
                "resized=%d ready=%d second=%u second-repeat=%u "
                "frame-loop=%d/%d, completed=%u, overflow=%u, count=%u, "
                "reset=%u, restored=%d/%u/%u/%u, stopped=%u\n",
                started,device,attached,swapchain,without_owner,
                without_flag,presented,duplicate,resized,resized_ready,
                resized_presented,resized_duplicate,
                frame_loop_attached,frame_loop_ready,completed,overflow,
                frame_count,reset_count,restored_attach,restored_frame,
                restored_repeat,restored_count,stopped_count);
            return 12;
        }
        std::puts("P05 native X11 swapchain: 16 consecutive GPU presents, bounded frames and 2 resizes PASS");
        if (raster_test)
            std::puts("P05 real indexed triangle graphics raster in X11 swapchain: PASS");
        if (test_world)
            std::puts("P05 camera-projected world XYZ indexed triangle in Vulkan: PASS");
        if (test_depth)
            std::puts("P05 native GPU depth occlusion (near over far) at 2 X11 extents: PASS");
        if (test_world_depth)
            std::puts("P05 native GPU world XYZ projected depth, anti-depth control, camera move/resize and stale reject: PASS");
        if (test_persistent)
            std::puts("P05 persistent Vulkan swapchain CLEAR: 16 GPU presents, one initialization, teardown-only queue idle, resize/recreate PASS");
        if (test_persistent_world)
            std::puts("P05 persistent Vulkan WORLD depth: 16 indexed GPU frames, camera motion, immutable mesh, resize/recreate PASS");
        if (test_rotation)
            std::puts("P05 GPU mesh-generation transitions: red/green/empty/red/green, bounded GPU fences, no per-frame queue idle PASS");
        if (test_region_barrier)
            std::puts("P05 real region barrier: two-slot GPU retirement, stale-camera rejection, new region mesh and Depth GPU present PASS");
    }
#else
    // Portable CI may omit X11 headers. Strict X11 acceptance mode must
    // FAIL, not silently pass, if that native test cannot be compiled.
    const char* native_flag=std::getenv("OGL_VULKAN_PRESENT_TEST_X11");
    if (native_flag && native_flag[0]=='1' && native_flag[1]=='\0') {
        std::fputs("Native X11 present QA unavailable in this build\n",stderr);
        return 12;
    }
#endif
    dlclose(library);
    std::puts("OGL RTX camera/sun frame bridge API: PASS (raytraced pixels inactive)");
}

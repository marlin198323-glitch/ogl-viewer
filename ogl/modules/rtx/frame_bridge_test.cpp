#include "ogl/ogl_rtx_api.h"
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <limits>
#include <vector>
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
    const auto world_sync = reinterpret_cast<uint32_t(*)()>(
        dlsym(library, "ogl_vulkan_world_submit_sync_probe"));
    const auto offscreen_frame = reinterpret_cast<uint32_t(*)()>(
        dlsym(library, "ogl_vulkan_world_render_offscreen_probe"));
    const auto world_upload = reinterpret_cast<int(*)(const ogl_rtx_triangle*,uint32_t)>(
        dlsym(library, "ogl_vulkan_world_upload_triangles"));
    const auto world_gpu_count = reinterpret_cast<uint32_t(*)()>(
        dlsym(library, "ogl_vulkan_world_gpu_triangle_count"));
    const auto world_camera = reinterpret_cast<int(*)(const ogl_rtx_frame_state*)>(
        dlsym(library,"ogl_vulkan_world_set_projection_camera"));
    const auto camera_ready = reinterpret_cast<uint32_t(*)()>(
        dlsym(library,"ogl_vulkan_world_projection_camera_ready"));
    const auto live_inputs = reinterpret_cast<uint32_t(*)()>(
        dlsym(library,"ogl_vulkan_world_live_inputs_ready"));
    const auto camera_updates = reinterpret_cast<uint64_t(*)()>(
        dlsym(library,"ogl_vulkan_world_camera_updates"));
    const auto mesh_uploads = reinterpret_cast<uint64_t(*)()>(
        dlsym(library,"ogl_vulkan_world_mesh_uploads"));
    const auto window_attach = reinterpret_cast<int(*)(void*,uint64_t,uint32_t,uint32_t)>(
        dlsym(library, "ogl_vulkan_world_attach_x11_window"));
    const auto window_ready = reinterpret_cast<int(*)()>(
        dlsym(library, "ogl_vulkan_world_swapchain_probe_ready"));
    const auto window_detach = reinterpret_cast<void(*)()>(
        dlsym(library, "ogl_vulkan_world_detach_window"));
    assert(world_camera && camera_ready && live_inputs &&
           camera_updates && mesh_uploads);
    assert(camera_ready()==0 && live_inputs()==0 &&
           camera_updates()==0 && mesh_uploads()==0);
    assert(world_camera(nullptr)==0);
    assert(exclusive_present && exclusive_count &&
           exclusive_count()==0 && exclusive_present(1)==0);
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
    assert(world_sync()==(ready ? 2u : 0u));
    unsetenv("OGL_VULKAN_FRAME_SYNC_PROBE");
    assert(world_sync()==0);
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
               camera_updates()==1);
        live_frame.aspect_ratio=2.f;
        assert(world_camera(&live_frame)==0); // invalid input clears camera
        assert(camera_ready()==0 && live_inputs()==0 &&
               camera_updates()==1);
        live_frame.aspect_ratio=4.f/3.f;
        assert(world_camera(&live_frame)==1 && live_inputs()==1 &&
               camera_updates()==2);
        assert(world_upload(&world_triangle,1)==1); // new scene invalidates pose
        assert(camera_ready()==0 && live_inputs()==0 && mesh_uploads()==2);
        assert(world_camera(&live_frame)==1 && live_inputs()==1 &&
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
               mesh_uploads()==3); // no stale camera across mesh revisions
        assert(world_upload(all.data(),8193)==0);
        assert(world_gpu_count()==8192);
        assert(world_upload(nullptr,0)==1);
        assert(world_gpu_count()==0); // region-clear frees GPU buffers
        assert(camera_ready()==0 && live_inputs()==0 &&
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
    assert(camera_ready()==0 && live_inputs()==0 &&
           camera_updates()==0 && mesh_uploads()==0);
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
                exclusive_present(1)==1 &&
                read_rgb(320,240,237,48,38) &&
                read_rgb(30,30,20,54,115);
            window_detach();

            // A failed camera update kills the previous projection, even
            // though the GPU meshes and their memory still exist.
            projection.vertical_fov_radians=0.f;
            world_depth_invalid=world_camera(&projection)==0 &&
                                camera_ready()==0 && live_inputs()==0;
            const bool rejected_attach=window_attach(
                display,static_cast<uint64_t>(window),640,480)==1;
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
                "moved=%d resized=%d invalid=%d clear=%d\n",
                world_depth_near?1:0,world_depth_control?1:0,
                world_depth_moved?1:0,world_depth_resized?1:0,
                world_depth_invalid?1:0,world_depth_cleared?1:0);
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
            (test_world_depth && (!world_depth_near ||
                                  !world_depth_control ||
                                  !world_depth_moved ||
                                  !world_depth_resized ||
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

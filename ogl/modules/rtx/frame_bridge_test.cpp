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
    const auto window_attach = reinterpret_cast<int(*)(void*,uint64_t,uint32_t,uint32_t)>(
        dlsym(library, "ogl_vulkan_world_attach_x11_window"));
    const auto window_ready = reinterpret_cast<int(*)()>(
        dlsym(library, "ogl_vulkan_world_swapchain_probe_ready"));
    const auto window_detach = reinterpret_cast<void(*)()>(
        dlsym(library, "ogl_vulkan_world_detach_window"));
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
        auto bad=world_triangle;
        bad.position[0]=std::numeric_limits<float>::quiet_NaN();
        assert(world_upload(&bad,1)==0);
        assert(world_gpu_count()==1); // rejected upload never clears old scene
        std::vector<ogl_rtx_triangle> all(8192,world_triangle);
        assert(world_upload(all.data(),8192)==1);
        assert(world_gpu_count()==8192);
        assert(world_upload(all.data(),8193)==0);
        assert(world_gpu_count()==8192);
        assert(world_upload(nullptr,0)==1);
        assert(world_gpu_count()==0); // region-clear frees GPU buffers
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
        stop();
        const uint32_t stopped_count=exclusive_count();
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
            stopped_count!=0) {
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

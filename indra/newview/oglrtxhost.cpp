#include "llviewerprecompiledheaders.h"
#include "oglrtxhost.h"
#include "llglheaders.h"
#include "llgl.h"
#include "llviewercontrol.h"
#include "../../ogl/modules/include/ogl/ogl_rtx_api.h"

// Only the opt-in Linux/X11 WSI probe borrows the already-existing SDL
// viewer window. Windows and macOS continue their unchanged OpenGL path.
#if LL_LINUX && LL_X11 && LL_SDL2
#include "llwindowsdl2.h"
#include "llviewerwindow.h"
#endif
#include "../../ogl/modules/include/ogl/ogl_world_shadow_policy.h"

#if LL_LINUX || LL_WINDOWS
#include <cstring>
#include <cstdlib>
#include <chrono>
#include <string>
#if LL_WINDOWS
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#else
#  include <dlfcn.h>
#endif

namespace {
#if LL_WINDOWS
using OGLModuleHandle = HMODULE;
OGLModuleHandle ogl_load_module() {
    // Explicit, trusted path next to the executable (no current-dir search).
    wchar_t buffer[MAX_PATH]{};
    const DWORD n = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    if (!n || n >= MAX_PATH) return nullptr;
    std::wstring exe(buffer, n);
    const auto slash = exe.find_last_of(L"\\/");
    if (slash == std::wstring::npos) return nullptr;
    const std::wstring dll = exe.substr(0, slash + 1) + L"ogl_rtx.dll";
    return LoadLibraryExW(dll.c_str(), nullptr,
                          LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
}
void* ogl_resolve_module(OGLModuleHandle module, const char* symbol) {
    return reinterpret_cast<void*>(GetProcAddress(module, symbol));
}
void ogl_unload_module(OGLModuleHandle module) { FreeLibrary(module); }
#else
using OGLModuleHandle = void*;
OGLModuleHandle ogl_load_module() { return dlopen("./lib/ogl_rtx.so", RTLD_NOW | RTLD_LOCAL); }
void* ogl_resolve_module(OGLModuleHandle module, const char* symbol) {
    return dlsym(module, symbol);
}
void ogl_unload_module(OGLModuleHandle module) { dlclose(module); }
#endif
OGLModuleHandle rtx_module_handle = nullptr;
bool rtx_hardware_ready = false;
bool rtx_rendering_enabled = false;
using ogl_rtx_submit_frame_fn = int (*)(const ogl_rtx_frame_state*);
ogl_rtx_submit_frame_fn rtx_submit_frame = nullptr;
using ogl_rtx_submit_geometry_fn = int (*)(const ogl_rtx_triangle*, uint32_t);
ogl_rtx_submit_geometry_fn rtx_submit_geometry = nullptr;
// P05/4 Vulkan graphics data is independent of RTX capability; the optional
// staging ABI uses the same triangulated OpenSim scene format.
using ogl_world_upload_fn = int (*)(const ogl_rtx_triangle*, uint32_t);
ogl_world_upload_fn world_upload_geometry = nullptr;
using ogl_world_camera_fn = int (*)(const ogl_rtx_frame_state*);
ogl_world_camera_fn world_set_camera = nullptr;
using ogl_world_input_ready_fn = uint32_t (*)();
ogl_world_input_ready_fn world_input_ready = nullptr;
using ogl_world_generation_fn = std::uint64_t (*)();
ogl_world_generation_fn world_mesh_generation = nullptr;
ogl_world_generation_fn world_camera_mesh_generation = nullptr;
using ogl_world_region_barrier_fn = int (*)();
ogl_world_region_barrier_fn world_invalidate_region = nullptr;
using ogl_world_offscreen_fn = uint32_t (*)();
ogl_world_offscreen_fn world_render_offscreen_scene = nullptr;
using ogl_world_offscreen_count_fn = std::uint64_t (*)();
ogl_world_offscreen_count_fn world_offscreen_scene_frames = nullptr;
using ogl_world_offscreen_pixels_fn = uint32_t (*)();
ogl_world_offscreen_pixels_fn world_offscreen_scene_pixels = nullptr;
ogl_world_offscreen_pixels_fn world_offscreen_scene_width = nullptr;
ogl_world_offscreen_pixels_fn world_offscreen_scene_height = nullptr;
ogl_world_offscreen_pixels_fn world_offscreen_cache_builds = nullptr;
ogl_world_offscreen_pixels_fn world_offscreen_cache_reuses = nullptr;
ogl_world_offscreen_pixels_fn world_offscreen_cache_releases = nullptr;
std::uint64_t world_offscreen_scene_failures = 0;
// The real Firestorm camera is sampled BEFORE the deferred scene scan.
// Retain it only after successful module validation, and invalidate it on
// every real region change. It may be rebound after accepted geometry,
// but MUST never be paired across region epochs.
ogl_rtx_frame_state last_live_world_camera{};
bool last_live_world_camera_valid = false;
std::uint64_t live_world_region_epoch = 0;
std::uint64_t live_world_rebinds = 0;
// In-world deferred-source receipt. Never obtained from the generic
// triangle ABI or a pre-login/synthetic GPU probe. Receipt IDs are local
// monotonic numbers only (no region UUID, name, account or coordinates).
bool live_deferred_capture_call = false;
bool live_deferred_capture_valid = false;
std::uint64_t live_deferred_receipts = 0;
std::uint64_t live_deferred_receipt_epoch = 0;
std::uint64_t live_deferred_mesh_generation = 0;
std::uint64_t live_deferred_gpu_frames = 0;
std::uint64_t live_deferred_camera_gpu_frames = 0;
// Camera-only samples are separately opted-in and rate-limited.
std::uint64_t live_world_camera_tick_calls = 0;
std::uint64_t live_world_camera_tick_attempts = 0;
std::uint64_t live_world_camera_tick_unchanged = 0;
ogl_rtx_frame_state live_world_last_offscreen_pose{};
bool live_world_last_offscreen_pose_valid = false;
std::uint64_t live_world_last_offscreen_mesh_generation = 0;
std::uint64_t live_world_last_offscreen_region_epoch = 0;
bool world_camera_live_logged = false;
bool world_inputs_live_logged = false;
using ogl_world_device_ready_fn = int (*)();
ogl_world_device_ready_fn world_device_ready = nullptr;
using ogl_track_object_fn = int (*)(const uint8_t*,const ogl_rtx_triangle*,
                                     uint32_t,const float*);
using ogl_track_pose_fn = int (*)(const uint8_t*,const float*);
using ogl_forget_object_fn = int (*)(const uint8_t*);
ogl_track_object_fn rtx_track_object = nullptr;
ogl_track_pose_fn rtx_track_pose = nullptr;
ogl_forget_object_fn rtx_forget_object = nullptr;
using ogl_rtx_triangles_fn = uint32_t (*)();
ogl_rtx_triangles_fn rtx_received_triangles = nullptr;
using ogl_rtx_copy_shadow_debug_fn = int (*)(ogl_rtx_shadow_debug*);
ogl_rtx_copy_shadow_debug_fn rtx_copy_shadow_debug = nullptr;
using ogl_rtx_copy_world_shadow_fn = int (*)(ogl_rtx_world_shadow_sample*);
ogl_rtx_copy_world_shadow_fn rtx_copy_world_shadow = nullptr;
using ogl_rtx_submit_dense_fn = int (*)(const ogl_rtx_dense_depth*);
using ogl_rtx_copy_dense_fn = int (*)(ogl_rtx_dense_shadow*);
ogl_rtx_submit_dense_fn rtx_submit_dense = nullptr;
ogl_rtx_copy_dense_fn rtx_copy_dense = nullptr;
using ogl_rtx_submit_depth_fn = int (*)(const ogl_rtx_depth_debug*);
ogl_rtx_submit_depth_fn rtx_submit_depth = nullptr;
ogl_module_stop_fn rtx_stop = nullptr;

#if LL_LINUX && LL_X11 && LL_SDL2
using OglWorldWindowAttach = int (*)(void*,uint64_t,uint32_t,uint32_t);
using OglWorldWindowDetach = void (*)();
OglWorldWindowAttach rtx_wsi_attach = nullptr;
OglWorldWindowDetach rtx_wsi_detach = nullptr;
bool rtx_wsi_persistent = false;
std::uint64_t rtx_wsi_xid = 0;
int rtx_wsi_width = 0;
int rtx_wsi_height = 0;

void ogl_x11_wsi_will_destroy() {
    // Called synchronously BEFORE SDL stops video or destroys/replaces XID.
    if (rtx_wsi_detach) rtx_wsi_detach();
    rtx_wsi_xid = 0;
    rtx_wsi_width = 0;
    rtx_wsi_height = 0;
}

void ogl_x11_wsi_refresh() {
    if (!rtx_wsi_persistent || !rtx_wsi_attach || !rtx_wsi_detach ||
        !rtx_module_handle) return;
    void* display=static_cast<void*>(LLWindowSDL::get_SDL_Display());
    std::uint64_t window=static_cast<std::uint64_t>(
        LLWindowSDL::get_SDL_XWindowID());
    int width=0,height=0;
    if (!display || !window ||
        !LLWindowSDL::getOglNativeWindowSize(&width,&height)) {
        ogl_x11_wsi_will_destroy();
        return;
    }
    if (rtx_wsi_xid==window && rtx_wsi_width==width &&
        rtx_wsi_height==height) return;
    // The attach API destroys old VkSwapchainKHR/VkSurfaceKHR before
    // recreating; it never acquires/presents Vulkan images over OpenGL.
    const bool ready=rtx_wsi_attach(display,window,
        static_cast<uint32_t>(width),static_cast<uint32_t>(height))!=0;
    rtx_wsi_xid=window;
    rtx_wsi_width=width;
    rtx_wsi_height=height;
    LL_INFOS("OGLRenderer")
        << "P05 persistent X11 WSI resize/window refresh: "
        << width << "x" << height
        << ", swapchain=" << (ready ? "ready" : "unavailable")
        << "; OpenGL remains present owner." << LL_ENDL;
}
#endif

void host_log(int severity, const char *message) {
    LL_INFOS("OGLRTX") << "module severity " << severity << ": "
                       << (message ? message : "") << LL_ENDL;
}
}  // namespace
#endif

namespace {
bool ogl_world_started = false;
bool ogl_world_frame_reported = false;
}

// Startup diagnostics are separate from the optional RTX plugin. Reporting
// hardware capability does NOT switch the world renderer or load a swapchain.
void ogl_report_world_backend_startup() {
    if (ogl_world_started) return;
    ogl_world_started = true;
    const std::string requested = gSavedSettings.getString("OGLWorldRequestedEngine");
    const bool asked_for_vulkan = requested == "vulkan";
    OGLWorldCapabilities caps{};
    caps.gl_context_ready = gGLManager.mInited;
    caps.vulkan_device_usable = ogl_rtx_hardware_supported();
    caps.vulkan_world_pipeline_complete = false; // P05 NOT implemented
    // Native GL/OS report and configured texture budget are distinct;
    // a VRAM override is not proof of a bigger physical device.
    caps.detected_vram_mb = gGLManager.mVRAMDetected > 0 ?
                            static_cast<std::uint32_t>(gGLManager.mVRAMDetected) : 0;
    const auto effective_vram_mb = gGLManager.mVRAM;
    const auto texture_budget_mb = gSavedSettings.getU32("RenderMaxVRAMBudget");
    caps.gl_version_major = static_cast<std::uint32_t>(gGLManager.mGLVersion);
    const auto decision = ogl_choose_world_backend(
        asked_for_vulkan ? OGLWorldBackend::Vulkan : OGLWorldBackend::OpenGL,
        caps);
    const char* fallback = decision.fallback == OGLWorldFallback::NoFallback ?
        "none" : decision.fallback == OGLWorldFallback::WorldPipelineUnavailable ?
        "world-pipeline-unavailable" : "device-unavailable";
    LL_INFOS("OGLRenderer")
        << "World backend: OpenGL (actual), requested=" << requested
        << ", last_working=" << gSavedSettings.getString("OGLWorldLastWorkingEngine")
        << ", GL_version=" << gGLManager.mGLVersionString
        << ", GL_driver=" << gGLManager.mDriverVersionVendorString
        << ", detected_VRAM_MB=" << caps.detected_vram_mb
        << ", effective_VRAM_MB=" << effective_vram_mb
        << ", configured_texture_budget_MB=" << texture_budget_mb
        << ", RTX_diagnostic_capability=" << (caps.vulkan_device_usable ? "yes":"no")
        << ", Vulkan_world_pipeline=unavailable"
        << ", fallback=" << fallback
        << ". Keeping existing GL world/GUI/present." << LL_ENDL;
}

void ogl_record_world_frame_presented() {
#if LL_LINUX && LL_X11 && LL_SDL2
    // O(1) native ID/size comparison after actual GL swap; only allocate a
    // new Vulkan WSI swapchain on resize/recreation, never per GL frame.
    ogl_x11_wsi_refresh();
#endif
    // Never write settings per frame or from a diagnostic ray-query result.
    if (ogl_world_frame_reported || !gGLManager.mInited) return;
    ogl_world_frame_reported = true;
    if (gSavedSettings.getString("OGLWorldLastWorkingEngine") != "opengl")
        gSavedSettings.setString("OGLWorldLastWorkingEngine", "opengl");
    LL_INFOS("OGLRenderer") << "OpenGL world frame presented; last working engine saved."
                            << LL_ENDL;
}

void ogl_report_world_backend_shutdown() {
    if (!ogl_world_started) return;
    ogl_world_started = false;
    ogl_world_frame_reported = false;
    LL_INFOS("OGLRenderer")
        << "World renderer shutdown: existing OpenGL context/LLPipeline owned by viewer."
        << LL_ENDL;
}

void ogl_init_rtx_module() {
#if LL_LINUX || LL_WINDOWS
    // Load only the bundled, trusted RTX module. Arbitrary paths and user-
    // supplied native modules are intentionally unsupported at this stage.
    rtx_module_handle = ogl_load_module();
    if (!rtx_module_handle) {
        LL_INFOS("OGLRTX") << "RTX capability module unavailable; keeping OpenGL."
                            << LL_ENDL;
        return;
    }
    auto info = reinterpret_cast<ogl_module_info_fn>(
        ogl_resolve_module(rtx_module_handle, "ogl_module_get_info"));
    auto start = reinterpret_cast<ogl_module_start_fn>(
        ogl_resolve_module(rtx_module_handle, "ogl_module_start"));
    rtx_stop = reinterpret_cast<ogl_module_stop_fn>(
        ogl_resolve_module(rtx_module_handle, "ogl_module_stop"));
    auto get_status = reinterpret_cast<const ogl_rtx_status *(*)()>(
        ogl_resolve_module(rtx_module_handle, "ogl_rtx_get_status"));

    if (!info || !start || !rtx_stop || !get_status) {
        LL_WARNS("OGLRTX") << "RTX module ABI incomplete; keeping OpenGL." << LL_ENDL;
        ogl_shutdown_rtx_module();
        return;
    }

    const ogl_module_info *identity = info();
    if (!identity || identity->struct_size < sizeof(ogl_module_info) ||
        identity->abi_version != OGL_MODULE_ABI_VERSION || !identity->id ||
        std::strcmp(identity->id, "de.nexverse.ogl.rtx") != 0) {
        LL_WARNS("OGLRTX") << "RTX module identity mismatch; keeping OpenGL." << LL_ENDL;
        ogl_shutdown_rtx_module();
        return;
    }

    rtx_submit_frame = reinterpret_cast<ogl_rtx_submit_frame_fn>(
        ogl_resolve_module(rtx_module_handle, "ogl_rtx_submit_frame"));
    rtx_submit_geometry = reinterpret_cast<ogl_rtx_submit_geometry_fn>(
        ogl_resolve_module(rtx_module_handle, "ogl_rtx_submit_geometry"));
    world_upload_geometry = reinterpret_cast<ogl_world_upload_fn>(
        ogl_resolve_module(rtx_module_handle, "ogl_vulkan_world_upload_triangles"));
    world_set_camera = reinterpret_cast<ogl_world_camera_fn>(
        ogl_resolve_module(rtx_module_handle,"ogl_vulkan_world_set_projection_camera"));
    world_input_ready = reinterpret_cast<ogl_world_input_ready_fn>(
        ogl_resolve_module(rtx_module_handle,"ogl_vulkan_world_live_inputs_ready"));
    world_mesh_generation = reinterpret_cast<ogl_world_generation_fn>(
        ogl_resolve_module(rtx_module_handle,"ogl_vulkan_world_mesh_generation"));
    world_camera_mesh_generation = reinterpret_cast<ogl_world_generation_fn>(
        ogl_resolve_module(rtx_module_handle,"ogl_vulkan_world_camera_mesh_generation"));
    world_invalidate_region = reinterpret_cast<ogl_world_region_barrier_fn>(
        ogl_resolve_module(rtx_module_handle,"ogl_vulkan_world_invalidate_region"));
    world_render_offscreen_scene = reinterpret_cast<ogl_world_offscreen_fn>(
        ogl_resolve_module(rtx_module_handle,"ogl_vulkan_world_render_offscreen_scene"));
    world_offscreen_scene_frames = reinterpret_cast<ogl_world_offscreen_count_fn>(
        ogl_resolve_module(rtx_module_handle,"ogl_vulkan_world_offscreen_scene_frames"));
    world_offscreen_scene_pixels = reinterpret_cast<ogl_world_offscreen_pixels_fn>(
        ogl_resolve_module(rtx_module_handle,"ogl_vulkan_world_offscreen_scene_pixels"));
    world_offscreen_scene_width = reinterpret_cast<ogl_world_offscreen_pixels_fn>(
        ogl_resolve_module(rtx_module_handle,"ogl_vulkan_world_offscreen_scene_width"));
    world_offscreen_scene_height = reinterpret_cast<ogl_world_offscreen_pixels_fn>(
        ogl_resolve_module(rtx_module_handle,"ogl_vulkan_world_offscreen_scene_height"));
    world_offscreen_cache_builds = reinterpret_cast<ogl_world_offscreen_pixels_fn>(
        ogl_resolve_module(rtx_module_handle,"ogl_vulkan_world_offscreen_cache_builds"));
    world_offscreen_cache_reuses = reinterpret_cast<ogl_world_offscreen_pixels_fn>(
        ogl_resolve_module(rtx_module_handle,"ogl_vulkan_world_offscreen_cache_reuses"));
    world_offscreen_cache_releases = reinterpret_cast<ogl_world_offscreen_pixels_fn>(
        ogl_resolve_module(rtx_module_handle,"ogl_vulkan_world_offscreen_cache_releases"));
    world_offscreen_scene_failures=0;
    last_live_world_camera={};
    last_live_world_camera_valid=false;
    live_world_region_epoch=0;
    live_world_rebinds=0;
    live_deferred_capture_call=false;
    live_deferred_capture_valid=false;
    live_deferred_receipts=0;
    live_deferred_receipt_epoch=0;
    live_deferred_mesh_generation=0;
    live_deferred_gpu_frames=0;
    live_deferred_camera_gpu_frames=0;
    live_world_camera_tick_calls=0;
    live_world_camera_tick_attempts=0;
    live_world_camera_tick_unchanged=0;
    live_world_last_offscreen_pose={};
    live_world_last_offscreen_pose_valid=false;
    live_world_last_offscreen_mesh_generation=0;
    live_world_last_offscreen_region_epoch=0;
    world_camera_live_logged=false;
    world_inputs_live_logged=false;
    world_device_ready = reinterpret_cast<ogl_world_device_ready_fn>(
        ogl_resolve_module(rtx_module_handle, "ogl_vulkan_world_device_bootstrapped"));
    rtx_track_object = reinterpret_cast<ogl_track_object_fn>(
        ogl_resolve_module(rtx_module_handle,"ogl_rtx_track_object"));
    rtx_track_pose = reinterpret_cast<ogl_track_pose_fn>(
        ogl_resolve_module(rtx_module_handle,"ogl_rtx_track_transform"));
    rtx_forget_object = reinterpret_cast<ogl_forget_object_fn>(
        ogl_resolve_module(rtx_module_handle,"ogl_rtx_forget_object"));
    rtx_received_triangles = reinterpret_cast<ogl_rtx_triangles_fn>(
        ogl_resolve_module(rtx_module_handle, "ogl_rtx_received_triangles"));
    rtx_copy_shadow_debug = reinterpret_cast<ogl_rtx_copy_shadow_debug_fn>(
        ogl_resolve_module(rtx_module_handle, "ogl_rtx_copy_shadow_debug"));
    rtx_copy_world_shadow = reinterpret_cast<ogl_rtx_copy_world_shadow_fn>(
        ogl_resolve_module(rtx_module_handle,"ogl_rtx_copy_world_shadow_sample"));
    rtx_submit_dense=reinterpret_cast<ogl_rtx_submit_dense_fn>(
        ogl_resolve_module(rtx_module_handle,"ogl_rtx_submit_dense_depth"));
    rtx_copy_dense=reinterpret_cast<ogl_rtx_copy_dense_fn>(
        ogl_resolve_module(rtx_module_handle,"ogl_rtx_copy_dense_shadow"));
    rtx_submit_depth = reinterpret_cast<ogl_rtx_submit_depth_fn>(
        ogl_resolve_module(rtx_module_handle, "ogl_rtx_submit_depth_debug"));

    const ogl_host_api api{sizeof(ogl_host_api), OGL_MODULE_ABI_VERSION, host_log};
    if (start(&api) != 0) {
        LL_WARNS("OGLRTX") << "RTX capability initialization rejected; keeping OpenGL."
                            << LL_ENDL;
        ogl_shutdown_rtx_module();
        return;
    }
    // All provenance receipts are scoped to this module/Viewer lifecycle;
    // the local log parser never merges them with previous sessions.
    const char* evidence=std::getenv("OGL_VULKAN_LIVE_WORLD_OFFSCREEN");
    if (evidence && std::strcmp(evidence,"1")==0)
        LL_INFOS("OGLRenderer")
            << "P05 deferred evidence session: begin=1; "
               "source receipts require in-world Firestorm deferred scan."
            << LL_ENDL;
    const ogl_rtx_status *status = get_status();
    if (status && status->struct_size >= sizeof(ogl_rtx_status) &&
        status->status_version == OGL_RTX_STATUS_VERSION) {
        rtx_hardware_ready = status->hardware_ready != 0;
        rtx_rendering_enabled = status->rendering_active != 0;
        LL_INFOS("OGLRTX") << "GPU " << status->gpu_name
            << " hardware_ready=" << status->hardware_ready
            << " rendering_active=" << status->rendering_active
            << ". OpenGL remains the active renderer." << LL_ENDL;
    }
    // Developer-only Vulkan graphics queue validation. This submits two
    // command buffers with completion fences but no world pixels, image
    // acquisition or presentation. It must not affect the GL renderer.
    const char* sync_flag=std::getenv("OGL_VULKAN_FRAME_SYNC_PROBE");
    if (sync_flag && std::strcmp(sync_flag,"1")==0) {
        auto sync_probe=reinterpret_cast<std::uint32_t(*)()>(
            ogl_resolve_module(rtx_module_handle, "ogl_vulkan_world_submit_sync_probe"));
        const std::uint32_t submissions=sync_probe ? sync_probe() : 0;
        LL_INFOS("OGLRenderer")
            << "P05 Vulkan graphics queue sync probe: "
            << submissions << "/2 real fence-backed submits; "
               "no image present, OpenGL world unchanged." << LL_ENDL;
    }
    // P05 offscreen render-pass validation is separate from the world
    // renderer. It verifies a GPU-produced color frame by readback once on
    // explicit developer request, without acquiring/presenting SDL pixels.
    const char* frame_flag=std::getenv("OGL_VULKAN_OFFSCREEN_FRAME_PROBE");
    if (frame_flag && std::strcmp(frame_flag,"1")==0) {
        auto frame_probe=reinterpret_cast<std::uint32_t(*)()>(
            ogl_resolve_module(rtx_module_handle,
                               "ogl_vulkan_world_render_offscreen_probe"));
        const std::uint32_t rendered=frame_probe ? frame_probe() : 0;
        LL_INFOS("OGLRenderer")
            << "P05 real Vulkan offscreen GPU frame: "
            << rendered << "/1 verified 32x32 images; "
               "no swapchain present, OpenGL unchanged." << LL_ENDL;
    }
#if LL_LINUX && LL_X11 && LL_SDL2
    // Never attempt to create a Vulkan SDL window beside the real GL window.
    // Explicit developer-only probe uses the actual SDL/X11 window to
    // validate a VkSurfaceKHR + VkSwapchainKHR without presenting images.
    // A temporary WSI attachment is released before any GL frame or window
    // resize; this cannot take over rendering or outlive the borrowed XID.
    const char* surface_flag=std::getenv("OGL_VULKAN_WINDOW_SURFACE");
    const char* device_flag=std::getenv("OGL_VULKAN_WORLD_BOOTSTRAP");
    if (surface_flag && std::strcmp(surface_flag,"1")==0 &&
        device_flag && std::strcmp(device_flag,"1")==0) {
        const char* persistent_flag=std::getenv("OGL_VULKAN_WINDOW_PERSISTENT");
        const bool persistent=persistent_flag &&
            std::strcmp(persistent_flag,"1")==0;
        auto attach = reinterpret_cast<int(*)(void*,uint64_t,uint32_t,uint32_t)>(
            ogl_resolve_module(rtx_module_handle, "ogl_vulkan_world_attach_x11_window"));
        auto ready = reinterpret_cast<int(*)()>(
            ogl_resolve_module(rtx_module_handle, "ogl_vulkan_world_swapchain_probe_ready"));
        auto detach = reinterpret_cast<void(*)()>(
            ogl_resolve_module(rtx_module_handle, "ogl_vulkan_world_detach_window"));
        bool success=false;
        if (attach && ready && detach && gViewerWindow) {
            LLCoordWindow size;
            if (gViewerWindow->getWindow()->getSize(&size) &&
                size.mX>0 && size.mY>0) {
                auto display=LLWindowSDL::get_SDL_Display();
                const auto window=LLWindowSDL::get_SDL_XWindowID();
                if (display && window) {
                    success=attach(static_cast<void*>(display),
                        static_cast<uint64_t>(window),
                        static_cast<uint32_t>(size.mX),
                        static_cast<uint32_t>(size.mY))!=0 && ready()!=0;
                }
            }
            if (success && persistent) {
                rtx_wsi_attach=attach;
                rtx_wsi_detach=detach;
                rtx_wsi_persistent=true;
                rtx_wsi_xid=static_cast<std::uint64_t>(
                    LLWindowSDL::get_SDL_XWindowID());
                // Use the real SDL window size, not cached GL surface pixels.
                if (!LLWindowSDL::getOglNativeWindowSize(
                    &rtx_wsi_width,&rtx_wsi_height)) {
                    rtx_wsi_width=0;
                    rtx_wsi_height=0;
                }
                LLWindowSDL::setOglWindowWillDestroyCallback(
                    ogl_x11_wsi_will_destroy);
                LLWindowSDL::setOglWindowResizedCallback(
                    ogl_x11_wsi_refresh);
            } else {
                detach(); // default one-shot probe must leave GL alone.
            }
        }
        if (success && persistent && rtx_wsi_persistent) {
            LL_INFOS("OGLRenderer")
                << "P05 persistent X11 Vulkan surface/swapchain attached; "
                   "no acquire/present, OpenGL still owns displayed frames."
                << LL_ENDL;
        } else {
            LL_INFOS("OGLRenderer") <<
                (success
                    ? "P05 real SDL/X11 Vulkan surface and swapchain probe passed; "
                      "handles released before OpenGL rendering, no Vulkan present."
                    : "P05 real SDL/X11 Vulkan surface/swapchain unavailable; "
                      "OpenGL remains unchanged.") << LL_ENDL;
        }
    }
#endif
#endif
}

void ogl_shutdown_rtx_module() {
#if LL_LINUX && LL_X11 && LL_SDL2
    LLWindowSDL::setOglWindowResizedCallback(nullptr);
    LLWindowSDL::setOglWindowWillDestroyCallback(nullptr);
    ogl_x11_wsi_will_destroy();
    rtx_wsi_attach=nullptr;
    rtx_wsi_detach=nullptr;
    rtx_wsi_persistent=false;
#endif
#if LL_LINUX || LL_WINDOWS
    if (rtx_module_handle) {
        if (rtx_stop) rtx_stop();
        ogl_unload_module(rtx_module_handle);
    }
    rtx_stop = nullptr;
    rtx_module_handle = nullptr;
    rtx_hardware_ready = false;
    rtx_rendering_enabled = false;
    rtx_submit_frame = nullptr;
    rtx_submit_geometry = nullptr;
    world_upload_geometry = nullptr;
    world_set_camera = nullptr;
    world_input_ready = nullptr;
    world_mesh_generation = nullptr;
    world_camera_mesh_generation = nullptr;
    world_invalidate_region = nullptr;
    world_render_offscreen_scene = nullptr;
    world_offscreen_scene_frames = nullptr;
    world_offscreen_scene_pixels = nullptr;
    world_offscreen_scene_width = nullptr;
    world_offscreen_scene_height = nullptr;
    world_offscreen_cache_builds = nullptr;
    world_offscreen_cache_reuses = nullptr;
    world_offscreen_cache_releases = nullptr;
    world_offscreen_scene_failures=0;
    last_live_world_camera={};
    last_live_world_camera_valid=false;
    live_world_region_epoch=0;
    live_world_rebinds=0;
    live_deferred_capture_call=false;
    live_deferred_capture_valid=false;
    live_deferred_receipts=0;
    live_deferred_receipt_epoch=0;
    live_deferred_mesh_generation=0;
    live_deferred_gpu_frames=0;
    live_deferred_camera_gpu_frames=0;
    live_world_camera_tick_calls=0;
    live_world_camera_tick_attempts=0;
    live_world_camera_tick_unchanged=0;
    live_world_last_offscreen_pose={};
    live_world_last_offscreen_pose_valid=false;
    live_world_last_offscreen_mesh_generation=0;
    live_world_last_offscreen_region_epoch=0;
    world_camera_live_logged=false;
    world_inputs_live_logged=false;
    world_device_ready = nullptr;
    rtx_track_object = nullptr;
    rtx_track_pose = nullptr;
    rtx_forget_object = nullptr;
    rtx_received_triangles = nullptr;
    rtx_copy_shadow_debug = nullptr;
    rtx_copy_world_shadow = nullptr;
    rtx_submit_dense=nullptr;
    rtx_copy_dense=nullptr;
    rtx_submit_depth = nullptr;
#endif
}

bool ogl_rtx_hardware_supported() {
#if LL_LINUX || LL_WINDOWS
    return rtx_hardware_ready;
#else
    return false;
#endif
}

bool ogl_rtx_rendering_active() {
#if LL_LINUX || LL_WINDOWS
    return rtx_rendering_enabled;
#else
    return false;
#endif
}

bool ogl_vulkan_world_mesh_staging_enabled() {
#if LL_LINUX || LL_WINDOWS
    // Only explicit developer sessions collect additional scene geometry;
    // do not make normal users pay the CPU mesh traversal cost.
    static const bool requested=[] {
        const char* bootstrap=std::getenv("OGL_VULKAN_WORLD_BOOTSTRAP");
        const char* upload=std::getenv("OGL_VULKAN_WORLD_MESH_UPLOAD");
        return bootstrap && upload &&
            std::strcmp(bootstrap,"1")==0 && std::strcmp(upload,"1")==0;
    }();
    return requested && world_upload_geometry && world_device_ready &&
        world_device_ready()==1;
#else
    return false;
#endif
}

// Private Vulkan OFFSCREEN GPU diagnostic while OpenGL owns the live
// window. Every external permission is opt-in; no GL framebuffer read
// and never a VkQueuePresentKHR or user/avatar/region image export.
bool ogl_live_offscreen_developer_enabled() {
#if LL_LINUX || LL_WINDOWS
    static const bool opted_in=[] {
        const char* live=std::getenv("OGL_VULKAN_LIVE_WORLD_OFFSCREEN");
        const char* base=std::getenv("OGL_VULKAN_OFFSCREEN_FRAME_PROBE");
        const char* raster=std::getenv("OGL_VULKAN_WORLD_OFFSCREEN_RASTER");
        return live && base && raster &&
            std::strcmp(live,"1")==0 && std::strcmp(base,"1")==0 &&
            std::strcmp(raster,"1")==0;
    }();
    return opted_in && world_render_offscreen_scene &&
        world_offscreen_scene_frames && world_offscreen_scene_pixels;
#else
    return false;
#endif
}

bool ogl_firestorm_offscreen_gpu_sample(const char* source) {
#if LL_LINUX || LL_WINDOWS
    if (!ogl_live_offscreen_developer_enabled() ||
        // Only actual Firestorm deferred world captures, never a generic
        // externally submitted or prelogin synthetic triangle batch.
        !live_deferred_capture_valid ||
        live_deferred_receipt_epoch!=live_world_region_epoch ||
        !world_mesh_generation ||
        live_deferred_mesh_generation!=world_mesh_generation() ||
        !world_input_ready || world_input_ready()!=1 ||
        !world_mesh_generation || !world_camera_mesh_generation ||
        world_mesh_generation()==0 ||
        world_camera_mesh_generation()!=world_mesh_generation())
        return false;
    const auto old_count=world_offscreen_scene_frames();
    const bool rendered=world_render_offscreen_scene()==1 &&
        world_offscreen_scene_frames()==old_count+1;
    if (!rendered) ++world_offscreen_scene_failures;
    else {
        ++live_deferred_gpu_frames;
        if (std::strcmp(source,"camera-tick")==0)
            ++live_deferred_camera_gpu_frames;
        // One receipt per completed GPU draw, not a claim of an
        // authenticated login; downstream report must also see STATE_STARTED.
        LL_INFOS("OGLRenderer")
            << "P05 deferred GPU receipt: epoch=" << live_world_region_epoch
            << " capture=" << live_deferred_receipts
            << " mesh_generation=" << live_deferred_mesh_generation
            << " source=" << source
            << " frames=" << live_deferred_gpu_frames
            << " camera_frames=" << live_deferred_camera_gpu_frames
            << " nonclear_pixels=" << world_offscreen_scene_pixels()
            << " width=" << (world_offscreen_scene_width ?
                world_offscreen_scene_width() : 32u)
            << " height=" << (world_offscreen_scene_height ?
                world_offscreen_scene_height() : 32u)
            << " status=completed; PRIVATE Vulkan image, not displayed."
            << LL_ENDL;
    }
    LL_INFOS("OGLRenderer")
        << "P05 real Firestorm world Vulkan OFFSCREEN GPU draw: "
        << (rendered ? "completed" : "unavailable")
        << " source=" << source
        << " frames=" << world_offscreen_scene_frames()
        << " nonclear_pixels=" << (rendered ?
            world_offscreen_scene_pixels() : 0)
        << " failures=" << world_offscreen_scene_failures
        << " width=" << (world_offscreen_scene_width ?
            world_offscreen_scene_width() : 32u)
        << " height=" << (world_offscreen_scene_height ?
            world_offscreen_scene_height() : 32u)
        << " cache_builds=" << (world_offscreen_cache_builds ?
            world_offscreen_cache_builds() : 0)
        << " cache_reuses=" << (world_offscreen_cache_reuses ?
            world_offscreen_cache_reuses() : 0)
        << " cache_releases=" << (world_offscreen_cache_releases ?
            world_offscreen_cache_releases() : 0)
        << "; bounded private VkImage, NO Vulkan present, "
           "OpenGL owns viewer window."
        << LL_ENDL;
    return rendered;
#else
    return false;
#endif
}

// The 32x32 GPU World shader consumes camera position/orientation/FOV,
// NOT the sun vector. Ignore unrelated sunlight changes to avoid expensive
// GPU diagnostics when the viewer camera has not actually moved.
bool ogl_same_world_camera_pose(const ogl_rtx_frame_state& a,
                                const ogl_rtx_frame_state& b) {
    return a.width==b.width && a.height==b.height &&
        a.vertical_fov_radians==b.vertical_fov_radians &&
        a.aspect_ratio==b.aspect_ratio &&
        a.near_clip_m==b.near_clip_m &&
        a.far_clip_m==b.far_clip_m &&
        std::memcmp(a.camera_position,b.camera_position,
                    sizeof(a.camera_position))==0 &&
        std::memcmp(a.camera_forward,b.camera_forward,
                    sizeof(a.camera_forward))==0 &&
        std::memcmp(a.camera_right,b.camera_right,
                    sizeof(a.camera_right))==0 &&
        std::memcmp(a.camera_up,b.camera_up,
                    sizeof(a.camera_up))==0;
}

// Must run AFTER the actual pipeline has checked the current region UUID.
// Does not upload a mesh, create a swapchain or own the OpenGL window.
// Every 120 eligible deferred frames, only when the validated pose CHANGED.
void ogl_rtx_after_region_camera_tick() {
#if LL_LINUX || LL_WINDOWS
    static const bool opted_in=[] {
        const char* tick=std::getenv("OGL_VULKAN_LIVE_CAMERA_TICK");
        return tick && std::strcmp(tick,"1")==0;
    }();
    if (!opted_in || !ogl_live_offscreen_developer_enabled() ||
        !last_live_world_camera_valid || !world_input_ready ||
        !world_mesh_generation || !world_camera_mesh_generation)
        return;
    ++live_world_camera_tick_calls;
    constexpr std::uint64_t camera_interval=120;
    if (!ogl_vulkan_camera_tick_due(live_world_camera_tick_calls))
        return;
    if (!world_input_ready() || world_mesh_generation()==0 ||
        world_camera_mesh_generation()!=world_mesh_generation())
        return;
    const auto revision=world_mesh_generation();
    if (live_world_last_offscreen_pose_valid &&
        live_world_last_offscreen_mesh_generation==revision &&
        live_world_last_offscreen_region_epoch==live_world_region_epoch &&
        ogl_same_world_camera_pose(live_world_last_offscreen_pose,
                                   last_live_world_camera)) {
        ++live_world_camera_tick_unchanged;
        return;
    }
    ++live_world_camera_tick_attempts;
    if (ogl_firestorm_offscreen_gpu_sample("camera-tick")) {
        live_world_last_offscreen_pose=last_live_world_camera;
        live_world_last_offscreen_pose_valid=true;
        live_world_last_offscreen_mesh_generation=revision;
        live_world_last_offscreen_region_epoch=live_world_region_epoch;
        if (live_world_camera_tick_attempts<=3 ||
            live_world_camera_tick_attempts%64==0)
            LL_INFOS("OGLRenderer")
                << "P05 live Vulkan camera-only GPU tick: "
                << "attempts=" << live_world_camera_tick_attempts
                << " unchanged=" << live_world_camera_tick_unchanged
                << " camera_interval=" << camera_interval
                << "; same mesh generation, OpenGL remains active."
                << LL_ENDL;
    }
#endif
}

// The region UUID is compared by the actual Firestorm pipeline. No UUID
// or position is exported to diagnostics; only invalidate the cached pose.
// Called AFTER the pipeline has detected a genuine OpenSim region change,
// NOT when a partial geometry scan returns zero candidates.
bool ogl_rtx_on_region_transition() {
#if LL_LINUX || LL_WINDOWS
    last_live_world_camera={};
    last_live_world_camera_valid=false;
    ++live_world_region_epoch;
    live_deferred_capture_call=false;
    live_deferred_capture_valid=false;
    live_deferred_receipt_epoch=0;
    live_deferred_mesh_generation=0;
    const char* provenance=std::getenv("OGL_VULKAN_LIVE_WORLD_OFFSCREEN");
    if (provenance && std::strcmp(provenance,"1")==0)
        LL_INFOS("OGLRenderer")
            << "P05 deferred region boundary: epoch=" << live_world_region_epoch
            << "; previous capture and GPU evidence invalidated." << LL_ENDL;
    live_world_camera_tick_calls=0;
    live_world_last_offscreen_pose={};
    live_world_last_offscreen_pose_valid=false;
    live_world_last_offscreen_mesh_generation=0;
    live_world_last_offscreen_region_epoch=0;
    world_camera_live_logged=false;
    world_inputs_live_logged=false;
    // New region may arrive while old Vulkan frames still reference
    // their VkBuffers. A dedicated opt-in barrier invalidates GPU camera
    // first, waits only pending submit fences, then releases old geometry.
    // No Vulkan present and no SDL window access in this callback.
    const char* enabled=std::getenv("OGL_VULKAN_LIVE_REGION_BARRIER");
    if (enabled && std::strcmp(enabled,"1")==0 &&
        world_invalidate_region && world_device_ready &&
        world_device_ready()==1) {
        const bool done=world_invalidate_region()==1;
        if (!done)
            LL_WARNS("OGLRenderer")
                << "P05 region transition GPU barrier unavailable; "
                   "Vulkan world input remains invalid, OpenGL continues."
                << LL_ENDL;
        return done;
    }
#endif
    return false;
}

void ogl_rtx_submit_camera_sun(unsigned width, unsigned height,
    const float* camera_position, const float* camera_forward,
    const float* camera_right, const float* camera_up,
    float vertical_fov_radians, float aspect_ratio,
    float near_clip_m, float far_clip_m,
    const float* sun_direction) {
#if LL_LINUX || LL_WINDOWS
    // The live OpenSim camera is captured by the existing deferred world
    // pass, independently of NVIDIA/RTX. Vulkan receives it only with
    // THREE explicit developer permissions; no WSI present occurs here.
    static const bool camera_developer_opt_in=[] {
        const char* device=std::getenv("OGL_VULKAN_WORLD_BOOTSTRAP");
        const char* mesh=std::getenv("OGL_VULKAN_WORLD_MESH_UPLOAD");
        const char* camera=std::getenv("OGL_VULKAN_WORLD_CAMERA_PROBE");
        return device && mesh && camera &&
            std::strcmp(device,"1")==0 && std::strcmp(mesh,"1")==0 &&
            std::strcmp(camera,"1")==0;
    }();
    const bool gpu_camera=camera_developer_opt_in && world_set_camera &&
        world_upload_geometry && world_device_ready &&
        world_device_ready()==1;
    const bool rt_camera=rtx_submit_frame && rtx_hardware_ready;
    if ((!gpu_camera && !rt_camera) ||
        !camera_position || !camera_forward || !camera_right ||
        !camera_up || !sun_direction) return;
    ogl_rtx_frame_state frame{};
    frame.struct_size = sizeof(frame);
    frame.frame_version = OGL_RTX_FRAME_VERSION;
    frame.width = width;
    frame.height = height;
    for (unsigned i=0; i<3; ++i) {
        frame.camera_position[i] = camera_position[i];
        frame.camera_forward[i] = camera_forward[i];
        frame.camera_right[i] = camera_right[i];
        frame.camera_up[i] = camera_up[i];
        frame.sun_direction[i] = sun_direction[i];
    }
    frame.vertical_fov_radians = vertical_fov_radians;
    frame.aspect_ratio = aspect_ratio;
    frame.near_clip_m = near_clip_m;
    frame.far_clip_m = far_clip_m;
    if (gpu_camera) {
        // Never reuse a prior pose if this frame was rejected. The region
        // callback invalidates the copy independently of GPU state.
        last_live_world_camera_valid=false;
    }
    const bool accepted_gpu_camera=gpu_camera &&
        world_set_camera(&frame)==1;
    if (gpu_camera && !accepted_gpu_camera)
        world_inputs_live_logged=false; // stale camera cannot remain "ready"
    if (accepted_gpu_camera) {
        last_live_world_camera=frame;
        last_live_world_camera_valid=true;
        // First real viewer-camera input, NOT a displayed Vulkan frame.
        if (!world_camera_live_logged) {
            world_camera_live_logged=true;
            LL_INFOS("OGLRenderer")
                << "P05 live Firestorm camera accepted by Vulkan world-matrix ABI; "
                   "CPU projection only, OpenGL remains present owner."
                << LL_ENDL;
        }
        // This requires a GPU scene uploaded before this camera frame,
        // so stale geometry after region changes is never considered ready.
        if (!world_inputs_live_logged && world_input_ready &&
            world_mesh_generation && world_camera_mesh_generation &&
            world_mesh_generation()>0 &&
            world_camera_mesh_generation()==world_mesh_generation() &&
            world_input_ready()==1) {
            world_inputs_live_logged=true;
            LL_INFOS("OGLRenderer")
                << "P05 live Firestorm camera + actual captured OpenSim mesh "
                   "paired as Vulkan GPU inputs; no Vulkan world frame presented."
                << LL_ENDL;
        }
    }
    // Existing RTX diagnostics remain conditional on RT-capable hardware.
    if (rt_camera) rtx_submit_frame(&frame);
#endif
}

void ogl_rtx_submit_world_triangles(const float *triangle_xyz, unsigned count) {
#if LL_LINUX || LL_WINDOWS
    if (count > OGL_RTX_MAX_SCENE_TRIANGLES || (count && !triangle_xyz)) return;
    // Existing scene ABI: exact, tightly packed XYZ floats per triangle.
    static_assert(sizeof(ogl_rtx_triangle) == 9 * sizeof(float), "RTX ABI padding mismatch");
    const auto* triangles=reinterpret_cast<const ogl_rtx_triangle*>(triangle_xyz);
    const char* staging=std::getenv("OGL_VULKAN_WORLD_MESH_UPLOAD");
    if (world_upload_geometry && staging && std::strcmp(staging,"1")==0) {
        const std::uint64_t before=world_mesh_generation ?
            world_mesh_generation() : 0u;
        const bool uploaded=world_upload_geometry(triangles,count)==1;
        const bool changed=!world_mesh_generation ||
            before!=world_mesh_generation();
        if (count==0) {
            world_inputs_live_logged=false;
            live_deferred_capture_valid=false;
            live_deferred_receipt_epoch=0;
            live_deferred_mesh_generation=0;
        } // scene intentionally cleared; no GPU provenance survives
        if (uploaded && count>0) {
            // A later generic/synthetic upload (including deduplicated
            // content) cannot inherit evidence from the last real scan.
            if (!live_deferred_capture_call) {
                live_deferred_capture_valid=false;
                live_deferred_receipt_epoch=0;
                live_deferred_mesh_generation=0;
            }
            if (live_deferred_capture_call && world_mesh_generation &&
                world_mesh_generation()>0) {
                live_deferred_capture_valid=true;
                live_deferred_receipt_epoch=live_world_region_epoch;
                live_deferred_mesh_generation=world_mesh_generation();
                ++live_deferred_receipts;
                // The reporter parses only numeric source receipts.
                // Never include region UUIDs, camera coordinates, names,
                // asset IDs, URLs, tokens or user information.
                LL_INFOS("OGLRenderer")
                    << "P05 deferred capture receipt: epoch="
                    << live_deferred_receipt_epoch
                    << " capture=" << live_deferred_receipts
                    << " triangles=" << count
                    << " mesh_generation=" << live_deferred_mesh_generation
                    << "; actual Firestorm deferred scan, not GPU proof."
                    << LL_ENDL;
            }
            // A byte-identical, opted-in mesh snapshot is already paired
            // with this pose and uses the same VkBuffers. Never generate
            // synthetic camera revisions or misleading upload logs.
            if (changed) world_inputs_live_logged=false;
            // The real deferred camera was sampled earlier in THIS frame
            // than the region geometry. A successful upload invalidates
            // that camera, so explicitly rebind its VALIDATED current pose.
            // This never takes over GL's window or submits Vulkan presents.
            // A region-transition callback discards the cache: no old-region
            // matrix can be attached to a new-region mesh. Entirely opt-in.
            const char* rebind=std::getenv("OGL_VULKAN_LIVE_CAMERA_REBIND");
            const bool enabled=rebind && std::strcmp(rebind,"1")==0;
            if (enabled && !world_inputs_live_logged &&
                last_live_world_camera_valid && world_set_camera &&
                world_input_ready && world_mesh_generation &&
                world_camera_mesh_generation && world_device_ready &&
                world_device_ready()==1 &&
                world_set_camera(&last_live_world_camera)==1 &&
                world_mesh_generation()>0 &&
                world_camera_mesh_generation()==world_mesh_generation() &&
                world_input_ready()==1) {
                ++live_world_rebinds;
                world_inputs_live_logged=true;
                // Newly published region meshes get a bounded validation.
                // Camera-only redraws have a separate, slower tick AFTER
                // the real OpenSim region-ID comparison in pipeline.cpp.
                if ((live_world_rebinds<=3 || live_world_rebinds%64==0) &&
                    ogl_firestorm_offscreen_gpu_sample("mesh-rebind")) {
                    live_world_last_offscreen_pose=last_live_world_camera;
                    live_world_last_offscreen_pose_valid=true;
                    live_world_last_offscreen_mesh_generation=
                        world_mesh_generation();
                    live_world_last_offscreen_region_epoch=
                        live_world_region_epoch;
                }
                // Bounded diagnostics without region IDs, avatar positions,
                // object names or unbounded per-frame logging.
                if (live_world_rebinds<=3 || live_world_rebinds%64==0)
                    LL_INFOS("OGLRenderer")
                        << "P05 real Firestorm/OpenSim mesh and camera "
                           "generation paired after deferred capture: "
                        << "triangles=" << count
                        << " mesh_generation=" << world_mesh_generation()
                        << " region_epoch=" << live_world_region_epoch
                        << " rebinds=" << live_world_rebinds
                        << "; NO Vulkan world present, OpenGL remains active."
                        << LL_ENDL;
            }
        }
    }
    // The historical RTX diagnostic path retains its feature/opt-in gating.
    if (!rtx_submit_geometry || !rtx_hardware_ready) return;
    rtx_submit_geometry(triangles,count);
#endif
}


// The only permitted provenance marker for a real in-world OpenSim
// scanner. Called only from LLPipeline after validating the active
// LLViewerRegion and non-null region UUID. The generic developer
// triangle ABI NEVER marks a batch as deferred-source evidence.
void ogl_rtx_submit_deferred_world_triangles(
    const float* triangle_xyz,unsigned count)
{
#if LL_LINUX || LL_WINDOWS
    if (!triangle_xyz || count==0 ||
        count>OGL_RTX_MAX_SCENE_TRIANGLES)
        return;
    // This C++ host runs on the viewer rendering thread. No callback
    // persists the caller's borrowed triangle buffer.
    live_deferred_capture_call=true;
    ogl_rtx_submit_world_triangles(triangle_xyz,count);
    live_deferred_capture_call=false;
#else
    (void)triangle_xyz;
    (void)count;
#endif
}

bool ogl_rtx_track_local_object(const std::uint8_t* uuid,
    const float* local_xyz,unsigned count,const float* pose)
{
#if LL_LINUX || LL_WINDOWS
    if (!rtx_hardware_ready || !rtx_track_object || !uuid || !local_xyz ||
        !pose || count==0 || count>OGL_RTX_MAX_SCENE_TRIANGLES) return false;
    return rtx_track_object(uuid,reinterpret_cast<const ogl_rtx_triangle*>(local_xyz),
                            count,pose)==1;
#else
    return false;
#endif
}
bool ogl_rtx_update_object_pose(const std::uint8_t* uuid,const float* pose)
{
#if LL_LINUX || LL_WINDOWS
    return rtx_hardware_ready && rtx_track_pose && uuid && pose &&
           rtx_track_pose(uuid,pose)==1;
#else
    return false;
#endif
}
void ogl_rtx_forget_local_object(const std::uint8_t* uuid)
{
#if LL_LINUX || LL_WINDOWS
    if (rtx_forget_object && uuid) rtx_forget_object(uuid);
#endif
}

// Opt-in 128x72 Vulkan-guided sun shadow on the ACTUAL OpenGL deferred
// sunlight target. No fullscreen 8x8 UI effect: each fragment reads current
// GL depth, its own Vulkan source tile, and rejects stale geometry.
bool ogl_rtx_compose_dense_sun_shadow(unsigned depth_texture,
    unsigned width,unsigned height,const float* eye,const float* forward,
    const float* right,const float* up,float fov,float aspect,
    float near_clip,float far_clip,const float* sun,
    bool diagnostic_overlay)
{
#if LL_LINUX || LL_WINDOWS
    static const bool debug_opt_in=[] {
        const char* value=std::getenv("OGL_RTX_SHADOW_VISUALIZE");
        return value && std::strcmp(value,"1")==0;
    }();
    if (diagnostic_overlay && !debug_opt_in) return false;
    static const bool opt_in=[]() {
        const char* v=std::getenv("OGL_RTX_WORLD_COMPOSITE");
        const char* d=std::getenv("OGL_RTX_DENSE_PREVIEW");
        return v && d && std::strcmp(v,"1")==0 &&
               std::strcmp(d,"1")==0;
    }();
    // Log *actual fragment coverage*, not the number of Vulkan tile
    // shadows. A valid 128x72 mask might shade ZERO OpenGL pixels after
    // the depth check; previously the log wrongly implied visible output.
    static GLuint rt_coverage_query=0;
    static bool rt_coverage_pending=false;
    static uint64_t rt_query_sequence=0,rt_query_scene=0;
    static uint64_t rt_last_measured_sequence=0,rt_last_measured_scene=0;
    if (rt_coverage_query && !glIsQuery(rt_coverage_query)) {
        rt_coverage_query=0;rt_coverage_pending=false;
        rt_last_measured_sequence=0;rt_last_measured_scene=0;
    }
    if (rt_coverage_query && rt_coverage_pending) {
        GLuint available=0;
        glGetQueryObjectuiv(rt_coverage_query,GL_QUERY_RESULT_AVAILABLE,
                            &available);
        if (available) {
            GLuint samples=0;
            glGetQueryObjectuiv(rt_coverage_query,GL_QUERY_RESULT,&samples);
            rt_coverage_pending=false;
            LL_INFOS("OGLRTX")
                << "OGL RTX GL shadow pixel coverage: samples=" << samples
                << " source=" << rt_query_sequence
                << " scene=" << rt_query_scene
                << " (post-depth-discard fragments in deferred lightmap; "
                   "not proof of final displayed color)"
                << LL_ENDL;
        }
    }
    if (!opt_in || !rtx_hardware_ready || !rtx_copy_dense ||
        !depth_texture || !glIsTexture(depth_texture) ||
        !eye || !forward || !right || !up || !sun ||
        width<OGL_RTX_DENSE_WIDTH || height<OGL_RTX_DENSE_HEIGHT)
        return false;
    ogl_rtx_frame_state current{};
    current.struct_size=sizeof(current);
    current.frame_version=OGL_RTX_FRAME_VERSION;
    current.width=width;current.height=height;
    for (unsigned i=0;i<3;++i) {
        current.camera_position[i]=eye[i];
        current.camera_forward[i]=forward[i];
        current.camera_right[i]=right[i];
        current.camera_up[i]=up[i];
        current.sun_direction[i]=sun[i];
    }
    current.vertical_fov_radians=fov;
    current.aspect_ratio=aspect;
    current.near_clip_m=near_clip;
    current.far_clip_m=far_clip;
    ogl_rtx_dense_shadow source{};
    source.struct_size=sizeof(source);
    source.version=OGL_RTX_DENSE_VERSION;
    if (rtx_copy_dense(&source)!=1 ||
        !ogl::valid_dense_shadow(source,source.geometry_revision,current) ||
        source.shadow_pixels==0)
        return false;

    // Strict depth-gated fragment pass. No bilinear interpolation; a tile
    // can only darken a pixel whose LIVE deferred depth agrees with the
    // GPU input depth and whose primary Vulkan ray hit was validated.
    static const char* vertex_source=R"GLSL(
#version 330 core
void main() {
    vec2 p=vec2((gl_VertexID==1)?3.0:-1.0,
                (gl_VertexID==2)?3.0:-1.0);
    gl_Position=vec4(p,0.0,1.0);
}
)GLSL";
    static const char* fragment_source=R"GLSL(
#version 330 core
uniform sampler2D uLiveDepth;
uniform sampler2D uVulkanTile;
uniform ivec2 uViewport;
uniform vec2 uNearFar;
uniform int uDiagnosticTint;
out vec4 outColor;
float viewDistance(float d) {
    float n=uNearFar.x,f=uNearFar.y;
    float den=f+n-(2.0*d-1.0)*(f-n);
    return (den>0.0) ? (2.0*n*f)/den : -1.0;
}
void main() {
    ivec2 pos=ivec2(gl_FragCoord.xy);
    if (any(lessThan(pos,ivec2(0))) ||
        any(greaterThanEqual(pos,uViewport))) discard;
    ivec2 t=clamp(pos*ivec2(128,72)/uViewport,
                  ivec2(0),ivec2(127,71));
    vec4 ray=texelFetch(uVulkanTile,t,0);
    if (ray.r<0.5 || ray.g<0.5) discard;
    float live=texelFetch(uLiveDepth,pos,0).r;
    // Sparse source meshes, rigged avatars and moving world geometry must
    // not be darkened unless current depth still matches capture depth.
    if (live>=0.999999 || ray.b>=0.999999) discard;
    float reference=viewDistance(ray.b);
    float now=viewDistance(live);
    if (reference<=0.0 || now<=0.0) discard;
    // Nonlinear-depth epsilon 1e-5 discarded most FULL-RES pixels:
    // the 128x72 source represents one center sample per large tile.
    // Conservative, distance-scaled acceptance, NOT cross-object blur.
    float tolerance=clamp(reference*0.02,0.25,1.5);
    if (abs(now-reference)>tolerance) discard;
    // Soft per-tile transition mitigates hard 54x39-pixel blocks at
    // ultrawide resolutions. This NEVER invents an unvalidated ray hit:
    // we soften solely WITHIN a shadowed, live-depth-matched tile.
    vec2 cell=fract((vec2(pos)+vec2(0.5))*
                    vec2(128.0,72.0)/vec2(uViewport));
    float distanceToTileEdge=min(min(cell.x,1.0-cell.x),
                                 min(cell.y,1.0-cell.y));
    float feather=smoothstep(0.0,0.18,distanceToTileEdge);
    // First-gen direct sunlight attenuation, not GI or path tracing.
    // Distinct debug tint demonstrates which valid pixels actually
    // received ray-query shadows, without inventing shadow receivers.
    // R is sun visibility in Firestorm's lightMap. The atmospheric
    // deferred soften shader samples lightMap.r for sunlight and
    // lightMap.g for SSAO. A red color in the LIGHTMAP is NOT a
    // red screen overlay; in fact writing all channels damages SSAO.
    // Normal pass: attenuate R only. Debug overlay: run a second,
    // independently opt-in pass on the final 3D color target.
    outColor=(uDiagnosticTint!=0) ?
        vec4(1.0,0.0,0.18,0.9*feather) :
        vec4(0.0,0.0,0.0,0.48*feather);
}
)GLSL";
    static GLuint program=0,vao=0,tile_texture=0;
    static bool compile_failed=false;
    static uint64_t last_upload=0;
    if (compile_failed) return false;
    // Firestorm can rebuild its OpenGL context after graphics preferences
    // change. Never keep using a stale shader/VAO/texture handle from the
    // previous GL context; the renderer must reinitialize safely.
    if (program && (!glIsProgram(program) ||
                    !glIsVertexArray(vao) ||
                    !glIsTexture(tile_texture))) {
        program=0;vao=0;tile_texture=0;
        last_upload=0;
    }
    if (!program) {
        const auto compile=[](GLenum type,const char* source)->GLuint {
            GLuint shader=glCreateShader(type);
            if (!shader) return 0;
            glShaderSource(shader,1,&source,nullptr);
            glCompileShader(shader);
            GLint good=0;
            glGetShaderiv(shader,GL_COMPILE_STATUS,&good);
            if (!good) {glDeleteShader(shader);return 0;}
            return shader;
        };
        const GLuint vs=compile(GL_VERTEX_SHADER,vertex_source);
        const GLuint fs=compile(GL_FRAGMENT_SHADER,fragment_source);
        if (vs && fs) {
            program=glCreateProgram();
            glAttachShader(program,vs);
            glAttachShader(program,fs);
            glLinkProgram(program);
            GLint linked=0;
            glGetProgramiv(program,GL_LINK_STATUS,&linked);
            if (!linked) {glDeleteProgram(program);program=0;}
        }
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        if (!program) {
            compile_failed=true;
            LL_WARNS("OGLRTX") << "Experimental OpenGL shadow compositor "
                               << "shader rejected; original renderer kept."
                               << LL_ENDL;
            return false;
        }
        glGenVertexArrays(1,&vao);
        glGenTextures(1,&tile_texture);
        if (!vao || !tile_texture) {
            compile_failed=true;
            return false;
        }
        LL_INFOS("OGLRTX") << "Experimental 128x72 Vulkan-to-OpenGL "
                             "depth-gated sunlight compositor initialized"
                          << LL_ENDL;
    }
    GLint old_program=0,old_vao=0,old_active=0;
    GLint old_tex0=0,old_tex1=0;
    GLint old_viewport[4]{};
    GLint old_blend_src_rgb=0,old_blend_dst_rgb=0;
    GLint old_blend_src_alpha=0,old_blend_dst_alpha=0;
    GLint old_equation_rgb=0,old_equation_alpha=0;
    GLboolean old_blend=glIsEnabled(GL_BLEND);
    GLboolean old_depth=glIsEnabled(GL_DEPTH_TEST);
    GLboolean old_cull=glIsEnabled(GL_CULL_FACE);
    GLboolean old_scissor=glIsEnabled(GL_SCISSOR_TEST);
    GLboolean old_color_mask[4]{};
    glGetBooleanv(GL_COLOR_WRITEMASK,old_color_mask);
    glGetIntegerv(GL_CURRENT_PROGRAM,&old_program);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&old_vao);
    glGetIntegerv(GL_ACTIVE_TEXTURE,&old_active);
    glGetIntegerv(GL_VIEWPORT,old_viewport);
    glGetIntegerv(GL_BLEND_SRC_RGB,&old_blend_src_rgb);
    glGetIntegerv(GL_BLEND_DST_RGB,&old_blend_dst_rgb);
    glGetIntegerv(GL_BLEND_SRC_ALPHA,&old_blend_src_alpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA,&old_blend_dst_alpha);
    glGetIntegerv(GL_BLEND_EQUATION_RGB,&old_equation_rgb);
    glGetIntegerv(GL_BLEND_EQUATION_ALPHA,&old_equation_alpha);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D,&old_tex0);
    glActiveTexture(GL_TEXTURE1);
    glGetIntegerv(GL_TEXTURE_BINDING_2D,&old_tex1);
    glBindTexture(GL_TEXTURE_2D,tile_texture);
    if (last_upload!=source.sample_sequence) {
        std::array<float,OGL_RTX_DENSE_PIXELS*4u> pixels{};
        for (unsigned i=0;i<OGL_RTX_DENSE_PIXELS;++i) {
            pixels[i*4+0]=float(source.valid_mask[i]);
            pixels[i*4+1]=float(source.shadow_mask[i]);
            pixels[i*4+2]=source.capture_depth[i];
        }
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA32F,
                     OGL_RTX_DENSE_WIDTH,OGL_RTX_DENSE_HEIGHT,0,
                     GL_RGBA,GL_FLOAT,pixels.data());
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        last_upload=source.sample_sequence;
    }
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,depth_texture);
    glUseProgram(program);
    glUniform1i(glGetUniformLocation(program,"uLiveDepth"),0);
    glUniform1i(glGetUniformLocation(program,"uVulkanTile"),1);
    glUniform2i(glGetUniformLocation(program,"uViewport"),width,height);
    glUniform2f(glGetUniformLocation(program,"uNearFar"),
                near_clip,far_clip);
    glUniform1i(glGetUniformLocation(program,"uDiagnosticTint"),
                diagnostic_overlay ? 1 : 0);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA,
                        GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquationSeparate(GL_FUNC_ADD,GL_FUNC_ADD);
    glViewport(0,0,width,height);
    // The deferred light map is RGBA but channel R alone carries
    // directional-sun visibility, G=ambient occlusion, B/A=spotlights.
    // Writing all channels was corrupting other Firestorm lighting.
    // A separate debug overlay deliberately writes RGBA on the final
    // scene color target rather than trying to paint a red lightmap.
    glColorMask(GL_TRUE,
                diagnostic_overlay ? GL_TRUE : GL_FALSE,
                diagnostic_overlay ? GL_TRUE : GL_FALSE,
                diagnostic_overlay ? GL_TRUE : GL_FALSE);
    glBindVertexArray(vao);
    // GL_SAMPLES_PASSED counts fragments surviving discard. This detects
    // an ineffective compositor even when thousands of Vulkan rays hit.
    // Query once per captured scene, never synchronously wait on the GPU.
    GLint other_query=0;
    glGetQueryiv(GL_SAMPLES_PASSED,GL_CURRENT_QUERY,&other_query);
    if (!rt_coverage_query && !other_query)
        glGenQueries(1,&rt_coverage_query);
    const bool record_coverage=!diagnostic_overlay && rt_coverage_query &&
        !rt_coverage_pending && !other_query &&
        (rt_last_measured_sequence!=source.sample_sequence ||
         rt_last_measured_scene!=source.geometry_revision);
    if (record_coverage)
        glBeginQuery(GL_SAMPLES_PASSED,rt_coverage_query);
    glDrawArrays(GL_TRIANGLES,0,3);
    if (record_coverage) {
        glEndQuery(GL_SAMPLES_PASSED);
        rt_coverage_pending=true;
        rt_query_sequence=source.sample_sequence;
        rt_query_scene=source.geometry_revision;
        rt_last_measured_sequence=source.sample_sequence;
        rt_last_measured_scene=source.geometry_revision;
    }
    // Restore everything modified; subsequent Firestorm lighting/UI passes
    // must observe exactly their original global GL state.
    glBindVertexArray(old_vao);
    glUseProgram(old_program);
    glColorMask(old_color_mask[0],old_color_mask[1],
                old_color_mask[2],old_color_mask[3]);
    glViewport(old_viewport[0],old_viewport[1],
               old_viewport[2],old_viewport[3]);
    glBlendFuncSeparate(old_blend_src_rgb,old_blend_dst_rgb,
                        old_blend_src_alpha,old_blend_dst_alpha);
    glBlendEquationSeparate(old_equation_rgb,old_equation_alpha);
    if (!old_blend) glDisable(GL_BLEND);
    if (old_depth) glEnable(GL_DEPTH_TEST);
    if (old_cull) glEnable(GL_CULL_FACE);
    if (old_scissor) glEnable(GL_SCISSOR_TEST);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D,old_tex1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,old_tex0);
    glActiveTexture(old_active);
    static uint64_t last_logged=0,last_logged_scene=0;
    static uint64_t debug_last_logged=0,debug_last_scene=0;
    if (diagnostic_overlay &&
        (debug_last_logged!=source.sample_sequence ||
         debug_last_scene!=source.geometry_revision)) {
        debug_last_logged=source.sample_sequence;
        debug_last_scene=source.geometry_revision;
        LL_INFOS("OGLRTX")
            << "OGL RTX shadow diagnostic color overlay: source="
            << source.sample_sequence << " scene="
            << source.geometry_revision << " shadow_tile_pixels="
            << source.shadow_pixels
            << " (drawn to 3D screen target, NOT the sunlight map; "
               "depth-gated; image visibility not yet independently verified)"
            << LL_ENDL;
    }
    if (!diagnostic_overlay &&
        (last_logged!=source.sample_sequence ||
         last_logged_scene!=source.geometry_revision)) {
        last_logged=source.sample_sequence;
        last_logged_scene=source.geometry_revision;
        LL_INFOS("OGLRTX") << "OGL RTX dense world sunlight composed: "
                           << source.shadow_pixels << " GPU shadow tile(s) / "
                           << source.matched_pixels
                           << " depth-matched, at 128x72 sample resolution; "
                           << "GPU shadow mask submitted to GL; pixel "
                              "coverage separately measured (may be zero)"
                           << LL_ENDL;
    }
    return true;
#else
    (void)depth_texture;(void)width;(void)height;(void)eye;(void)forward;
    (void)right;(void)up;(void)fov;(void)aspect;
    (void)near_clip;(void)far_clip;(void)sun;
    (void)diagnostic_overlay;
    return false;
#endif
}

// Separate GPU->world bridge. This reports an eligible *input*, never
// enables shadow compositing: CURRENT per-pixel GL depth and opaque material
// must still be checked by a real full-resolution compositor.
bool ogl_rtx_world_shadow_source_ready(unsigned width,unsigned height,
    const float* eye,const float* forward,const float* right,
    const float* up,float fov,float aspect,float near_clip,float far_clip,
    const float* sun,unsigned* matched,unsigned* occluded,
    std::uint64_t* sequence)
{
#if LL_LINUX || LL_WINDOWS
    if (!rtx_hardware_ready || !rtx_copy_world_shadow ||
        !eye || !forward || !right || !up || !sun ||
        !matched || !occluded || !sequence) return false;
    ogl_rtx_frame_state current{};
    current.struct_size=sizeof(current);
    current.frame_version=OGL_RTX_FRAME_VERSION;
    current.width=width;
    current.height=height;
    for (unsigned i=0;i<3;++i) {
        current.camera_position[i]=eye[i];
        current.camera_forward[i]=forward[i];
        current.camera_right[i]=right[i];
        current.camera_up[i]=up[i];
        current.sun_direction[i]=sun[i];
    }
    current.vertical_fov_radians=fov;
    current.aspect_ratio=aspect;
    current.near_clip_m=near_clip;
    current.far_clip_m=far_clip;
    ogl_rtx_world_shadow_sample tile{};
    tile.struct_size=sizeof(tile);
    tile.version=OGL_RTX_WORLD_SHADOW_VERSION;
    if (rtx_copy_world_shadow(&tile)!=1 ||
        !ogl::valid_world_shadow_sample(
            tile,tile.geometry_revision,current))
        return false;
    // Do NOT invent a high-res shadow map by stretching these 64 samples.
    *matched=tile.matched_surface_pixels;
    *occluded=tile.occluded_pixels;
    *sequence=tile.sample_sequence;
    return true;
#else
    (void)width;(void)height;(void)eye;(void)forward;(void)right;
    (void)up;(void)fov;(void)aspect;(void)near_clip;(void)far_clip;
    (void)sun;(void)matched;(void)occluded;(void)sequence;
    return false;
#endif
}

// Firestorm's future diagnostic compositor can request a _current_ sample.
// The GPU module remains a black box; no OpenGL/Vulkan calls here.
bool ogl_rtx_copy_debug_shadow_sample(std::uint8_t* pixels,
    std::size_t capacity, std::uint64_t* geometry_revision,
    std::uint64_t* sample_sequence,
    std::uint8_t* surface_mask, std::uint32_t* surfaces,
    std::uint32_t* background)
{
#if LL_LINUX || LL_WINDOWS
    if (!rtx_module_handle || !rtx_hardware_ready ||
        !rtx_copy_shadow_debug || !pixels ||
        capacity < OGL_RTX_SHADOW_DEBUG_PIXELS)
        return false;
    ogl_rtx_shadow_debug debug{};
    debug.struct_size = sizeof(debug);
    debug.version = OGL_RTX_SHADOW_DEBUG_VERSION;
    if (rtx_copy_shadow_debug(&debug) != 1 ||
        debug.width != 8 || debug.height != 8)
        return false;
    unsigned hit_count = 0;
    for (const auto pixel : debug.pixels) {
        if (pixel > 1) return false;
        hit_count += pixel;
    }
    if (hit_count != debug.occluded) return false;
    unsigned surface_count = 0;
    for (unsigned i=0; i<OGL_RTX_SHADOW_DEBUG_PIXELS; ++i) {
        if (debug.surface_mask[i] > 1) return false;
        surface_count += debug.surface_mask[i];
        if (debug.surface_pixels+debug.background_pixels==64 &&
            debug.pixels[i] && !debug.surface_mask[i]) return false;
    }
    if (debug.surface_pixels+debug.background_pixels==64 &&
        surface_count!=debug.surface_pixels) return false;
    std::memcpy(pixels, debug.pixels, OGL_RTX_SHADOW_DEBUG_PIXELS);
    if (surface_mask)
        std::memcpy(surface_mask, debug.surface_mask, OGL_RTX_SHADOW_DEBUG_PIXELS);
    if (geometry_revision) *geometry_revision = debug.geometry_revision;
    if (sample_sequence) *sample_sequence = debug.sample_sequence;
    if (surfaces) *surfaces = debug.surface_pixels;
    if (background) *background = debug.background_pixels;
    return true;
#else
    (void)pixels;
    (void)capacity;
    (void)geometry_revision;
    (void)sample_sequence;
    (void)surface_mask;
    (void)surfaces;
    (void)background;
    return false;
#endif
}

bool ogl_rtx_copy_debug_shadow_mask(std::uint8_t* pixels,
    std::size_t capacity, std::uint64_t* geometry_revision)
{
    return ogl_rtx_copy_debug_shadow_sample(
        pixels, capacity, geometry_revision, nullptr, nullptr, nullptr, nullptr);
}


// One optional readback of a whole-viewport depth grid. GPU-side NEAREST
// depth-only blit downsamples to 8x8 first, so one glReadPixels() replaces
// 64 independent readbacks. Sampling centres match camera ray centres.
// This remains a synchronous developer-only diagnostic (once per minute).
void ogl_rtx_probe_deferred_depth(unsigned texture,
    unsigned width, unsigned height)
{
#if LL_LINUX || LL_WINDOWS
    static const bool enabled = [] {
        const char* env = std::getenv("OGL_RTX_DEPTH_PROBE");
        return env && std::strcmp(env, "1") == 0;
    }();
    static const bool dense_enabled=[] {
        const char* env=std::getenv("OGL_RTX_DENSE_PREVIEW");
        return env && std::strcmp(env,"1")==0;
    }();
    if (!enabled || !rtx_hardware_ready ||
        (dense_enabled ? !rtx_submit_dense : !rtx_submit_depth) ||
        !rtx_received_triangles || rtx_received_triangles() == 0 || !texture ||
        width < 8 || height < 8 || width > 32768 || height > 32768)
        return;
    using Clock = std::chrono::steady_clock;
    static Clock::time_point previous{};
    const auto now = Clock::now();
    if (previous != Clock::time_point{} &&
        now - previous < (dense_enabled ?
            std::chrono::seconds(2) : std::chrono::seconds(60)))
        return;
    previous = now; // even failure must never flood GL with readbacks

    // Preserve both framebuffer bindings, renderbuffer binding, scissor and
    // all pack state touched by glReadPixels(). No changes to color targets.
    GLint old_read = 0, old_draw = 0, old_renderbuffer = 0;
    GLint old_pack_buffer = 0, old_pack_alignment = 4;
    GLint old_row_length = 0, old_skip_pixels = 0, old_skip_rows = 0;
    const GLboolean old_scissor = glIsEnabled(GL_SCISSOR_TEST);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &old_read);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &old_draw);
    glGetIntegerv(GL_RENDERBUFFER_BINDING, &old_renderbuffer);
    glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &old_pack_buffer);
    glGetIntegerv(GL_PACK_ALIGNMENT, &old_pack_alignment);
    glGetIntegerv(GL_PACK_ROW_LENGTH, &old_row_length);
    glGetIntegerv(GL_PACK_SKIP_PIXELS, &old_skip_pixels);
    glGetIntegerv(GL_PACK_SKIP_ROWS, &old_skip_rows);

    const unsigned grid_w=dense_enabled ? OGL_RTX_DENSE_WIDTH : 8u;
    const unsigned grid_h=dense_enabled ? OGL_RTX_DENSE_HEIGHT : 8u;
    if (width<grid_w || height<grid_h) return;
    GLuint source = 0, reduced = 0, depth_renderbuffer = 0;
    glGenFramebuffers(1, &source);
    glGenFramebuffers(1, &reduced);
    glGenRenderbuffers(1, &depth_renderbuffer);
    if (source && reduced && depth_renderbuffer) {
        glDisable(GL_SCISSOR_TEST); // scissor must not crop the blit
        glBindFramebuffer(GL_READ_FRAMEBUFFER, source);
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                               GL_TEXTURE_2D, texture, 0);
        glReadBuffer(GL_NONE);

        glBindRenderbuffer(GL_RENDERBUFFER, depth_renderbuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24,
                              grid_w,grid_h);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, reduced);
        glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                  GL_RENDERBUFFER, depth_renderbuffer);
        glDrawBuffer(GL_NONE);
        if (glCheckFramebufferStatus(GL_READ_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE &&
            glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
            // Pixel (x,y) maps to UV ((x+.5)/8, (y+.5)/8). Both GL and
            // Vulkan diagnostic shader use origin=bottom-left here.
            glBlitFramebuffer(0, 0, width, height, 0, 0,
                              grid_w,grid_h,
                              GL_DEPTH_BUFFER_BIT, GL_NEAREST);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, reduced);
            glReadBuffer(GL_NONE);
            glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
            glPixelStorei(GL_PACK_ALIGNMENT, 4);
            glPixelStorei(GL_PACK_ROW_LENGTH, 0);
            glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
            glPixelStorei(GL_PACK_SKIP_ROWS, 0);
            if (dense_enabled) {
                ogl_rtx_dense_depth sample{};
                sample.struct_size=sizeof(sample);
                sample.version=OGL_RTX_DENSE_VERSION;
                sample.viewport_width=width;
                sample.viewport_height=height;
                sample.width=grid_w;
                sample.height=grid_h;
                glReadPixels(0,0,grid_w,grid_h,GL_DEPTH_COMPONENT,GL_FLOAT,
                             sample.normalized_depth);
                if (glGetError()==GL_NO_ERROR &&
                    rtx_submit_dense(&sample)==1)
                    LL_INFOS("OGLRTX")
                        << "Deferred depth dense grid captured "
                        << width << "x" << height << " -> "
                        << grid_w << "x" << grid_h
                        << " (experimental Vulkan ray input)" << LL_ENDL;
            } else {
                ogl_rtx_depth_debug sample{};
                sample.struct_size = sizeof(sample);
                sample.version = OGL_RTX_DEPTH_DEBUG_VERSION;
                sample.viewport_width = width;
                sample.viewport_height = height;
                sample.grid_width = 8;
                sample.grid_height = 8;
                glReadPixels(0, 0, 8, 8, GL_DEPTH_COMPONENT, GL_FLOAT,
                             sample.normalized_depth);
                if (glGetError() == GL_NO_ERROR &&
                    rtx_submit_depth(&sample) == 1)
                    LL_INFOS("OGLRTX")
                        << "Deferred depth full-view grid captured "
                        << width << "x" << height
                        << " -> 8x8 (64 aligned samples); no visible RT output."
                        << LL_ENDL;
            }
        }
    }
    // Release the temporary framebuffer resources while the correct context
    // is current. Restore bindings even on allocation/FBO failure.
    glBindFramebuffer(GL_READ_FRAMEBUFFER, old_read);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, old_draw);
    glBindRenderbuffer(GL_RENDERBUFFER, old_renderbuffer);
    if (source) glDeleteFramebuffers(1, &source);
    if (reduced) glDeleteFramebuffers(1, &reduced);
    if (depth_renderbuffer) glDeleteRenderbuffers(1, &depth_renderbuffer);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, old_pack_buffer);
    glPixelStorei(GL_PACK_ALIGNMENT, old_pack_alignment);
    glPixelStorei(GL_PACK_ROW_LENGTH, old_row_length);
    glPixelStorei(GL_PACK_SKIP_PIXELS, old_skip_pixels);
    glPixelStorei(GL_PACK_SKIP_ROWS, old_skip_rows);
    if (old_scissor) glEnable(GL_SCISSOR_TEST);
    else glDisable(GL_SCISSOR_TEST);
#else
    (void)texture;
    (void)width;
    (void)height;
#endif
}

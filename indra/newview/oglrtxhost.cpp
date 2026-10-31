#include "llviewerprecompiledheaders.h"
#include "oglrtxhost.h"
#include "../../ogl/modules/include/ogl/ogl_rtx_api.h"

#if LL_LINUX || LL_WINDOWS
#include <cstring>
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
using ogl_rtx_copy_shadow_debug_fn = int (*)(ogl_rtx_shadow_debug*);
ogl_rtx_copy_shadow_debug_fn rtx_copy_shadow_debug = nullptr;
ogl_module_stop_fn rtx_stop = nullptr;

void host_log(int severity, const char *message) {
    LL_INFOS("OGLRTX") << "module severity " << severity << ": "
                       << (message ? message : "") << LL_ENDL;
}
}  // namespace
#endif

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
    rtx_copy_shadow_debug = reinterpret_cast<ogl_rtx_copy_shadow_debug_fn>(
        ogl_resolve_module(rtx_module_handle, "ogl_rtx_copy_shadow_debug"));

    const ogl_host_api api{sizeof(ogl_host_api), OGL_MODULE_ABI_VERSION, host_log};
    if (start(&api) != 0) {
        LL_WARNS("OGLRTX") << "RTX capability initialization rejected; keeping OpenGL."
                            << LL_ENDL;
        ogl_shutdown_rtx_module();
        return;
    }
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
#endif
}

void ogl_shutdown_rtx_module() {
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
    rtx_copy_shadow_debug = nullptr;
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

void ogl_rtx_submit_camera_sun(unsigned width, unsigned height,
    const float* camera_position, const float* camera_forward,
    const float* sun_direction) {
#if LL_LINUX || LL_WINDOWS
    if (!rtx_submit_frame || !rtx_hardware_ready ||
        !camera_position || !camera_forward || !sun_direction) return;
    ogl_rtx_frame_state frame{};
    frame.struct_size = sizeof(frame);
    frame.frame_version = OGL_RTX_FRAME_VERSION;
    frame.width = width;
    frame.height = height;
    for (unsigned i=0; i<3; ++i) {
        frame.camera_position[i] = camera_position[i];
        frame.camera_forward[i] = camera_forward[i];
        frame.sun_direction[i] = sun_direction[i];
    }
    rtx_submit_frame(&frame);
#endif
}

void ogl_rtx_submit_world_triangles(const float *triangle_xyz, unsigned count) {
#if LL_LINUX || LL_WINDOWS
    if (!rtx_submit_geometry || !rtx_hardware_ready ||
        count > OGL_RTX_MAX_SCENE_TRIANGLES || (count && !triangle_xyz)) return;
    // Geometry data layout is 9 tightly packed floats per triangle.
    static_assert(sizeof(ogl_rtx_triangle) == 9 * sizeof(float), "RTX ABI padding mismatch");
    rtx_submit_geometry(reinterpret_cast<const ogl_rtx_triangle*>(triangle_xyz), count);
#endif
}


// Firestorm's future diagnostic compositor can request a _current_ sample.
// The GPU module remains a black box; no OpenGL/Vulkan calls here.
bool ogl_rtx_copy_debug_shadow_mask(std::uint8_t* pixels,
    std::size_t capacity, std::uint64_t* geometry_revision)
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
    std::memcpy(pixels, debug.pixels, OGL_RTX_SHADOW_DEBUG_PIXELS);
    if (geometry_revision) *geometry_revision = debug.geometry_revision;
    return true;
#else
    (void)pixels;
    (void)capacity;
    (void)geometry_revision;
    return false;
#endif
}

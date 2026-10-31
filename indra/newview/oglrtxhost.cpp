#include "llviewerprecompiledheaders.h"
#include "oglrtxhost.h"
#include "../../ogl/modules/include/ogl/ogl_rtx_api.h"

#if LL_LINUX
#include <dlfcn.h>
#include <cstring>
#include <string>

namespace {
void *rtx_module_handle = nullptr;
bool rtx_hardware_ready = false;
bool rtx_rendering_enabled = false;
using ogl_rtx_submit_frame_fn = int (*)(const ogl_rtx_frame_state*);
ogl_rtx_submit_frame_fn rtx_submit_frame = nullptr;
using ogl_rtx_submit_geometry_fn = int (*)(const ogl_rtx_triangle*, uint32_t);
ogl_rtx_submit_geometry_fn rtx_submit_geometry = nullptr;
ogl_module_stop_fn rtx_stop = nullptr;

void host_log(int severity, const char *message) {
    LL_INFOS("OGLRTX") << "module severity " << severity << ": "
                       << (message ? message : "") << LL_ENDL;
}
}  // namespace
#endif

void ogl_init_rtx_module() {
#if LL_LINUX
    // Load only the bundled, trusted RTX module. Arbitrary paths and user-
    // supplied native modules are intentionally unsupported at this stage.
    rtx_module_handle = dlopen("./lib/ogl_rtx.so", RTLD_NOW | RTLD_LOCAL);
    if (!rtx_module_handle) {
        LL_INFOS("OGLRTX") << "RTX capability module unavailable; keeping OpenGL."
                            << LL_ENDL;
        return;
    }
    auto info = reinterpret_cast<ogl_module_info_fn>(
        dlsym(rtx_module_handle, "ogl_module_get_info"));
    auto start = reinterpret_cast<ogl_module_start_fn>(
        dlsym(rtx_module_handle, "ogl_module_start"));
    rtx_stop = reinterpret_cast<ogl_module_stop_fn>(
        dlsym(rtx_module_handle, "ogl_module_stop"));
    auto get_status = reinterpret_cast<const ogl_rtx_status *(*)()>(
        dlsym(rtx_module_handle, "ogl_rtx_get_status"));

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
        dlsym(rtx_module_handle, "ogl_rtx_submit_frame"));
    rtx_submit_geometry = reinterpret_cast<ogl_rtx_submit_geometry_fn>(
        dlsym(rtx_module_handle, "ogl_rtx_submit_geometry"));

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
#if LL_LINUX
    if (rtx_module_handle) {
        if (rtx_stop) rtx_stop();
        dlclose(rtx_module_handle);
    }
    rtx_stop = nullptr;
    rtx_module_handle = nullptr;
    rtx_hardware_ready = false;
    rtx_rendering_enabled = false;
    rtx_submit_frame = nullptr;
    rtx_submit_geometry = nullptr;
#endif
}

bool ogl_rtx_hardware_supported() {
#if LL_LINUX
    return rtx_hardware_ready;
#else
    return false;
#endif
}

bool ogl_rtx_rendering_active() {
#if LL_LINUX
    return rtx_rendering_enabled;
#else
    return false;
#endif
}

void ogl_rtx_submit_camera_sun(unsigned width, unsigned height,
    const float* camera_position, const float* camera_forward,
    const float* sun_direction) {
#if LL_LINUX
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
#if LL_LINUX
    if (!rtx_submit_geometry || !rtx_hardware_ready ||
        count > OGL_RTX_MAX_SCENE_TRIANGLES || (count && !triangle_xyz)) return;
    // Geometry data layout is 9 tightly packed floats per triangle.
    static_assert(sizeof(ogl_rtx_triangle) == 9 * sizeof(float), "RTX ABI padding mismatch");
    rtx_submit_geometry(reinterpret_cast<const ogl_rtx_triangle*>(triangle_xyz), count);
#endif
}

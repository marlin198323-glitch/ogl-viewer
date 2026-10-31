#include "ogl/ogl_rtx_api.h"
#include <cassert>
#include <cstdio>
#include <limits>
#include <vector>
#include <dlfcn.h>

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
    assert(start && stop && status && submit && count &&
           submit_geometry && triangle_count);
    ogl_host_api host{sizeof(host), OGL_MODULE_ABI_VERSION, log_cb};
    assert(start(&host) == 0);
    assert(count() == 0);

    ogl_rtx_frame_state valid{};
    valid.struct_size = sizeof(valid);
    valid.frame_version = OGL_RTX_FRAME_VERSION;
    valid.width = 3440;
    valid.height = 1440;
    valid.camera_forward[0] = 1.0f;
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
    } else {
        assert(submit(&valid) == 0);
        assert(count() == 0);
    }
    assert(triangle_count() == 0);
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
    if (status()->hardware_ready) {
        assert(submit_geometry(&valid_triangle, 1) == 1);
        assert(triangle_count() == 1);
        assert(submit_geometry(&nan_triangle, 1) == 0);
        assert(triangle_count() == 1); // invalid updates cannot destroy last valid geometry
        assert(submit_geometry(nullptr, 0) == 1);
        assert(triangle_count() == 0); // teleport/region change invalidates stale geometry
    } else {
        assert(submit_geometry(&valid_triangle, 1) == 0);
        assert(triangle_count() == 0);
    }
    assert(status()->rendering_active == 0);
    stop();
    dlclose(library);
    std::puts("OGL RTX camera/sun frame bridge API: PASS (raytraced pixels inactive)");
}

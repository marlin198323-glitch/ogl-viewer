#include "ogl/ogl_rtx_api.h"
#include <cassert>
#include <cstdio>
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
    assert(start && stop && status && submit && count);
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
    assert(status()->rendering_active == 0);
    stop();
    dlclose(library);
    std::puts("OGL RTX camera/sun frame bridge API: PASS (raytraced pixels inactive)");
}

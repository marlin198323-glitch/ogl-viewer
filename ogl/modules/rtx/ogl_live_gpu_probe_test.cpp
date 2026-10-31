// OGL developer probe acceptance: real module scene -> optional Vulkan GPU.
// This does not render shadows into Firestorm's OpenGL image.
#include "ogl/ogl_rtx_api.h"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <dlfcn.h>
#include <vector>

void test_log(int, const char* message) {
    // Do not print usernames or private OpenSim objects.
    if (message) std::printf("%s\n", message);
}

int main(int argc, char** argv) {
    const bool missing = argc == 3 && std::strcmp(argv[2], "--no-gpu-library") == 0;
    if (argc != 2 && !missing) return 1;
    void* library = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!library) {
        std::puts("FAIL: could not load OGL RTX module");
        return 1;
    }
    auto start = reinterpret_cast<ogl_module_start_fn>(
        dlsym(library, "ogl_module_start"));
    auto stop = reinterpret_cast<ogl_module_stop_fn>(
        dlsym(library, "ogl_module_stop"));
    auto status = reinterpret_cast<const ogl_rtx_status*(*)()>(
        dlsym(library, "ogl_rtx_get_status"));
    auto frame = reinterpret_cast<int(*)(const ogl_rtx_frame_state*)>(
        dlsym(library, "ogl_rtx_submit_frame"));
    auto submit = reinterpret_cast<int(*)(const ogl_rtx_triangle*, uint32_t)>(
        dlsym(library, "ogl_rtx_submit_geometry"));
    auto attempts = reinterpret_cast<uint64_t(*)()>(
        dlsym(library, "ogl_rtx_gpu_probe_attempts"));
    auto successes = reinterpret_cast<uint64_t(*)()>(
        dlsym(library, "ogl_rtx_gpu_probe_successes"));
    assert(start && stop && status && frame && submit && attempts && successes);
    ogl_host_api host{sizeof(host), OGL_MODULE_ABI_VERSION, test_log};
    assert(start(&host) == 0);
    assert(attempts() == 0 && successes() == 0);

    ogl_rtx_frame_state state{};
    state.struct_size = sizeof(state);
    state.frame_version = OGL_RTX_FRAME_VERSION;
    state.width = 800;
    state.height = 600;
    state.sun_direction[2] = 1.f;
    std::vector<ogl_rtx_triangle> tris(64);
    for (unsigned i = 0; i < tris.size(); ++i) {
        const float off = i == 0 ? 0.f : 10.f + i * 2.f;
        tris[i].position[0] = off;
        tris[i].position[3] = off + 1.f;
        tris[i].position[7] = 1.f;
    }

    if (status()->hardware_ready && status()->ray_query) {
        assert(frame(&state) == 1);
        assert(submit(tris.data(), tris.size()) == 1);
        assert(attempts() == (missing ? 0u : 1u));
        assert(successes() == (missing ? 0u : 1u));
        // Same geometry: no new GPU invocation.
        assert(submit(tris.data(), tris.size()) == 1);
        assert(attempts() == (missing ? 0u : 1u));
        // Changed geometry: rate limiting prevents immediate second call.
        tris[0].position[0] = 0.15f;
        assert(submit(tris.data(), tris.size()) == 1);
        assert(attempts() == (missing ? 0u : 1u));
        assert(successes() == (missing ? 0u : 1u));
    } else {
        assert(attempts() == 0);
        assert(successes() == 0);
    }
    assert(status()->rendering_active == 0);
    stop();
    dlclose(library);
    std::puts("PASS: optional GPU diagnostics are isolated; OpenGL remains active");
}

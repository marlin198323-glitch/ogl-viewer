// OGL developer probe acceptance: real module scene -> optional Vulkan GPU.
// This does not render shadows into Firestorm's OpenGL image.
#include "ogl/ogl_rtx_api.h"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <chrono>
#include <thread>
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
    auto copy_debug = reinterpret_cast<int(*)(ogl_rtx_shadow_debug*)>(
        dlsym(library, "ogl_rtx_copy_shadow_debug"));
    assert(start && stop && status && frame && submit && attempts && successes && copy_debug);
    ogl_host_api host{sizeof(host), OGL_MODULE_ABI_VERSION, test_log};
    assert(start(&host) == 0);
    assert(attempts() == 0 && successes() == 0);
    ogl_rtx_shadow_debug debug{};
    debug.struct_size = sizeof(debug);
    debug.version = OGL_RTX_SHADOW_DEBUG_VERSION;
    assert(copy_debug(&debug) == 0); // no stale output before GPU runs
    debug.version = 999;
    assert(copy_debug(&debug) == 0); // reject incompatible ABI
    debug.version = OGL_RTX_SHADOW_DEBUG_VERSION;

    ogl_rtx_frame_state state{};
    state.struct_size = sizeof(state);
    state.frame_version = OGL_RTX_FRAME_VERSION;
    state.width = 800;
    state.height = 600;
    state.sun_direction[2] = 1.f;
    state.camera_position[2] = 2.f;
    state.camera_forward[2] = -1.f;
    std::vector<ogl_rtx_triangle> tris(64);
    for (unsigned i = 0; i < tris.size(); ++i) {
        const float off = i == 0 ? 0.f : 10.f + i * 2.f;
        tris[i].position[0] = off;
        tris[i].position[3] = off + 1.f;
        tris[i].position[7] = 1.f;
    }

    if (status()->hardware_ready && status()->ray_query) {
        assert(frame(&state) == 1);
        const auto beginning = std::chrono::steady_clock::now();
        assert(submit(tris.data(), tris.size()) == 1);
        // Submit must never wait for GPU BLAS/TLAS in this thread.
        const auto elapsed = std::chrono::steady_clock::now() - beginning;
        assert(elapsed < std::chrono::milliseconds(250));
        if (!missing) {
            for (unsigned i = 0; i < 500 && successes() == 0; ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                frame(&state); // poll completion without submitting a new job
            }
        }
        assert(attempts() == (missing ? 0u : 1u));
        assert(successes() == (missing ? 0u : 1u));
        if (!missing) {
            // Worker completion and main-thread publication are separate.
            for (unsigned i = 0; i < 30; ++i) assert(frame(&state) == 1);
            assert(copy_debug(&debug) == 1);
            assert(debug.width == 8 && debug.height == 8);
            assert(debug.geometry_revision == 1);
            uint32_t hits = 0;
            for (const auto pixel : debug.pixels) {
                assert(pixel <= 1);
                hits += pixel;
            }
            assert(hits == debug.occluded);
            assert(copy_debug(nullptr) == 0);
            ogl_rtx_shadow_debug too_small{};
            too_small.struct_size = sizeof(too_small) - 1;
            too_small.version = OGL_RTX_SHADOW_DEBUG_VERSION;
            assert(copy_debug(&too_small) == 0);
            // Camera motion invalidates the old GPU shadow mask, even if
            // the old worker reply had already completed.
            state.camera_position[0] += 0.4f;
            assert(frame(&state) == 1);
            assert(copy_debug(&debug) == 0);
            // A change to EEP sun direction invalidates an old GPU sample;
            // future renderer must enqueue new lighting work separately.
            state.sun_direction[0] = 0.5f;
            assert(frame(&state) == 1);
            assert(copy_debug(&debug) == 0);
        } else {
            assert(copy_debug(&debug) == 0);
        }
        // Same geometry: no new GPU invocation.
        assert(submit(tris.data(), tris.size()) == 1);
        assert(attempts() == (missing ? 0u : 1u));
        // Changed geometry: rate limiting prevents immediate second call.
        tris[0].position[0] = 0.15f;
        assert(submit(tris.data(), tris.size()) == 1);
        assert(copy_debug(&debug) == 0); // old mask invalid after mesh change
        assert(attempts() == (missing ? 0u : 1u));
        assert(successes() == (missing ? 0u : 1u));
        assert(submit(nullptr, 0) == 1); // teleport drops all saved mask state
        assert(copy_debug(&debug) == 0);
    } else {
        assert(attempts() == 0);
        assert(successes() == 0);
    }
    assert(status()->rendering_active == 0);
    stop();
    dlclose(library);
    std::puts("PASS: optional GPU diagnostics are isolated; OpenGL remains active");
}

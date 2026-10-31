// Portable GPU lifecycle test: actual logical Vulkan device reuse and
// scene/sun cache. This is diagnostic-only; no OpenGL compositing.
#include "ogl/ogl_gpu_probe_api.h"
#ifdef NDEBUG
#undef NDEBUG // Acceptance checks must not disappear in Release builds.
#endif
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <vector>

int main(int argc, char** argv) {
    if (argc != 3) return 1;
    void* library = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!library) return 1;
    auto run = reinterpret_cast<decltype(&ogl_gpu_probe_run)>(
        dlsym(library, "ogl_gpu_probe_run"));
    if (!run) return 1;
    ogl_rtx_triangle triangle{};
    triangle.position[3] = 1.f;
    triangle.position[7] = 1.f;
    const float sun_a[3]{0.f, 0.f, 1.f};
    const float sun_b[3]{0.f, 1.f, 1.f};
    auto invoke = [&](const float* sun) {
        ogl_gpu_probe_result r{};
        r.struct_size = sizeof(r);
        r.version = OGL_GPU_PROBE_VERSION;
        const int code = run(&triangle, 1, sun, argv[2], &r);
        if (code == 77) std::puts("SKIP: GPU ray-query hardware unavailable");
        else if (code) std::printf("FAIL: probe returned %d\n", code);
        if (code != 0) std::exit(code);
        assert(r.success == 1 && r.ray_count == 64);
        assert(r.triangle_count == 1 && r.occluded <= 64);
        assert(r.mask_width == 8 && r.mask_height == 8);
        uint32_t mask_sum = 0;
        for (auto pixel : r.shadow_mask) {
            assert(pixel <= 1u);
            mask_sum += pixel;
        }
        assert(mask_sum == r.occluded);
        return r;
    };
    const auto first = invoke(sun_a);
    assert(first.vulkan_device_creations == 1);
    assert(first.blas_builds == 1);
    assert(first.cache_hits == 0);
    // A single triangle must produce the exact 8x8 triangular ray mask.
    assert(first.occluded == 36);
    for (unsigned y = 0; y < 8; ++y)
        for (unsigned x = 0; x < 8; ++x)
            assert(first.shadow_mask[y * 8 + x] == (x + y <= 7 ? 1 : 0));
    const auto same = invoke(sun_a);
    assert(same.vulkan_device_creations == 1);
    assert(same.blas_builds == 1);
    assert(same.cache_hits == 1);
    assert(same.occluded == first.occluded);
    assert(std::memcmp(same.shadow_mask, first.shadow_mask, 64) == 0);

    // Lighting changes invalidate the cached rays, not the GPU device.
    const auto new_light = invoke(sun_b);
    assert(new_light.vulkan_device_creations == 1);
    assert(new_light.blas_builds == 1); // unchanged geometry, GPU BLAS/TLAS reused
    assert(new_light.cache_hits == 1);

    // Geometry changes do require another BLAS build at this milestone.
    triangle.position[0] = 0.1f;
    const auto moved = invoke(sun_a);
    assert(moved.vulkan_device_creations == 1);
    assert(moved.blas_builds == 2); // changed geometry rebuilds GPU BLAS

    // Invalid data must not run Vulkan or overwrite the cached valid scene.
    triangle.position[0] = NAN;
    ogl_gpu_probe_result invalid{};
    invalid.struct_size = sizeof(invalid);
    invalid.version = OGL_GPU_PROBE_VERSION;
    assert(run(&triangle, 1, sun_a, argv[2], &invalid) == 1);

    // Largest supported snapshot must reuse the same Vulkan logical device
    // while replacing its old BLAS/TLAS with the new 1,024-triangle scene.
    std::vector<ogl_rtx_triangle> many(OGL_RTX_MAX_SCENE_TRIANGLES);
    for (uint32_t i = 0; i < many.size(); ++i) {
        const float x = float(i % 32) * 2.f;
        const float y = float(i / 32) * 2.f;
        many[i].position[0] = x;
        many[i].position[1] = y;
        many[i].position[3] = x + 1.f;
        many[i].position[4] = y;
        many[i].position[6] = x;
        many[i].position[7] = y + 1.f;
    }
    auto run_many = [&]() {
        ogl_gpu_probe_result r{};
        r.struct_size = sizeof(r);
        r.version = OGL_GPU_PROBE_VERSION;
        assert(run(many.data(), uint32_t(many.size()), sun_a, argv[2], &r) == 0);
        assert(r.success && r.triangle_count == many.size());
        assert(r.mask_width == 8 && r.mask_height == 8);
        uint32_t mask_sum = 0;
        for (auto pixel : r.shadow_mask) {
            assert(pixel <= 1);
            mask_sum += pixel;
        }
        assert(mask_sum == r.occluded);
        return r;
    };
    const auto large = run_many();
    assert(large.vulkan_device_creations == 1);
    assert(large.blas_builds == 3);
    const auto cached_large = run_many();
    assert(cached_large.vulkan_device_creations == 1);
    assert(cached_large.blas_builds == 3);
    assert(cached_large.cache_hits == 2);
    std::printf("PASS: persistent Vulkan device=%u GPU BLAS builds=%u cache hits=%u\n",
        cached_large.vulkan_device_creations, cached_large.blas_builds,
        cached_large.cache_hits);
    dlclose(library);
    return 0;
}

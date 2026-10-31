// Actual GPU test: moving an instance refits TLAS without rebuilding BLAS.
// This is one-instance infrastructure acceptance, not full OpenSim streaming.
#include "ogl/ogl_gpu_probe_api.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <dlfcn.h>
int main(int argc,char** argv) {
    if (argc != 3) return 1;
    void* lib=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);
    if (!lib) return 1;
    auto run=reinterpret_cast<decltype(&ogl_gpu_probe_transform_run)>(
        dlsym(lib,"ogl_gpu_probe_transform_run"));
    assert(run);
    ogl_rtx_triangle tri{};
    tri.position[3]=1.f;
    tri.position[7]=1.f;
    const float sun[3]{0.f,0.f,1.f};
    const float still[3]{0.f,0.f,0.f};
    const float shifted[3]{10.f,0.f,0.f};
    auto invoke=[&](const float* pos) {
        ogl_gpu_probe_result result{};
        result.struct_size=sizeof(result);
        result.version=OGL_GPU_PROBE_VERSION;
        const int code=run(&tri,1,sun,pos,argv[2],&result);
        if (code==77) std::puts("SKIP: no Vulkan ray-query device");
        if (code) std::exit(code);
        assert(result.success && result.triangle_count==1);
        return result;
    };
    const auto before=invoke(still);
    assert(before.occluded==36 && before.blas_builds==1 &&
           before.blas_refits==0 && before.tlas_refits==0);
    const auto moved=invoke(shifted);
    assert(moved.occluded==0 && moved.blas_builds==1 &&
           moved.blas_refits==0 && moved.tlas_refits==1);
    const auto cached=invoke(shifted);
    assert(cached.occluded==0 && cached.tlas_refits==1 &&
           cached.cache_hits==1);
    const auto restored=invoke(still);
    assert(restored.occluded==36 && restored.blas_builds==1 &&
           restored.blas_refits==0 && restored.tlas_refits==2);
    std::puts("PASS: Vulkan TLAS transform-only refit changes ray visibility without BLAS rebuild");
    dlclose(lib);
}

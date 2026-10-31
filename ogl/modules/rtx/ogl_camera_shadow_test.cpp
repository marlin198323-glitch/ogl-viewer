// Real Vulkan test of camera primary rays and secondary sunlight rays.
// This validates ray-query behaviour; no OpenGL image is generated.
#include "ogl/ogl_gpu_probe_api.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <dlfcn.h>
#include <vector>

static void quad(std::vector<ogl_rtx_triangle>& geometry,
                 float minx, float miny, float maxx, float maxy, float z) {
    ogl_rtx_triangle a{};
    const float v0[]{minx,miny,z}, v1[]{maxx,miny,z}, v2[]{minx,maxy,z};
    std::memcpy(a.position, v0, 3*sizeof(float));
    std::memcpy(a.position+3, v1, 3*sizeof(float));
    std::memcpy(a.position+6, v2, 3*sizeof(float));
    geometry.push_back(a);
    ogl_rtx_triangle b{};
    const float w0[]{maxx,miny,z}, w1[]{maxx,maxy,z}, w2[]{minx,maxy,z};
    std::memcpy(b.position, w0, 3*sizeof(float));
    std::memcpy(b.position+3, w1, 3*sizeof(float));
    std::memcpy(b.position+6, w2, 3*sizeof(float));
    geometry.push_back(b);
}
int main(int argc, char** argv) {
    if (argc != 3) return 1;
    void* lib = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!lib) return 1;
    auto run = reinterpret_cast<decltype(&ogl_gpu_probe_camera_run)>(
        dlsym(lib, "ogl_gpu_probe_camera_run"));
    assert(run);
    ogl_rtx_frame_state camera{};
    camera.struct_size = sizeof(camera);
    camera.frame_version = OGL_RTX_FRAME_VERSION;
    camera.width = 800;
    camera.height = 800;
    camera.camera_position[2] = 2.f;
    camera.camera_forward[2] = -1.f;
    camera.camera_right[0] = 1.f;
    camera.camera_up[1] = 1.f;
    camera.vertical_fov_radians = 1.04719755f;
    camera.aspect_ratio = 1.f;
    camera.near_clip_m = 0.1f;
    camera.far_clip_m = 100.f;
    camera.sun_direction[0] = 0.8f;
    camera.sun_direction[2] = 1.f;

    std::vector<ogl_rtx_triangle> scene;
    quad(scene,-3.f,-3.f,3.f,3.f,0.f);
    auto probe = [&]() {
        ogl_gpu_probe_result r{};
        r.struct_size=sizeof(r);
        r.version=OGL_GPU_PROBE_VERSION;
        const int rc=run(scene.data(),static_cast<uint32_t>(scene.size()),
                         &camera,argv[2],&r);
        if (rc == 77) std::puts("SKIP: Vulkan ray queries unsupported");
        if (rc) std::exit(rc);
        assert(r.success && r.mask_width==8 && r.mask_height==8);
        unsigned total=0;
        for (auto v:r.shadow_mask) {assert(v<=1);total+=v;}
        assert(total==r.occluded);
        return r;
    };
    const auto base=probe();
    assert(base.occluded==0); // floor alone cannot shadow itself
    const auto cached=probe();
    assert(cached.cache_hits==1 && cached.blas_builds==1);
    // Occluder elevated above floor casts an offset hard sun shadow.
    quad(scene,-0.4f,-0.4f,0.4f,0.4f,0.5f);
    const auto blocked=probe();
    std::printf("CAMERA SHADOW: %u occluded pixels of 64\n",blocked.occluded);
    assert(blocked.occluded>0 && blocked.occluded<64);
    assert(blocked.blas_builds==2);
    camera.camera_position[0] += 0.5f;
    const auto moved=probe();
    assert(moved.vulkan_device_creations==1);
    assert(moved.blas_builds==2);
    assert(moved.cache_hits==1); // moving camera must invalidate result
    // Real Firestorm camera can roll, zoom and change aspect ratio.
    // All of these must change GPU rays without rebuilding scene BLAS.
    camera.camera_right[0] = 0.f;
    camera.camera_right[1] = 1.f;
    camera.camera_up[0] = -1.f;
    camera.camera_up[1] = 0.f;
    camera.vertical_fov_radians = 0.82f;
    camera.width = 1600;
    camera.height = 800;
    camera.aspect_ratio = 2.f;
    const auto rolled=probe();
    assert(rolled.blas_builds==2 && rolled.cache_hits==1);
    assert(rolled.vulkan_device_creations==1);
    const auto rolled_cached=probe();
    assert(rolled_cached.cache_hits==2 && rolled_cached.blas_builds==2);
    auto broken=camera;
    broken.camera_forward[2]=0.f;
    ogl_gpu_probe_result out{};
    out.struct_size=sizeof(out);out.version=OGL_GPU_PROBE_VERSION;
    assert(run(scene.data(),static_cast<uint32_t>(scene.size()),
               &broken,argv[2],&out)==1);
    std::puts("PASS: perspective camera and secondary sun rays, cached update and invalid frame");
    dlclose(lib);
}

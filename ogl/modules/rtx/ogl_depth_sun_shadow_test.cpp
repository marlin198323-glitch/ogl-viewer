// Vulkan ray-query test reading an actual 64-float deferred-depth grid.
// Synthetic depth is independently generated from a conventional GL
// perspective projection, not fabricated shadow-hit flags.
#include "ogl/ogl_gpu_probe_api.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <dlfcn.h>
#include <vector>
#include <array>
static void quad(std::vector<ogl_rtx_triangle>& geometry,
                 float minx, float miny, float maxx, float maxy, float z) {
    ogl_rtx_triangle a{}, b{};
    const float points[4][3]={{minx,miny,z},{maxx,miny,z},
                              {minx,maxy,z},{maxx,maxy,z}};
    for (unsigned v=0;v<3;++v) {
        std::memcpy(a.position+3*v,points[v],3*sizeof(float));
        const unsigned index[]={1u,3u,2u};
        std::memcpy(b.position+3*v,points[index[v]],3*sizeof(float));
    }
    geometry.push_back(a);
    geometry.push_back(b);
}
int main(int argc, char** argv) {
    if (argc != 3) return 1;
    void* library=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);
    if (!library) return 1;
    auto trace=reinterpret_cast<decltype(&ogl_gpu_probe_depth_run)>(
        dlsym(library,"ogl_gpu_probe_depth_run"));
    assert(trace);
    ogl_rtx_frame_state frame{};
    frame.struct_size=sizeof(frame);
    frame.frame_version=OGL_RTX_FRAME_VERSION;
    frame.width=800;frame.height=800;
    frame.camera_position[2]=2.f;
    frame.camera_forward[2]=-1.f;
    frame.camera_right[0]=1.f;
    frame.camera_up[1]=1.f;
    frame.sun_direction[0]=0.8f;
    frame.sun_direction[2]=1.f;
    frame.vertical_fov_radians=1.04719755f;
    frame.aspect_ratio=1.f;
    frame.near_clip_m=0.1f;
    frame.far_clip_m=100.f;
    const float depth_of_plane=(frame.far_clip_m/(frame.far_clip_m-frame.near_clip_m)) *
        (1.f-frame.near_clip_m/2.f);
    std::array<float,64> depth{};
    depth.fill(depth_of_plane);
    std::vector<ogl_rtx_triangle> scene;
    quad(scene,-3.f,-3.f,3.f,3.f,0.f);
    auto run=[&]() {
        ogl_gpu_probe_result output{};
        output.struct_size=sizeof(output);
        output.version=OGL_GPU_PROBE_VERSION;
        const int code=trace(scene.data(),uint32_t(scene.size()),&frame,
                             depth.data(),argv[2],&output);
        if (code==77) std::puts("SKIP: hardware lacks Vulkan ray query");
        if (code) std::exit(code);
        assert(output.success && output.mask_width==8 && output.mask_height==8);
        unsigned hits=0;
        for (auto pixel: output.shadow_mask) {assert(pixel<=1);hits+=pixel;}
        assert(hits==output.occluded);
        return output;
    };
    const auto no_blocker=run();
    assert(no_blocker.occluded==0);
    assert(no_blocker.primary_hits==64 && no_blocker.primary_depth_matches==64);
    assert(no_blocker.primary_closer==0 && no_blocker.primary_farther==0);
    // GPU scene contains the far floor, but the simulated OpenGL buffer
    // has a closer surface that GPU does NOT know about.
    const auto cached=run();
    assert(cached.cache_hits==1 && cached.blas_builds==1);
    const float nearer=(frame.far_clip_m/(frame.far_clip_m-frame.near_clip_m)) *
        (1.f-frame.near_clip_m/1.5f);
    depth.fill(nearer);
    const auto missing_front_geometry=run();
    assert(missing_front_geometry.primary_hits==64);
    assert(missing_front_geometry.primary_farther==64);
    assert(missing_front_geometry.primary_closer==0 &&
           missing_front_geometry.primary_depth_matches==0);
    depth.fill(depth_of_plane);
    quad(scene,-0.4f,-0.4f,0.4f,0.4f,0.5f);
    const auto shadow=run();
    std::printf("DEPTH GUIDED SHADOW: %u/64 hits\n",shadow.occluded);
    assert(shadow.occluded>0 && shadow.occluded<64);
    assert(shadow.primary_hits==64);
    assert(shadow.primary_depth_matches<shadow.primary_hits);
    assert(shadow.primary_closer>0 && shadow.primary_farther==0);
    assert(shadow.primary_hits==shadow.primary_depth_matches+
           shadow.primary_closer+shadow.primary_farther);
    assert(shadow.blas_builds==2);
    // Sparse sky/transparent areas: a test pixel with depth=1 may never
    // claim shadow, while opaque neighbouring samples still run ray queries.
    // This is essential when OpenSim's viewport includes sky.
    depth.fill(depth_of_plane);
    depth[0] = 1.f;
    depth[16] = 1.f;
    depth[28] = 1.f;
    const auto mixed=run();
    assert(mixed.shadow_mask[0]==0 && mixed.shadow_mask[16]==0 &&
           mixed.shadow_mask[28]==0);
    assert(mixed.occluded<=shadow.occluded && mixed.blas_builds==2);
    // GL background depth must never be marked as a world shadow.
    depth.fill(1.f);
    const auto background=run();
    assert(background.occluded==0 && background.blas_builds==2);
    assert(background.primary_hits==0 && background.primary_depth_matches==0);
    // A depth change with the exact same mesh and camera must invalidate
    // the GPU result cache without rebuilding the BLAS.
    depth.fill(depth_of_plane);
    const auto restored=run();
    assert(restored.occluded==shadow.occluded && restored.blas_builds==2);
    assert(restored.cache_hits==1);
    auto bad=depth;
    bad[17]=NAN;
    ogl_gpu_probe_result output{};
    output.struct_size=sizeof(output);
    output.version=OGL_GPU_PROBE_VERSION;
    assert(trace(scene.data(),uint32_t(scene.size()),&frame,
                 bad.data(),argv[2],&output)==1);
    assert(trace(scene.data(),uint32_t(scene.size()),&frame,
                 nullptr,argv[2],&output)==1);
    std::puts("PASS: depth-guided GPU sun shadow, background, cache and invalid pixels");
    dlclose(library);
}

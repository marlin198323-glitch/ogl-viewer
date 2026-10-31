// Real Vulkan 128x72 ray-query regression, entirely independent of 8x8.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "ogl/ogl_gpu_probe_api.h"
#include <dlfcn.h>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
#include <array>
static void quad(std::vector<ogl_rtx_triangle>& scene,
                 float x0,float y0,float x1,float y1,float z) {
    ogl_rtx_triangle a{},b{};
    const float pts[4][3]{{x0,y0,z},{x1,y0,z},{x0,y1,z},{x1,y1,z}};
    for (unsigned v=0;v<3;++v) {
        std::memcpy(a.position+v*3,pts[v],sizeof(pts[v]));
        const unsigned idx[]{1,3,2};
        std::memcpy(b.position+v*3,pts[idx[v]],sizeof(pts[v]));
    }
    scene.push_back(a);scene.push_back(b);
}
int main(int argc,char** argv) {
    if (argc!=3) return 1;
    void* lib=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);
    assert(lib);
    auto trace=reinterpret_cast<decltype(&ogl_gpu_probe_dense_run)>(
        dlsym(lib,"ogl_gpu_probe_dense_run"));
    assert(trace);
    ogl_rtx_frame_state cam{};
    cam.struct_size=sizeof(cam);
    cam.frame_version=OGL_RTX_FRAME_VERSION;
    cam.width=1920;cam.height=1080;
    cam.camera_position[2]=2.f;
    cam.camera_forward[2]=-1.f;
    cam.camera_right[0]=1.f;
    cam.camera_up[1]=1.f;
    cam.sun_direction[0]=0.8f;cam.sun_direction[2]=1.f;
    cam.vertical_fov_radians=1.04719755f;
    cam.aspect_ratio=float(cam.width)/cam.height;
    cam.near_clip_m=0.1f;cam.far_clip_m=100.f;
    const float plane_depth=(cam.far_clip_m/(cam.far_clip_m-cam.near_clip_m))*
        (1.f-cam.near_clip_m/2.f);
    std::array<float,OGL_GPU_DENSE_PIXELS> depth{};
    depth.fill(plane_depth);
    std::vector<ogl_rtx_triangle> scene;
    quad(scene,-5.f,-5.f,5.f,5.f,0.f);
    auto run=[&]() {
        ogl_gpu_dense_result out{};
        out.struct_size=sizeof(out);
        out.version=OGL_GPU_DENSE_VERSION;
        const int rc=trace(scene.data(),uint32_t(scene.size()),&cam,
                           depth.data(),argv[2],&out);
        if (rc==77) return out;
        assert(rc==0 && out.success==1);
        assert(out.width==OGL_GPU_DENSE_WIDTH && out.height==OGL_GPU_DENSE_HEIGHT);
        assert(out.ray_count==OGL_GPU_DENSE_PIXELS);
        assert(out.primary_hits>=out.depth_matches);
        assert(out.occluded<=out.depth_matches);
        unsigned shadows=0u,matches=0u;
        for (unsigned i=0;i<OGL_GPU_DENSE_PIXELS;++i) {
            assert(out.primary_mask[i]<=1u);
            assert(out.depth_match_mask[i]<=out.primary_mask[i]);
            assert(out.shadow_mask[i]<=out.depth_match_mask[i]);
            shadows+=out.shadow_mask[i];
            matches+=out.depth_match_mask[i];
        }
        assert(shadows==out.occluded && matches==out.depth_matches);
        return out;
    };
    auto empty=run();
    if (!empty.success) {
        std::puts("SKIP: no Vulkan ray-query device");
        return 77;
    }
    assert(empty.occluded==0);
    assert(empty.depth_matches>8000u);
    quad(scene,-0.45f,-0.45f,0.45f,0.45f,0.5f);
    auto blocked=run();
    assert(blocked.occluded>0u);
    assert(blocked.occluded<blocked.depth_matches);
    std::printf("PASS: dense 128x72 Vulkan shadow pixels %u/%u, "
                "full GL depth matches %u\n",blocked.occluded,
                blocked.ray_count,blocked.depth_matches);
    depth.fill(1.f);
    auto sky=run();
    assert(sky.occluded==0 && sky.depth_matches==0);
    for(unsigned i=0;i<OGL_GPU_DENSE_PIXELS;++i)
        assert(sky.shadow_mask[i]==0 && sky.depth_match_mask[i]==0);
    depth[15]=NAN;
    ogl_gpu_dense_result invalid{};
    invalid.struct_size=sizeof(invalid);
    invalid.version=OGL_GPU_DENSE_VERSION;
    assert(trace(scene.data(),uint32_t(scene.size()),&cam,
                 depth.data(),argv[2],&invalid)==1);
    dlclose(lib);
    return 0;
}

#ifndef OGL_SCENE_DEPTH_COMPARE_H
#define OGL_SCENE_DEPTH_COMPARE_H

// Independent CPU reference probe for the development 8x8 Vulkan comparison.
// This proves whether a wrong GL depth is already explained by missing CPU
// mesh coverage rather than incorrectly assuming a Vulkan ray-query fault.
// 64 samples; caller must enforce the real scene triangle budget.
#include "ogl_scene_ray_visibility.h"
#include "ogl_depth_diagnostics.h"
#include <algorithm>
#include <cstdint>
#include <cmath>

namespace ogl {
struct SceneDepthAgreement {
    uint32_t surface=0;
    uint32_t background=0;
    uint32_t primary_hits=0;
    uint32_t matching=0;
    uint32_t closer=0;
    uint32_t farther=0;
    uint32_t ray_samples=0;
};
inline bool compare_cpu_scene_to_gl_depth(const ogl_rtx_triangle* scene,
    uint32_t count, const float* depth, const float* eye,
    const float* forward, const float* right, const float* up,
    float vertical_fov, float aspect, float near_clip, float far_clip,
    SceneDepthAgreement& result) {
    if (!depth || !eye || !forward || !right || !up ||
        (!scene && count != 0) || count > 8192 ||
        !std::isfinite(vertical_fov) || vertical_fov <=0.f ||
        vertical_fov >=3.14f || !std::isfinite(aspect) || aspect<=0.f ||
        !std::isfinite(near_clip) || !std::isfinite(far_clip) ||
        near_clip<0.005f || far_clip<=near_clip) return false;
    float forward_length_sq=0.f;
    for (unsigned j=0;j<3;++j) {
        if (!std::isfinite(forward[j]) || !std::isfinite(eye[j]) ||
            !std::isfinite(right[j]) || !std::isfinite(up[j]))
            return false;
        forward_length_sq+=forward[j]*forward[j];
    }
    if (forward_length_sq < 0.5f || forward_length_sq > 1.5f)
        return false;
    const float inv_forward=1.f/std::sqrt(forward_length_sq);
    const auto rays=camera_probe_rays(eye,forward,right,up,
                                      vertical_fov,aspect);
    SceneRayHits nearest{};
    for (uint32_t t=0;t<count;++t) {
        if (is_valid_triangle(scene[t]))
            retain_nearest_camera_triangles(rays,nearest,scene[t]);
    }
    SceneDepthAgreement out{};
    out.ray_samples=count_camera_hit_samples(nearest);
    for (unsigned i=0;i<64;++i) {
        float view_depth=0.f;
        if (!gl_depth_to_view_distance(
                depth[i],near_clip,far_clip,view_depth)) {
            if (!std::isfinite(depth[i]) || depth[i]<0.f || depth[i]>1.f)
                return false;
            ++out.background;
            continue;
        }
        ++out.surface;
        float cosine=0.f;
        for (unsigned axis=0;axis<3;++axis)
            cosine+=rays[i].direction[axis]*forward[axis]*inv_forward;
        if (cosine<=0.001f) continue;
        const float gl_distance=view_depth/cosine;
        if (!std::isfinite(nearest[i].distance)) continue;
        ++out.primary_hits;
        const float diff=nearest[i].distance-gl_distance;
        const float epsilon=std::max(0.25f,0.02f*gl_distance);
        if (std::fabs(diff)<=epsilon) ++out.matching;
        else if (diff<0.f) ++out.closer;
        else ++out.farther;
    }
    result=out;
    return true;
}
} // namespace ogl
#endif

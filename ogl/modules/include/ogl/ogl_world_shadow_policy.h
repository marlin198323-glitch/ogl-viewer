#ifndef OGL_WORLD_SHADOW_POLICY_H
#define OGL_WORLD_SHADOW_POLICY_H

#include "ogl_rtx_api.h"
#include <cmath>
#include <cstdint>

// Independent CPU pre-composition guards for a future real GPU->OpenGL
// shadow pass. No OpenGL/Vulkan; rejects any unbound/mismatched snapshot.
namespace ogl {
// A native Vulkan ray query can be a valid *diagnostic* even when its
// output cannot be trusted as a world-shadow sample. Keep those decisions
// independent: never disable the GPU worker solely because a new rendering
// policy rejects some optional world-image pixels.
inline bool world_shadow_mask_matches_depth(
    const uint8_t* shadows, const uint8_t* depth_matches,
    uint32_t count=64u) {
    if (!shadows || !depth_matches || count!=64u) return false;
    for (uint32_t i=0;i<count;++i)
        if (shadows[i]>1u || depth_matches[i]>1u ||
            (shadows[i] && !depth_matches[i]))
            return false;
    return true;
}

inline bool same_rt_frame(const ogl_rtx_frame_state& capture,
                          const ogl_rtx_frame_state& current) {
    if (capture.struct_size!=sizeof(ogl_rtx_frame_state) ||
        current.struct_size!=sizeof(ogl_rtx_frame_state) ||
        capture.frame_version!=OGL_RTX_FRAME_VERSION ||
        current.frame_version!=OGL_RTX_FRAME_VERSION ||
        capture.width!=current.width || capture.height!=current.height ||
        capture.width<8 || capture.height<8)
        return false;
    // Strict opt-in still-frame prototype; NOT temporal reprojection.
    auto same=[](float a,float b) {
        return std::isfinite(a) && std::isfinite(b) &&
               std::fabs(a-b)<=0.00001f;
    };
    for (unsigned i=0;i<3;++i) {
        if (!same(capture.camera_position[i],current.camera_position[i]) ||
            !same(capture.camera_forward[i],current.camera_forward[i]) ||
            !same(capture.camera_right[i],current.camera_right[i]) ||
            !same(capture.camera_up[i],current.camera_up[i]) ||
            !same(capture.sun_direction[i],current.sun_direction[i]))
            return false;
    }
    return same(capture.vertical_fov_radians,current.vertical_fov_radians) &&
           same(capture.aspect_ratio,current.aspect_ratio) &&
           same(capture.near_clip_m,current.near_clip_m) &&
           same(capture.far_clip_m,current.far_clip_m);
}

inline bool valid_world_shadow_sample(
    const ogl_rtx_world_shadow_sample& sample,
    uint64_t current_geometry_revision,
    const ogl_rtx_frame_state& current_frame) {
    if (sample.struct_size!=sizeof(sample) ||
        sample.version!=OGL_RTX_WORLD_SHADOW_VERSION ||
        sample.sample_sequence==0 || sample.geometry_revision==0 ||
        sample.geometry_revision!=current_geometry_revision ||
        !same_rt_frame(sample.capture_frame,current_frame) ||
        sample.matched_surface_pixels>64u || sample.occluded_pixels>64u ||
        sample.occluded_pixels>sample.matched_surface_pixels)
        return false;
    uint32_t matched=0u, shadowed=0u;
    for (unsigned i=0;i<64;++i) {
        const uint8_t valid=sample.valid_mask[i];
        const uint8_t shadow=sample.occlusion_mask[i];
        if (valid>1 || shadow>1 || shadow>valid ||
            !std::isfinite(sample.capture_depth[i]) ||
            sample.capture_depth[i]<0.f ||
            sample.capture_depth[i]>1.f)
            return false;
        if (valid && sample.capture_depth[i]>=0.999999f)
            return false; // sky cannot be a receiver
        matched+=valid;
        shadowed+=shadow;
    }
    return matched==sample.matched_surface_pixels &&
           shadowed==sample.occluded_pixels;
}

// The only safe way to apply a captured shadow bit to a *current* surface
// until a per-frame dense renderer is implemented: current GL depth must
// match the sampled source depth. Never interpolate 8x8 occlusion across
// different objects/surfaces or into sky.
inline bool world_shadow_pixel_eligible(
    const ogl_rtx_world_shadow_sample& sample,
    unsigned index, float current_depth, float tolerance=0.00001f) {
    return index<64u && sample.valid_mask[index]==1 &&
           sample.occlusion_mask[index]==1 &&
           std::isfinite(current_depth) &&
           std::isfinite(tolerance) && tolerance>=0.f &&
           current_depth>=0.f && current_depth<0.999999f &&
           std::fabs(current_depth-sample.capture_depth[index])<=tolerance;
}
} // namespace ogl
#endif

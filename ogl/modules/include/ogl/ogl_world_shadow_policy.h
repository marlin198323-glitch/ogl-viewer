#ifndef OGL_WORLD_SHADOW_POLICY_H
#define OGL_WORLD_SHADOW_POLICY_H

#include "ogl_rtx_api.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

// Independent CPU pre-composition guards for a future real GPU->OpenGL
// shadow pass. No OpenGL/Vulkan; rejects any unbound/mismatched snapshot.
namespace ogl {
// The viewer performs GL swaps for login, teleport, logout and recovery
// screens too. Only a truly visible, authenticated, non-transitioning
// world frame may establish the last-working world renderer OR trigger a
// private post-swap Vulkan sample. This guard never selects Vulkan.
inline bool visible_world_swap_eligible(
    bool startup_complete, bool has_world_region,
    bool teleport_overlay, bool logout_pending,
    bool gl_restore_pending, bool app_exiting,
    bool window_visible, bool window_minimized) noexcept {
    return startup_complete && has_world_region &&
        !teleport_overlay && !logout_pending &&
        !gl_restore_pending && !app_exiting &&
        window_visible && !window_minimized;
}

// Bound private Vulkan diagnostics after repeated in-world GPU failures.
// This is a recovery throttle, NOT a change to OpenGL world frame pacing.
// Base cadence remains one eligible GPU attempt per 120 visible swaps.
// Failed attempts back off 120 -> 240 -> 480 -> 960 world swaps and
// remain retryable; a new module/region resets the failure state.
inline uint32_t present_coupled_retry_delay(uint32_t failures) noexcept {
    if (!failures) return 0u;
    const uint32_t capped=std::min(failures,4u);
    return 120u << (capped-1u);
}
inline bool present_coupled_retry_due(
    uint64_t current_world_swaps, uint64_t last_failed_swap,
    uint32_t failures) noexcept {
    return failures==0u ||
        (current_world_swaps>=last_failed_swap &&
         current_world_swaps-last_failed_swap>=
             present_coupled_retry_delay(failures));
}

// P05/3: source-identity guards at the actual GL-world-swap boundary.
// This is a CPU-only policy; no GL/Vulkan window ownership is implied.
// Every rendered GPU sample must belong to a genuine deferred receipt,
// a matching region epoch and the same GPU mesh/camera generation.
inline bool present_coupled_source_current(
    uint64_t region_epoch, uint64_t capture_epoch, uint64_t capture_receipt,
    uint64_t capture_mesh, uint64_t gpu_mesh, uint64_t gpu_camera_mesh) noexcept {
    return capture_receipt!=0 && region_epoch==capture_epoch &&
           gpu_mesh!=0 && capture_mesh==gpu_mesh &&
           gpu_camera_mesh==gpu_mesh;
}

// A private Vulkan image counts as evidence for the *visible-world-swap*
// integration only when the actual GPU draw completed, the original
// source receipt/region/mesh survived the draw unchanged, and readback
// contains bounded non-clear pixels. Otherwise a blank or stale private
// image must never be reported as a new world-rendering success.
inline bool present_coupled_gpu_evidence_current(
    uint64_t before_epoch, uint64_t before_receipt, uint64_t before_mesh,
    uint64_t current_epoch, uint64_t current_capture_epoch,
    uint64_t current_receipt, uint64_t current_capture_mesh,
    uint64_t current_gpu_mesh, uint64_t current_camera_mesh,
    uint64_t before_frame, uint64_t after_frame,
    uint32_t width, uint32_t height, uint32_t nonclear_pixels) noexcept {
    constexpr uint32_t max_private_extent = 4096u;
    return before_receipt!=0 && before_mesh!=0 &&
        before_epoch==current_epoch && before_receipt==current_receipt &&
        before_mesh==current_capture_mesh &&
        present_coupled_source_current(current_epoch,current_capture_epoch,
            current_receipt,current_capture_mesh,current_gpu_mesh,
            current_camera_mesh) &&
        before_frame<UINT64_MAX && after_frame==before_frame+1u &&
        width>=8u && height>=8u &&
        width<=max_private_extent && height<=max_private_extent &&
        nonclear_pixels>0u &&
        static_cast<uint64_t>(nonclear_pixels)<=
            static_cast<uint64_t>(width)*height;
}

// Identical pose and mesh do NOT prove that the captured real-world
// material/source data stayed unchanged: the deferred receipt also matters.
inline bool present_coupled_needs_redraw(
    bool old_pose_valid, uint64_t old_epoch, uint64_t current_epoch,
    uint64_t old_mesh, uint64_t current_mesh,
    uint64_t old_receipt, uint64_t current_receipt,
    bool same_pose) noexcept {
    return current_receipt!=0 &&
        (!old_pose_valid || old_epoch!=current_epoch ||
         old_mesh!=current_mesh || old_receipt!=current_receipt || !same_pose);
}

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

// Validate the first denser GPU ray-result tile. The receiver must also
// compare this capture against the live deferred-depth texture and reject
// surface/edge discontinuities; validating this ABI is necessary but not
// sufficient to render a world-space shadow.
inline bool valid_dense_shadow(
    const ogl_rtx_dense_shadow& sample,uint64_t current_revision,
    const ogl_rtx_frame_state& current) {
    if (sample.struct_size!=sizeof(sample) ||
        sample.version!=OGL_RTX_DENSE_VERSION ||
        sample.width!=OGL_RTX_DENSE_WIDTH ||
        sample.height!=OGL_RTX_DENSE_HEIGHT ||
        sample.sample_sequence==0 ||
        sample.geometry_revision==0 ||
        sample.geometry_revision!=current_revision ||
        !same_rt_frame(sample.capture_frame,current) ||
        sample.matched_pixels>OGL_RTX_DENSE_PIXELS ||
        sample.shadow_pixels>sample.matched_pixels ||
        sample.surface_pixels>OGL_RTX_DENSE_PIXELS)
        return false;
    uint32_t matched=0,shadow=0,surface=0;
    for (unsigned i=0;i<OGL_RTX_DENSE_PIXELS;++i) {
        const float depth=sample.capture_depth[i];
        if (!std::isfinite(depth) || depth<0.f || depth>1.f)
            return false;
        const auto valid=sample.valid_mask[i];
        const auto occlusion=sample.shadow_mask[i];
        if (valid>1u || occlusion>valid) return false;
        if (valid && depth>=0.999999f) return false;
        matched+=valid;
        shadow+=occlusion;
        surface+=depth<0.999999f;
    }
    return matched==sample.matched_pixels &&
           shadow==sample.shadow_pixels &&
           surface==sample.surface_pixels;
}
inline bool dense_shadow_pixel_current(
    const ogl_rtx_dense_shadow& sample,uint32_t pixel,
    float current_depth,float tolerance=0.00001f) {
    return pixel<OGL_RTX_DENSE_PIXELS &&
           sample.valid_mask[pixel]==1 &&
           sample.shadow_mask[pixel]==1 &&
           std::isfinite(current_depth) &&
           current_depth>=0.f && current_depth<0.999999f &&
           std::isfinite(tolerance) && tolerance>=0.f &&
           std::fabs(current_depth-sample.capture_depth[pixel])<=tolerance;
}

// A 128x72 captured depth represents ONE texel per 27x19 output pixels
// at 3440x1390. Comparing raw nonlinear depth with epsilon=1e-5
// incorrectly rejects pixels on the SAME sloped ground/wall within
// that texel. Compare eye-space distances instead and bound the
// conservative acceptance by 0.25m..1.5m (2% with distance).
// This is still a coarse diagnostic: mesh/object discontinuities
// require a higher-res material ID/normal buffer later.
inline float rt_view_depth(float depth,float near_m,float far_m) {
    if (!std::isfinite(depth) || !std::isfinite(near_m) ||
        !std::isfinite(far_m) || depth<0.f || depth>=0.999999f ||
        near_m<=0.f || far_m<=near_m) return 0.f;
    const float den=far_m+near_m-(2.f*depth-1.f)*(far_m-near_m);
    if (!std::isfinite(den) || den<=0.f) return 0.f;
    const float d=2.f*near_m*far_m/den;
    return std::isfinite(d) && d>0.f ? d : 0.f;
}
inline bool dense_view_depth_matches(float captured,float current,
                                    float near_m,float far_m) {
    const float original=rt_view_depth(captured,near_m,far_m);
    const float now=rt_view_depth(current,near_m,far_m);
    if (original<=0.f || now<=0.f) return false;
    const float tolerance=std::clamp(original*0.02f,0.25f,1.5f);
    return std::fabs(now-original)<=tolerance;
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

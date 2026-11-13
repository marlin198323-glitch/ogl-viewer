#ifdef NDEBUG
#undef NDEBUG
#endif
#include "ogl/ogl_world_shadow_policy.h"
#include <cassert>
#include <cmath>
#include <limits>
int main() {
    // The real SDL/OpenGL swap occurs for UI and world frames alike.
    // Never mark last-working-engine or trigger Vulkan based on UI frames.
    const auto visible_world=[](bool startup,bool region,bool teleport,
        bool logout,bool restore,bool exiting,bool visible,bool minimized) {
        return ogl::visible_world_swap_eligible(startup,region,teleport,
            logout,restore,exiting,visible,minimized);
    };
    assert(visible_world(true,true,false,false,false,false,true,false));
    assert(!visible_world(false,true,false,false,false,false,true,false));
    assert(!visible_world(true,false,false,false,false,false,true,false));
    assert(!visible_world(true,true,true,false,false,false,true,false));
    assert(!visible_world(true,true,false,true,false,false,true,false));
    assert(!visible_world(true,true,false,false,true,false,true,false));
    assert(!visible_world(true,true,false,false,false,true,true,false));
    assert(!visible_world(true,true,false,false,false,false,false,false));
    assert(!visible_world(true,true,false,false,false,false,true,true));
    // Live source may become valid at arbitrarily low FPS. The first
    // qualified GL world swap can trigger the private GPU immediately;
    // subsequent synchronous attempts obey a >=120 world-swap gap.
    assert(!ogl::present_coupled_sample_due(0,0));
    assert(ogl::present_coupled_sample_due(1,0));
    assert(ogl::present_coupled_sample_due(119,0));
    assert(ogl::present_coupled_sample_due(120,0));
    assert(!ogl::present_coupled_sample_due(1,1));
    assert(!ogl::present_coupled_sample_due(2,1));
    assert(!ogl::present_coupled_sample_due(120,1));
    assert(ogl::present_coupled_sample_due(121,1));
    assert(!ogl::present_coupled_sample_due(240,121));
    assert(ogl::present_coupled_sample_due(241,121));
    assert(!ogl::present_coupled_sample_due(100,101));
    assert(ogl::present_coupled_sample_due(UINT64_MAX,UINT64_MAX-120u));
    assert(!ogl::present_coupled_sample_due(UINT64_MAX,UINT64_MAX-119u));
    assert(ogl::present_coupled_sample_due(1,0) &&
           ogl::present_coupled_retry_due(1,0,0));
    assert(!ogl::present_coupled_retry_due(121,1,2));
    assert(ogl::present_coupled_retry_due(241,1,2));
    // Real-world Vulkan sampling uses an exponential recovery throttle.
    // The visible OpenGL swap never enters this policy.
    assert(ogl::present_coupled_retry_delay(0)==0);
    assert(ogl::present_coupled_retry_delay(1)==120);
    assert(ogl::present_coupled_retry_delay(2)==240);
    assert(ogl::present_coupled_retry_delay(3)==480);
    assert(ogl::present_coupled_retry_delay(4)==960);
    assert(ogl::present_coupled_retry_delay(1000)==960);
    assert(ogl::present_coupled_retry_due(1,0,0));
    assert(!ogl::present_coupled_retry_due(120,1,1));
    assert(ogl::present_coupled_retry_due(121,1,1));
    assert(!ogl::present_coupled_retry_due(360,121,2));
    assert(ogl::present_coupled_retry_due(361,121,2));
    assert(!ogl::present_coupled_retry_due(840,361,3));
    assert(ogl::present_coupled_retry_due(841,361,3));
    assert(!ogl::present_coupled_retry_due(1800,841,4));
    assert(ogl::present_coupled_retry_due(1801,841,4));
    assert(!ogl::present_coupled_retry_due(10,50,3));
    assert(ogl::present_coupled_retry_due(10,50,0));
    // These are the actual native, platform-neutral gates consumed by the
    // post-visible-world-swap Vulkan worker; no live user session needed.
    assert(ogl::present_coupled_source_current(3,3,1,11,11,11));
    assert(!ogl::present_coupled_source_current(3,3,0,11,11,11));
    assert(!ogl::present_coupled_source_current(3,4,1,11,11,11));
    assert(!ogl::present_coupled_source_current(3,3,1,10,11,11));
    assert(!ogl::present_coupled_source_current(3,3,1,11,0,0));
    assert(!ogl::present_coupled_source_current(3,3,1,11,11,10));
    assert(ogl::present_coupled_needs_redraw(false,3,3,11,11,4,4,true));
    assert(ogl::present_coupled_needs_redraw(true,2,3,11,11,4,4,true));
    assert(ogl::present_coupled_needs_redraw(true,3,3,10,11,4,4,true));
    assert(ogl::present_coupled_needs_redraw(true,3,3,11,11,3,4,true));
    assert(ogl::present_coupled_needs_redraw(true,3,3,11,11,4,4,false));
    assert(!ogl::present_coupled_needs_redraw(true,3,3,11,11,4,4,true));
    assert(!ogl::present_coupled_needs_redraw(false,3,3,11,11,0,0,false));
    // GPU proof is stricter than a successful VkQueueSubmit: same
    // source across the draw, bounded dimensions, and non-clear pixels.
    const auto evidence=[](uint64_t old_epoch,uint64_t old_receipt,
        uint64_t old_mesh,uint64_t epoch,uint64_t capture_epoch,
        uint64_t receipt,uint64_t capture_mesh,uint64_t gpu_mesh,
        uint64_t camera_mesh,uint64_t before,uint64_t after,
        uint32_t width,uint32_t height,uint32_t pixels) {
        return ogl::present_coupled_gpu_evidence_current(
            old_epoch,old_receipt,old_mesh,epoch,capture_epoch,receipt,
            capture_mesh,gpu_mesh,camera_mesh,before,after,
            width,height,pixels);
    };
    assert(evidence(3,4,11,3,3,4,11,11,11,9,10,32,32,500));
    assert(!evidence(3,4,11,3,3,4,11,11,11,9,10,32,32,0));
    assert(!evidence(3,4,11,3,3,4,11,11,11,9,10,32,32,1025));
    assert(!evidence(3,4,11,3,3,4,11,11,11,9,10,7,32,50));
    assert(!evidence(3,4,11,3,3,4,11,11,11,9,10,32,0,50));
    assert(!evidence(3,4,11,3,3,4,11,11,11,9,10,4097,32,50));
    assert(!evidence(3,4,11,3,3,4,11,11,11,9,9,32,32,50));
    assert(!evidence(3,4,11,3,3,4,11,11,11,UINT64_MAX,0,32,32,50));
    assert(!evidence(3,4,11,4,4,4,11,11,11,9,10,32,32,50));
    assert(!evidence(3,4,11,3,3,5,11,11,11,9,10,32,32,50));
    assert(!evidence(3,4,11,3,3,4,12,12,12,9,10,32,32,50));
    assert(!evidence(3,4,11,3,3,4,11,11,12,9,10,32,32,50));
    assert(!evidence(3,4,11,3,2,4,11,11,11,9,10,32,32,50));
    assert(!evidence(3,0,11,3,3,0,11,11,11,9,10,32,32,50));
    ogl_rtx_frame_state frame{};
    frame.struct_size=sizeof(frame);
    frame.frame_version=OGL_RTX_FRAME_VERSION;
    frame.width=1920;
    frame.height=1080;
    frame.camera_forward[2]=-1.f;
    frame.camera_right[0]=1.f;
    frame.camera_up[1]=1.f;
    frame.sun_direction[2]=1.f;
    frame.vertical_fov_radians=1.0f;
    frame.aspect_ratio=16.f/9.f;
    frame.near_clip_m=0.1f;
    frame.far_clip_m=1024.f;
    ogl_rtx_world_shadow_sample sample{};
    sample.struct_size=sizeof(sample);
    sample.version=OGL_RTX_WORLD_SHADOW_VERSION;
    sample.geometry_revision=11;
    sample.sample_sequence=3;
    sample.capture_frame=frame;
    for (float& d:sample.capture_depth) d=1.f;
    sample.capture_depth[3]=0.84f;
    sample.valid_mask[3]=1;
    sample.occlusion_mask[3]=1;
    sample.matched_surface_pixels=1;
    sample.occluded_pixels=1;
    assert(ogl::valid_world_shadow_sample(sample,11,frame));
    assert(ogl::world_shadow_mask_matches_depth(
        sample.occlusion_mask,sample.valid_mask));
    assert(!ogl::world_shadow_mask_matches_depth(nullptr,sample.valid_mask));
    assert(!ogl::world_shadow_mask_matches_depth(
        sample.occlusion_mask,sample.valid_mask,63));
    sample.occlusion_mask[9]=1;
    assert(!ogl::world_shadow_mask_matches_depth(
        sample.occlusion_mask,sample.valid_mask));
    sample.occlusion_mask[9]=0;
    assert(ogl::world_shadow_mask_matches_depth(
        sample.occlusion_mask,sample.valid_mask));
    assert(ogl::world_shadow_pixel_eligible(sample,3,0.84f));
    assert(!ogl::world_shadow_pixel_eligible(sample,3,0.85f));
    assert(!ogl::world_shadow_pixel_eligible(sample,4,1.f));
    assert(!ogl::world_shadow_pixel_eligible(sample,64,0.84f));
    assert(!ogl::valid_world_shadow_sample(sample,10,frame));
    auto changed=frame;
    changed.camera_position[0]+=0.001f;
    assert(!ogl::valid_world_shadow_sample(sample,11,changed));
    changed=frame;
    changed.sun_direction[1]=0.2f;
    assert(!ogl::valid_world_shadow_sample(sample,11,changed));
    changed=frame;
    changed.width=1280;
    assert(!ogl::valid_world_shadow_sample(sample,11,changed));
    sample.occlusion_mask[4]=1; // never shade unmatched foreground
    assert(!ogl::valid_world_shadow_sample(sample,11,frame));
    sample.occlusion_mask[4]=0;
    sample.capture_depth[3]=1.f; // sky not receiver
    assert(!ogl::valid_world_shadow_sample(sample,11,frame));
    sample.capture_depth[3]=0.84f;
    sample.capture_depth[7]=std::numeric_limits<float>::quiet_NaN();
    assert(!ogl::valid_world_shadow_sample(sample,11,frame));
    sample.capture_depth[7]=1.f;
    assert(ogl::valid_world_shadow_sample(sample,11,frame));
    sample.sample_sequence=0;
    assert(!ogl::valid_world_shadow_sample(sample,11,frame));
    ogl_rtx_dense_shadow dense{};
    dense.struct_size=sizeof(dense);
    dense.version=OGL_RTX_DENSE_VERSION;
    dense.geometry_revision=11;
    dense.sample_sequence=1;
    dense.capture_frame=frame;
    dense.width=OGL_RTX_DENSE_WIDTH;dense.height=OGL_RTX_DENSE_HEIGHT;
    for(float& v:dense.capture_depth) v=1.f;
    dense.capture_depth[123]=0.76f;
    dense.valid_mask[123]=1;
    dense.shadow_mask[123]=1;
    dense.surface_pixels=1;
    dense.matched_pixels=1;
    dense.shadow_pixels=1;
    assert(ogl::valid_dense_shadow(dense,11,frame));
    assert(ogl::dense_shadow_pixel_current(dense,123,0.76f));
    assert(!ogl::dense_shadow_pixel_current(dense,123,0.77f));
    assert(!ogl::dense_shadow_pixel_current(dense,OGL_RTX_DENSE_PIXELS,0.76f));
    assert(!ogl::valid_dense_shadow(dense,12,frame));
    dense.shadow_mask[124]=1;
    assert(!ogl::valid_dense_shadow(dense,11,frame));
    dense.shadow_mask[124]=0;
    dense.capture_depth[123]=1.f;
    assert(!ogl::valid_dense_shadow(dense,11,frame));
    dense.capture_depth[123]=0.76f;
    assert(ogl::valid_dense_shadow(dense,11,frame));
    // Strict raw depth == 1e-5 was too strict for a sloped 128x72 tile.
    auto project=[&](float view_m){
        const float n=frame.near_clip_m,f=frame.far_clip_m;
        return f/(f-n)*(1.f-n/view_m);
    };
    assert(ogl::dense_view_depth_matches(project(10.f),
                                         project(10.18f),
                                         frame.near_clip_m,frame.far_clip_m));
    assert(!ogl::dense_view_depth_matches(project(10.f),
                                          project(11.f),
                                          frame.near_clip_m,frame.far_clip_m));
    assert(ogl::dense_view_depth_matches(project(200.f),
                                         project(201.4f),
                                         frame.near_clip_m,frame.far_clip_m));
    assert(!ogl::dense_view_depth_matches(project(200.f),
                                          project(202.1f),
                                          frame.near_clip_m,frame.far_clip_m));
    assert(!ogl::dense_view_depth_matches(1.f,project(10.f),
                                          frame.near_clip_m,frame.far_clip_m));
    assert(!ogl::dense_view_depth_matches(project(10.f),1.f,
                                          frame.near_clip_m,frame.far_clip_m));
    assert(!ogl::dense_view_depth_matches(project(10.f),
                                          std::numeric_limits<float>::quiet_NaN(),
                                          frame.near_clip_m,frame.far_clip_m));
    assert(!ogl::dense_view_depth_matches(project(10.f),project(10.f),
                                          0.f,frame.far_clip_m));
    return 0;
}

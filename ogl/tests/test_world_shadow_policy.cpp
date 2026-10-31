#ifdef NDEBUG
#undef NDEBUG
#endif
#include "ogl/ogl_world_shadow_policy.h"
#include <cassert>
#include <cmath>
#include <limits>
int main() {
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

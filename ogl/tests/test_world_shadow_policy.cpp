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
    return 0;
}

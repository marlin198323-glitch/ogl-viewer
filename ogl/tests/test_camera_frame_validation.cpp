// Cross-platform Firestorm projection ABI regression (no Vulkan required).
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "ogl/ogl_camera_validation.h"
#include <cassert>
#include <limits>

int main() {
    ogl_rtx_frame_state camera{};
    camera.struct_size = sizeof(camera);
    camera.frame_version = OGL_RTX_FRAME_VERSION;
    camera.width = 1920;
    camera.height = 1080;
    camera.camera_forward[0] = 1.f;
    camera.camera_right[1] = -1.f;
    camera.camera_up[2] = 1.f;
    camera.sun_direction[2] = 1.f;
    camera.vertical_fov_radians = 1.0f;
    camera.aspect_ratio = 16.f / 9.f;
    camera.near_clip_m = 0.1f;
    camera.far_clip_m = 256.f;
    assert(ogl::valid_camera_frame(camera));

    auto bad = camera;
    bad.frame_version -= 1;
    assert(!ogl::valid_camera_frame(bad));
    bad = camera;
    bad.struct_size -= 4;
    assert(!ogl::valid_camera_frame(bad));
    bad = camera;
    bad.vertical_fov_radians = 0.f;
    assert(!ogl::valid_camera_frame(bad));
    bad = camera;
    bad.vertical_fov_radians = 3.2f;
    assert(!ogl::valid_camera_frame(bad));
    bad = camera;
    bad.camera_right[1] = 0.f;
    assert(!ogl::valid_camera_frame(bad));
    bad = camera;
    bad.camera_up[1] = 1.f; // orthogonality failure
    assert(!ogl::valid_camera_frame(bad));
    bad = camera;
    bad.camera_position[0] = std::numeric_limits<float>::quiet_NaN();
    assert(!ogl::valid_camera_frame(bad));
    bad = camera;
    bad.sun_direction[0] = std::numeric_limits<float>::infinity();
    assert(!ogl::valid_camera_frame(bad));
    bad = camera;
    bad.aspect_ratio = 0.f;
    assert(!ogl::valid_camera_frame(bad));
    bad = camera;
    bad.width = 0;
    assert(!ogl::valid_camera_frame(bad));

    bad = camera;
    bad.near_clip_m = 0.f;
    assert(!ogl::valid_camera_frame(bad));
    bad = camera;
    bad.far_clip_m = bad.near_clip_m;
    assert(!ogl::valid_camera_frame(bad));
    bad = camera;
    bad.far_clip_m = std::numeric_limits<float>::infinity();
    assert(!ogl::valid_camera_frame(bad));

    // A 90-degree roll changes the right/up axes while staying orthogonal.
    camera.camera_right[1] = 0.f;
    camera.camera_right[2] = 1.f;
    camera.camera_up[2] = 0.f;
    camera.camera_up[1] = 1.f;
    assert(ogl::valid_camera_frame(camera));

    // Same ABI must accept portrait, ultrawide, and arbitrary resolutions.
    camera.width = 1080;
    camera.height = 1920;
    camera.aspect_ratio = 1080.f/1920.f;
    assert(ogl::valid_camera_frame(camera));
    camera.width = 5120;
    camera.height = 1440;
    camera.aspect_ratio = 5120.f/1440.f;
    assert(ogl::valid_camera_frame(camera));
}

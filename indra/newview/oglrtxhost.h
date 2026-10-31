#ifndef OGL_RTX_HOST_H
#define OGL_RTX_HOST_H

#include <cstddef>
#include <cstdint>

/* Optional Linux Vulkan/RTX module lifecycle; safe no-op on other platforms. */
void ogl_init_rtx_module();
void ogl_shutdown_rtx_module();
// Capability is not rendering: reporting RTX hardware does not enable raytracing.
bool ogl_rtx_hardware_supported();
bool ogl_rtx_rendering_active();
// Actual Firestorm frame/camera bridge for opt-in Vulkan diagnostics.
 // The active OpenGL world renderer remains unchanged.
// Triangles are copied synchronously; never retained by the host.
void ogl_rtx_submit_world_triangles(const float *triangle_xyz, unsigned count);
void ogl_rtx_submit_camera_sun(unsigned width, unsigned height,
    const float* camera_position, const float* camera_forward,
    const float* camera_right, const float* camera_up,
    float vertical_fov_radians, float aspect_ratio,
    float near_clip_m, float far_clip_m,
    const float* sun_direction);

// Developer-only 8x8 GPU ray-hit mask. Returns false without changing the
// caller's output if no matching, current scene sample exists. NOT pixels
// for the world's OpenGL framebuffer, and not enabled for public users.
bool ogl_rtx_copy_debug_shadow_mask(std::uint8_t* pixels,
    std::size_t capacity, std::uint64_t* geometry_revision);

// Optional whole-view 8x8 NEAREST depth grid from the REAL deferred OpenGL
// depth texture. GPU-only downsample, single synchronous readback/minute with
// OGL_RTX_DEPTH_PROBE=1. Not used for visible shadows.
void ogl_rtx_probe_deferred_depth(unsigned depth_texture,
    unsigned render_width, unsigned render_height);

#endif

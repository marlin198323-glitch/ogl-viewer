#ifndef OGL_RTX_HOST_H
#define OGL_RTX_HOST_H

/* Optional Linux Vulkan/RTX module lifecycle; safe no-op on other platforms. */
void ogl_init_rtx_module();
void ogl_shutdown_rtx_module();
// Capability is not rendering: reporting RTX hardware does not enable raytracing.
bool ogl_rtx_hardware_supported();
bool ogl_rtx_rendering_active();
// Preliminary frame data connection. No Vulkan tracing or scene geometry yet.
// Triangles are copied synchronously; never retained by the host.
void ogl_rtx_submit_world_triangles(const float *triangle_xyz, unsigned count);
void ogl_rtx_submit_camera_sun(unsigned width, unsigned height,
    const float* camera_position, const float* camera_forward,
    const float* sun_direction);

#endif

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
// Opt-in stable OpenSim UUID object registry (local mesh and 3x4 pose).
// Registry is CPU-side metadata; current Vulkan prototype still has one TLAS.
bool ogl_rtx_track_local_object(const std::uint8_t* uuid16,
    const float* local_triangle_xyz,unsigned count,const float* affine3x4);
bool ogl_rtx_update_object_pose(const std::uint8_t* uuid16,
    const float* affine3x4);
void ogl_rtx_forget_local_object(const std::uint8_t* uuid16);
void ogl_rtx_submit_camera_sun(unsigned width, unsigned height,
    const float* camera_position, const float* camera_forward,
    const float* camera_right, const float* camera_up,
    float vertical_fov_radians, float aspect_ratio,
    float near_clip_m, float far_clip_m,
    const float* sun_direction);

// Experimental, opt-in Vulkan dense mask compositing into the already
// bound Firestorm deferred sunlight target. Returns true only if real,
/// synchronized Vulkan-shadow pixels were submitted to OpenGL. Does not
// modify normals, UI, sky, unverified surfaces or public OpenGL defaults.
bool ogl_rtx_compose_dense_sun_shadow(unsigned current_depth_texture,
    unsigned width,unsigned height,const float* eye,const float* forward,
    const float* right,const float* up,float fov,float aspect,
    float near_clip,float far_clip,const float* sun);

// World renderer input handshake: verifies the Vulkan tile's exact
// source camera/geometry and returns aggregate validated pixels. It does NOT
// composite or upscale this incomplete 8x8 sample into the world.
bool ogl_rtx_world_shadow_source_ready(unsigned width,unsigned height,
    const float* eye,const float* forward,const float* right,
    const float* up,float fov,float aspect,float near_clip,float far_clip,
    const float* sun,unsigned* matched,unsigned* occluded,
    std::uint64_t* sequence);

// Developer-only 8x8 GPU ray-hit mask. Returns false without changing the
// caller's output if no matching, current scene sample exists. NOT pixels
// for the world's OpenGL framebuffer, and not enabled for public users.
bool ogl_rtx_copy_debug_shadow_mask(std::uint8_t* pixels,
    std::size_t capacity, std::uint64_t* geometry_revision);

// Same validated shadow mask, plus monotonic GPU publication sequence.
// UI diagnostics may use the sequence to expire or refresh old samples.
bool ogl_rtx_copy_debug_shadow_sample(std::uint8_t* pixels,
    std::size_t capacity, std::uint64_t* geometry_revision,
    std::uint64_t* sample_sequence,
    std::uint8_t* surface_mask, std::uint32_t* surfaces,
    std::uint32_t* background);

// Optional whole-view 8x8 NEAREST depth grid from the REAL deferred OpenGL
// depth texture. GPU-only downsample, single synchronous readback/minute with
// OGL_RTX_DEPTH_PROBE=1. Not used for visible shadows.
void ogl_rtx_probe_deferred_depth(unsigned depth_texture,
    unsigned render_width, unsigned render_height);

#endif

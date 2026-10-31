#ifndef OGL_GPU_PROBE_API_H
#define OGL_GPU_PROBE_API_H

#include "ogl_rtx_api.h"

#define OGL_GPU_PROBE_VERSION 3u

#ifdef __cplusplus
extern "C" {
#endif

// Experimental diagnostics, never a production shadow renderer.
// The full scene is checked, uploaded, and ray queried synchronously.
// The caller may invoke it only when explicitly opted in for testing.
// A real renderer must be persistent and asynchronous.
typedef struct ogl_gpu_probe_result {
    uint32_t struct_size;
    uint32_t version;
    uint32_t success;
    uint32_t triangle_count;
    uint32_t ray_count;
    uint32_t occluded;
    uint32_t unoccluded;
    uint32_t vulkan_device_creations; /* lifetime of the worker thread */
    uint32_t blas_builds;             /* actual GPU BLAS builds */
    uint32_t cache_hits;              /* same mesh + sun + shader reused */
    uint32_t mask_width;              /* debug-only 8, not world-resolution */
    uint32_t mask_height;             /* debug-only 8, not world-resolution */
    uint8_t shadow_mask[64];          /* row-major; 1 = ray hit, 0 = miss */
} ogl_gpu_probe_result;

// Return 0 for successful Vulkan compute (including an empty mask),
// 77 for unavailable Vulkan ray-query GPU, 1 for a failure.
// Requires valid bounded scene, trusted bundled SPIR-V and output struct.
OGL_MODULE_EXPORT int ogl_gpu_probe_run(
    const ogl_rtx_triangle* triangles, uint32_t count,
    const float* sun_direction_xyz,
    const char* shader_path, ogl_gpu_probe_result* result);

// Dedicated experimental perspective primary + sun-ray shader.
// Needs a real camera sample. Still a low-resolution diagnostic only.
OGL_MODULE_EXPORT int ogl_gpu_probe_camera_run(
    const ogl_rtx_triangle* triangles, uint32_t count,
    const ogl_rtx_frame_state* frame,
    const char* shader_path, ogl_gpu_probe_result* result);

#ifdef __cplusplus
}
#endif

#endif

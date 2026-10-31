#ifndef OGL_GPU_PROBE_API_H
#define OGL_GPU_PROBE_API_H

#include "ogl_rtx_api.h"

#define OGL_GPU_PROBE_VERSION 7u

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
    uint8_t shadow_mask[64];          /* row-major; 1 = sun shadow */
    uint32_t primary_hits;            /* incomplete BLAS hits from eye */
    uint32_t primary_depth_matches;   /* BLAS hit agrees with OpenGL depth */
    uint8_t primary_mask[64];         /* 1 = Vulkan geometry hit */
    uint8_t depth_match_mask[64];     /* 1 = agrees with GL depth */
    uint32_t blas_refits;             /* Vulkan in-place BLAS UPDATE operations */
    uint32_t tlas_refits;             /* instance-transform TLAS UPDATE operations */
    uint32_t primary_closer;          /* Vulkan hit in front of OpenGL depth */
    uint32_t primary_farther;         /* Vulkan hit behind OpenGL depth */
} ogl_gpu_probe_result;

// Return 0 for successful Vulkan compute (including an empty mask),
// 77 for unavailable Vulkan ray-query GPU, 1 for a failure.
// Requires valid bounded scene, trusted bundled SPIR-V and output struct.
OGL_MODULE_EXPORT int ogl_gpu_probe_run(
    const ogl_rtx_triangle* triangles, uint32_t count,
    const float* sun_direction_xyz,
    const char* shader_path, ogl_gpu_probe_result* result);

// First bounded developer dense ray-queried frame, independent of 8x8.
#define OGL_GPU_DENSE_VERSION 1u
#define OGL_GPU_DENSE_WIDTH 64u
#define OGL_GPU_DENSE_HEIGHT 36u
#define OGL_GPU_DENSE_PIXELS (OGL_GPU_DENSE_WIDTH*OGL_GPU_DENSE_HEIGHT)
typedef struct ogl_gpu_dense_result {
    uint32_t struct_size;
    uint32_t version;
    uint32_t success;
    uint32_t width;
    uint32_t height;
    uint32_t triangle_count;
    uint32_t ray_count;
    uint32_t surface_pixels;
    uint32_t primary_hits;
    uint32_t depth_matches;
    uint32_t occluded;
    uint8_t shadow_mask[OGL_GPU_DENSE_PIXELS];
    uint8_t depth_match_mask[OGL_GPU_DENSE_PIXELS];
    uint8_t primary_mask[OGL_GPU_DENSE_PIXELS];
} ogl_gpu_dense_result;

// Dense GL-depth-guided Vulkan sun ray queries; depth is row-major
// bottom-left and sampled at the 64x36 pixel centers. The caller must
// validate pose and *current* GL depth before any world compositing.
// Developer-only, synchronous on the GPU worker; no UI calls.
OGL_MODULE_EXPORT int ogl_gpu_probe_dense_run(
    const ogl_rtx_triangle* triangles,uint32_t count,
    const ogl_rtx_frame_state* frame,const float* depth_2304,
    const char* shader_path,ogl_gpu_dense_result* result);

// Transform-only diagnostic for an existing BLAS instance. Position must
// be finite. Unchanged geometry can reuse the BLAS while TLAS is refitted.
// Deliberately supports ONE GPU instance at this milestone, not the entire
// OpenSim object hierarchy.
OGL_MODULE_EXPORT int ogl_gpu_probe_transform_run(
    const ogl_rtx_triangle* triangles, uint32_t count,
    const float* sun_direction_xyz, const float* instance_position_xyz,
    const char* shader_path, ogl_gpu_probe_result* result);

// Dedicated experimental perspective primary + sun-ray shader.
// Needs a real camera sample. Still a low-resolution diagnostic only.
OGL_MODULE_EXPORT int ogl_gpu_probe_camera_run(
    const ogl_rtx_triangle* triangles, uint32_t count,
    const ogl_rtx_frame_state* frame,
    const char* shader_path, ogl_gpu_probe_result* result);

// Optional real OpenGL depth-guided shadow ray queries, 8x8 only.
// The pointer is copied before worker GPU submission. No visual output.
// This is a diagnostic of standard perspective-depth reconstruction.
OGL_MODULE_EXPORT int ogl_gpu_probe_depth_run(
    const ogl_rtx_triangle* triangles, uint32_t count,
    const ogl_rtx_frame_state* frame, const float* depth_64,
    const char* shader_path, ogl_gpu_probe_result* result);

#ifdef __cplusplus
}
#endif

#endif

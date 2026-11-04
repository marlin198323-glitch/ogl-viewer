#ifndef OGL_RTX_API_H
#define OGL_RTX_API_H

#include <stdint.h>
#include "ogl_module_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OGL_RTX_STATUS_VERSION 1u

/*
 * Hardware/driver capability only. No Vulkan rendering engine, ray-traced
 * lighting, or scene bridge is implied by hardware_ready == 1.
 */
typedef struct ogl_rtx_status {
    uint32_t struct_size;
    uint32_t status_version;
    uint32_t api_version;
    uint32_t vulkan_gpu; /* vendor-neutral Vulkan physical-device found */
    uint32_t acceleration_structure;
    uint32_t ray_query;
    uint32_t ray_tracing_pipeline;
    uint32_t hardware_ready;
    uint32_t rendering_active; /* always zero until a real RTX renderer exists */
    char gpu_name[256];
    char message[256];
} ogl_rtx_status;

/* Renderer data bridge v3: exact Firestorm camera/light sample per frame.
 * Actual acceleration structures, mesh data, GPU Vulkan tracing and
 * OpenGL compositing are NOT implemented by this bridge. Do not set
 * rendering_active until real ray-traced pixels are displayed.
 */
#define OGL_RTX_FRAME_VERSION 3u
typedef struct ogl_rtx_frame_state {
    uint32_t struct_size;
    uint32_t frame_version;
    uint32_t width;
    uint32_t height;
    float camera_position[3];
    float camera_forward[3];
    float sun_direction[3];
    float camera_right[3];      /* -LLCamera::getLeftAxis() */
    float camera_up[3];         /* LLCamera::getUpAxis() */
    float vertical_fov_radians; /* LLCamera::getView(), radians */
    float aspect_ratio;         /* LLCamera::getAspect() */
    float near_clip_m;          /* LLCamera::getNear() */
    float far_clip_m;           /* LLCamera::getFar(), perspective convention */
} ogl_rtx_frame_state;

/* Copies the latest exact Firestorm camera axes, FOV and light into the module.
 * Returns 1 if accepted, 0 if missing hardware or invalid arguments.
 */
OGL_MODULE_EXPORT int ogl_rtx_submit_frame(const ogl_rtx_frame_state *frame);
OGL_MODULE_EXPORT uint64_t ogl_rtx_received_frames(void);

/* World geometry exchange: bounded batch of real triangulated OpenSim
 * volume faces in agent coordinates. No GPU ray tracing is implied.
 * Caller owns the memory; callee copies it synchronously.
 */
#define OGL_RTX_MAX_SCENE_TRIANGLES 8192u
typedef struct ogl_rtx_triangle {
    float position[9]; /* three xyz vertices, in agent-space meters */
} ogl_rtx_triangle;
OGL_MODULE_EXPORT int ogl_rtx_submit_geometry(const ogl_rtx_triangle *triangles,
                                               uint32_t count);
OGL_MODULE_EXPORT uint32_t ogl_rtx_received_triangles(void);

/* Prototype per-object registry for incremental RT geometry/transform
 * tracking. IDs are the full 16 UUID bytes; object vertices are LOCAL.
 * Affine transform is 3x4 row-major (translation in cols 3,7,11).
 * This registry is not yet uploaded as a multi-instance Vulkan TLAS.
 * All APIs are called on the viewer render thread.
 */
OGL_MODULE_EXPORT int ogl_rtx_track_object(const uint8_t* uuid16,
    const ogl_rtx_triangle* local_triangles, uint32_t count,
    const float* affine3x4);
OGL_MODULE_EXPORT int ogl_rtx_track_transform(const uint8_t* uuid16,
    const float* affine3x4);
OGL_MODULE_EXPORT int ogl_rtx_forget_object(const uint8_t* uuid16);
OGL_MODULE_EXPORT uint32_t ogl_rtx_tracked_objects(void);
OGL_MODULE_EXPORT uint64_t ogl_rtx_object_geometry_generation(void);
OGL_MODULE_EXPORT uint64_t ogl_rtx_object_transform_generation(void);
/* Diagnostic only, counted only with OGL_RTX_SCENE_PROBE=1.
 * NEVER means shadows are visible in the viewer.
 */
OGL_MODULE_EXPORT uint64_t ogl_rtx_gpu_probe_attempts(void);
OGL_MODULE_EXPORT uint64_t ogl_rtx_gpu_probe_successes(void);
/* Monotonic revision. Increments only for a changed, accepted snapshot. */
OGL_MODULE_EXPORT uint64_t ogl_rtx_geometry_revision(void);
/* Copy last snapshot into caller-owned storage (all-or-nothing).
 * Return number copied or zero when empty / insufficient capacity.
 * C ABI operations currently run on the same viewer rendering thread.
 */
OGL_MODULE_EXPORT uint32_t ogl_rtx_copy_geometry(ogl_rtx_triangle *dest,
                                                 uint32_t capacity);

/* Developer diagnostics only. A low-resolution ray-hit mask is NOT a screen-
 * space shadow map and MUST NOT be composited into the world image.
 * Read only from the same main/render thread as the frame bridge.
 * A region or mesh update invalidates the previous debug mask.
 */
#define OGL_RTX_SHADOW_DEBUG_VERSION 3u
#define OGL_RTX_SHADOW_DEBUG_PIXELS 64u
typedef struct ogl_rtx_shadow_debug {
    uint32_t struct_size;
    uint32_t version;
    uint64_t geometry_revision;
    uint64_t sample_sequence;  /* unique successful async GPU job sequence */
    uint32_t width;
    uint32_t height;
    uint32_t occluded;
    uint32_t surface_pixels;
    uint32_t background_pixels;
    uint8_t pixels[OGL_RTX_SHADOW_DEBUG_PIXELS]; /* 1 = shadow */
    uint8_t surface_mask[OGL_RTX_SHADOW_DEBUG_PIXELS]; /* 1 = non-sky */
} ogl_rtx_shadow_debug;
OGL_MODULE_EXPORT int ogl_rtx_copy_shadow_debug(ogl_rtx_shadow_debug *output);

/* Initial GPU-to-world shadow *input* contract. This is a validated
 * immutable Vulkan ray-query tile, NOT a composited full-resolution image.
 * A renderer must compare capture_pose and capture_depth against CURRENT
 * camera and framebuffer before using any bit; moving avatars/transparency
 * and frames not present in the AS must still be handled. Fail closed.
 * Separate ABI from historical 8x8 HUD diagnostics.
 */
#define OGL_RTX_WORLD_SHADOW_VERSION 1u
typedef struct ogl_rtx_world_shadow_sample {
    uint32_t struct_size;
    uint32_t version;
    uint64_t geometry_revision;
    uint64_t sample_sequence;
    ogl_rtx_frame_state capture_frame;
    uint32_t matched_surface_pixels;
    uint32_t occluded_pixels;
    uint8_t valid_mask[64];       /* Vulkan primary matches OpenGL depth */
    uint8_t occlusion_mask[64];   /* shadow ray hit, only if valid */
    float capture_depth[64];     /* GL normalized depth at capture time */
} ogl_rtx_world_shadow_sample;
/* No result on absent, obsolete, incomplete or invalid GPU frames. */
OGL_MODULE_EXPORT int ogl_rtx_copy_world_shadow_sample(
    ogl_rtx_world_shadow_sample* output);

/* Explicit developer-only 64x36 Vulkan depth-shadow mode.
 * Separate from the legacy 8x8 diagnostic. This remains a bounded source
 * tile; it is NOT an automatically upscaled world-space shading texture.
 * The capture_frame is immutable and must be checked before application.
 */
#define OGL_RTX_DENSE_VERSION 2u
#define OGL_RTX_DENSE_WIDTH 128u
#define OGL_RTX_DENSE_HEIGHT 72u
#define OGL_RTX_DENSE_PIXELS (OGL_RTX_DENSE_WIDTH*OGL_RTX_DENSE_HEIGHT)
typedef struct ogl_rtx_dense_depth {
    uint32_t struct_size;
    uint32_t version;
    uint32_t viewport_width;
    uint32_t viewport_height;
    uint32_t width;
    uint32_t height;
    float normalized_depth[OGL_RTX_DENSE_PIXELS];
} ogl_rtx_dense_depth;
typedef struct ogl_rtx_dense_shadow {
    uint32_t struct_size;
    uint32_t version;
    uint64_t geometry_revision;
    uint64_t sample_sequence;
    ogl_rtx_frame_state capture_frame;
    uint32_t width;
    uint32_t height;
    uint32_t surface_pixels;
    uint32_t matched_pixels;
    uint32_t shadow_pixels;
    uint8_t valid_mask[OGL_RTX_DENSE_PIXELS];
    uint8_t shadow_mask[OGL_RTX_DENSE_PIXELS];
    float capture_depth[OGL_RTX_DENSE_PIXELS];
} ogl_rtx_dense_shadow;
OGL_MODULE_EXPORT int ogl_rtx_submit_dense_depth(
    const ogl_rtx_dense_depth* input);
OGL_MODULE_EXPORT int ogl_rtx_copy_dense_shadow(
    ogl_rtx_dense_shadow* output);

/* OpenGL deferred depth, WHOLE-VIEWPORT 8x8 NEAREST SAMPLE GRID.
 * Pixel (x,y) corresponds to camera ray UV ((x+.5)/8,(y+.5)/8).
 * OpenGL origin is bottom-left, matching the diagnostic Vulkan shader.
 * A new camera frame invalidates previous samples. This is NOT a real
 * world shadow map, and does not enable OpenGL/Vulkan compositing.
 */
#define OGL_RTX_DEPTH_DEBUG_VERSION 2u
#define OGL_RTX_DEPTH_DEBUG_PIXELS 64u
typedef struct ogl_rtx_depth_debug {
    uint32_t struct_size;
    uint32_t version;
    uint32_t viewport_width;  /* original depth target resolution */
    uint32_t viewport_height;
    uint32_t grid_width;      /* exactly 8 */
    uint32_t grid_height;     /* exactly 8 */
    float normalized_depth[OGL_RTX_DEPTH_DEBUG_PIXELS]; /* row-major */
} ogl_rtx_depth_debug;
OGL_MODULE_EXPORT int ogl_rtx_submit_depth_debug(const ogl_rtx_depth_debug *sample);
OGL_MODULE_EXPORT int ogl_rtx_copy_depth_debug(ogl_rtx_depth_debug *output);

/* P05 experimental world-device bootstrap, independent of RTX support.
 * Developer opt-in only via OGL_VULKAN_WORLD_BOOTSTRAP=1. Returns 1 when
 * a persistent graphics VkDevice and VkQueue were actually created.
 * This is NOT surface/present readiness, not a world renderer, and MUST NOT
 * affect the OpenGL fallback or the rendering_active RTX status field.
 * Owned by the module and released on ogl_module_stop.
 */
OGL_MODULE_EXPORT int ogl_vulkan_world_device_bootstrapped(void);
/* P05 developer-only Linux/X11 WSI probe on the ACTUAL viewer window.
 * Requires both OGL_VULKAN_WORLD_BOOTSTRAP=1 and
 * OGL_VULKAN_WINDOW_SURFACE=1. Creates real VkSurfaceKHR and VkSwapchainKHR
 * if supported by the selected graphics/present queue. No image acquisition,
 * rendering, queue submission or presentation occurs: OpenGL remains live.
 * Do not call after SDL destroys the supplied X11 window/display.
 * Returns 1 only when a swapchain with >=2 images exists.
 * Exported as optional C ABI symbols; unsupported OSes return 0.
 */
OGL_MODULE_EXPORT int ogl_vulkan_world_attach_x11_window(
    void* x11_display, uint64_t x11_window, uint32_t width, uint32_t height);
OGL_MODULE_EXPORT int ogl_vulkan_world_swapchain_probe_ready(void);
OGL_MODULE_EXPORT void ogl_vulkan_world_detach_window(void);

/* Read-only status; valid until module is unloaded. Never store credentials. */
OGL_MODULE_EXPORT const ogl_rtx_status *ogl_rtx_get_status(void);

#ifdef __cplusplus
}
#endif

#endif

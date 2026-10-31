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
#define OGL_RTX_SHADOW_DEBUG_VERSION 2u
#define OGL_RTX_SHADOW_DEBUG_PIXELS 64u
typedef struct ogl_rtx_shadow_debug {
    uint32_t struct_size;
    uint32_t version;
    uint64_t geometry_revision;
    uint64_t sample_sequence;  /* unique successful async GPU job sequence */
    uint32_t width;
    uint32_t height;
    uint32_t occluded;
    uint8_t pixels[OGL_RTX_SHADOW_DEBUG_PIXELS]; /* row-major, 0 or 1 */
} ogl_rtx_shadow_debug;
OGL_MODULE_EXPORT int ogl_rtx_copy_shadow_debug(ogl_rtx_shadow_debug *output);

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

/* Read-only status; valid until module is unloaded. Never store credentials. */
OGL_MODULE_EXPORT const ogl_rtx_status *ogl_rtx_get_status(void);

#ifdef __cplusplus
}
#endif

#endif

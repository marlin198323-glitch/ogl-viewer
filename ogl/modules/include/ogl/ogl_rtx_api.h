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
    uint32_t nvidia_gpu;
    uint32_t acceleration_structure;
    uint32_t ray_query;
    uint32_t ray_tracing_pipeline;
    uint32_t hardware_ready;
    uint32_t rendering_active; /* always zero until a real RTX renderer exists */
    char gpu_name[256];
    char message[256];
} ogl_rtx_status;

/* Renderer data bridge v1: one world-view camera/light sample per frame.
 * Actual acceleration structures, mesh data, GPU Vulkan tracing and
 * OpenGL compositing are NOT implemented by this bridge. Do not set
 * rendering_active until real ray-traced pixels are displayed.
 */
#define OGL_RTX_FRAME_VERSION 1u
typedef struct ogl_rtx_frame_state {
    uint32_t struct_size;
    uint32_t frame_version;
    uint32_t width;
    uint32_t height;
    float camera_position[3];
    float camera_forward[3];
    float sun_direction[3];
} ogl_rtx_frame_state;

/* Copies only the latest world-view camera/light state into the module.
 * Returns 1 if accepted, 0 if missing hardware or invalid arguments.
 */
OGL_MODULE_EXPORT int ogl_rtx_submit_frame(const ogl_rtx_frame_state *frame);
OGL_MODULE_EXPORT uint64_t ogl_rtx_received_frames(void);

/* World geometry exchange: bounded batch of real triangulated OpenSim
 * volume faces in agent coordinates. No GPU ray tracing is implied.
 * Caller owns the memory; callee copies it synchronously.
 */
#define OGL_RTX_MAX_SCENE_TRIANGLES 1024u
typedef struct ogl_rtx_triangle {
    float position[9]; /* three xyz vertices, in agent-space meters */
} ogl_rtx_triangle;
OGL_MODULE_EXPORT int ogl_rtx_submit_geometry(const ogl_rtx_triangle *triangles,
                                               uint32_t count);
OGL_MODULE_EXPORT uint32_t ogl_rtx_received_triangles(void);

/* Read-only status; valid until module is unloaded. Never store credentials. */
OGL_MODULE_EXPORT const ogl_rtx_status *ogl_rtx_get_status(void);

#ifdef __cplusplus
}
#endif

#endif

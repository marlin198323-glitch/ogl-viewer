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

/* Read-only status; valid until module is unloaded. Never store credentials. */
OGL_MODULE_EXPORT const ogl_rtx_status *ogl_rtx_get_status(void);

#ifdef __cplusplus
}
#endif

#endif

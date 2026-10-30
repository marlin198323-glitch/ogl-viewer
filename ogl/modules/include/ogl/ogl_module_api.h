#ifndef OGL_MODULE_API_H
#define OGL_MODULE_API_H

#include <stdint.h>

/* Experimental stable-C-ABI boundary, version 1. Not yet integrated into Firestorm. */
#define OGL_MODULE_ABI_VERSION 1u

#if defined(_WIN32)
#  define OGL_MODULE_EXPORT __declspec(dllexport)
#elif defined(__GNUC__)
#  define OGL_MODULE_EXPORT __attribute__((visibility("default")))
#else
#  define OGL_MODULE_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ogl_log_fn)(int severity, const char *message);

typedef struct ogl_host_api {
    uint32_t struct_size;
    uint32_t abi_version;
    ogl_log_fn log_message;
} ogl_host_api;

typedef struct ogl_module_info {
    uint32_t struct_size;
    uint32_t abi_version;
    const char *id;
    const char *display_name;
    const char *version;
} ogl_module_info;

typedef const ogl_module_info *(*ogl_module_info_fn)(void);
typedef int (*ogl_module_start_fn)(const ogl_host_api *host);
typedef void (*ogl_module_stop_fn)(void);

/* Required exported symbols from an OGL module:
 * ogl_module_get_info, ogl_module_start, ogl_module_stop.
 */
OGL_MODULE_EXPORT const ogl_module_info *ogl_module_get_info(void);
OGL_MODULE_EXPORT int ogl_module_start(const ogl_host_api *host);
OGL_MODULE_EXPORT void ogl_module_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* OGL_MODULE_API_H */

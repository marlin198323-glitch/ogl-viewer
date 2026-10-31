#ifndef OGL_RTX_HOST_H
#define OGL_RTX_HOST_H

/* Optional Linux Vulkan/RTX module lifecycle; safe no-op on other platforms. */
void ogl_init_rtx_module();
void ogl_shutdown_rtx_module();

#endif

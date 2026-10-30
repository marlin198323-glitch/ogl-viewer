#include "ogl/ogl_module_api.h"

namespace {
const ogl_module_info module_info{
    sizeof(ogl_module_info),
    OGL_MODULE_ABI_VERSION,
    "de.nexverse.ogl.hello",
    "OGL Hello Module",
    "0.1.0"
};
}

extern "C" OGL_MODULE_EXPORT const ogl_module_info *ogl_module_get_info() {
    return &module_info;
}

extern "C" OGL_MODULE_EXPORT int ogl_module_start(const ogl_host_api *host) {
    if (!host ||
        host->struct_size < sizeof(ogl_host_api) ||
        host->abi_version != OGL_MODULE_ABI_VERSION ||
        !host->log_message) {
        return 1;
    }
    host->log_message(1, "OGL hello module initialized");
    return 0;
}

extern "C" OGL_MODULE_EXPORT void ogl_module_stop() {
    // No resources retained by this example module.
}

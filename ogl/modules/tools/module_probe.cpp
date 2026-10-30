#include "ogl/ogl_module_api.h"

#include <dlfcn.h>
#include <iostream>
#include <string>

namespace {
void log_message(int severity, const char *message) {
    std::cout << "OGL[" << severity << "]: "
              << (message ? message : "") << '\n';
}
}

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: ogl-module-probe PATH_TO_TRUSTED_MODULE\n";
        return 2;
    }

    void *handle = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        std::cerr << "dlopen: " << dlerror() << '\n';
        return 3;
    }
    auto get_info = reinterpret_cast<ogl_module_info_fn>(
        dlsym(handle, "ogl_module_get_info"));
    auto start = reinterpret_cast<ogl_module_start_fn>(
        dlsym(handle, "ogl_module_start"));
    auto stop = reinterpret_cast<ogl_module_stop_fn>(
        dlsym(handle, "ogl_module_stop"));
    if (!get_info || !start || !stop) {
        std::cerr << "Module missing required OGL exports\n";
        dlclose(handle);
        return 4;
    }

    const ogl_module_info *info = get_info();
    if (!info ||
        info->struct_size < sizeof(ogl_module_info) ||
        info->abi_version != OGL_MODULE_ABI_VERSION ||
        !info->id || !info->display_name || !info->version ||
        std::string(info->id).empty()) {
        std::cerr << "Incompatible module metadata/ABI\n";
        dlclose(handle);
        return 5;
    }

    std::cout << "Module: " << info->display_name
              << " (" << info->id << "), version " << info->version << '\n';
    const ogl_host_api api{
        sizeof(ogl_host_api),
        OGL_MODULE_ABI_VERSION,
        log_message
    };
    const int code = start(&api);
    if (code == 0) {
        stop();
    } else {
        std::cerr << "Module startup failed with " << code << '\n';
    }
    dlclose(handle);
    return code == 0 ? 0 : 6;
}

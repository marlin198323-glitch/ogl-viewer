#include "ogl/ogl_module_api.h"

#include <iostream>
#include <string>
#if defined(_WIN32)
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#else
#  include <dlfcn.h>
#endif

// Build-time verification only. Never loads arbitrary installed plugins.
namespace {
#if defined(_WIN32)
using ModuleHandle=HMODULE;
ModuleHandle open_module(const char* utf8_path) {
    const int n=MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                   utf8_path, -1, nullptr, 0);
    if(n<=0) return nullptr;
    std::wstring path(static_cast<size_t>(n), L'\0');
    if(!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8_path,
                            -1, &path[0], n)) return nullptr;
    return LoadLibraryExW(path.c_str(),nullptr,
           LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
}
void* symbol(ModuleHandle m,const char* name) {
    return reinterpret_cast<void*>(GetProcAddress(m,name));
}
void close_module(ModuleHandle m) { FreeLibrary(m); }
#else
using ModuleHandle=void*;
ModuleHandle open_module(const char* path) {
    return dlopen(path, RTLD_NOW | RTLD_LOCAL);
}
void* symbol(ModuleHandle m,const char* name) { return dlsym(m,name); }
void close_module(ModuleHandle m) { dlclose(m); }
#endif
} // namespace

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

    ModuleHandle handle=open_module(argv[1]);
    if(!handle) {
        std::cerr << "Cannot open trusted test module\n";
        return 3;
    }
    auto get_info=reinterpret_cast<ogl_module_info_fn>(
        symbol(handle,"ogl_module_get_info"));
    auto start=reinterpret_cast<ogl_module_start_fn>(
        symbol(handle,"ogl_module_start"));
    auto stop=reinterpret_cast<ogl_module_stop_fn>(
        symbol(handle,"ogl_module_stop"));
    if(!get_info || !start || !stop) {
        std::cerr << "Module missing required OGL exports\n";
        close_module(handle);
        return 4;
    }

    const ogl_module_info *info = get_info();
    if (!info ||
        info->struct_size < sizeof(ogl_module_info) ||
        info->abi_version != OGL_MODULE_ABI_VERSION ||
        !info->id || !info->display_name || !info->version ||
        std::string(info->id).empty()) {
        std::cerr << "Incompatible module metadata/ABI\n";
        close_module(handle);
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
    close_module(handle);
    return code == 0 ? 0 : 6;
}

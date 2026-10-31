#include "llviewerprecompiledheaders.h"
#include "oglrtxhost.h"
#include "llglheaders.h"
#include "../../ogl/modules/include/ogl/ogl_rtx_api.h"

#if LL_LINUX || LL_WINDOWS
#include <cstring>
#include <cstdlib>
#include <chrono>
#include <string>
#if LL_WINDOWS
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#else
#  include <dlfcn.h>
#endif

namespace {
#if LL_WINDOWS
using OGLModuleHandle = HMODULE;
OGLModuleHandle ogl_load_module() {
    // Explicit, trusted path next to the executable (no current-dir search).
    wchar_t buffer[MAX_PATH]{};
    const DWORD n = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    if (!n || n >= MAX_PATH) return nullptr;
    std::wstring exe(buffer, n);
    const auto slash = exe.find_last_of(L"\\/");
    if (slash == std::wstring::npos) return nullptr;
    const std::wstring dll = exe.substr(0, slash + 1) + L"ogl_rtx.dll";
    return LoadLibraryExW(dll.c_str(), nullptr,
                          LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
}
void* ogl_resolve_module(OGLModuleHandle module, const char* symbol) {
    return reinterpret_cast<void*>(GetProcAddress(module, symbol));
}
void ogl_unload_module(OGLModuleHandle module) { FreeLibrary(module); }
#else
using OGLModuleHandle = void*;
OGLModuleHandle ogl_load_module() { return dlopen("./lib/ogl_rtx.so", RTLD_NOW | RTLD_LOCAL); }
void* ogl_resolve_module(OGLModuleHandle module, const char* symbol) {
    return dlsym(module, symbol);
}
void ogl_unload_module(OGLModuleHandle module) { dlclose(module); }
#endif
OGLModuleHandle rtx_module_handle = nullptr;
bool rtx_hardware_ready = false;
bool rtx_rendering_enabled = false;
using ogl_rtx_submit_frame_fn = int (*)(const ogl_rtx_frame_state*);
ogl_rtx_submit_frame_fn rtx_submit_frame = nullptr;
using ogl_rtx_submit_geometry_fn = int (*)(const ogl_rtx_triangle*, uint32_t);
ogl_rtx_submit_geometry_fn rtx_submit_geometry = nullptr;
using ogl_rtx_triangles_fn = uint32_t (*)();
ogl_rtx_triangles_fn rtx_received_triangles = nullptr;
using ogl_rtx_copy_shadow_debug_fn = int (*)(ogl_rtx_shadow_debug*);
ogl_rtx_copy_shadow_debug_fn rtx_copy_shadow_debug = nullptr;
using ogl_rtx_submit_depth_fn = int (*)(const ogl_rtx_depth_debug*);
ogl_rtx_submit_depth_fn rtx_submit_depth = nullptr;
ogl_module_stop_fn rtx_stop = nullptr;

void host_log(int severity, const char *message) {
    LL_INFOS("OGLRTX") << "module severity " << severity << ": "
                       << (message ? message : "") << LL_ENDL;
}
}  // namespace
#endif

void ogl_init_rtx_module() {
#if LL_LINUX || LL_WINDOWS
    // Load only the bundled, trusted RTX module. Arbitrary paths and user-
    // supplied native modules are intentionally unsupported at this stage.
    rtx_module_handle = ogl_load_module();
    if (!rtx_module_handle) {
        LL_INFOS("OGLRTX") << "RTX capability module unavailable; keeping OpenGL."
                            << LL_ENDL;
        return;
    }
    auto info = reinterpret_cast<ogl_module_info_fn>(
        ogl_resolve_module(rtx_module_handle, "ogl_module_get_info"));
    auto start = reinterpret_cast<ogl_module_start_fn>(
        ogl_resolve_module(rtx_module_handle, "ogl_module_start"));
    rtx_stop = reinterpret_cast<ogl_module_stop_fn>(
        ogl_resolve_module(rtx_module_handle, "ogl_module_stop"));
    auto get_status = reinterpret_cast<const ogl_rtx_status *(*)()>(
        ogl_resolve_module(rtx_module_handle, "ogl_rtx_get_status"));

    if (!info || !start || !rtx_stop || !get_status) {
        LL_WARNS("OGLRTX") << "RTX module ABI incomplete; keeping OpenGL." << LL_ENDL;
        ogl_shutdown_rtx_module();
        return;
    }

    const ogl_module_info *identity = info();
    if (!identity || identity->struct_size < sizeof(ogl_module_info) ||
        identity->abi_version != OGL_MODULE_ABI_VERSION || !identity->id ||
        std::strcmp(identity->id, "de.nexverse.ogl.rtx") != 0) {
        LL_WARNS("OGLRTX") << "RTX module identity mismatch; keeping OpenGL." << LL_ENDL;
        ogl_shutdown_rtx_module();
        return;
    }

    rtx_submit_frame = reinterpret_cast<ogl_rtx_submit_frame_fn>(
        ogl_resolve_module(rtx_module_handle, "ogl_rtx_submit_frame"));
    rtx_submit_geometry = reinterpret_cast<ogl_rtx_submit_geometry_fn>(
        ogl_resolve_module(rtx_module_handle, "ogl_rtx_submit_geometry"));
    rtx_received_triangles = reinterpret_cast<ogl_rtx_triangles_fn>(
        ogl_resolve_module(rtx_module_handle, "ogl_rtx_received_triangles"));
    rtx_copy_shadow_debug = reinterpret_cast<ogl_rtx_copy_shadow_debug_fn>(
        ogl_resolve_module(rtx_module_handle, "ogl_rtx_copy_shadow_debug"));
    rtx_submit_depth = reinterpret_cast<ogl_rtx_submit_depth_fn>(
        ogl_resolve_module(rtx_module_handle, "ogl_rtx_submit_depth_debug"));

    const ogl_host_api api{sizeof(ogl_host_api), OGL_MODULE_ABI_VERSION, host_log};
    if (start(&api) != 0) {
        LL_WARNS("OGLRTX") << "RTX capability initialization rejected; keeping OpenGL."
                            << LL_ENDL;
        ogl_shutdown_rtx_module();
        return;
    }
    const ogl_rtx_status *status = get_status();
    if (status && status->struct_size >= sizeof(ogl_rtx_status) &&
        status->status_version == OGL_RTX_STATUS_VERSION) {
        rtx_hardware_ready = status->hardware_ready != 0;
        rtx_rendering_enabled = status->rendering_active != 0;
        LL_INFOS("OGLRTX") << "GPU " << status->gpu_name
            << " hardware_ready=" << status->hardware_ready
            << " rendering_active=" << status->rendering_active
            << ". OpenGL remains the active renderer." << LL_ENDL;
    }
#endif
}

void ogl_shutdown_rtx_module() {
#if LL_LINUX || LL_WINDOWS
    if (rtx_module_handle) {
        if (rtx_stop) rtx_stop();
        ogl_unload_module(rtx_module_handle);
    }
    rtx_stop = nullptr;
    rtx_module_handle = nullptr;
    rtx_hardware_ready = false;
    rtx_rendering_enabled = false;
    rtx_submit_frame = nullptr;
    rtx_submit_geometry = nullptr;
    rtx_received_triangles = nullptr;
    rtx_copy_shadow_debug = nullptr;
    rtx_submit_depth = nullptr;
#endif
}

bool ogl_rtx_hardware_supported() {
#if LL_LINUX || LL_WINDOWS
    return rtx_hardware_ready;
#else
    return false;
#endif
}

bool ogl_rtx_rendering_active() {
#if LL_LINUX || LL_WINDOWS
    return rtx_rendering_enabled;
#else
    return false;
#endif
}

void ogl_rtx_submit_camera_sun(unsigned width, unsigned height,
    const float* camera_position, const float* camera_forward,
    const float* camera_right, const float* camera_up,
    float vertical_fov_radians, float aspect_ratio,
    float near_clip_m, float far_clip_m,
    const float* sun_direction) {
#if LL_LINUX || LL_WINDOWS
    if (!rtx_submit_frame || !rtx_hardware_ready ||
        !camera_position || !camera_forward || !camera_right ||
        !camera_up || !sun_direction) return;
    ogl_rtx_frame_state frame{};
    frame.struct_size = sizeof(frame);
    frame.frame_version = OGL_RTX_FRAME_VERSION;
    frame.width = width;
    frame.height = height;
    for (unsigned i=0; i<3; ++i) {
        frame.camera_position[i] = camera_position[i];
        frame.camera_forward[i] = camera_forward[i];
        frame.camera_right[i] = camera_right[i];
        frame.camera_up[i] = camera_up[i];
        frame.sun_direction[i] = sun_direction[i];
    }
    frame.vertical_fov_radians = vertical_fov_radians;
    frame.aspect_ratio = aspect_ratio;
    frame.near_clip_m = near_clip_m;
    frame.far_clip_m = far_clip_m;
    rtx_submit_frame(&frame);
#endif
}

void ogl_rtx_submit_world_triangles(const float *triangle_xyz, unsigned count) {
#if LL_LINUX || LL_WINDOWS
    if (!rtx_submit_geometry || !rtx_hardware_ready ||
        count > OGL_RTX_MAX_SCENE_TRIANGLES || (count && !triangle_xyz)) return;
    // Geometry data layout is 9 tightly packed floats per triangle.
    static_assert(sizeof(ogl_rtx_triangle) == 9 * sizeof(float), "RTX ABI padding mismatch");
    rtx_submit_geometry(reinterpret_cast<const ogl_rtx_triangle*>(triangle_xyz), count);
#endif
}


// Firestorm's future diagnostic compositor can request a _current_ sample.
// The GPU module remains a black box; no OpenGL/Vulkan calls here.
bool ogl_rtx_copy_debug_shadow_sample(std::uint8_t* pixels,
    std::size_t capacity, std::uint64_t* geometry_revision,
    std::uint64_t* sample_sequence)
{
#if LL_LINUX || LL_WINDOWS
    if (!rtx_module_handle || !rtx_hardware_ready ||
        !rtx_copy_shadow_debug || !pixels ||
        capacity < OGL_RTX_SHADOW_DEBUG_PIXELS)
        return false;
    ogl_rtx_shadow_debug debug{};
    debug.struct_size = sizeof(debug);
    debug.version = OGL_RTX_SHADOW_DEBUG_VERSION;
    if (rtx_copy_shadow_debug(&debug) != 1 ||
        debug.width != 8 || debug.height != 8)
        return false;
    unsigned hit_count = 0;
    for (const auto pixel : debug.pixels) {
        if (pixel > 1) return false;
        hit_count += pixel;
    }
    if (hit_count != debug.occluded) return false;
    std::memcpy(pixels, debug.pixels, OGL_RTX_SHADOW_DEBUG_PIXELS);
    if (geometry_revision) *geometry_revision = debug.geometry_revision;
    if (sample_sequence) *sample_sequence = debug.sample_sequence;
    return true;
#else
    (void)pixels;
    (void)capacity;
    (void)geometry_revision;
    (void)sample_sequence;
    return false;
#endif
}

bool ogl_rtx_copy_debug_shadow_mask(std::uint8_t* pixels,
    std::size_t capacity, std::uint64_t* geometry_revision)
{
    return ogl_rtx_copy_debug_shadow_sample(
        pixels, capacity, geometry_revision, nullptr);
}


// One optional readback of a whole-viewport depth grid. GPU-side NEAREST
// depth-only blit downsamples to 8x8 first, so one glReadPixels() replaces
// 64 independent readbacks. Sampling centres match camera ray centres.
// This remains a synchronous developer-only diagnostic (once per minute).
void ogl_rtx_probe_deferred_depth(unsigned texture,
    unsigned width, unsigned height)
{
#if LL_LINUX || LL_WINDOWS
    static const bool enabled = [] {
        const char* env = std::getenv("OGL_RTX_DEPTH_PROBE");
        return env && std::strcmp(env, "1") == 0;
    }();
    if (!enabled || !rtx_hardware_ready || !rtx_submit_depth ||
        !rtx_received_triangles || rtx_received_triangles() == 0 || !texture ||
        width < 8 || height < 8 || width > 32768 || height > 32768)
        return;
    using Clock = std::chrono::steady_clock;
    static Clock::time_point previous{};
    const auto now = Clock::now();
    if (previous != Clock::time_point{} &&
        now - previous < std::chrono::seconds(60))
        return;
    previous = now; // even failure must never flood GL with readbacks

    // Preserve both framebuffer bindings, renderbuffer binding, scissor and
    // all pack state touched by glReadPixels(). No changes to color targets.
    GLint old_read = 0, old_draw = 0, old_renderbuffer = 0;
    GLint old_pack_buffer = 0, old_pack_alignment = 4;
    GLint old_row_length = 0, old_skip_pixels = 0, old_skip_rows = 0;
    const GLboolean old_scissor = glIsEnabled(GL_SCISSOR_TEST);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &old_read);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &old_draw);
    glGetIntegerv(GL_RENDERBUFFER_BINDING, &old_renderbuffer);
    glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &old_pack_buffer);
    glGetIntegerv(GL_PACK_ALIGNMENT, &old_pack_alignment);
    glGetIntegerv(GL_PACK_ROW_LENGTH, &old_row_length);
    glGetIntegerv(GL_PACK_SKIP_PIXELS, &old_skip_pixels);
    glGetIntegerv(GL_PACK_SKIP_ROWS, &old_skip_rows);

    GLuint source = 0, reduced = 0, depth_renderbuffer = 0;
    glGenFramebuffers(1, &source);
    glGenFramebuffers(1, &reduced);
    glGenRenderbuffers(1, &depth_renderbuffer);
    if (source && reduced && depth_renderbuffer) {
        glDisable(GL_SCISSOR_TEST); // scissor must not crop the blit
        glBindFramebuffer(GL_READ_FRAMEBUFFER, source);
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                               GL_TEXTURE_2D, texture, 0);
        glReadBuffer(GL_NONE);

        glBindRenderbuffer(GL_RENDERBUFFER, depth_renderbuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 8, 8);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, reduced);
        glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                  GL_RENDERBUFFER, depth_renderbuffer);
        glDrawBuffer(GL_NONE);
        if (glCheckFramebufferStatus(GL_READ_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE &&
            glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
            // Pixel (x,y) maps to UV ((x+.5)/8, (y+.5)/8). Both GL and
            // Vulkan diagnostic shader use origin=bottom-left here.
            glBlitFramebuffer(0, 0, width, height, 0, 0, 8, 8,
                              GL_DEPTH_BUFFER_BIT, GL_NEAREST);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, reduced);
            glReadBuffer(GL_NONE);
            glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
            glPixelStorei(GL_PACK_ALIGNMENT, 4);
            glPixelStorei(GL_PACK_ROW_LENGTH, 0);
            glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
            glPixelStorei(GL_PACK_SKIP_ROWS, 0);
            ogl_rtx_depth_debug sample{};
            sample.struct_size = sizeof(sample);
            sample.version = OGL_RTX_DEPTH_DEBUG_VERSION;
            sample.viewport_width = width;
            sample.viewport_height = height;
            sample.grid_width = 8;
            sample.grid_height = 8;
            glReadPixels(0, 0, 8, 8, GL_DEPTH_COMPONENT, GL_FLOAT,
                         sample.normalized_depth);
            if (glGetError() == GL_NO_ERROR && rtx_submit_depth(&sample) == 1)
                LL_INFOS("OGLRTX") << "Deferred depth full-view grid captured "
                    << width << "x" << height << " -> 8x8 (64 aligned samples); "
                    << "no visible RT output." << LL_ENDL;
        }
    }
    // Release the temporary framebuffer resources while the correct context
    // is current. Restore bindings even on allocation/FBO failure.
    glBindFramebuffer(GL_READ_FRAMEBUFFER, old_read);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, old_draw);
    glBindRenderbuffer(GL_RENDERBUFFER, old_renderbuffer);
    if (source) glDeleteFramebuffers(1, &source);
    if (reduced) glDeleteFramebuffers(1, &reduced);
    if (depth_renderbuffer) glDeleteRenderbuffers(1, &depth_renderbuffer);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, old_pack_buffer);
    glPixelStorei(GL_PACK_ALIGNMENT, old_pack_alignment);
    glPixelStorei(GL_PACK_ROW_LENGTH, old_row_length);
    glPixelStorei(GL_PACK_SKIP_PIXELS, old_skip_pixels);
    glPixelStorei(GL_PACK_SKIP_ROWS, old_skip_rows);
    if (old_scissor) glEnable(GL_SCISSOR_TEST);
    else glDisable(GL_SCISSOR_TEST);
#else
    (void)texture;
    (void)width;
    (void)height;
#endif
}

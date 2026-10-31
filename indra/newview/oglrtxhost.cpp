#include "llviewerprecompiledheaders.h"
#include "oglrtxhost.h"
#include "llglheaders.h"
#include "../../ogl/modules/include/ogl/ogl_rtx_api.h"
#include "../../ogl/modules/include/ogl/ogl_world_shadow_policy.h"

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
using ogl_track_object_fn = int (*)(const uint8_t*,const ogl_rtx_triangle*,
                                     uint32_t,const float*);
using ogl_track_pose_fn = int (*)(const uint8_t*,const float*);
using ogl_forget_object_fn = int (*)(const uint8_t*);
ogl_track_object_fn rtx_track_object = nullptr;
ogl_track_pose_fn rtx_track_pose = nullptr;
ogl_forget_object_fn rtx_forget_object = nullptr;
using ogl_rtx_triangles_fn = uint32_t (*)();
ogl_rtx_triangles_fn rtx_received_triangles = nullptr;
using ogl_rtx_copy_shadow_debug_fn = int (*)(ogl_rtx_shadow_debug*);
ogl_rtx_copy_shadow_debug_fn rtx_copy_shadow_debug = nullptr;
using ogl_rtx_copy_world_shadow_fn = int (*)(ogl_rtx_world_shadow_sample*);
ogl_rtx_copy_world_shadow_fn rtx_copy_world_shadow = nullptr;
using ogl_rtx_submit_dense_fn = int (*)(const ogl_rtx_dense_depth*);
using ogl_rtx_copy_dense_fn = int (*)(ogl_rtx_dense_shadow*);
ogl_rtx_submit_dense_fn rtx_submit_dense = nullptr;
ogl_rtx_copy_dense_fn rtx_copy_dense = nullptr;
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
    rtx_track_object = reinterpret_cast<ogl_track_object_fn>(
        ogl_resolve_module(rtx_module_handle,"ogl_rtx_track_object"));
    rtx_track_pose = reinterpret_cast<ogl_track_pose_fn>(
        ogl_resolve_module(rtx_module_handle,"ogl_rtx_track_transform"));
    rtx_forget_object = reinterpret_cast<ogl_forget_object_fn>(
        ogl_resolve_module(rtx_module_handle,"ogl_rtx_forget_object"));
    rtx_received_triangles = reinterpret_cast<ogl_rtx_triangles_fn>(
        ogl_resolve_module(rtx_module_handle, "ogl_rtx_received_triangles"));
    rtx_copy_shadow_debug = reinterpret_cast<ogl_rtx_copy_shadow_debug_fn>(
        ogl_resolve_module(rtx_module_handle, "ogl_rtx_copy_shadow_debug"));
    rtx_copy_world_shadow = reinterpret_cast<ogl_rtx_copy_world_shadow_fn>(
        ogl_resolve_module(rtx_module_handle,"ogl_rtx_copy_world_shadow_sample"));
    rtx_submit_dense=reinterpret_cast<ogl_rtx_submit_dense_fn>(
        ogl_resolve_module(rtx_module_handle,"ogl_rtx_submit_dense_depth"));
    rtx_copy_dense=reinterpret_cast<ogl_rtx_copy_dense_fn>(
        ogl_resolve_module(rtx_module_handle,"ogl_rtx_copy_dense_shadow"));
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
    rtx_track_object = nullptr;
    rtx_track_pose = nullptr;
    rtx_forget_object = nullptr;
    rtx_received_triangles = nullptr;
    rtx_copy_shadow_debug = nullptr;
    rtx_copy_world_shadow = nullptr;
    rtx_submit_dense=nullptr;
    rtx_copy_dense=nullptr;
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


bool ogl_rtx_track_local_object(const std::uint8_t* uuid,
    const float* local_xyz,unsigned count,const float* pose)
{
#if LL_LINUX || LL_WINDOWS
    if (!rtx_hardware_ready || !rtx_track_object || !uuid || !local_xyz ||
        !pose || count==0 || count>OGL_RTX_MAX_SCENE_TRIANGLES) return false;
    return rtx_track_object(uuid,reinterpret_cast<const ogl_rtx_triangle*>(local_xyz),
                            count,pose)==1;
#else
    return false;
#endif
}
bool ogl_rtx_update_object_pose(const std::uint8_t* uuid,const float* pose)
{
#if LL_LINUX || LL_WINDOWS
    return rtx_hardware_ready && rtx_track_pose && uuid && pose &&
           rtx_track_pose(uuid,pose)==1;
#else
    return false;
#endif
}
void ogl_rtx_forget_local_object(const std::uint8_t* uuid)
{
#if LL_LINUX || LL_WINDOWS
    if (rtx_forget_object && uuid) rtx_forget_object(uuid);
#endif
}

// Opt-in 64x36 Vulkan-guided sun shadow on the ACTUAL OpenGL deferred
// sunlight target. No fullscreen 8x8 UI effect: each fragment reads current
// GL depth, its own Vulkan source tile, and rejects stale geometry.
bool ogl_rtx_compose_dense_sun_shadow(unsigned depth_texture,
    unsigned width,unsigned height,const float* eye,const float* forward,
    const float* right,const float* up,float fov,float aspect,
    float near_clip,float far_clip,const float* sun)
{
#if LL_LINUX || LL_WINDOWS
    static const bool opt_in=[]() {
        const char* v=std::getenv("OGL_RTX_WORLD_COMPOSITE");
        const char* d=std::getenv("OGL_RTX_DENSE_PREVIEW");
        return v && d && std::strcmp(v,"1")==0 &&
               std::strcmp(d,"1")==0;
    }();
    if (!opt_in || !rtx_hardware_ready || !rtx_copy_dense ||
        !depth_texture || !glIsTexture(depth_texture) ||
        !eye || !forward || !right || !up || !sun ||
        width<64u || height<36u)
        return false;
    ogl_rtx_frame_state current{};
    current.struct_size=sizeof(current);
    current.frame_version=OGL_RTX_FRAME_VERSION;
    current.width=width;current.height=height;
    for (unsigned i=0;i<3;++i) {
        current.camera_position[i]=eye[i];
        current.camera_forward[i]=forward[i];
        current.camera_right[i]=right[i];
        current.camera_up[i]=up[i];
        current.sun_direction[i]=sun[i];
    }
    current.vertical_fov_radians=fov;
    current.aspect_ratio=aspect;
    current.near_clip_m=near_clip;
    current.far_clip_m=far_clip;
    ogl_rtx_dense_shadow source{};
    source.struct_size=sizeof(source);
    source.version=OGL_RTX_DENSE_VERSION;
    if (rtx_copy_dense(&source)!=1 ||
        !ogl::valid_dense_shadow(source,source.geometry_revision,current) ||
        source.shadow_pixels==0)
        return false;

    // Strict depth-gated fragment pass. No bilinear interpolation; a tile
    // can only darken a pixel whose LIVE deferred depth agrees with the
    // GPU input depth and whose primary Vulkan ray hit was validated.
    static const char* vertex_source=R"GLSL(
#version 330 core
void main() {
    vec2 p=vec2((gl_VertexID==1)?3.0:-1.0,
                (gl_VertexID==2)?3.0:-1.0);
    gl_Position=vec4(p,0.0,1.0);
}
)GLSL";
    static const char* fragment_source=R"GLSL(
#version 330 core
uniform sampler2D uLiveDepth;
uniform sampler2D uVulkanTile;
uniform ivec2 uViewport;
out vec4 outColor;
void main() {
    ivec2 pos=ivec2(gl_FragCoord.xy);
    if (any(lessThan(pos,ivec2(0))) ||
        any(greaterThanEqual(pos,uViewport))) discard;
    ivec2 t=clamp(pos*ivec2(64,36)/uViewport,
                  ivec2(0),ivec2(63,35));
    vec4 ray=texelFetch(uVulkanTile,t,0);
    if (ray.r<0.5 || ray.g<0.5) discard;
    float live=texelFetch(uLiveDepth,pos,0).r;
    // Sparse source meshes, rigged avatars and moving world geometry must
    // not be darkened unless current depth still matches capture depth.
    if (live>=0.999999 || abs(live-ray.b)>0.00001) discard;
    // Limited first-gen direct-sun attenuation, not GI or path tracing.
    outColor=vec4(0.0,0.0,0.0,0.36);
}
)GLSL";
    static GLuint program=0,vao=0,tile_texture=0;
    static bool compile_failed=false;
    static uint64_t last_upload=0;
    if (compile_failed) return false;
    if (!program) {
        const auto compile=[](GLenum type,const char* source)->GLuint {
            GLuint shader=glCreateShader(type);
            if (!shader) return 0;
            glShaderSource(shader,1,&source,nullptr);
            glCompileShader(shader);
            GLint good=0;
            glGetShaderiv(shader,GL_COMPILE_STATUS,&good);
            if (!good) {glDeleteShader(shader);return 0;}
            return shader;
        };
        const GLuint vs=compile(GL_VERTEX_SHADER,vertex_source);
        const GLuint fs=compile(GL_FRAGMENT_SHADER,fragment_source);
        if (vs && fs) {
            program=glCreateProgram();
            glAttachShader(program,vs);
            glAttachShader(program,fs);
            glLinkProgram(program);
            GLint linked=0;
            glGetProgramiv(program,GL_LINK_STATUS,&linked);
            if (!linked) {glDeleteProgram(program);program=0;}
        }
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        if (!program) {
            compile_failed=true;
            LL_WARNS("OGLRTX") << "Experimental OpenGL shadow compositor "
                               << "shader rejected; original renderer kept."
                               << LL_ENDL;
            return false;
        }
        glGenVertexArrays(1,&vao);
        glGenTextures(1,&tile_texture);
        if (!vao || !tile_texture) {
            compile_failed=true;
            return false;
        }
        LL_INFOS("OGLRTX") << "Experimental 64x36 Vulkan-to-OpenGL "
                             "depth-gated sunlight compositor initialized"
                          << LL_ENDL;
    }
    GLint old_program=0,old_vao=0,old_active=0;
    GLint old_tex0=0,old_tex1=0;
    GLint old_viewport[4]{};
    GLint old_blend_src_rgb=0,old_blend_dst_rgb=0;
    GLint old_blend_src_alpha=0,old_blend_dst_alpha=0;
    GLint old_equation_rgb=0,old_equation_alpha=0;
    GLboolean old_blend=glIsEnabled(GL_BLEND);
    GLboolean old_depth=glIsEnabled(GL_DEPTH_TEST);
    GLboolean old_cull=glIsEnabled(GL_CULL_FACE);
    GLboolean old_scissor=glIsEnabled(GL_SCISSOR_TEST);
    glGetIntegerv(GL_CURRENT_PROGRAM,&old_program);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&old_vao);
    glGetIntegerv(GL_ACTIVE_TEXTURE,&old_active);
    glGetIntegerv(GL_VIEWPORT,old_viewport);
    glGetIntegerv(GL_BLEND_SRC_RGB,&old_blend_src_rgb);
    glGetIntegerv(GL_BLEND_DST_RGB,&old_blend_dst_rgb);
    glGetIntegerv(GL_BLEND_SRC_ALPHA,&old_blend_src_alpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA,&old_blend_dst_alpha);
    glGetIntegerv(GL_BLEND_EQUATION_RGB,&old_equation_rgb);
    glGetIntegerv(GL_BLEND_EQUATION_ALPHA,&old_equation_alpha);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D,&old_tex0);
    glActiveTexture(GL_TEXTURE1);
    glGetIntegerv(GL_TEXTURE_BINDING_2D,&old_tex1);
    glBindTexture(GL_TEXTURE_2D,tile_texture);
    if (last_upload!=source.sample_sequence) {
        std::array<float,OGL_RTX_DENSE_PIXELS*4u> pixels{};
        for (unsigned i=0;i<OGL_RTX_DENSE_PIXELS;++i) {
            pixels[i*4+0]=float(source.valid_mask[i]);
            pixels[i*4+1]=float(source.shadow_mask[i]);
            pixels[i*4+2]=source.capture_depth[i];
        }
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA32F,
                     OGL_RTX_DENSE_WIDTH,OGL_RTX_DENSE_HEIGHT,0,
                     GL_RGBA,GL_FLOAT,pixels.data());
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        last_upload=source.sample_sequence;
    }
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,depth_texture);
    glUseProgram(program);
    glUniform1i(glGetUniformLocation(program,"uLiveDepth"),0);
    glUniform1i(glGetUniformLocation(program,"uVulkanTile"),1);
    glUniform2i(glGetUniformLocation(program,"uViewport"),width,height);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA,
                        GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquationSeparate(GL_FUNC_ADD,GL_FUNC_ADD);
    glViewport(0,0,width,height);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES,0,3);
    // Restore everything modified; subsequent Firestorm lighting/UI passes
    // must observe exactly their original global GL state.
    glBindVertexArray(old_vao);
    glUseProgram(old_program);
    glViewport(old_viewport[0],old_viewport[1],
               old_viewport[2],old_viewport[3]);
    glBlendFuncSeparate(old_blend_src_rgb,old_blend_dst_rgb,
                        old_blend_src_alpha,old_blend_dst_alpha);
    glBlendEquationSeparate(old_equation_rgb,old_equation_alpha);
    if (!old_blend) glDisable(GL_BLEND);
    if (old_depth) glEnable(GL_DEPTH_TEST);
    if (old_cull) glEnable(GL_CULL_FACE);
    if (old_scissor) glEnable(GL_SCISSOR_TEST);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D,old_tex1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,old_tex0);
    glActiveTexture(old_active);
    static uint64_t last_logged=0;
    if (last_logged!=source.sample_sequence) {
        last_logged=source.sample_sequence;
        LL_INFOS("OGLRTX") << "OGL RTX dense world sunlight composed: "
                           << source.shadow_pixels << " GPU shadow tile(s) / "
                           << source.matched_pixels
                           << " depth-matched, at 64x36 sample resolution; "
                           << "current OpenGL depth gates every output pixel"
                           << LL_ENDL;
    }
    return true;
#else
    (void)depth_texture;(void)width;(void)height;(void)eye;(void)forward;
    (void)right;(void)up;(void)fov;(void)aspect;
    (void)near_clip;(void)far_clip;(void)sun;
    return false;
#endif
}

// Separate GPU->world bridge. This reports an eligible *input*, never
// enables shadow compositing: CURRENT per-pixel GL depth and opaque material
// must still be checked by a real full-resolution compositor.
bool ogl_rtx_world_shadow_source_ready(unsigned width,unsigned height,
    const float* eye,const float* forward,const float* right,
    const float* up,float fov,float aspect,float near_clip,float far_clip,
    const float* sun,unsigned* matched,unsigned* occluded,
    std::uint64_t* sequence)
{
#if LL_LINUX || LL_WINDOWS
    if (!rtx_hardware_ready || !rtx_copy_world_shadow ||
        !eye || !forward || !right || !up || !sun ||
        !matched || !occluded || !sequence) return false;
    ogl_rtx_frame_state current{};
    current.struct_size=sizeof(current);
    current.frame_version=OGL_RTX_FRAME_VERSION;
    current.width=width;
    current.height=height;
    for (unsigned i=0;i<3;++i) {
        current.camera_position[i]=eye[i];
        current.camera_forward[i]=forward[i];
        current.camera_right[i]=right[i];
        current.camera_up[i]=up[i];
        current.sun_direction[i]=sun[i];
    }
    current.vertical_fov_radians=fov;
    current.aspect_ratio=aspect;
    current.near_clip_m=near_clip;
    current.far_clip_m=far_clip;
    ogl_rtx_world_shadow_sample tile{};
    tile.struct_size=sizeof(tile);
    tile.version=OGL_RTX_WORLD_SHADOW_VERSION;
    if (rtx_copy_world_shadow(&tile)!=1 ||
        !ogl::valid_world_shadow_sample(
            tile,tile.geometry_revision,current))
        return false;
    // Do NOT invent a high-res shadow map by stretching these 64 samples.
    *matched=tile.matched_surface_pixels;
    *occluded=tile.occluded_pixels;
    *sequence=tile.sample_sequence;
    return true;
#else
    (void)width;(void)height;(void)eye;(void)forward;(void)right;
    (void)up;(void)fov;(void)aspect;(void)near_clip;(void)far_clip;
    (void)sun;(void)matched;(void)occluded;(void)sequence;
    return false;
#endif
}

// Firestorm's future diagnostic compositor can request a _current_ sample.
// The GPU module remains a black box; no OpenGL/Vulkan calls here.
bool ogl_rtx_copy_debug_shadow_sample(std::uint8_t* pixels,
    std::size_t capacity, std::uint64_t* geometry_revision,
    std::uint64_t* sample_sequence,
    std::uint8_t* surface_mask, std::uint32_t* surfaces,
    std::uint32_t* background)
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
    unsigned surface_count = 0;
    for (unsigned i=0; i<OGL_RTX_SHADOW_DEBUG_PIXELS; ++i) {
        if (debug.surface_mask[i] > 1) return false;
        surface_count += debug.surface_mask[i];
        if (debug.surface_pixels+debug.background_pixels==64 &&
            debug.pixels[i] && !debug.surface_mask[i]) return false;
    }
    if (debug.surface_pixels+debug.background_pixels==64 &&
        surface_count!=debug.surface_pixels) return false;
    std::memcpy(pixels, debug.pixels, OGL_RTX_SHADOW_DEBUG_PIXELS);
    if (surface_mask)
        std::memcpy(surface_mask, debug.surface_mask, OGL_RTX_SHADOW_DEBUG_PIXELS);
    if (geometry_revision) *geometry_revision = debug.geometry_revision;
    if (sample_sequence) *sample_sequence = debug.sample_sequence;
    if (surfaces) *surfaces = debug.surface_pixels;
    if (background) *background = debug.background_pixels;
    return true;
#else
    (void)pixels;
    (void)capacity;
    (void)geometry_revision;
    (void)sample_sequence;
    (void)surface_mask;
    (void)surfaces;
    (void)background;
    return false;
#endif
}

bool ogl_rtx_copy_debug_shadow_mask(std::uint8_t* pixels,
    std::size_t capacity, std::uint64_t* geometry_revision)
{
    return ogl_rtx_copy_debug_shadow_sample(
        pixels, capacity, geometry_revision, nullptr, nullptr, nullptr, nullptr);
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
    static const bool dense_enabled=[] {
        const char* env=std::getenv("OGL_RTX_DENSE_PREVIEW");
        return env && std::strcmp(env,"1")==0;
    }();
    if (!enabled || !rtx_hardware_ready ||
        (dense_enabled ? !rtx_submit_dense : !rtx_submit_depth) ||
        !rtx_received_triangles || rtx_received_triangles() == 0 || !texture ||
        width < 8 || height < 8 || width > 32768 || height > 32768)
        return;
    using Clock = std::chrono::steady_clock;
    static Clock::time_point previous{};
    const auto now = Clock::now();
    if (previous != Clock::time_point{} &&
        now - previous < (dense_enabled ?
            std::chrono::seconds(2) : std::chrono::seconds(60)))
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

    const unsigned grid_w=dense_enabled ? OGL_RTX_DENSE_WIDTH : 8u;
    const unsigned grid_h=dense_enabled ? OGL_RTX_DENSE_HEIGHT : 8u;
    if (width<grid_w || height<grid_h) return;
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
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24,
                              grid_w,grid_h);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, reduced);
        glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                  GL_RENDERBUFFER, depth_renderbuffer);
        glDrawBuffer(GL_NONE);
        if (glCheckFramebufferStatus(GL_READ_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE &&
            glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
            // Pixel (x,y) maps to UV ((x+.5)/8, (y+.5)/8). Both GL and
            // Vulkan diagnostic shader use origin=bottom-left here.
            glBlitFramebuffer(0, 0, width, height, 0, 0,
                              grid_w,grid_h,
                              GL_DEPTH_BUFFER_BIT, GL_NEAREST);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, reduced);
            glReadBuffer(GL_NONE);
            glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
            glPixelStorei(GL_PACK_ALIGNMENT, 4);
            glPixelStorei(GL_PACK_ROW_LENGTH, 0);
            glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
            glPixelStorei(GL_PACK_SKIP_ROWS, 0);
            if (dense_enabled) {
                ogl_rtx_dense_depth sample{};
                sample.struct_size=sizeof(sample);
                sample.version=OGL_RTX_DENSE_VERSION;
                sample.viewport_width=width;
                sample.viewport_height=height;
                sample.width=grid_w;
                sample.height=grid_h;
                glReadPixels(0,0,grid_w,grid_h,GL_DEPTH_COMPONENT,GL_FLOAT,
                             sample.normalized_depth);
                if (glGetError()==GL_NO_ERROR &&
                    rtx_submit_dense(&sample)==1)
                    LL_INFOS("OGLRTX")
                        << "Deferred depth dense grid captured "
                        << width << "x" << height << " -> "
                        << grid_w << "x" << grid_h
                        << " (experimental Vulkan ray input)" << LL_ENDL;
            } else {
                ogl_rtx_depth_debug sample{};
                sample.struct_size = sizeof(sample);
                sample.version = OGL_RTX_DEPTH_DEBUG_VERSION;
                sample.viewport_width = width;
                sample.viewport_height = height;
                sample.grid_width = 8;
                sample.grid_height = 8;
                glReadPixels(0, 0, 8, 8, GL_DEPTH_COMPONENT, GL_FLOAT,
                             sample.normalized_depth);
                if (glGetError() == GL_NO_ERROR &&
                    rtx_submit_depth(&sample) == 1)
                    LL_INFOS("OGLRTX")
                        << "Deferred depth full-view grid captured "
                        << width << "x" << height
                        << " -> 8x8 (64 aligned samples); no visible RT output."
                        << LL_ENDL;
            }
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

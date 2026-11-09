#ifndef OGL_RTX_HOST_H
#define OGL_RTX_HOST_H

#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <vector>


// P05/5: convert ONLY an already-decoded, opaque, packed RGB/RGBA
// Firestorm LLImageRaw into a bounded RGBA8 snapshot for the private
// Vulkan material experiment. This is not an OpenSim texture decoder.
// Pixels are copied synchronously while the image's shared-data lock
// is held by the caller; never store the source pointer or a texture ID.
// Nearest-neighbor downsampling retains aspect ratio, max 64x64.
// Explicit output reset on every failure; no GL readback, logging or I/O.
inline bool ogl_rtx_thumbnail_opaque_rgba8(
    const std::uint8_t* pixels, std::size_t data_size,
    std::uint32_t width,std::uint32_t height,std::uint32_t components,
    std::vector<std::uint8_t>& result,
    std::uint32_t& out_width,std::uint32_t& out_height)
{
    result.clear();
    out_width=out_height=0;
    if (!pixels || width==0 || height==0 || width>1024 || height>1024 ||
        (components!=3 && components!=4)) return false;
    const std::size_t needed=std::size_t(width)*height*components;
    if (data_size<needed) return false;
    // Any transparent source pixel makes the ONE opaque texture mode
    // unsuitable; never pretend it can blend with the actual 3D scene.
    if (components==4)
        for (std::size_t px=3;px<needed;px+=4)
            if (pixels[px]!=255u) return false;
    const std::uint32_t maximum=width>height ? width : height;
    const std::uint32_t target_width=maximum>64u ?
        std::max(1u,(width*64u)/maximum) : width;
    const std::uint32_t target_height=maximum>64u ?
        std::max(1u,(height*64u)/maximum) : height;
    if (!target_width || !target_height) return false;
    std::vector<std::uint8_t> converted(
        std::size_t(target_width)*target_height*4u,255u);
    for (std::uint32_t y=0;y<target_height;++y) {
        const auto sy=std::uint32_t(
            (std::uint64_t(y)*height)/target_height);
        for (std::uint32_t x=0;x<target_width;++x) {
            const auto sx=std::uint32_t(
                (std::uint64_t(x)*width)/target_width);
            const auto input=(std::size_t(sy)*width+sx)*components;
            const auto output=(std::size_t(y)*target_width+x)*4u;
            converted[output]=pixels[input];
            converted[output+1]=pixels[input+1];
            converted[output+2]=pixels[input+2];
        }
    }
    result.swap(converted);
    out_width=target_width;
    out_height=target_height;
    return true;
}

// P05/6: four explicit opaque RGBA8 materials, packed to a single
// 64x64 GPU atlas with four independent 32x32 tiles. No image
// download, texture UUID exposure or forced decode. All pixel pointers
// are borrowed for this synchronous copy; no persistent source refs.
struct OGLRgbaTileView {
    const std::uint8_t* rgba = nullptr;
    std::size_t byte_count = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
};

inline bool ogl_rtx_pack_rgba_atlas_2x2(
    const OGLRgbaTileView* tiles,std::uint32_t tile_count,
    std::vector<std::uint8_t>& atlas)
{
    atlas.clear();
    if (!tiles || tile_count==0 || tile_count>4) return false;
    for (std::uint32_t slot=0;slot<tile_count;++slot) {
        const auto& tile=tiles[slot];
        if (!tile.rgba || tile.width==0 || tile.height==0 ||
            tile.width>64 || tile.height>64 ||
            tile.byte_count<std::size_t(tile.width)*tile.height*4u)
            return false;
        for (std::size_t pixel=3;pixel<std::size_t(tile.width)*
             tile.height*4u;pixel+=4)
            if (tile.rgba[pixel]!=255u) return false;
    }
    // Unused slots are opaque white, but may not be referenced by
    // geometry: the material-slot sidecar requires <tile_count.
    std::vector<std::uint8_t> packed(64u*64u*4u,255u);
    for (std::uint32_t slot=0;slot<tile_count;++slot) {
        const auto& tile=tiles[slot];
        const auto sx=(slot%2u)*32u;
        const auto sy=(slot/2u)*32u;
        for (std::uint32_t y=0;y<32;++y)
            for (std::uint32_t x=0;x<32;++x) {
                const auto from=(
                    std::size_t((std::uint64_t(y)*tile.height)/32u)*
                    tile.width+std::size_t((std::uint64_t(x)*tile.width)/32u))*4u;
                const auto to=(std::size_t(sy+y)*64u+sx+x)*4u;
                for (unsigned channel=0;channel<4;++channel)
                    packed[to+channel]=tile.rgba[from+channel];
            }
    }
    atlas.swap(packed);
    return true;
}

// P05/7: optional 16-material atlas, deliberately separate from the
// original four-slot 2x2 atlas. Original CPU layout/ShaderMode6 remains
// byte-for-byte stable. Four-by-four layout uses 16x16 texels per tile.
// Source pixels are borrowed only for the synchronous bounded copy;
// no asset lookup, upload scheduling, cache or renderer presentation.
inline bool ogl_rtx_pack_rgba_atlas_4x4(
    const OGLRgbaTileView* tiles,std::uint32_t tile_count,
    std::vector<std::uint8_t>& atlas)
{
    atlas.clear();
    if (!tiles || tile_count==0 || tile_count>16u) return false;
    for (std::uint32_t slot=0;slot<tile_count;++slot) {
        const auto& tile=tiles[slot];
        if (!tile.rgba || !tile.width || !tile.height ||
            tile.width>64u || tile.height>64u ||
            tile.byte_count<std::size_t(tile.width)*tile.height*4u)
            return false;
        for (std::size_t i=3;i<std::size_t(tile.width)*tile.height*4u;i+=4)
            if (tile.rgba[i]!=255u) return false;
    }
    std::vector<std::uint8_t> packed(64u*64u*4u,255u);
    for (std::uint32_t slot=0;slot<tile_count;++slot) {
        const auto& tile=tiles[slot];
        const auto dx=(slot%4u)*16u,dy=(slot/4u)*16u;
        for (std::uint32_t y=0;y<16u;++y)
            for (std::uint32_t x=0;x<16u;++x) {
                const std::size_t from=(
                    std::size_t((std::uint64_t(y)*tile.height)/16u)*
                    tile.width+std::size_t((std::uint64_t(x)*tile.width)/16u))*4u;
                const std::size_t to=(std::size_t(dy+y)*64u+dx+x)*4u;
                for (unsigned channel=0;channel<4u;++channel)
                    packed[to+channel]=tile.rgba[from+channel];
            }
    }
    atlas.swap(packed);
    return true;
}

// P05/8: higher-detail optional 16-tile atlas. Each independent
// 64x64 tile occupies a quadrant of the 4x4 grid in a 256x256
// PRIVATE sampled VkImage (no window, streaming or GL interference).
// Images are already decoded 1..64px opaque RGBA8; the caller holds
// source locks. No persistent source pointer and no asset UUID.
inline bool ogl_rtx_pack_rgba_atlas_4x4_hq(
    const OGLRgbaTileView* tiles,std::uint32_t tile_count,
    std::vector<std::uint8_t>& atlas)
{
    atlas.clear();
    if (!tiles || !tile_count || tile_count>16u) return false;
    for (std::uint32_t slot=0;slot<tile_count;++slot) {
        const auto& tile=tiles[slot];
        if (!tile.rgba || !tile.width || !tile.height ||
            tile.width>64u || tile.height>64u ||
            tile.byte_count<std::size_t(tile.width)*tile.height*4u)
            return false;
        for (std::size_t pixel=3;
             pixel<std::size_t(tile.width)*tile.height*4u;pixel+=4u)
            if (tile.rgba[pixel]!=255u) return false;
    }
    std::vector<std::uint8_t> packed(256u*256u*4u,255u);
    for (std::uint32_t slot=0;slot<tile_count;++slot) {
        const auto& tile=tiles[slot];
        const auto dst_x=(slot%4u)*64u;
        const auto dst_y=(slot/4u)*64u;
        for (std::uint32_t y=0;y<64u;++y) {
            const auto source_y=std::uint32_t(
                (std::uint64_t(y)*tile.height)/64u);
            for (std::uint32_t x=0;x<64u;++x) {
                const auto source_x=std::uint32_t(
                    (std::uint64_t(x)*tile.width)/64u);
                const auto src=(
                    std::size_t(source_y)*tile.width+source_x)*4u;
                const auto dst=(
                    std::size_t(dst_y+y)*256u+dst_x+x)*4u;
                for (unsigned channel=0;channel<4u;++channel)
                    packed[dst+channel]=tile.rgba[src+channel];
            }
        }
    }
    atlas.swap(packed);
    return true;
}

// P04/2: world-renderer backend contract. This is a decision model, NOT a
// Vulkan world-renderer implementation or a user-visible engine selector.
// OpenGL owns the current GL context, world pipeline, UI, framebuffer and
// window/present lifecycle. The optional RTX module owns only its isolated
// diagnostic resources and must never be mistaken for a working Vulkan
// world/present backend.
//
// Future Vulkan implementations must explicitly certify BOTH a complete
// world/present pipeline and a usable device before a backend is selected.
// A GPU probe alone never qualifies. No ownership is transferred here.
// Backend changes require a controlled shutdown/startup (future P04 blocks).
enum class OGLWorldBackend : std::uint8_t {
    OpenGL,
    Vulkan
};

enum class OGLWorldFallback : std::uint8_t {
    NoFallback,
    WorldPipelineUnavailable,
    DeviceUnavailable
};

struct OGLWorldBackendDecision {
    OGLWorldBackend requested;
    OGLWorldBackend active;
    OGLWorldFallback fallback;
};

// Side-effect-free, platform-neutral decision; safe to test without a GPU.
// Vulkan diagnostics, ray-hit masks and GPU capability probing are NOT a
// world renderer. The real P05 world/present pipeline must explicitly attest
// readiness before the Vulkan branch can ever be used.
constexpr OGLWorldBackendDecision ogl_choose_world_backend(
    OGLWorldBackend requested, bool complete_world_pipeline,
    bool usable_device) noexcept
{
    if (requested == OGLWorldBackend::OpenGL)
        return {requested, OGLWorldBackend::OpenGL, OGLWorldFallback::NoFallback};
    if (!complete_world_pipeline)
        return {requested, OGLWorldBackend::OpenGL,
                OGLWorldFallback::WorldPipelineUnavailable};
    if (!usable_device)
        return {requested, OGLWorldBackend::OpenGL,
                OGLWorldFallback::DeviceUnavailable};
    return {requested, OGLWorldBackend::Vulkan, OGLWorldFallback::NoFallback};
}

// Current installed viewer: no complete Vulkan world renderer/swapchain and
// no Vulkan engine selection yet. GPU diagnostics must not change this state.
// All callers, including OpenSim pre-login, observe the same OpenGL baseline.
constexpr OGLWorldBackendDecision ogl_current_world_backend() noexcept
{
    return ogl_choose_world_backend(OGLWorldBackend::OpenGL, false, false);
}

static_assert(ogl_current_world_backend().active == OGLWorldBackend::OpenGL);
static_assert(ogl_choose_world_backend(
    OGLWorldBackend::Vulkan, false, true).active == OGLWorldBackend::OpenGL);

// Cross-platform capability snapshot: no vendor/CPU family is hard-coded.
// GL version and detected VRAM are diagnostics, NOT Vulkan readiness.
// Until P05 delivers the full world renderer and swapchain, the
// world_pipeline_complete bit MUST remain false even with RTX support.
struct OGLWorldCapabilities {
    bool gl_context_ready = false;
    bool vulkan_device_usable = false;
    bool vulkan_world_pipeline_complete = false;
    std::uint32_t detected_vram_mb = 0;
    std::uint32_t gl_version_major = 0;
    std::uint32_t gl_version_minor = 0;
};

constexpr OGLWorldBackendDecision ogl_choose_world_backend(
    OGLWorldBackend requested, const OGLWorldCapabilities& caps) noexcept
{
    return ogl_choose_world_backend(requested,
        caps.vulkan_world_pipeline_complete, caps.vulkan_device_usable);
}

// OpenGL never depends on the presence of the optional Vulkan probe.
static_assert(ogl_choose_world_backend(
    OGLWorldBackend::OpenGL, OGLWorldCapabilities{}).active == OGLWorldBackend::OpenGL);
static_assert(ogl_choose_world_backend(
    OGLWorldBackend::Vulkan,
    OGLWorldCapabilities{true, true, false, 8192, 4, 6}).fallback ==
    OGLWorldFallback::WorldPipelineUnavailable);

// Lifecycle ownership contract: OpenGL world/present/context state is owned
// by the existing Firestorm window/LLPipeline on the viewer main thread.
// The separate RTX module owns its diagnostic device and must shut down
// before the viewer destroys its GL context. No runtime engine hot-switch.
// The optional status call is non-rendering and never mutates user settings.
void ogl_report_world_backend_startup();
void ogl_report_world_backend_shutdown();
// Call only AFTER actual existing OpenGL swapBuffers returns; idempotent.
// A GPU probe, diagnostics sample or login screen is never a Vulkan frame.
void ogl_record_world_frame_presented();


/* Optional Linux Vulkan/RTX module lifecycle; safe no-op on other platforms. */
void ogl_init_rtx_module();
void ogl_shutdown_rtx_module();
// Capability is not rendering: reporting RTX hardware does not enable raytracing.
bool ogl_rtx_hardware_supported();
bool ogl_rtx_rendering_active();
// P05/4: true for developer-opted-in Vulkan geometry staging on any
// usable graphics VkDevice; ray tracing capability is not required.
bool ogl_vulkan_world_mesh_staging_enabled();
// Actual Firestorm frame/camera bridge for opt-in Vulkan diagnostics.
 // The active OpenGL world renderer remains unchanged.
// Triangles are copied synchronously; never retained by the host.
// Real OpenSim region transitions invalidate the cached Vulkan camera.
// The pipeline detects the UUID change; zero-triangle samples are NOT
// region transitions. This does not change the active OpenGL engine.
// True iff explicit Vulkan region barrier already retired old GPU geometry;
// false means the existing generic clear path remains responsible. The real
// OpenGL region transition proceeds regardless of Vulkan availability.
bool ogl_rtx_on_region_transition();
// Portable, side-effect-free developer sampling policy: first eligible
// deferred camera frame, then one every 120. OpenGL remains the backend.
constexpr bool ogl_vulkan_camera_tick_due(std::uint64_t frame) noexcept {
    return frame>0 && (frame-1u)%120u==0;
}
static_assert(!ogl_vulkan_camera_tick_due(0));
static_assert(ogl_vulkan_camera_tick_due(1));
static_assert(!ogl_vulkan_camera_tick_due(2));
static_assert(!ogl_vulkan_camera_tick_due(120));
static_assert(ogl_vulkan_camera_tick_due(121));
static_assert(!ogl_vulkan_camera_tick_due(240));
static_assert(ogl_vulkan_camera_tick_due(241));
// Only after the actual Firestorm region-UUID comparison: optionally
// render a changed, validated camera using already-accepted GPU mesh.
void ogl_rtx_after_region_camera_tick();
void ogl_rtx_submit_world_triangles(const float *triangle_xyz, unsigned count);
// Source-qualified world scanner entrypoint. Only LLPipeline invokes
// this after checking current OpenSim region ID; do not call it from
// prelogin or standalone synthetic GPU test paths.
void ogl_rtx_submit_deferred_world_triangles(
    const float *triangle_xyz,const float *face_tints_rgb,
    const float *face_uvs,const float *face_material_slots,
    unsigned atlas_tile_count,const std::uint8_t *decoded_rgba,
    unsigned image_width,unsigned image_height,unsigned count);
// Opt-in stable OpenSim UUID object registry (local mesh and 3x4 pose).
// Registry is CPU-side metadata; current Vulkan prototype still has one TLAS.
bool ogl_rtx_track_local_object(const std::uint8_t* uuid16,
    const float* local_triangle_xyz,unsigned count,const float* affine3x4);
bool ogl_rtx_update_object_pose(const std::uint8_t* uuid16,
    const float* affine3x4);
void ogl_rtx_forget_local_object(const std::uint8_t* uuid16);
void ogl_rtx_submit_camera_sun(unsigned width, unsigned height,
    const float* camera_position, const float* camera_forward,
    const float* camera_right, const float* camera_up,
    float vertical_fov_radians, float aspect_ratio,
    float near_clip_m, float far_clip_m,
    const float* sun_direction);

// Experimental, opt-in Vulkan dense mask compositing into the already
// bound Firestorm deferred sunlight target. Returns true only if real,
/// synchronized Vulkan-shadow pixels were submitted to OpenGL. Does not
// modify normals, UI, sky, unverified surfaces or public OpenGL defaults.
bool ogl_rtx_compose_dense_sun_shadow(unsigned current_depth_texture,
    unsigned width,unsigned height,const float* eye,const float* forward,
    const float* right,const float* up,float fov,float aspect,
    float near_clip,float far_clip,const float* sun,
    bool diagnostic_overlay=false);

// World renderer input handshake: verifies the Vulkan tile's exact
// source camera/geometry and returns aggregate validated pixels. It does NOT
// composite or upscale this incomplete 8x8 sample into the world.
bool ogl_rtx_world_shadow_source_ready(unsigned width,unsigned height,
    const float* eye,const float* forward,const float* right,
    const float* up,float fov,float aspect,float near_clip,float far_clip,
    const float* sun,unsigned* matched,unsigned* occluded,
    std::uint64_t* sequence);

// Developer-only 8x8 GPU ray-hit mask. Returns false without changing the
// caller's output if no matching, current scene sample exists. NOT pixels
// for the world's OpenGL framebuffer, and not enabled for public users.
bool ogl_rtx_copy_debug_shadow_mask(std::uint8_t* pixels,
    std::size_t capacity, std::uint64_t* geometry_revision);

// Same validated shadow mask, plus monotonic GPU publication sequence.
// UI diagnostics may use the sequence to expire or refresh old samples.
bool ogl_rtx_copy_debug_shadow_sample(std::uint8_t* pixels,
    std::size_t capacity, std::uint64_t* geometry_revision,
    std::uint64_t* sample_sequence,
    std::uint8_t* surface_mask, std::uint32_t* surfaces,
    std::uint32_t* background);

// Optional whole-view 8x8 NEAREST depth grid from the REAL deferred OpenGL
// depth texture. GPU-only downsample, single synchronous readback/minute with
// OGL_RTX_DEPTH_PROBE=1. Not used for visible shadows.
void ogl_rtx_probe_deferred_depth(unsigned depth_texture,
    unsigned render_width, unsigned render_height);

#endif

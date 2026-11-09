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
    uint32_t vulkan_gpu; /* vendor-neutral Vulkan physical-device found */
    uint32_t acceleration_structure;
    uint32_t ray_query;
    uint32_t ray_tracing_pipeline;
    uint32_t hardware_ready;
    uint32_t rendering_active; /* always zero until a real RTX renderer exists */
    char gpu_name[256];
    char message[256];
} ogl_rtx_status;

/* Renderer data bridge v3: exact Firestorm camera/light sample per frame.
 * Actual acceleration structures, mesh data, GPU Vulkan tracing and
 * OpenGL compositing are NOT implemented by this bridge. Do not set
 * rendering_active until real ray-traced pixels are displayed.
 */
#define OGL_RTX_FRAME_VERSION 3u
typedef struct ogl_rtx_frame_state {
    uint32_t struct_size;
    uint32_t frame_version;
    uint32_t width;
    uint32_t height;
    float camera_position[3];
    float camera_forward[3];
    float sun_direction[3];
    float camera_right[3];      /* -LLCamera::getLeftAxis() */
    float camera_up[3];         /* LLCamera::getUpAxis() */
    float vertical_fov_radians; /* LLCamera::getView(), radians */
    float aspect_ratio;         /* LLCamera::getAspect() */
    float near_clip_m;          /* LLCamera::getNear() */
    float far_clip_m;           /* LLCamera::getFar(), perspective convention */
} ogl_rtx_frame_state;

/* Copies the latest exact Firestorm camera axes, FOV and light into the module.
 * Returns 1 if accepted, 0 if missing hardware or invalid arguments.
 */
OGL_MODULE_EXPORT int ogl_rtx_submit_frame(const ogl_rtx_frame_state *frame);
OGL_MODULE_EXPORT uint64_t ogl_rtx_received_frames(void);

/* World geometry exchange: bounded batch of real triangulated OpenSim
 * volume faces in agent coordinates. No GPU ray tracing is implied.
 * Caller owns the memory; callee copies it synchronously.
 */
#define OGL_RTX_MAX_SCENE_TRIANGLES 8192u
typedef struct ogl_rtx_triangle {
    float position[9]; /* three xyz vertices, in agent-space meters */
} ogl_rtx_triangle;
OGL_MODULE_EXPORT int ogl_rtx_submit_geometry(const ogl_rtx_triangle *triangles,
                                               uint32_t count);
OGL_MODULE_EXPORT uint32_t ogl_rtx_received_triangles(void);
/* P05/4 opt-in Vulkan vertex/index buffer upload independent of ray tracing.
 * Requires OGL_VULKAN_WORLD_BOOTSTRAP=1 and OGL_VULKAN_WORLD_MESH_UPLOAD=1.
 * Copies the existing bounded OpenSim XYZ triangle ABI into VkBuffers and
 * host-coherent VkDeviceMemory, with sequential 32-bit indices.
 * On invalid input or partial failure, retains the last valid GPU scene.
 * count=0 explicitly clears scene/region resources.
 * This only stages GPU geometry, it does NOT render world triangles.
 */
OGL_MODULE_EXPORT int ogl_vulkan_world_upload_triangles(
    const ogl_rtx_triangle* triangles, uint32_t count);
OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_gpu_triangle_count(void);
/* P05/4 optional developer-only OpenSim face tint sidecar.
 * REAL per-face LLTextureEntry RGB diffuse tint from the deferred scanner,
 * not texture pixels or a complete PBR/GLTF material. One RGB triple per
 * triangle, exact order of the accepted ogl_rtx_triangle batch.
 * Prepass/terrain triangles may use neutral fallback RGB.
 * Explicit OGL_VULKAN_WORLD_FACE_TINT_PROBE=1, also requires the opt-in
 * Vulkan World+Camera+Lambert+Sun GPU paths. Never user-visible.
 * One VkBuffer bound as vertex binding 1, three copies of each face RGB,
 * independently versioned and strictly coupled to the current GPU mesh
 * generation. The legacy uncolored triangle ABI is unchanged.
 */
typedef struct ogl_rtx_face_tint {
    float rgb[3]; /* finite, normalized 0.0 .. 1.0, opaque only */
} ogl_rtx_face_tint;
OGL_MODULE_EXPORT int ogl_vulkan_world_upload_face_tints(
    const ogl_rtx_face_tint* tints, uint32_t triangle_count);
OGL_MODULE_EXPORT uint64_t ogl_vulkan_world_face_tint_mesh_generation(void);
OGL_MODULE_EXPORT uint64_t ogl_vulkan_world_face_tint_uploads(void);
OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_offscreen_scene_mean_red(void);
OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_offscreen_scene_mean_blue(void);

/* P05/4 opt-in REAL OpenSim volume-face UV coordinates: one float2 per
 * vertex, ordered exactly with the original 3*count XYZ world vertices.
 * These are UVs only, NOT loaded OpenSim texture assets or material maps.
 * The first private renderer visualizes UV wrapping using a procedural
 * 4x4 diagnostic checker (NOT a sampled VkImage).
 * Developer-only OGL_VULKAN_WORLD_UV_CHECKER_PROBE=1 and ALL existing
 * FACE_TINT/LAMBERT/SUN/WORLD/MESH/CAMERA/OFFSCREEN permissions required.
 * count=0 clears only the UV sidecar. Valid UV range: finite [-4096,4096].
 * Any mesh/region clear invalidates UV generation, no stale GPU sampling.
 */
typedef struct ogl_rtx_face_uv {
    float uv[6]; /* u0,v0,u1,v1,u2,v2 */
} ogl_rtx_face_uv;
OGL_MODULE_EXPORT int ogl_vulkan_world_upload_face_uvs(
    const ogl_rtx_face_uv* uv_triangles,uint32_t triangle_count);
OGL_MODULE_EXPORT uint64_t ogl_vulkan_world_face_uv_mesh_generation(void);
OGL_MODULE_EXPORT uint64_t ogl_vulkan_world_face_uv_uploads(void);
OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_offscreen_scene_mean_green(void);
/* Developer-only P05 RGBA pixel -> actual sampled Vulkan VkImage. This
 * accepts DECODED, tightly packed opaque RGBA8 pixels supplied by a
 * trusted caller, not compressed OpenSim JPEG2000/GLTF asset data.
 * Current proof supports ONE 1..64px square/rectangle image per CURRENT
 * world mesh revision; no material-ID or face-to-texture routing yet.
 * No texture source is read from the logged-in viewer automatically.
 * Requires OGL_VULKAN_WORLD_RGBA_IMAGE_PROBE=1 plus all existing
 * UV_CHECKER/FACE_TINT/LAMBERT/SUN/WORLD/CAMERA/MESH GPU permissions.
 * Width/height 1..64, full alpha=255, size exactly width*height*4.
 * Image uploads are synchronous and must not race a pending frame.
 * The image GPU pipeline is PRIVATE (no SDL window/texture export).
 */
OGL_MODULE_EXPORT int ogl_vulkan_world_upload_rgba_image(
    const uint8_t* rgba8, uint32_t width, uint32_t height);
OGL_MODULE_EXPORT uint64_t ogl_vulkan_world_rgba_mesh_generation(void);
OGL_MODULE_EXPORT uint64_t ogl_vulkan_world_rgba_uploads(void);
/* P05/6 four-material private Vulkan atlas: ONE 64x64 RGBA8 VkImage,
 * four strictly independent 32x32 tiles (slots 0..3). This is still
 * NOT a general asset streamer, image-format decoder or world present.
 * Requires OGL_VULKAN_WORLD_MATERIAL_ATLAS_PROBE=1 in addition to
 * all current RGBA/UV/TINT/LAMBERT/SUN/MESH/CAMERA opt-in gates.
 * Each triangle has one integer-valued slot, repeated 3x (one per
 * vertex). The slots are checked and GPU-coupled to the current mesh.
 * atlas slot_count may be 1..4 normally; with explicit
 * OGL_VULKAN_WORLD_MATERIAL_ATLAS_16_PROBE=1 it may be 1..16, using
 * sixteen 16x16 texel tiles instead of four 32x32 tiles. The caller
 * must supply the matching exact 4x4 atlas layout, not reinterpret
 * an old 2x2 atlas. The actual Firestorm source additionally requires
 * OGL_VULKAN_LIVE_MATERIAL_ATLAS_16_SOURCE=1 and all prior source gates.
 * Changes invalidate old atlas material source. count=0 clears slots.
 */
typedef struct ogl_rtx_face_material_slot {
    float slot[3];
} ogl_rtx_face_material_slot;
OGL_MODULE_EXPORT int ogl_vulkan_world_upload_material_slots(
    const ogl_rtx_face_material_slot* slots,uint32_t triangle_count,
    uint32_t slot_count);
OGL_MODULE_EXPORT uint64_t ogl_vulkan_world_material_slot_mesh_generation(void);
OGL_MODULE_EXPORT uint64_t ogl_vulkan_world_material_slot_uploads(void);






/* P05 experimental independent Vulkan world-to-clip matrix from the same
 * v3 Firestorm frame ABI. Opt-in OGL_VULKAN_WORLD_CAMERA_PROBE=1 and a real
 * Vulkan VkDevice are required. Validates exact basis handedness, orthonormal
 * axes, viewport aspect, near/far and FOV; rejects stale/invalid input.
 * Fails closed by invalidating any previous Vulkan camera on bad input.
 * The graphics shader consumes the generated 4x4 matrix via 64-byte
 * VkPushConstantRange only if OGL_VULKAN_EXCLUSIVE_WORLD_PROBE=1,
 * OGL_VULKAN_EXCLUSIVE_RASTER_PROBE=1 and a GPU mesh are all present.
 * Camera projection alone is NOT a working OpenSim in-world renderer.
 */
OGL_MODULE_EXPORT int ogl_vulkan_world_set_projection_camera(
    const ogl_rtx_frame_state* frame);
OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_projection_camera_ready(void);
/* Read-only WSI viewport handshake. Successful mesh+camera acceptance
 * alone is insufficient after an aspect-changing swapchain resize.
 * Returns 1 only for a ready camera whose viewport aspect matches the
 * currently attached VkSwapchainKHR. Never indicates a world present.
 */
OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_camera_matches_swapchain(void);

/* P05 actual Firestorm input bridge: diagnostics only. These read-only
 * counters distinguish successfully accepted CPU camera matrices and actual
 * Vulkan-GPU mesh uploads from world frame presentation. They are NOT FPS.
 * live_inputs_ready==1 means a validated camera arrived AFTER the last
 * accepted mesh upload, and the mesh buffers exist. A region clear, any
 * successful mesh replacement, invalid camera or module stop invalidates
 * that handshake. No actual Vulkan world frame/present is implied.
 */
OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_live_inputs_ready(void);
OGL_MODULE_EXPORT uint64_t ogl_vulkan_world_camera_updates(void);
OGL_MODULE_EXPORT uint64_t ogl_vulkan_world_mesh_uploads(void);
/* Developer-only persistent world mesh rotation. Disabled unless the
 * caller explicitly sets OGL_VULKAN_PERSISTENT_MESH_ROTATION_PROBE=1,
 * in addition to all normal world/mesh/window developer opt-ins.
 * The OLD VkVertex/VkIndexBuffers are released only after one bounded
 * (<=2 seconds) wait-all of the outstanding submit fences that reference
 * their generation. This does NOT wait for entire VkQueue or GPU present.
 * Invalid GPU wait blocks the persistent probe, preserving old buffers.
 * A successful mesh/empty-region clear advances the scene generation,
 * invalidates the old camera and requires a fresh camera before drawing.
 */
/* Explicit real OpenSim region-transition safety barrier. Requires BOTH
 * OGL_VULKAN_WORLD_MESH_UPLOAD=1 and OGL_VULKAN_LIVE_REGION_BARRIER=1.
 * Invalidates the previous region camera BEFORE bounded waits and only
 * frees old VkBuffers after the in-flight mesh-generation submit fences.
 * No queue idle, no SDL/OpenGL window action. On GPU timeout it blocks
 * further persistent Vulkan presents and leaves the old GPU memory alive.
 * 1 = barrier accepted and old mesh retired; 0 = no permission/device,
 * or failed wait. A failure NEVER authorizes a new Vulkan world frame.
 */
OGL_MODULE_EXPORT int ogl_vulkan_world_invalidate_region(void);
OGL_MODULE_EXPORT uint64_t ogl_vulkan_world_region_barrier_accepts(void);
OGL_MODULE_EXPORT uint64_t ogl_vulkan_world_region_barrier_failures(void);
OGL_MODULE_EXPORT uint64_t ogl_vulkan_world_mesh_generation(void);
/* Optional bit-exact unchanged-geometry coalescing:
 * OGL_VULKAN_WORLD_MESH_DEDUP=1. Returns success for byte-identical
 * snapshots without reallocating VkBuffers, invalidating a valid camera,
 * advancing scene generation or waiting on in-flight graphics fences.
 * Explicit region clear still advances the generation. This is not a
 * proof that a world frame was rendered under Vulkan.
 */
OGL_MODULE_EXPORT uint64_t ogl_vulkan_world_mesh_dedup_skips(void);
/* The camera is only usable with the exact accepted mesh generation.
 * Scene replacement/region clear and failed camera submissions invalidate it.
 * This counter alone is NOT proof of a visible GPU world frame.
 */
OGL_MODULE_EXPORT uint64_t ogl_vulkan_world_camera_mesh_generation(void);
OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_mesh_retire_waits(void);
OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_mesh_retire_failures(void);

/* Prototype per-object registry for incremental RT geometry/transform
 * tracking. IDs are the full 16 UUID bytes; object vertices are LOCAL.
 * Affine transform is 3x4 row-major (translation in cols 3,7,11).
 * This registry is not yet uploaded as a multi-instance Vulkan TLAS.
 * All APIs are called on the viewer render thread.
 */
OGL_MODULE_EXPORT int ogl_rtx_track_object(const uint8_t* uuid16,
    const ogl_rtx_triangle* local_triangles, uint32_t count,
    const float* affine3x4);
OGL_MODULE_EXPORT int ogl_rtx_track_transform(const uint8_t* uuid16,
    const float* affine3x4);
OGL_MODULE_EXPORT int ogl_rtx_forget_object(const uint8_t* uuid16);
OGL_MODULE_EXPORT uint32_t ogl_rtx_tracked_objects(void);
OGL_MODULE_EXPORT uint64_t ogl_rtx_object_geometry_generation(void);
OGL_MODULE_EXPORT uint64_t ogl_rtx_object_transform_generation(void);
/* Diagnostic only, counted only with OGL_RTX_SCENE_PROBE=1.
 * NEVER means shadows are visible in the viewer.
 */
OGL_MODULE_EXPORT uint64_t ogl_rtx_gpu_probe_attempts(void);
OGL_MODULE_EXPORT uint64_t ogl_rtx_gpu_probe_successes(void);
/* Monotonic revision. Increments only for a changed, accepted snapshot. */
OGL_MODULE_EXPORT uint64_t ogl_rtx_geometry_revision(void);
/* Copy last snapshot into caller-owned storage (all-or-nothing).
 * Return number copied or zero when empty / insufficient capacity.
 * C ABI operations currently run on the same viewer rendering thread.
 */
OGL_MODULE_EXPORT uint32_t ogl_rtx_copy_geometry(ogl_rtx_triangle *dest,
                                                 uint32_t capacity);

/* Developer diagnostics only. A low-resolution ray-hit mask is NOT a screen-
 * space shadow map and MUST NOT be composited into the world image.
 * Read only from the same main/render thread as the frame bridge.
 * A region or mesh update invalidates the previous debug mask.
 */
#define OGL_RTX_SHADOW_DEBUG_VERSION 3u
#define OGL_RTX_SHADOW_DEBUG_PIXELS 64u
typedef struct ogl_rtx_shadow_debug {
    uint32_t struct_size;
    uint32_t version;
    uint64_t geometry_revision;
    uint64_t sample_sequence;  /* unique successful async GPU job sequence */
    uint32_t width;
    uint32_t height;
    uint32_t occluded;
    uint32_t surface_pixels;
    uint32_t background_pixels;
    uint8_t pixels[OGL_RTX_SHADOW_DEBUG_PIXELS]; /* 1 = shadow */
    uint8_t surface_mask[OGL_RTX_SHADOW_DEBUG_PIXELS]; /* 1 = non-sky */
} ogl_rtx_shadow_debug;
OGL_MODULE_EXPORT int ogl_rtx_copy_shadow_debug(ogl_rtx_shadow_debug *output);

/* Initial GPU-to-world shadow *input* contract. This is a validated
 * immutable Vulkan ray-query tile, NOT a composited full-resolution image.
 * A renderer must compare capture_pose and capture_depth against CURRENT
 * camera and framebuffer before using any bit; moving avatars/transparency
 * and frames not present in the AS must still be handled. Fail closed.
 * Separate ABI from historical 8x8 HUD diagnostics.
 */
#define OGL_RTX_WORLD_SHADOW_VERSION 1u
typedef struct ogl_rtx_world_shadow_sample {
    uint32_t struct_size;
    uint32_t version;
    uint64_t geometry_revision;
    uint64_t sample_sequence;
    ogl_rtx_frame_state capture_frame;
    uint32_t matched_surface_pixels;
    uint32_t occluded_pixels;
    uint8_t valid_mask[64];       /* Vulkan primary matches OpenGL depth */
    uint8_t occlusion_mask[64];   /* shadow ray hit, only if valid */
    float capture_depth[64];     /* GL normalized depth at capture time */
} ogl_rtx_world_shadow_sample;
/* No result on absent, obsolete, incomplete or invalid GPU frames. */
OGL_MODULE_EXPORT int ogl_rtx_copy_world_shadow_sample(
    ogl_rtx_world_shadow_sample* output);

/* Explicit developer-only 64x36 Vulkan depth-shadow mode.
 * Separate from the legacy 8x8 diagnostic. This remains a bounded source
 * tile; it is NOT an automatically upscaled world-space shading texture.
 * The capture_frame is immutable and must be checked before application.
 */
#define OGL_RTX_DENSE_VERSION 2u
#define OGL_RTX_DENSE_WIDTH 128u
#define OGL_RTX_DENSE_HEIGHT 72u
#define OGL_RTX_DENSE_PIXELS (OGL_RTX_DENSE_WIDTH*OGL_RTX_DENSE_HEIGHT)
typedef struct ogl_rtx_dense_depth {
    uint32_t struct_size;
    uint32_t version;
    uint32_t viewport_width;
    uint32_t viewport_height;
    uint32_t width;
    uint32_t height;
    float normalized_depth[OGL_RTX_DENSE_PIXELS];
} ogl_rtx_dense_depth;
typedef struct ogl_rtx_dense_shadow {
    uint32_t struct_size;
    uint32_t version;
    uint64_t geometry_revision;
    uint64_t sample_sequence;
    ogl_rtx_frame_state capture_frame;
    uint32_t width;
    uint32_t height;
    uint32_t surface_pixels;
    uint32_t matched_pixels;
    uint32_t shadow_pixels;
    uint8_t valid_mask[OGL_RTX_DENSE_PIXELS];
    uint8_t shadow_mask[OGL_RTX_DENSE_PIXELS];
    float capture_depth[OGL_RTX_DENSE_PIXELS];
} ogl_rtx_dense_shadow;
OGL_MODULE_EXPORT int ogl_rtx_submit_dense_depth(
    const ogl_rtx_dense_depth* input);
OGL_MODULE_EXPORT int ogl_rtx_copy_dense_shadow(
    ogl_rtx_dense_shadow* output);

/* OpenGL deferred depth, WHOLE-VIEWPORT 8x8 NEAREST SAMPLE GRID.
 * Pixel (x,y) corresponds to camera ray UV ((x+.5)/8,(y+.5)/8).
 * OpenGL origin is bottom-left, matching the diagnostic Vulkan shader.
 * A new camera frame invalidates previous samples. This is NOT a real
 * world shadow map, and does not enable OpenGL/Vulkan compositing.
 */
#define OGL_RTX_DEPTH_DEBUG_VERSION 2u
#define OGL_RTX_DEPTH_DEBUG_PIXELS 64u
typedef struct ogl_rtx_depth_debug {
    uint32_t struct_size;
    uint32_t version;
    uint32_t viewport_width;  /* original depth target resolution */
    uint32_t viewport_height;
    uint32_t grid_width;      /* exactly 8 */
    uint32_t grid_height;     /* exactly 8 */
    float normalized_depth[OGL_RTX_DEPTH_DEBUG_PIXELS]; /* row-major */
} ogl_rtx_depth_debug;
OGL_MODULE_EXPORT int ogl_rtx_submit_depth_debug(const ogl_rtx_depth_debug *sample);
OGL_MODULE_EXPORT int ogl_rtx_copy_depth_debug(ogl_rtx_depth_debug *output);

/* P05 experimental world-device bootstrap, independent of RTX support.
 * Developer opt-in only via OGL_VULKAN_WORLD_BOOTSTRAP=1. Returns 1 when
 * a persistent graphics VkDevice and VkQueue were actually created.
 * This is NOT surface/present readiness, not a world renderer, and MUST NOT
 * affect the OpenGL fallback or the rendering_active RTX status field.
 * Owned by the module and released on ogl_module_stop.
 */
OGL_MODULE_EXPORT int ogl_vulkan_world_device_bootstrapped(void);
/* P05/3 developer-only VkQueueSubmit/fence test on the persistent graphics
 * device. Requires OGL_VULKAN_FRAME_SYNC_PROBE=1; returns 2 only if two
 * real graphics queue submissions and GPU fence waits succeeded, else 0.
 * Performs no image acquire, world rendering, vkQueuePresentKHR or GL calls.
 * NOT evidence that Vulkan world frames are working.
 */
OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_submit_sync_probe(void);
/* P05 opt-in GPU queue prerequisite: dual commands MUST be queued before
 * waiting for BOTH completion fences (single bounded wait-all).
 * max_outstanding counts submitted-but-not-yet-waited commands, not
 * truly parallel GPU execution or swapchain presents. The values reset on
 * module stop; the main Vulkan present path still uses a blocking queue-idle.
 */
OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_sync_max_outstanding(void);
OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_sync_last_submitted(void);
OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_sync_last_completed(void);
/* P05/3: opt-in Vulkan offscreen graphics render pass, 32x32 RGBA clear.
 * Returns 1 ONLY after GPU fence completion, buffer readback, and
 * pixel verification; 0 when disabled, unsupported or failed.
 * Requires OGL_VULKAN_OFFSCREEN_FRAME_PROBE=1 and bootstrapped VkDevice.
 * NOT a swapchain frame, scene draw, Vulkan world or active renderer.
 */
OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_render_offscreen_probe(void);
/* P05 real world GPU bridge WITHOUT Vulkan window ownership.
 * Independently gated by OGL_VULKAN_OFFSCREEN_FRAME_PROBE=1,
 * OGL_VULKAN_WORLD_OFFSCREEN_RASTER=1,
 * OGL_VULKAN_WORLD_MESH_UPLOAD=1 and
 * OGL_VULKAN_WORLD_CAMERA_PROBE=1. A successfully paired camera and
 * vertex/index GPU mesh MUST already exist. Renders VkDrawIndexed with
 * the validated world_to_clip matrix, depth attachment and shader into
 * PRIVATE bounded-resolution VkImage (default 32x32; explicit
 * OGL_VULKAN_WORLD_OFFSCREEN_RESOLUTION accepts only 32x32, 128x96,
 * 256x144 or 512x288). No SDL window or visible framebuffer change.
 * Reads back RGBA after a 2-second GPU fence.
 * 1 = Vulkan indexed draw/readback completed, 0 = not ready/failure.
 * The pixel counter is the number of non-clear pixels,
 * 0..(offscreen_scene_width*offscreen_scene_height). The two width/height
 * accessors report the dimensions of the LAST COMPLETED GPU frame;
 * zero does not mean failure (geometry may be outside the camera).
 * NO display, NO user/avatar/region image exported, NO VkQueuePresentKHR.
 * This is NOT a finished Vulkan world renderer and must not be called
 * every frame in the production/GL world-render loop.
 */
OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_render_offscreen_scene(void);
OGL_MODULE_EXPORT uint64_t ogl_vulkan_world_offscreen_scene_frames(void);
OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_offscreen_scene_pixels(void);
OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_offscreen_scene_width(void);
OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_offscreen_scene_height(void);
/* Optional developer-only face-normal Lambert shading from GPU pixel-space
 * derivatives of real XYZ interpolated positions. Enabled only when
 * OGL_VULKAN_WORLD_LAMBERT_PROBE=1 on top of existing world/mesh/camera/
 * offscreen permissions. Invalid flag strings fail closed.
 * Default remains the original red/green depth-debug shader.
 * Optional second-level OGL_VULKAN_WORLD_SUN_LIGHT_PROBE=1 requires
 * LAMBERT_PROBE=1 and a valid nonzero Firestorm frame sun_direction.
 * The 3D direction is normalized, passed as a fifth vec4 via 80-byte
 * Vulkan push constants (mat4 camera + vec4 sun), and updates lighting
 * without recreating the GPU pipeline or re-uploading any world mesh.
 * Invalid/zero/non-finite sun fails closed. Both flags OFF by default.
 * This is still NOT live world material shading or visible Vulkan output.
 * Output NEVER enters the user window or user image cache.
 * Counters are bounded aggregates from the last GPU completed PRIVATE
 * target; mean_luminance is average (R+G+B) of non-clear pixels, 0..765.
 */
OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_offscreen_scene_lit_pixels(void);
OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_offscreen_scene_mean_luminance(void);
/* Device-owned private bounded-resolution World/Depth cache (developer only).
 * Changing the allowlisted resolution safely releases old attachments,
 * shader pipeline and readback after the previous GPU fence and rebuilds
 * once at the new size; never owns a VkSurfaceKHR or VkSwapchainKHR.
 * VkImages/VkDeviceMemory, shaders, pipeline, framebuffer, readback,
 * command pool and GPU fence are allocated once and reused across accepted
 * camera/mesh generations, with vkResetCommandBuffer/vkResetFences only
 * AFTER each bounded GPU completion wait. This is NOT a world FPS metric.
 * The original offscreen-CLEAR QA remains allocation-per-probe.
 * Release is explicit or automatic at device stop BEFORE VkDevice teardown.
 * All counters reset at device stop, never on region change.
 */
OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_offscreen_cache_builds(void);
OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_offscreen_cache_reuses(void);
OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_offscreen_cache_releases(void);
OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_offscreen_cache_release(void);
/* P05 developer-only Linux/X11 WSI probe on the ACTUAL viewer window.
 * Requires both OGL_VULKAN_WORLD_BOOTSTRAP=1 and
 * OGL_VULKAN_WINDOW_SURFACE=1. Creates real VkSurfaceKHR and VkSwapchainKHR
 * if supported by the selected graphics/present queue. No image acquisition,
 * rendering, queue submission or presentation occurs: OpenGL remains live.
 * Do not call after SDL destroys the supplied X11 window/display.
 * Returns 1 only when a swapchain with >=2 images exists.
 * Exported as optional C ABI symbols; unsupported OSes return 0.
 */
OGL_MODULE_EXPORT int ogl_vulkan_world_attach_x11_window(
    void* x11_display, uint64_t x11_window, uint32_t width, uint32_t height);
OGL_MODULE_EXPORT int ogl_vulkan_world_swapchain_probe_ready(void);
/* Developer-only native X11 WSI QA. Caller MUST own the window exclusively
 * (no OpenGL presentation), pass gl_released=1, and set
 * OGL_VULKAN_EXCLUSIVE_PRESENT_PROBE=1. Called either from the isolated
 * native X11 test OR once from the actual SDL window BEFORE GL context
 * creation, under an additional explicit developer opt-in. NOT callable
 * during GL world rendering, and NOT an interactive Vulkan world renderer.
 * Extra opt-in OGL_VULKAN_EXCLUSIVE_RASTER_PROBE=1 enables a genuine indexed
 * VkGraphicsPipeline draw ONLY when validated vertex and index VkBuffers
 * were uploaded. The test vertex shader treats XYZ as CLIP-SPACE; actual
 * OpenSim agent coordinates still require a camera/world matrix projection.
 * The default behavior is an RGBA clear without shaders or vertex draws.
 * An additional OGL_VULKAN_EXCLUSIVE_DEPTH_PROBE=1 enables an isolated
 * D32_SFLOAT/D16_UNORM depth attachment, depth compare LESS, and a depth-
 * dependent fragment color for hardware occlusion verification. This is
 * explicitly developer-only and NOT a live Vulkan world/frame renderer.
 * The real SDL pre-GL caller also requires its own distinct opt-in
 * OGL_VULKAN_SDL_PREGL_DEPTH_PROBE=1; no GL/Vulkan overlap is allowed.
 * Isolated GPU regression ONLY: OGL_VULKAN_EXCLUSIVE_DEPTH_COLOR_PROBE=1
 * selects the same Z-colored fragment shader but deliberately omits depth
 * attachments. The far last-drawn geometry MUST then overwrite the near
 * geometry, proving that the depth acceptance test is not a false positive.
 * Never enable either depth control for production OpenSim drawing.
 */
OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_present_exclusive_test_frame(
    uint32_t gl_released);
/* Isolated native QA: sequence count for successfully completed presents,
 * reset on swapchain detach/recreate. Frame-loop opt-in (at most 16) is
 * OGL_VULKAN_EXCLUSIVE_FRAME_LOOP=1; regular single-shot mode stays default.
 * This does not count or present any frames of the real OpenSim Viewer.
 */
/* Developer-only persistent WSI clear-frame gate.
 * Explicit opt-in: OGL_VULKAN_EXCLUSIVE_PERSISTENT_CLEAR_PROBE=1.
 * The calling viewer must independently authorize the real SDL pre-GL
 * probe; never invoke while GL owns the window. Requires an attached
 * X11 Vulkan swapchain and gl_released==1. Framebuffer/image views,
 * renderpass, command pool, 2 fences/command buffers and swapchain-image-
 * indexed present semaphores persist across <=16 frames PER ATTACH.
 * No per-frame vkQueueWaitIdle; ONE queue idle on detach to retire presents.
 * With additional OGL_VULKAN_EXCLUSIVE_PERSISTENT_WORLD_PROBE=1
 * and independent SDL approval, the existing indexed World-XYZ shader,
 * 64-byte camera matrix, VkDepthStencilState and image-indexed GPU depth
 * attachments render a SYNTHETIC OpenSim-style scene across multiple
 * frames. World inputs MUST be provided and camera aspect must match the
 * current swapchain. Mesh changes/region-clear while any persistent GPU
 * submission may reference the VkBuffers are rejected until detach.
 * This is developer-only, pre-GL and NOT live authenticated OpenSim world
 * rendering, a usable Vulkan engine selection, or finished UI/terrain.
 */
OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_persistent_clear_frame(
    uint32_t gl_released);
OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_persistent_frame_count(void);
OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_persistent_build_count(void);
OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_persistent_release_count(void);
OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_persistent_teardown_idles(void);
OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_exclusive_present_count(void);
OGL_MODULE_EXPORT void ogl_vulkan_world_detach_window(void);

/* Read-only status; valid until module is unloaded. Never store credentials. */
OGL_MODULE_EXPORT const ogl_rtx_status *ogl_rtx_get_status(void);

#ifdef __cplusplus
}
#endif

#endif

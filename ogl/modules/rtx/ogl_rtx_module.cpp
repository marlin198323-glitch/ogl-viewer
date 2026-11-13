#include "ogl/ogl_rtx_api.h"
#include "ogl/ogl_world_shadow_policy.h"
#include "ogl/ogl_scene_validation.h"
#include "ogl/ogl_camera_validation.h"
#include "ogl/ogl_depth_validation.h"
#include "ogl/ogl_depth_diagnostics.h"
#include "ogl/ogl_scene_depth_compare.h"
#include "ogl/ogl_dynamic_scene.h"
#include "ogl/ogl_gpu_probe_api.h"
#include "ogl/ogl_async_gpu_queue.h"
// Linux/X11 WSI is optional at compile time. Windows/macOS and builds
// without Xlib headers continue to support the existing Vulkan GPU probe.
#if defined(__linux__) && defined(OGL_XLIB_LINKED) && __has_include(<X11/Xlib.h>)
#  include <X11/Xlib.h>
#  define VK_USE_PLATFORM_XLIB_KHR 1
#  define OGL_HAS_XLIB_WSI 1
#endif
#include <vulkan/vulkan.h>
#if defined(_WIN32)
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#else
#  include <dlfcn.h>
#endif
#include <cstdio>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <limits>
#include <algorithm>
#include <array>
#include <memory>
#include <chrono>
#include <cstdlib>
#include <set>
#include <string>
#include <vector>

namespace {
#if defined(_WIN32)
using VulkanLibrary = HMODULE;
VulkanLibrary open_vulkan_loader() {
    // Only load from the Windows system library directory.
    return LoadLibraryExW(L"vulkan-1.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
}
void* lookup_vulkan_symbol(VulkanLibrary library, const char* symbol) {
    return reinterpret_cast<void*>(GetProcAddress(library, symbol));
}
void close_vulkan_loader(VulkanLibrary library) { FreeLibrary(library); }
#else
using VulkanLibrary = void*;
VulkanLibrary open_vulkan_loader() { return dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL); }
void* lookup_vulkan_symbol(VulkanLibrary library, const char* symbol) {
    return dlsym(library, symbol);
}
void close_vulkan_loader(VulkanLibrary library) { dlclose(library); }
#endif
ogl_rtx_status status{sizeof(ogl_rtx_status), OGL_RTX_STATUS_VERSION};
const ogl_module_info info{
    sizeof(ogl_module_info), OGL_MODULE_ABI_VERSION,
    "de.nexverse.ogl.rtx", "OGL Vulkan RT Capability", "0.2.0"
};
// Explicit diagnostics only. Never used as an in-world renderer.
#if defined(_WIN32)
using GpuProbeLibrary = HMODULE;
GpuProbeLibrary load_gpu_probe() {
    wchar_t path[MAX_PATH]{};
    const DWORD n = GetModuleFileNameW(nullptr, path, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) return nullptr;
    std::wstring directory(path, n);
    const auto slash = directory.find_last_of(L"\\/");
    if (slash == std::wstring::npos) return nullptr;
    directory.resize(slash + 1);
    return LoadLibraryExW((directory + L"ogl_gpu_probe.dll").c_str(), nullptr,
        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
}
void* gpu_symbol(GpuProbeLibrary lib, const char* name) {
    return reinterpret_cast<void*>(GetProcAddress(lib, name));
}
void gpu_close(GpuProbeLibrary lib) { FreeLibrary(lib); }
#else
using GpuProbeLibrary = void*;
GpuProbeLibrary load_gpu_probe() {
    return dlopen("./lib/ogl_gpu_probe.so", RTLD_NOW | RTLD_LOCAL);
}
void* gpu_symbol(GpuProbeLibrary lib, const char* name) {
    return dlsym(lib, name);
}
void gpu_close(GpuProbeLibrary lib) { dlclose(lib); }
#endif
using ProbeRun = int (*)(const ogl_rtx_triangle*, uint32_t,
                        const float*, const char*, ogl_gpu_probe_result*);
using CameraProbeRun = int (*)(const ogl_rtx_triangle*, uint32_t,
                        const ogl_rtx_frame_state*, const char*,
                        ogl_gpu_probe_result*);
using DepthProbeRun = int (*)(const ogl_rtx_triangle*, uint32_t,
                        const ogl_rtx_frame_state*, const float*,
                        const char*, ogl_gpu_probe_result*);
using DenseProbeRun = int (*)(const ogl_rtx_triangle*,uint32_t,
                        const ogl_rtx_frame_state*,const float*,
                        const char*,ogl_gpu_dense_result*);
GpuProbeLibrary probe_library = nullptr;
ProbeRun probe_run = nullptr;
CameraProbeRun probe_camera_run = nullptr;
DepthProbeRun probe_depth_run = nullptr;
DenseProbeRun probe_dense_run = nullptr;
bool probe_depth_enabled = false;
bool probe_dense_enabled = false;
bool dense_depth_valid = false;
ogl_rtx_dense_depth last_dense_depth{};
ogl_rtx_dense_shadow last_dense_shadow{};
bool dense_shadow_valid = false;
uint64_t dense_busy_depth_skips=0;
bool probe_opt_in = false;
bool probe_camera_enabled = false;
std::unique_ptr<ogl::AsyncGpuQueue> probe_worker;
uint64_t probe_last_revision = 0;
uint64_t probe_job_sequence = 0; // distinct from mesh revision
bool probe_view_dirty = false;
std::chrono::steady_clock::time_point probe_last_time{};
std::chrono::milliseconds probe_interval{60000}; // production developer default
const char* const trusted_shader = "./app_settings/ogl/rtx/ray_query_shadow.comp.spv";
const char* const trusted_camera_shader = "./app_settings/ogl/rtx/camera_sun_shadow.comp.spv";
const char* const trusted_depth_shader = "./app_settings/ogl/rtx/depth_guided_sun.comp.spv";
const char* const trusted_dense_shader =
    "./app_settings/ogl/rtx/dense_depth_guided_sun.comp.spv";

// Swapchain-owned, persistent *developer-only* clear-frame resources.
 // Semaphores signaling presentation are associated with acquired image
// indices, not CPU frame slots: they are reused only AFTER reacquiring the
// corresponding swapchain image. This avoids the classic early reuse bug.
struct OglPersistentClearFrames {
    VkRenderPass pass = VK_NULL_HANDLE;
    VkCommandPool pool = VK_NULL_HANDLE;
    std::vector<VkImageView> views;
    std::vector<VkFramebuffer> framebuffers;
    // World+depth mode: depth resources are PER SWAPCHAIN IMAGE, never
    // shared across simultaneously outstanding GPU frame submissions.
    std::vector<VkImage> depth_images;
    std::vector<VkDeviceMemory> depth_memories;
    std::vector<VkImageView> depth_views;
    VkFormat depth_format = VK_FORMAT_UNDEFINED;
    VkShaderModule vertex_shader = VK_NULL_HANDLE;
    VkShaderModule fragment_shader = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    bool world_depth = false;
    std::array<uint64_t,2> submitted_mesh_generations{};
    std::vector<VkSemaphore> presented_by_image;
    std::vector<VkFence> image_in_flight;
    std::array<VkCommandBuffer,2> commands{};
    std::array<VkSemaphore,2> image_acquired{};
    std::array<VkFence,2> submit_fences{};
    std::array<bool,2> submit_pending{};
    uint32_t frame_count = 0;
    uint32_t max_frames = 16; // frozen for this swapchain attachment
    uint32_t image_count = 0;
    bool submitted_any = false;
    bool blocked = false;
    bool initialized = false;
};

// Device-owned, private bounded-resolution world GPU targets. NO window,
// VkSurfaceKHR or VkSwapchainKHR. All resources are destroyed before VkDevice.
struct OglOffscreenWorldCache {
    VkImage color=VK_NULL_HANDLE, depth=VK_NULL_HANDLE;
    VkDeviceMemory color_mem=VK_NULL_HANDLE, depth_mem=VK_NULL_HANDLE,
                   readback_mem=VK_NULL_HANDLE;
    VkImageView view=VK_NULL_HANDLE, depth_view=VK_NULL_HANDLE;
    VkFormat depth_format=VK_FORMAT_UNDEFINED;
    VkRenderPass render_pass=VK_NULL_HANDLE;
    VkFramebuffer framebuffer=VK_NULL_HANDLE;
    VkBuffer readback=VK_NULL_HANDLE;
    VkCommandPool command_pool=VK_NULL_HANDLE;
    VkCommandBuffer command=VK_NULL_HANDLE;
    VkFence fence=VK_NULL_HANDLE;
    VkShaderModule vertex_shader=VK_NULL_HANDLE;
    VkShaderModule fragment_shader=VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout=VK_NULL_HANDLE;
    VkPipeline pipeline=VK_NULL_HANDLE;
    VkImage texture_image=VK_NULL_HANDLE;
    VkDeviceMemory texture_memory=VK_NULL_HANDLE;
    VkImageView texture_view=VK_NULL_HANDLE;
    VkSampler texture_sampler=VK_NULL_HANDLE;
    VkDescriptorSetLayout texture_set_layout=VK_NULL_HANDLE;
    VkDescriptorPool texture_descriptor_pool=VK_NULL_HANDLE;
    VkDescriptorSet texture_descriptor=VK_NULL_HANDLE;
    VkRenderPass pass=VK_NULL_HANDLE;
    VkExtent2D extent{}; // private target, independent of SDL window
    uint32_t shading_mode=0; // 0=depth,1=fixed,2=sun,3=tint,4=UV,5=RGBA,6=4-tile,7=16-tile,8=HQ16 atlas
    bool initialized=false;
    bool submitted=false; // fence owns possibly in-flight work
};

// P05/1 developer-only Vulkan graphics device bootstrap. Its lifetime is
// independent from the ray-query probe and is never a replacement for GL.
// A future world engine still needs permanent SDL-surface ownership,
// frame synchronization, world rendering and actual Vulkan presentation.
struct WorldGraphicsDevice {
    VulkanLibrary loader = nullptr;
    VkInstance instance = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue queue = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    uint32_t queue_family = UINT32_MAX;
    PFN_vkGetInstanceProcAddr instance_proc = nullptr;
    PFN_vkGetDeviceProcAddr device_proc = nullptr;
    PFN_vkDestroyInstance destroy_instance = nullptr;
    PFN_vkDestroyDevice destroy_device = nullptr;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
#if defined(OGL_HAS_XLIB_WSI)
    Display* borrowed_xlib_display = nullptr;
    Window borrowed_xlib_window = 0;
#endif
    PFN_vkDestroySurfaceKHR destroy_surface = nullptr;
    PFN_vkDestroySwapchainKHR destroy_swapchain = nullptr;
    bool xlib_surface_extension = false;
    bool swapchain_probe_ready = false;
    bool exclusive_present_used = false;
    uint32_t exclusive_present_count = 0; // bounded, QA-only completed presents
    VkFormat swapchain_format = VK_FORMAT_UNDEFINED;
    VkColorSpaceKHR swapchain_color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    VkCompositeAlphaFlagBitsKHR swapchain_composite_alpha =
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    VkExtent2D swapchain_extent{};
    VkSurfaceTransformFlagBitsKHR swapchain_transform =
        VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    OglPersistentClearFrames persistent;
    OglOffscreenWorldCache offscreen_world_cache;
    uint32_t offscreen_world_builds=0;
    uint32_t offscreen_world_reuses=0;
    uint32_t offscreen_world_releases=0;
    // Cumulative QA counters (survive window detach; reset on device stop).
    uint32_t persistent_builds = 0;
    uint32_t persistent_releases = 0;
    uint32_t persistent_total_presents = 0;
    uint32_t persistent_teardown_idles = 0;
    uint64_t mesh_generation = 0; // accepted scene revisions, including clear
    uint64_t camera_mesh_generation = 0; // accepted pose binds ONE mesh generation
    uint32_t mesh_retire_waits = 0; // bounded GPU-fence waits, NOT queue idle
    uint32_t mesh_retire_failures = 0;

    // P05/4: private Vulkan world-geometry data, not an RTX BLAS/TLAS and
    // not a completed draw pipeline. Both buffers belong to this VkDevice.
    VkBuffer mesh_vertices = VK_NULL_HANDLE;
    VkDeviceMemory mesh_vertex_memory = VK_NULL_HANDLE;
    VkBuffer mesh_indices = VK_NULL_HANDLE;
    VkDeviceMemory mesh_index_memory = VK_NULL_HANDLE;
    // Optional face RGB sidecar: exact vertex ordering of the current mesh.
    // This buffer is not used by the historical WSI/Depth GPU draw paths.
    VkBuffer mesh_tint_vertices = VK_NULL_HANDLE;
    VkDeviceMemory mesh_tint_memory = VK_NULL_HANDLE;
    uint64_t tint_mesh_generation = 0;
    uint64_t tint_uploads = 0;
    uint32_t tint_vertex_count = 0;
    // Texture coordinates are a separate optional Vulkan vertex binding2.
    VkBuffer mesh_uv_vertices=VK_NULL_HANDLE;
    VkDeviceMemory mesh_uv_memory=VK_NULL_HANDLE;
    uint64_t uv_mesh_generation=0;
    uint64_t uv_uploads=0;
    uint32_t uv_vertex_count=0;
    std::vector<uint8_t> rgba_cpu_texels;
    uint32_t rgba_width=0,rgba_height=0;
    uint64_t rgba_mesh_generation=0,rgba_uploads=0;
    // P05 per-triangle 4-slot material binding3: three equal float
    // slots per triangle, exact original indexed vertex ordering.
    VkBuffer mesh_material_slot_vertices=VK_NULL_HANDLE;
    VkDeviceMemory mesh_material_slot_memory=VK_NULL_HANDLE;
    uint64_t material_slot_mesh_generation=0,material_slot_uploads=0;
    uint32_t material_slot_vertex_count=0,material_slot_count=0;
    uint32_t mesh_triangle_count = 0;
    // Optional exact byte-for-byte dedup for live captured scene snapshots.
    // Never suppress a real scene change based on a hash collision.
    std::vector<ogl_rtx_triangle> mesh_cpu_snapshot;
    uint64_t mesh_dedup_skips = 0;
    uint64_t region_barrier_accepts = 0;
    uint64_t region_barrier_failures = 0;
    // CPU-generated Vulkan clip projection from a validated Firestorm
    // camera. Exactly 16 column-major floats for the 64-byte push constant.
    std::array<float,16> world_to_clip{};
    bool world_camera_ready = false;
    float world_camera_aspect = 0.f;
    // Source: validated exact OpenSim/Firestorm v3 camera frame sun vector.
    // Normalized for a private GPU lighting push constant, never logged.
    std::array<float,4> world_sun_direction{};
    bool world_sun_ready=false;
    uint64_t world_camera_accepts = 0; // CPU matrix updates, no GPU presents
    uint64_t mesh_uploads = 0; // accepted snapshots, never a draw counter
    uint64_t offscreen_world_frames = 0; // completed VkDrawIndexed/readback
    uint32_t offscreen_world_covered_pixels = 0; // non-clear count
    uint32_t offscreen_world_width = 0, offscreen_world_height = 0;
    uint32_t offscreen_world_lit_pixels = 0;
    uint32_t offscreen_world_mean_luminance = 0; // 0..765 RGB sum
    uint32_t offscreen_world_mean_red = 0;
    uint32_t offscreen_world_mean_green = 0;
    uint32_t offscreen_world_mean_blue = 0;
    // P05/3: bounded GPU queue submission diagnostic. These counters refer
    // to commands submitted BEFORE waiting on their completion fences,
    // NOT to simultaneous GPU execution or completed swapchain presents.
    uint32_t sync_max_outstanding = 0;
    uint32_t sync_last_submitted = 0;
    uint32_t sync_last_completed = 0;
    PFN_vkDestroyBuffer destroy_buffer = nullptr;
    PFN_vkFreeMemory free_memory = nullptr;

    void release_offscreen_world_cache() {
        auto& c=offscreen_world_cache;
        if (!device) { c={}; return; }
        if (c.submitted) {
            // Do not release resources still referenced by VkQueueSubmit.
            // Prefer a bounded fence wait over vkQueueWaitIdle; a failure
            // quarantines the resources until device destruction.
            const auto wait=reinterpret_cast<PFN_vkWaitForFences>(
                device_proc(device,"vkWaitForFences"));
            if (!wait || !c.fence ||
                wait(device,1,&c.fence,VK_TRUE,2000000000ULL)!=VK_SUCCESS)
                return;
            c.submitted=false;
        }
        const auto proc=[&](const char* name) {
            return device_proc ? device_proc(device,name) : nullptr;
        };
#define OGL_OFFSCREEN_DESTROY(name) auto name=reinterpret_cast<PFN_##name>(proc(#name))
        OGL_OFFSCREEN_DESTROY(vkDestroyPipeline);
        OGL_OFFSCREEN_DESTROY(vkDestroyPipelineLayout);
        OGL_OFFSCREEN_DESTROY(vkDestroyDescriptorPool);
        OGL_OFFSCREEN_DESTROY(vkDestroyDescriptorSetLayout);
        OGL_OFFSCREEN_DESTROY(vkDestroySampler);
        OGL_OFFSCREEN_DESTROY(vkDestroyShaderModule);
        OGL_OFFSCREEN_DESTROY(vkDestroyFence);
        OGL_OFFSCREEN_DESTROY(vkDestroyCommandPool);
        OGL_OFFSCREEN_DESTROY(vkDestroyFramebuffer);
        OGL_OFFSCREEN_DESTROY(vkDestroyRenderPass);
        OGL_OFFSCREEN_DESTROY(vkDestroyImageView);
        OGL_OFFSCREEN_DESTROY(vkDestroyImage);
        OGL_OFFSCREEN_DESTROY(vkDestroyBuffer);
        OGL_OFFSCREEN_DESTROY(vkFreeMemory);
#undef OGL_OFFSCREEN_DESTROY
        if (vkDestroyPipeline && c.pipeline)
            vkDestroyPipeline(device,c.pipeline,nullptr);
        if (vkDestroyPipelineLayout && c.pipeline_layout)
            vkDestroyPipelineLayout(device,c.pipeline_layout,nullptr);
        if (vkDestroyDescriptorPool && c.texture_descriptor_pool)
            vkDestroyDescriptorPool(device,c.texture_descriptor_pool,nullptr);
        if (vkDestroyDescriptorSetLayout && c.texture_set_layout)
            vkDestroyDescriptorSetLayout(device,c.texture_set_layout,nullptr);
        if (vkDestroySampler && c.texture_sampler)
            vkDestroySampler(device,c.texture_sampler,nullptr);
        if (vkDestroyShaderModule) {
            if (c.vertex_shader) vkDestroyShaderModule(device,c.vertex_shader,nullptr);
            if (c.fragment_shader) vkDestroyShaderModule(device,c.fragment_shader,nullptr);
        }
        if (vkDestroyFence && c.fence) vkDestroyFence(device,c.fence,nullptr);
        if (vkDestroyCommandPool && c.command_pool)
            vkDestroyCommandPool(device,c.command_pool,nullptr);
        if (vkDestroyFramebuffer && c.framebuffer)
            vkDestroyFramebuffer(device,c.framebuffer,nullptr);
        if (vkDestroyRenderPass && c.render_pass)
            vkDestroyRenderPass(device,c.render_pass,nullptr);
        if (vkDestroyImageView) {
            if (c.texture_view) vkDestroyImageView(device,c.texture_view,nullptr);
            if (c.depth_view) vkDestroyImageView(device,c.depth_view,nullptr);
            if (c.view) vkDestroyImageView(device,c.view,nullptr);
        }
        if (vkDestroyImage) {
            if (c.texture_image) vkDestroyImage(device,c.texture_image,nullptr);
            if (c.depth) vkDestroyImage(device,c.depth,nullptr);
            if (c.color) vkDestroyImage(device,c.color,nullptr);
        }
        if (vkDestroyBuffer && c.readback)
            vkDestroyBuffer(device,c.readback,nullptr);
        if (vkFreeMemory) {
            if (c.texture_memory) vkFreeMemory(device,c.texture_memory,nullptr);
            if (c.depth_mem) vkFreeMemory(device,c.depth_mem,nullptr);
            if (c.color_mem) vkFreeMemory(device,c.color_mem,nullptr);
            if (c.readback_mem) vkFreeMemory(device,c.readback_mem,nullptr);
        }
        if (c.initialized) ++offscreen_world_releases;
        c={};
    }

    void clear_face_tints() {
        if (device && destroy_buffer && mesh_tint_vertices)
            destroy_buffer(device,mesh_tint_vertices,nullptr);
        if (device && free_memory && mesh_tint_memory)
            free_memory(device,mesh_tint_memory,nullptr);
        mesh_tint_vertices=VK_NULL_HANDLE;
        mesh_tint_memory=VK_NULL_HANDLE;
        tint_mesh_generation=0;
        tint_vertex_count=0;
    }

    void clear_face_uvs() {
        if (device && destroy_buffer && mesh_uv_vertices)
            destroy_buffer(device,mesh_uv_vertices,nullptr);
        if (device && free_memory && mesh_uv_memory)
            free_memory(device,mesh_uv_memory,nullptr);
        mesh_uv_vertices=VK_NULL_HANDLE;
        mesh_uv_memory=VK_NULL_HANDLE;
        uv_mesh_generation=0;
        uv_vertex_count=0;
    }

    void clear_material_slots() {
        if (device && destroy_buffer && mesh_material_slot_vertices)
            destroy_buffer(device,mesh_material_slot_vertices,nullptr);
        if (device && free_memory && mesh_material_slot_memory)
            free_memory(device,mesh_material_slot_memory,nullptr);
        mesh_material_slot_vertices=VK_NULL_HANDLE;
        mesh_material_slot_memory=VK_NULL_HANDLE;
        material_slot_mesh_generation=0;
        material_slot_vertex_count=0;
        material_slot_count=0;
    }

    void clear_rgba_image() {
        rgba_cpu_texels.clear();
        rgba_width=rgba_height=0;
        rgba_mesh_generation=0;
    }

    void clear_mesh() {
        clear_rgba_image();
        clear_material_slots();
        clear_face_uvs();
        clear_face_tints();
        if (device && destroy_buffer) {
            if (mesh_vertices) destroy_buffer(device,mesh_vertices,nullptr);
            if (mesh_indices) destroy_buffer(device,mesh_indices,nullptr);
        }
        mesh_vertices=VK_NULL_HANDLE;
        mesh_indices=VK_NULL_HANDLE;
        if (device && free_memory) {
            if (mesh_vertex_memory) free_memory(device,mesh_vertex_memory,nullptr);
            if (mesh_index_memory) free_memory(device,mesh_index_memory,nullptr);
        }
        mesh_vertex_memory=VK_NULL_HANDLE;
        mesh_index_memory=VK_NULL_HANDLE;
        mesh_triangle_count=0;
        mesh_cpu_snapshot.clear();
    }

    void release_persistent_frames() {
        if (!device) { persistent={}; return; }
        // Unlike the legacy test, this persistent path does NOT idle the
        // queue after every frame. A one-time queue idle on teardown is
        // required before freeing presentation semaphores and framebuffers.
        // The entire path is opt-in pre-GL QA, never an interactive renderer.
        if (persistent.submitted_any) {
            auto idle=reinterpret_cast<PFN_vkQueueWaitIdle>(
                device_proc(device,"vkQueueWaitIdle"));
            if (!idle || !queue || idle(queue)!=VK_SUCCESS) {
                persistent.blocked=true;
                // Device-loss case: keep objects allocated until VkDevice
                // destruction, instead of illegally freeing pending work.
                // Do not destruct resources still possibly referenced by GPU.
                return;
            }
            ++persistent_teardown_idles;
        }
        auto destroy_view=reinterpret_cast<PFN_vkDestroyImageView>(
            device_proc(device,"vkDestroyImageView"));
        auto destroy_fb=reinterpret_cast<PFN_vkDestroyFramebuffer>(
            device_proc(device,"vkDestroyFramebuffer"));
        auto destroy_pass=reinterpret_cast<PFN_vkDestroyRenderPass>(
            device_proc(device,"vkDestroyRenderPass"));
        auto destroy_pool=reinterpret_cast<PFN_vkDestroyCommandPool>(
            device_proc(device,"vkDestroyCommandPool"));
        auto destroy_semaphore=reinterpret_cast<PFN_vkDestroySemaphore>(
            device_proc(device,"vkDestroySemaphore"));
        auto destroy_fence=reinterpret_cast<PFN_vkDestroyFence>(
            device_proc(device,"vkDestroyFence"));
        auto destroy_pipeline=reinterpret_cast<PFN_vkDestroyPipeline>(
            device_proc(device,"vkDestroyPipeline"));
        auto destroy_layout=reinterpret_cast<PFN_vkDestroyPipelineLayout>(
            device_proc(device,"vkDestroyPipelineLayout"));
        auto destroy_shader=reinterpret_cast<PFN_vkDestroyShaderModule>(
            device_proc(device,"vkDestroyShaderModule"));
        auto destroy_image=reinterpret_cast<PFN_vkDestroyImage>(
            device_proc(device,"vkDestroyImage"));
        auto free_memory=reinterpret_cast<PFN_vkFreeMemory>(
            device_proc(device,"vkFreeMemory"));
        if (destroy_pipeline && persistent.pipeline)
            destroy_pipeline(device,persistent.pipeline,nullptr);
        if (destroy_layout && persistent.pipeline_layout)
            destroy_layout(device,persistent.pipeline_layout,nullptr);
        if (destroy_shader) {
            if (persistent.vertex_shader)
                destroy_shader(device,persistent.vertex_shader,nullptr);
            if (persistent.fragment_shader)
                destroy_shader(device,persistent.fragment_shader,nullptr);
        }
        if (destroy_fb)
            for (auto fb:persistent.framebuffers)
                if (fb) destroy_fb(device,fb,nullptr);
        if (destroy_view)
            for (auto view:persistent.depth_views)
                if (view) destroy_view(device,view,nullptr);
        if (destroy_image)
            for (auto image:persistent.depth_images)
                if (image) destroy_image(device,image,nullptr);
        if (free_memory)
            for (auto memory:persistent.depth_memories)
                if (memory) free_memory(device,memory,nullptr);
        if (destroy_pass && persistent.pass)
            destroy_pass(device,persistent.pass,nullptr);
        if (destroy_view)
            for (auto view:persistent.views)
                if (view) destroy_view(device,view,nullptr);
        if (destroy_pool && persistent.pool)
            destroy_pool(device,persistent.pool,nullptr);
        if (destroy_semaphore) {
            for (auto sem:persistent.image_acquired)
                if (sem) destroy_semaphore(device,sem,nullptr);
            for (auto sem:persistent.presented_by_image)
                if (sem) destroy_semaphore(device,sem,nullptr);
        }
        if (destroy_fence)
            for (auto fence:persistent.submit_fences)
                if (fence) destroy_fence(device,fence,nullptr);
        if (persistent.initialized) ++persistent_releases;
        persistent={};
    }
    void detach_window() {
        release_persistent_frames();
        // Never let VkSurfaceKHR/VkSwapchainKHR outlive the borrowed SDL XID.
        // Do not let either native handle outlive the borrowed SDL/X11 window.
        if (swapchain && destroy_swapchain && device)
            destroy_swapchain(device, swapchain, nullptr);
        swapchain = VK_NULL_HANDLE;
        swapchain_probe_ready = false;
        exclusive_present_used = false;
        exclusive_present_count = 0;
        swapchain_format = VK_FORMAT_UNDEFINED;
        swapchain_color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        swapchain_composite_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchain_extent = {};
        swapchain_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        if (surface && destroy_surface && instance)
            destroy_surface(instance, surface, nullptr);
        surface = VK_NULL_HANDLE;
#if defined(OGL_HAS_XLIB_WSI)
        borrowed_xlib_display = nullptr;
        borrowed_xlib_window = 0;
#endif
    }
    void close() {
        // Complete and release the private GL-independent GPU target
        // BEFORE freeing scene meshes or destroying VkDevice.
        release_offscreen_world_cache();
        detach_window(); // WSI handles must die before VkDevice/VkInstance.
        world_camera_ready = false;
        world_camera_aspect = 0.f;
        world_sun_direction.fill(0.f);
        world_sun_ready=false;
        camera_mesh_generation = 0;
        world_to_clip.fill(0.f);
        world_camera_accepts = 0;
        mesh_uploads = 0;
        offscreen_world_frames = 0;
        offscreen_world_covered_pixels = 0;
        offscreen_world_width = offscreen_world_height = 0;
        offscreen_world_lit_pixels = offscreen_world_mean_luminance = 0;
        offscreen_world_mean_red = offscreen_world_mean_green =
            offscreen_world_mean_blue = 0;
        tint_uploads = uv_uploads = rgba_uploads = material_slot_uploads = 0;
        offscreen_world_builds=offscreen_world_reuses=offscreen_world_releases=0;
        mesh_generation = 0;
        mesh_dedup_skips = 0;
        region_barrier_accepts = region_barrier_failures = 0;
        mesh_retire_waits = mesh_retire_failures = 0;
        sync_max_outstanding = 0;
        sync_last_submitted = 0;
        sync_last_completed = 0;
        persistent_builds = persistent_releases = 0;
        persistent_total_presents = persistent_teardown_idles = 0;
        clear_mesh();     // Vertex/index buffers never outlive VkDevice.
        if (device && destroy_device) destroy_device(device, nullptr);
        device = VK_NULL_HANDLE;
        queue = VK_NULL_HANDLE;
        physical_device = VK_NULL_HANDLE;
        queue_family = UINT32_MAX;
        if (instance && destroy_instance) destroy_instance(instance, nullptr);
        instance = VK_NULL_HANDLE;
        destroy_device = nullptr;
        destroy_instance = nullptr;
        destroy_surface = nullptr;
        destroy_swapchain = nullptr;
        instance_proc = nullptr;
        device_proc = nullptr;
        destroy_buffer = nullptr;
        free_memory = nullptr;
        xlib_surface_extension = false;
        if (loader) close_vulkan_loader(loader);
        loader = nullptr;
    }
    ~WorldGraphicsDevice() { close(); }
};
WorldGraphicsDevice world_device;

bool init_world_graphics_device() {
    world_device.close();
    world_device.loader = open_vulkan_loader();
    if (!world_device.loader) return false;
    auto get_proc = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
        lookup_vulkan_symbol(world_device.loader, "vkGetInstanceProcAddr"));
    if (!get_proc) { world_device.close(); return false; }
    world_device.instance_proc = get_proc;
    auto create_instance = reinterpret_cast<PFN_vkCreateInstance>(
        get_proc(VK_NULL_HANDLE, "vkCreateInstance"));
    if (!create_instance) { world_device.close(); return false; }
    // Surface WSI is never enabled without a second explicit developer flag.
    // Never require Xlib extensions for the normal cross-platform GPU probe.
    std::vector<const char*> required_extensions;
#if defined(OGL_HAS_XLIB_WSI)
    const char* surface_env=std::getenv("OGL_VULKAN_WINDOW_SURFACE");
    if (surface_env && std::strcmp(surface_env,"1")==0) {
        auto enumerate_ext=reinterpret_cast<PFN_vkEnumerateInstanceExtensionProperties>(
            get_proc(VK_NULL_HANDLE, "vkEnumerateInstanceExtensionProperties"));
        uint32_t extension_count=0;
        if (enumerate_ext &&
            enumerate_ext(nullptr,&extension_count,nullptr)==VK_SUCCESS &&
            extension_count>0 && extension_count<=4096) {
            std::vector<VkExtensionProperties> extensions(extension_count);
            if (enumerate_ext(nullptr,&extension_count,extensions.data())==VK_SUCCESS) {
                auto has_extension=[&](const char* name) {
                    return std::any_of(extensions.begin(),extensions.end(),
                        [name](const VkExtensionProperties& e) {
                            return std::strcmp(e.extensionName,name)==0;
                        });
                };
                if (has_extension(VK_KHR_SURFACE_EXTENSION_NAME) &&
                    has_extension(VK_KHR_XLIB_SURFACE_EXTENSION_NAME)) {
                    required_extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
                    required_extensions.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
                    world_device.xlib_surface_extension = true;
                }
            }
        }
    }
#endif
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "OGL World Device Bootstrap";
    app.apiVersion = VK_API_VERSION_1_1;
    VkInstanceCreateInfo ci{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    ci.pApplicationInfo = &app;
    ci.enabledExtensionCount = static_cast<uint32_t>(required_extensions.size());
    ci.ppEnabledExtensionNames = required_extensions.data();
    if (create_instance(&ci, nullptr, &world_device.instance)!=VK_SUCCESS ||
        !world_device.instance) {
        world_device.close(); return false;
    }
    auto proc=[&](const char* name) {
        return get_proc(world_device.instance, name);
    };
    world_device.destroy_instance =
        reinterpret_cast<PFN_vkDestroyInstance>(proc("vkDestroyInstance"));
    world_device.destroy_device =
        reinterpret_cast<PFN_vkDestroyDevice>(proc("vkDestroyDevice"));
    auto enumerate = reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(
        proc("vkEnumeratePhysicalDevices"));
    auto queue_families = reinterpret_cast<PFN_vkGetPhysicalDeviceQueueFamilyProperties>(
        proc("vkGetPhysicalDeviceQueueFamilyProperties"));
    auto get_extensions = reinterpret_cast<PFN_vkEnumerateDeviceExtensionProperties>(
        proc("vkEnumerateDeviceExtensionProperties"));
    auto create_device = reinterpret_cast<PFN_vkCreateDevice>(
        proc("vkCreateDevice"));
    auto get_device_proc = reinterpret_cast<PFN_vkGetDeviceProcAddr>(
        proc("vkGetDeviceProcAddr"));
    world_device.device_proc = get_device_proc;
    if (!world_device.destroy_instance || !world_device.destroy_device ||
        !enumerate || !queue_families || !get_extensions ||
        !create_device || !get_device_proc) {
        world_device.close(); return false;
    }
    uint32_t count=0;
    if (enumerate(world_device.instance, &count, nullptr)!=VK_SUCCESS ||
        count==0 || count>64) {
        world_device.close(); return false;
    }
    std::vector<VkPhysicalDevice> gpus(count);
    if (enumerate(world_device.instance, &count, gpus.data())!=VK_SUCCESS) {
        world_device.close(); return false;
    }
    for (uint32_t i=0; i<count; ++i) {
        uint32_t n=0;
        if (get_extensions(gpus[i],nullptr,&n,nullptr)!=VK_SUCCESS || n>4096)
            continue;
        std::vector<VkExtensionProperties> extensions(n);
        if (get_extensions(gpus[i],nullptr,&n,extensions.data())!=VK_SUCCESS)
            continue;
        const bool has_swapchain=std::any_of(extensions.begin(),extensions.end(),
            [](const VkExtensionProperties& extension) {
                return std::strcmp(extension.extensionName,
                    VK_KHR_SWAPCHAIN_EXTENSION_NAME)==0;
            });
        if (!has_swapchain) continue;
        uint32_t families=0;
        queue_families(gpus[i], &families, nullptr);
        if (!families || families>256) continue;
        std::vector<VkQueueFamilyProperties> props(families);
        queue_families(gpus[i], &families, props.data());
        for (uint32_t q=0; q<families; ++q) {
            if (!props[q].queueCount ||
                !(props[q].queueFlags & VK_QUEUE_GRAPHICS_BIT)) continue;
            float priority=1.f;
            VkDeviceQueueCreateInfo qi{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
            qi.queueFamilyIndex=q;
            qi.queueCount=1;
            qi.pQueuePriorities=&priority;
            const char* names[]={VK_KHR_SWAPCHAIN_EXTENSION_NAME};
            VkDeviceCreateInfo di{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
            di.queueCreateInfoCount=1;
            di.pQueueCreateInfos=&qi;
            di.enabledExtensionCount=1;
            di.ppEnabledExtensionNames=names;
            VkDevice device=VK_NULL_HANDLE;
            if (create_device(gpus[i],&di,nullptr,&device)!=VK_SUCCESS || !device)
                continue;
            // Dispatch device-level commands through the actual VkDevice.
            // A graphics queue must never be fabricated from a feature probe.
            auto destroy = reinterpret_cast<PFN_vkDestroyDevice>(
                get_device_proc(device,"vkDestroyDevice"));
            auto get_queue = reinterpret_cast<PFN_vkGetDeviceQueue>(
                get_device_proc(device,"vkGetDeviceQueue"));
            if (!destroy) destroy=world_device.destroy_device;
            if (!get_queue) {
                destroy(device,nullptr);
                continue;
            }
            VkQueue queue=VK_NULL_HANDLE;
            get_queue(device,q,0,&queue);
            if (!queue) {
                destroy(device,nullptr);
                continue;
            }
            world_device.destroy_device=destroy;
            world_device.device=device;
            world_device.queue=queue;
            world_device.physical_device=gpus[i];
            world_device.queue_family=q;
            world_device.destroy_buffer=
                reinterpret_cast<PFN_vkDestroyBuffer>(
                    get_device_proc(device,"vkDestroyBuffer"));
            world_device.free_memory=
                reinterpret_cast<PFN_vkFreeMemory>(
                    get_device_proc(device,"vkFreeMemory"));
            return true;
        }
    }
    world_device.close();
    return false;
}

// P05 WSI boundary: VkSurfaceKHR/VkSwapchainKHR on the REAL, already created
// SDL/X11 window. This never acquires images, submits commands or presents;
// OpenGL continues to own and render into that window. Vulkan ownership is
// opt-in and must terminate before the SDL/X11 window is destroyed.
bool attach_world_xlib_window(void* native_display, uint64_t native_window,
                              uint32_t width, uint32_t height) {
    world_device.detach_window();
#if defined(OGL_HAS_XLIB_WSI)
    if (!world_device.xlib_surface_extension || !world_device.instance ||
        !world_device.device || !world_device.queue ||
        !world_device.physical_device ||
        world_device.queue_family==UINT32_MAX ||
        !world_device.instance_proc || !world_device.device_proc ||
        !native_display || native_window==0 ||
        native_window>static_cast<uint64_t>(std::numeric_limits<Window>::max()) ||
        width==0 || height==0 || width>16384 || height>16384)
        return false;

    const auto instance_fn=[&](const char* name) {
        return world_device.instance_proc(world_device.instance,name);
    };
    const auto device_fn=[&](const char* name) {
        return world_device.device_proc(world_device.device,name);
    };
    const auto create_surface=reinterpret_cast<PFN_vkCreateXlibSurfaceKHR>(
        instance_fn("vkCreateXlibSurfaceKHR"));
    world_device.destroy_surface=reinterpret_cast<PFN_vkDestroySurfaceKHR>(
        instance_fn("vkDestroySurfaceKHR"));
    const auto supports_present=reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceSupportKHR>(
        instance_fn("vkGetPhysicalDeviceSurfaceSupportKHR"));
    const auto get_caps=reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>(
        instance_fn("vkGetPhysicalDeviceSurfaceCapabilitiesKHR"));
    const auto get_formats=reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>(
        instance_fn("vkGetPhysicalDeviceSurfaceFormatsKHR"));
    const auto get_modes=reinterpret_cast<PFN_vkGetPhysicalDeviceSurfacePresentModesKHR>(
        instance_fn("vkGetPhysicalDeviceSurfacePresentModesKHR"));
    const auto create_swapchain=reinterpret_cast<PFN_vkCreateSwapchainKHR>(
        device_fn("vkCreateSwapchainKHR"));
    world_device.destroy_swapchain=reinterpret_cast<PFN_vkDestroySwapchainKHR>(
        device_fn("vkDestroySwapchainKHR"));
    const auto get_images=reinterpret_cast<PFN_vkGetSwapchainImagesKHR>(
        device_fn("vkGetSwapchainImagesKHR"));
    if (!create_surface || !world_device.destroy_surface || !supports_present ||
        !get_caps || !get_formats || !get_modes || !create_swapchain ||
        !world_device.destroy_swapchain || !get_images)
        return false;

    VkXlibSurfaceCreateInfoKHR info{VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR};
    info.dpy=static_cast<Display*>(native_display);
    info.window=static_cast<Window>(native_window);
    if (create_surface(world_device.instance,&info,nullptr,
                       &world_device.surface)!=VK_SUCCESS ||
        !world_device.surface) {
        world_device.detach_window();
        return false;
    }
    VkBool32 present=VK_FALSE;
    if (supports_present(world_device.physical_device,
                         world_device.queue_family,world_device.surface,
                         &present)!=VK_SUCCESS || present!=VK_TRUE) {
        world_device.detach_window();
        return false;
    }
    VkSurfaceCapabilitiesKHR caps{};
    if (get_caps(world_device.physical_device,world_device.surface,
                 &caps)!=VK_SUCCESS ||
        !(caps.supportedUsageFlags&VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)) {
        world_device.detach_window();
        return false;
    }
    uint32_t count=0;
    if (get_formats(world_device.physical_device,world_device.surface,
                    &count,nullptr)!=VK_SUCCESS || count==0 || count>256) {
        world_device.detach_window();
        return false;
    }
    const uint32_t expected_formats=count;
    std::vector<VkSurfaceFormatKHR> formats(expected_formats);
    if (get_formats(world_device.physical_device,world_device.surface,
                    &count,formats.data())!=VK_SUCCESS ||
        count!=expected_formats ||
        // VK_FORMAT_UNDEFINED is legal ONLY for a single surface-format
        // entry: it asks the application to select its own compatible
        // format. It must not be chosen as a concrete swapchain format.
        std::any_of(formats.begin(),formats.end(),
            [expected_formats](const VkSurfaceFormatKHR& entry) {
                return entry.format==VK_FORMAT_UNDEFINED &&
                    (expected_formats!=1 ||
                     entry.colorSpace!=VK_COLOR_SPACE_SRGB_NONLINEAR_KHR);
            })) {
        world_device.detach_window();
        return false;
    }
    VkSurfaceFormatKHR format=formats.front();
    if (expected_formats==1 && format.format==VK_FORMAT_UNDEFINED)
        format={VK_FORMAT_B8G8R8A8_UNORM,VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    else for (const auto& entry : formats) {
        if (entry.format==VK_FORMAT_B8G8R8A8_UNORM &&
            entry.colorSpace==VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            format=entry; break;
        }
    }
    count=0;
    if (get_modes(world_device.physical_device,world_device.surface,
                  &count,nullptr)!=VK_SUCCESS || count==0 || count>256) {
        world_device.detach_window();
        return false;
    }
    const uint32_t expected_modes=count;
    std::vector<VkPresentModeKHR> modes(expected_modes);
    if (get_modes(world_device.physical_device,world_device.surface,
                  &count,modes.data())!=VK_SUCCESS ||
        count!=expected_modes ||
        std::find(modes.begin(),modes.end(),VK_PRESENT_MODE_FIFO_KHR)==modes.end()) {
        world_device.detach_window();
        return false;
    }
    // A malformed WSI capabilities snapshot must not reach swapchain
    // creation, especially when the surface provides a fixed extent.
    if (!caps.minImageCount || caps.minImageCount>16 ||
        !caps.maxImageArrayLayers ||
        !caps.minImageExtent.width || !caps.minImageExtent.height ||
        caps.minImageExtent.width>caps.maxImageExtent.width ||
        caps.minImageExtent.height>caps.maxImageExtent.height ||
        // Vulkan specifies either BOTH dimensions as variable, or BOTH
        // as fixed. Never combine a sentinel with a fixed dimension.
        ((caps.currentExtent.width==UINT32_MAX) !=
         (caps.currentExtent.height==UINT32_MAX)) ||
        !(caps.supportedTransforms&caps.currentTransform) ||
        (caps.maxImageCount && caps.maxImageCount<caps.minImageCount)) {
        world_device.detach_window();
        return false;
    }
    VkExtent2D extent=caps.currentExtent;
    if (extent.width==UINT32_MAX) {
        extent.width=std::max(caps.minImageExtent.width,
                              std::min(width,caps.maxImageExtent.width));
        extent.height=std::max(caps.minImageExtent.height,
                               std::min(height,caps.maxImageExtent.height));
    }
    if (extent.width==0 || extent.height==0 ||
        extent.width>16384 || extent.height>16384 ||
        extent.width<caps.minImageExtent.width ||
        extent.height<caps.minImageExtent.height ||
        extent.width>caps.maxImageExtent.width ||
        extent.height>caps.maxImageExtent.height) {
        world_device.detach_window();
        return false;
    }
    uint32_t images=std::max(2u,caps.minImageCount);
    if (caps.maxImageCount && images>caps.maxImageCount) {
        world_device.detach_window();
        return false;
    }
    VkCompositeAlphaFlagBitsKHR composite=VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    bool supported_alpha=false;
    for (auto alpha : {VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                       VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
                       VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
                       VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR}) {
        if (caps.supportedCompositeAlpha&alpha) {
            composite=alpha; supported_alpha=true; break;
        }
    }
    if (!supported_alpha) {
        world_device.detach_window();
        return false;
    }
    VkSwapchainCreateInfoKHR swap{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swap.surface=world_device.surface;
    swap.minImageCount=images;
    swap.imageFormat=format.format;
    swap.imageColorSpace=format.colorSpace;
    swap.imageExtent=extent;
    swap.imageArrayLayers=1;
    swap.imageUsage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swap.imageSharingMode=VK_SHARING_MODE_EXCLUSIVE;
    swap.preTransform=caps.currentTransform;
    swap.compositeAlpha=composite;
    swap.presentMode=VK_PRESENT_MODE_FIFO_KHR;
    swap.clipped=VK_TRUE;
    if (create_swapchain(world_device.device,&swap,nullptr,
                         &world_device.swapchain)!=VK_SUCCESS ||
        !world_device.swapchain) {
        world_device.detach_window();
        return false;
    }
    count=0;
    if (get_images(world_device.device,world_device.swapchain,
                   &count,nullptr)!=VK_SUCCESS || count<2 || count>16) {
        world_device.detach_window();
        return false;
    }
    // A count-only query does not prove the swapchain exposes usable images.
    // Verify the second enumeration before publishing WSI readiness, and
    // reject a concurrent image-count change rather than retaining a
    // partially enumerated or uninitialized attachment set.
    const uint32_t expected_count=count;
    std::vector<VkImage> swapchain_images(expected_count,VK_NULL_HANDLE);
    if (get_images(world_device.device,world_device.swapchain,
                   &count,swapchain_images.data())!=VK_SUCCESS ||
        count!=expected_count ||
        std::any_of(swapchain_images.begin(),swapchain_images.end(),
                    [](VkImage image){ return image==VK_NULL_HANDLE; })) {
        world_device.detach_window();
        return false;
    }
    // Store borrowed X11 identity ONLY after successful WSI validation.
    // Never own or destroy the SDL/X11 display/window from this module.
    world_device.borrowed_xlib_display=static_cast<Display*>(native_display);
    world_device.borrowed_xlib_window=static_cast<Window>(native_window);
    world_device.swapchain_probe_ready=true;
    world_device.swapchain_format=format.format;
    world_device.swapchain_color_space=format.colorSpace;
    world_device.swapchain_composite_alpha=composite;
    world_device.swapchain_extent=extent;
    world_device.swapchain_transform=caps.currentTransform;
    // A camera belonging to a DIFFERENT viewport aspect must never
    // render into this swapchain after a resize, even when it is valid
    // for the same region/mesh. A new camera update is required.
    const float swap_aspect=float(extent.width)/float(extent.height);
    if (world_device.world_camera_ready &&
        std::fabs(world_device.world_camera_aspect-swap_aspect)>0.01f) {
        world_device.world_camera_ready=false;
        world_device.world_camera_aspect=0.f;
        world_device.world_sun_ready=false;
        world_device.world_sun_direction.fill(0.f);
        world_device.camera_mesh_generation=0;
        world_device.world_to_clip.fill(0.f);
    }
    // Passive WSI attach never acquires or presents; GL remains unchanged.
    return true;
#else
    (void)native_display; (void)native_window; (void)width; (void)height;
    return false;
#endif
}

// P05/4: frozen Vulkan 1.0 SPIR-V for an opt-in, clip-space test triangle.
// GLSL 450 generated with Khronos glslang 15.1.0, validated by spirv-val;
// source lives in this comment, no runtime shader compiler is required:
//   vertex: layout(location=0) in vec3 position;
//           void main(){gl_Position=vec4(position,1.0);}
//   fragment: layout(location=0) out vec4 color;
//           void main(){color=vec4(0.93,0.19,0.15,1.0);}
// Real OpenSim agent-space vertices still REQUIRE a camera transform.
// The ordinary GL Viewer never opts into this clip-space GPU diagnostic.
static constexpr uint32_t ogl_dev_vert_spv[] = {
    0x07230203u, 0x00010000u, 0x0008000bu, 0x0000001bu, 0x00000000u, 0x00020011u, 0x00000001u,
    0x0006000bu, 0x00000001u, 0x4c534c47u, 0x6474732eu, 0x3035342eu, 0x00000000u, 0x0003000eu,
    0x00000000u, 0x00000001u, 0x0007000fu, 0x00000000u, 0x00000004u, 0x6e69616du, 0x00000000u,
    0x0000000du, 0x00000012u, 0x00030003u, 0x00000002u, 0x000001c2u, 0x00040005u, 0x00000004u,
    0x6e69616du, 0x00000000u, 0x00060005u, 0x0000000bu, 0x505f6c67u, 0x65567265u, 0x78657472u,
    0x00000000u, 0x00060006u, 0x0000000bu, 0x00000000u, 0x505f6c67u, 0x7469736fu, 0x006e6f69u,
    0x00070006u, 0x0000000bu, 0x00000001u, 0x505f6c67u, 0x746e696fu, 0x657a6953u, 0x00000000u,
    0x00070006u, 0x0000000bu, 0x00000002u, 0x435f6c67u, 0x4470696cu, 0x61747369u, 0x0065636eu,
    0x00070006u, 0x0000000bu, 0x00000003u, 0x435f6c67u, 0x446c6c75u, 0x61747369u, 0x0065636eu,
    0x00030005u, 0x0000000du, 0x00000000u, 0x00050005u, 0x00000012u, 0x69736f70u, 0x6e6f6974u,
    0x00000000u, 0x00030047u, 0x0000000bu, 0x00000002u, 0x00050048u, 0x0000000bu, 0x00000000u,
    0x0000000bu, 0x00000000u, 0x00050048u, 0x0000000bu, 0x00000001u, 0x0000000bu, 0x00000001u,
    0x00050048u, 0x0000000bu, 0x00000002u, 0x0000000bu, 0x00000003u, 0x00050048u, 0x0000000bu,
    0x00000003u, 0x0000000bu, 0x00000004u, 0x00040047u, 0x00000012u, 0x0000001eu, 0x00000000u,
    0x00020013u, 0x00000002u, 0x00030021u, 0x00000003u, 0x00000002u, 0x00030016u, 0x00000006u,
    0x00000020u, 0x00040017u, 0x00000007u, 0x00000006u, 0x00000004u, 0x00040015u, 0x00000008u,
    0x00000020u, 0x00000000u, 0x0004002bu, 0x00000008u, 0x00000009u, 0x00000001u, 0x0004001cu,
    0x0000000au, 0x00000006u, 0x00000009u, 0x0006001eu, 0x0000000bu, 0x00000007u, 0x00000006u,
    0x0000000au, 0x0000000au, 0x00040020u, 0x0000000cu, 0x00000003u, 0x0000000bu, 0x0004003bu,
    0x0000000cu, 0x0000000du, 0x00000003u, 0x00040015u, 0x0000000eu, 0x00000020u, 0x00000001u,
    0x0004002bu, 0x0000000eu, 0x0000000fu, 0x00000000u, 0x00040017u, 0x00000010u, 0x00000006u,
    0x00000003u, 0x00040020u, 0x00000011u, 0x00000001u, 0x00000010u, 0x0004003bu, 0x00000011u,
    0x00000012u, 0x00000001u, 0x0004002bu, 0x00000006u, 0x00000014u, 0x3f800000u, 0x00040020u,
    0x00000019u, 0x00000003u, 0x00000007u, 0x00050036u, 0x00000002u, 0x00000004u, 0x00000000u,
    0x00000003u, 0x000200f8u, 0x00000005u, 0x0004003du, 0x00000010u, 0x00000013u, 0x00000012u,
    0x00050051u, 0x00000006u, 0x00000015u, 0x00000013u, 0x00000000u, 0x00050051u, 0x00000006u,
    0x00000016u, 0x00000013u, 0x00000001u, 0x00050051u, 0x00000006u, 0x00000017u, 0x00000013u,
    0x00000002u, 0x00070050u, 0x00000007u, 0x00000018u, 0x00000015u, 0x00000016u, 0x00000017u,
    0x00000014u, 0x00050041u, 0x00000019u, 0x0000001au, 0x0000000du, 0x0000000fu, 0x0003003eu,
    0x0000001au, 0x00000018u, 0x000100fdu, 0x00010038u,
};

static constexpr uint32_t ogl_dev_frag_spv[] = {
    0x07230203u, 0x00010000u, 0x0008000bu, 0x0000000fu, 0x00000000u, 0x00020011u, 0x00000001u,
    0x0006000bu, 0x00000001u, 0x4c534c47u, 0x6474732eu, 0x3035342eu, 0x00000000u, 0x0003000eu,
    0x00000000u, 0x00000001u, 0x0006000fu, 0x00000004u, 0x00000004u, 0x6e69616du, 0x00000000u,
    0x00000009u, 0x00030010u, 0x00000004u, 0x00000007u, 0x00030003u, 0x00000002u, 0x000001c2u,
    0x00040005u, 0x00000004u, 0x6e69616du, 0x00000000u, 0x00040005u, 0x00000009u, 0x6f6c6f63u,
    0x00000072u, 0x00040047u, 0x00000009u, 0x0000001eu, 0x00000000u, 0x00020013u, 0x00000002u,
    0x00030021u, 0x00000003u, 0x00000002u, 0x00030016u, 0x00000006u, 0x00000020u, 0x00040017u,
    0x00000007u, 0x00000006u, 0x00000004u, 0x00040020u, 0x00000008u, 0x00000003u, 0x00000007u,
    0x0004003bu, 0x00000008u, 0x00000009u, 0x00000003u, 0x0004002bu, 0x00000006u, 0x0000000au,
    0x3f6e147bu, 0x0004002bu, 0x00000006u, 0x0000000bu, 0x3e428f5cu, 0x0004002bu, 0x00000006u,
    0x0000000cu, 0x3e19999au, 0x0004002bu, 0x00000006u, 0x0000000du, 0x3f800000u, 0x0007002cu,
    0x00000007u, 0x0000000eu, 0x0000000au, 0x0000000bu, 0x0000000cu, 0x0000000du, 0x00050036u,
    0x00000002u, 0x00000004u, 0x00000000u, 0x00000003u, 0x000200f8u, 0x00000005u, 0x0003003eu,
    0x00000009u, 0x0000000eu, 0x000100fdu, 0x00010038u,
};

// Opt-in Vulkan 1.0 vertex shader for camera-space projection of OpenSim
// agent-local XYZ positions. GLSL 450, verified with Khronos spirv-val:
// layout(push_constant) uniform Camera { mat4 world_to_clip; } camera;
// layout(location=0) in vec3 position;
// void main(){gl_Position=camera.world_to_clip*vec4(position,1.0);}
// This only demonstrates GPU projection; it is NOT an OpenSim world renderer.
static constexpr uint32_t ogl_dev_world_vert_spv[] = {
    0x07230203u, 0x00010000u, 0x0008000bu, 0x00000023u, 0x00000000u, 0x00020011u, 0x00000001u,
    0x0006000bu, 0x00000001u, 0x4c534c47u, 0x6474732eu, 0x3035342eu, 0x00000000u, 0x0003000eu,
    0x00000000u, 0x00000001u, 0x0007000fu, 0x00000000u, 0x00000004u, 0x6e69616du, 0x00000000u,
    0x0000000du, 0x00000019u, 0x00030003u, 0x00000002u, 0x000001c2u, 0x00040005u, 0x00000004u,
    0x6e69616du, 0x00000000u, 0x00060005u, 0x0000000bu, 0x505f6c67u, 0x65567265u, 0x78657472u,
    0x00000000u, 0x00060006u, 0x0000000bu, 0x00000000u, 0x505f6c67u, 0x7469736fu, 0x006e6f69u,
    0x00070006u, 0x0000000bu, 0x00000001u, 0x505f6c67u, 0x746e696fu, 0x657a6953u, 0x00000000u,
    0x00070006u, 0x0000000bu, 0x00000002u, 0x435f6c67u, 0x4470696cu, 0x61747369u, 0x0065636eu,
    0x00070006u, 0x0000000bu, 0x00000003u, 0x435f6c67u, 0x446c6c75u, 0x61747369u, 0x0065636eu,
    0x00030005u, 0x0000000du, 0x00000000u, 0x00070005u, 0x00000011u, 0x656d6143u, 0x72506172u,
    0x63656a6fu, 0x6e6f6974u, 0x00000000u, 0x00070006u, 0x00000011u, 0x00000000u, 0x6c726f77u,
    0x6f745f64u, 0x696c635fu, 0x00000070u, 0x00040005u, 0x00000013u, 0x656d6163u, 0x00006172u,
    0x00050005u, 0x00000019u, 0x69736f70u, 0x6e6f6974u, 0x00000000u, 0x00030047u, 0x0000000bu,
    0x00000002u, 0x00050048u, 0x0000000bu, 0x00000000u, 0x0000000bu, 0x00000000u, 0x00050048u,
    0x0000000bu, 0x00000001u, 0x0000000bu, 0x00000001u, 0x00050048u, 0x0000000bu, 0x00000002u,
    0x0000000bu, 0x00000003u, 0x00050048u, 0x0000000bu, 0x00000003u, 0x0000000bu, 0x00000004u,
    0x00030047u, 0x00000011u, 0x00000002u, 0x00040048u, 0x00000011u, 0x00000000u, 0x00000005u,
    0x00050048u, 0x00000011u, 0x00000000u, 0x00000007u, 0x00000010u, 0x00050048u, 0x00000011u,
    0x00000000u, 0x00000023u, 0x00000000u, 0x00040047u, 0x00000019u, 0x0000001eu, 0x00000000u,
    0x00020013u, 0x00000002u, 0x00030021u, 0x00000003u, 0x00000002u, 0x00030016u, 0x00000006u,
    0x00000020u, 0x00040017u, 0x00000007u, 0x00000006u, 0x00000004u, 0x00040015u, 0x00000008u,
    0x00000020u, 0x00000000u, 0x0004002bu, 0x00000008u, 0x00000009u, 0x00000001u, 0x0004001cu,
    0x0000000au, 0x00000006u, 0x00000009u, 0x0006001eu, 0x0000000bu, 0x00000007u, 0x00000006u,
    0x0000000au, 0x0000000au, 0x00040020u, 0x0000000cu, 0x00000003u, 0x0000000bu, 0x0004003bu,
    0x0000000cu, 0x0000000du, 0x00000003u, 0x00040015u, 0x0000000eu, 0x00000020u, 0x00000001u,
    0x0004002bu, 0x0000000eu, 0x0000000fu, 0x00000000u, 0x00040018u, 0x00000010u, 0x00000007u,
    0x00000004u, 0x0003001eu, 0x00000011u, 0x00000010u, 0x00040020u, 0x00000012u, 0x00000009u,
    0x00000011u, 0x0004003bu, 0x00000012u, 0x00000013u, 0x00000009u, 0x00040020u, 0x00000014u,
    0x00000009u, 0x00000010u, 0x00040017u, 0x00000017u, 0x00000006u, 0x00000003u, 0x00040020u,
    0x00000018u, 0x00000001u, 0x00000017u, 0x0004003bu, 0x00000018u, 0x00000019u, 0x00000001u,
    0x0004002bu, 0x00000006u, 0x0000001bu, 0x3f800000u, 0x00040020u, 0x00000021u, 0x00000003u,
    0x00000007u, 0x00050036u, 0x00000002u, 0x00000004u, 0x00000000u, 0x00000003u, 0x000200f8u,
    0x00000005u, 0x00050041u, 0x00000014u, 0x00000015u, 0x00000013u, 0x0000000fu, 0x0004003du,
    0x00000010u, 0x00000016u, 0x00000015u, 0x0004003du, 0x00000017u, 0x0000001au, 0x00000019u,
    0x00050051u, 0x00000006u, 0x0000001cu, 0x0000001au, 0x00000000u, 0x00050051u, 0x00000006u,
    0x0000001du, 0x0000001au, 0x00000001u, 0x00050051u, 0x00000006u, 0x0000001eu, 0x0000001au,
    0x00000002u, 0x00070050u, 0x00000007u, 0x0000001fu, 0x0000001cu, 0x0000001du, 0x0000001eu,
    0x0000001bu, 0x00050091u, 0x00000007u, 0x00000020u, 0x00000016u, 0x0000001fu, 0x00050041u,
    0x00000021u, 0x00000022u, 0x0000000du, 0x0000000fu, 0x0003003eu, 0x00000022u, 0x00000020u,
    0x000100fdu, 0x00010038u,
};

// Vulkan 1.0 validated GLSL 450: private world-position / face-normal lighting.
// Embedded immutable shader bytes; no runtime shader compiler or extra assets.
static constexpr uint32_t ogl_dev_world_lambert_vert_spv[] = {
    0x07230203u, 0x00010000u, 0x0008000bu, 0x00000026u, 0x00000000u, 0x00020011u, 0x00000001u,
    0x0006000bu, 0x00000001u, 0x4c534c47u, 0x6474732eu, 0x3035342eu, 0x00000000u, 0x0003000eu,
    0x00000000u, 0x00000001u, 0x0008000fu, 0x00000000u, 0x00000004u, 0x6e69616du, 0x00000000u,
    0x00000009u, 0x0000000bu, 0x00000013u, 0x00030003u, 0x00000002u, 0x000001c2u, 0x00040005u,
    0x00000004u, 0x6e69616du, 0x00000000u, 0x00060005u, 0x00000009u, 0x6c726f77u, 0x6f705f64u,
    0x69746973u, 0x00006e6fu, 0x00050005u, 0x0000000bu, 0x69736f70u, 0x6e6f6974u, 0x00000000u,
    0x00060005u, 0x00000011u, 0x505f6c67u, 0x65567265u, 0x78657472u, 0x00000000u, 0x00060006u,
    0x00000011u, 0x00000000u, 0x505f6c67u, 0x7469736fu, 0x006e6f69u, 0x00070006u, 0x00000011u,
    0x00000001u, 0x505f6c67u, 0x746e696fu, 0x657a6953u, 0x00000000u, 0x00070006u, 0x00000011u,
    0x00000002u, 0x435f6c67u, 0x4470696cu, 0x61747369u, 0x0065636eu, 0x00070006u, 0x00000011u,
    0x00000003u, 0x435f6c67u, 0x446c6c75u, 0x61747369u, 0x0065636eu, 0x00030005u, 0x00000013u,
    0x00000000u, 0x00040005u, 0x00000017u, 0x656d6143u, 0x00006172u, 0x00070006u, 0x00000017u,
    0x00000000u, 0x6c726f77u, 0x6f745f64u, 0x696c635fu, 0x00000070u, 0x00040005u, 0x00000019u,
    0x656d6163u, 0x00006172u, 0x00040047u, 0x00000009u, 0x0000001eu, 0x00000000u, 0x00040047u,
    0x0000000bu, 0x0000001eu, 0x00000000u, 0x00030047u, 0x00000011u, 0x00000002u, 0x00050048u,
    0x00000011u, 0x00000000u, 0x0000000bu, 0x00000000u, 0x00050048u, 0x00000011u, 0x00000001u,
    0x0000000bu, 0x00000001u, 0x00050048u, 0x00000011u, 0x00000002u, 0x0000000bu, 0x00000003u,
    0x00050048u, 0x00000011u, 0x00000003u, 0x0000000bu, 0x00000004u, 0x00030047u, 0x00000017u,
    0x00000002u, 0x00040048u, 0x00000017u, 0x00000000u, 0x00000005u, 0x00050048u, 0x00000017u,
    0x00000000u, 0x00000007u, 0x00000010u, 0x00050048u, 0x00000017u, 0x00000000u, 0x00000023u,
    0x00000000u, 0x00020013u, 0x00000002u, 0x00030021u, 0x00000003u, 0x00000002u, 0x00030016u,
    0x00000006u, 0x00000020u, 0x00040017u, 0x00000007u, 0x00000006u, 0x00000003u, 0x00040020u,
    0x00000008u, 0x00000003u, 0x00000007u, 0x0004003bu, 0x00000008u, 0x00000009u, 0x00000003u,
    0x00040020u, 0x0000000au, 0x00000001u, 0x00000007u, 0x0004003bu, 0x0000000au, 0x0000000bu,
    0x00000001u, 0x00040017u, 0x0000000du, 0x00000006u, 0x00000004u, 0x00040015u, 0x0000000eu,
    0x00000020u, 0x00000000u, 0x0004002bu, 0x0000000eu, 0x0000000fu, 0x00000001u, 0x0004001cu,
    0x00000010u, 0x00000006u, 0x0000000fu, 0x0006001eu, 0x00000011u, 0x0000000du, 0x00000006u,
    0x00000010u, 0x00000010u, 0x00040020u, 0x00000012u, 0x00000003u, 0x00000011u, 0x0004003bu,
    0x00000012u, 0x00000013u, 0x00000003u, 0x00040015u, 0x00000014u, 0x00000020u, 0x00000001u,
    0x0004002bu, 0x00000014u, 0x00000015u, 0x00000000u, 0x00040018u, 0x00000016u, 0x0000000du,
    0x00000004u, 0x0003001eu, 0x00000017u, 0x00000016u, 0x00040020u, 0x00000018u, 0x00000009u,
    0x00000017u, 0x0004003bu, 0x00000018u, 0x00000019u, 0x00000009u, 0x00040020u, 0x0000001au,
    0x00000009u, 0x00000016u, 0x0004002bu, 0x00000006u, 0x0000001eu, 0x3f800000u, 0x00040020u,
    0x00000024u, 0x00000003u, 0x0000000du, 0x00050036u, 0x00000002u, 0x00000004u, 0x00000000u,
    0x00000003u, 0x000200f8u, 0x00000005u, 0x0004003du, 0x00000007u, 0x0000000cu, 0x0000000bu,
    0x0003003eu, 0x00000009u, 0x0000000cu, 0x00050041u, 0x0000001au, 0x0000001bu, 0x00000019u,
    0x00000015u, 0x0004003du, 0x00000016u, 0x0000001cu, 0x0000001bu, 0x0004003du, 0x00000007u,
    0x0000001du, 0x0000000bu, 0x00050051u, 0x00000006u, 0x0000001fu, 0x0000001du, 0x00000000u,
    0x00050051u, 0x00000006u, 0x00000020u, 0x0000001du, 0x00000001u, 0x00050051u, 0x00000006u,
    0x00000021u, 0x0000001du, 0x00000002u, 0x00070050u, 0x0000000du, 0x00000022u, 0x0000001fu,
    0x00000020u, 0x00000021u, 0x0000001eu, 0x00050091u, 0x0000000du, 0x00000023u, 0x0000001cu,
    0x00000022u, 0x00050041u, 0x00000024u, 0x00000025u, 0x00000013u, 0x00000015u, 0x0003003eu,
    0x00000025u, 0x00000023u, 0x000100fdu, 0x00010038u,
};

// Vulkan 1.0 validated GLSL 450: private world-position / face-normal lighting.
// Embedded immutable shader bytes; no runtime shader compiler or extra assets.
static constexpr uint32_t ogl_dev_world_lambert_frag_spv[] = {
    0x07230203u, 0x00010000u, 0x0008000bu, 0x00000047u, 0x00000000u, 0x00020011u, 0x00000001u,
    0x0006000bu, 0x00000001u, 0x4c534c47u, 0x6474732eu, 0x3035342eu, 0x00000000u, 0x0003000eu,
    0x00000000u, 0x00000001u, 0x0007000fu, 0x00000004u, 0x00000004u, 0x6e69616du, 0x00000000u,
    0x0000000bu, 0x00000041u, 0x00030010u, 0x00000004u, 0x00000007u, 0x00030003u, 0x00000002u,
    0x000001c2u, 0x00040005u, 0x00000004u, 0x6e69616du, 0x00000000u, 0x00030005u, 0x00000009u,
    0x00007864u, 0x00060005u, 0x0000000bu, 0x6c726f77u, 0x6f705f64u, 0x69746973u, 0x00006e6fu,
    0x00030005u, 0x0000000eu, 0x00007964u, 0x00050005u, 0x00000011u, 0x5f776172u, 0x6d726f6eu,
    0x00006c61u, 0x00040005u, 0x00000016u, 0x676e656cu, 0x00326874u, 0x00050005u, 0x0000001au,
    0x65636166u, 0x726f6e5fu, 0x006c616du, 0x00040005u, 0x0000002bu, 0x6867696cu, 0x00000074u,
    0x00040005u, 0x00000030u, 0x66666964u, 0x00657375u, 0x00060005u, 0x00000039u, 0x66666964u,
    0x5f657375u, 0x6f6c6f63u, 0x00000072u, 0x00050005u, 0x00000041u, 0x5f74756fu, 0x6f6c6f63u,
    0x00000072u, 0x00040047u, 0x0000000bu, 0x0000001eu, 0x00000000u, 0x00040047u, 0x00000041u,
    0x0000001eu, 0x00000000u, 0x00020013u, 0x00000002u, 0x00030021u, 0x00000003u, 0x00000002u,
    0x00030016u, 0x00000006u, 0x00000020u, 0x00040017u, 0x00000007u, 0x00000006u, 0x00000003u,
    0x00040020u, 0x00000008u, 0x00000007u, 0x00000007u, 0x00040020u, 0x0000000au, 0x00000001u,
    0x00000007u, 0x0004003bu, 0x0000000au, 0x0000000bu, 0x00000001u, 0x00040020u, 0x00000015u,
    0x00000007u, 0x00000006u, 0x0004002bu, 0x00000006u, 0x0000001cu, 0x2b8cbcccu, 0x00020014u,
    0x0000001du, 0x0004002bu, 0x00000006u, 0x00000027u, 0x00000000u, 0x0004002bu, 0x00000006u,
    0x00000028u, 0x3f800000u, 0x0006002cu, 0x00000007u, 0x00000029u, 0x00000027u, 0x00000027u,
    0x00000028u, 0x0004002bu, 0x00000006u, 0x0000002cu, 0x3f6bc40bu, 0x0004002bu, 0x00000006u,
    0x0000002du, 0x3e1d2d5du, 0x0004002bu, 0x00000006u, 0x0000002eu, 0x3eb75f97u, 0x0006002cu,
    0x00000007u, 0x0000002fu, 0x0000002cu, 0x0000002du, 0x0000002eu, 0x0004002bu, 0x00000006u,
    0x00000031u, 0x3e4ccccdu, 0x0004002bu, 0x00000006u, 0x00000032u, 0x3f4ccccdu, 0x0004002bu,
    0x00000006u, 0x0000003au, 0x3f47ae14u, 0x0004002bu, 0x00000006u, 0x0000003bu, 0x3ec7ae14u,
    0x0006002cu, 0x00000007u, 0x0000003cu, 0x00000031u, 0x0000003au, 0x0000003bu, 0x00040017u,
    0x0000003fu, 0x00000006u, 0x00000004u, 0x00040020u, 0x00000040u, 0x00000003u, 0x0000003fu,
    0x0004003bu, 0x00000040u, 0x00000041u, 0x00000003u, 0x00050036u, 0x00000002u, 0x00000004u,
    0x00000000u, 0x00000003u, 0x000200f8u, 0x00000005u, 0x0004003bu, 0x00000008u, 0x00000009u,
    0x00000007u, 0x0004003bu, 0x00000008u, 0x0000000eu, 0x00000007u, 0x0004003bu, 0x00000008u,
    0x00000011u, 0x00000007u, 0x0004003bu, 0x00000015u, 0x00000016u, 0x00000007u, 0x0004003bu,
    0x00000008u, 0x0000001au, 0x00000007u, 0x0004003bu, 0x00000008u, 0x0000001fu, 0x00000007u,
    0x0004003bu, 0x00000008u, 0x0000002bu, 0x00000007u, 0x0004003bu, 0x00000015u, 0x00000030u,
    0x00000007u, 0x0004003bu, 0x00000008u, 0x00000039u, 0x00000007u, 0x0004003du, 0x00000007u,
    0x0000000cu, 0x0000000bu, 0x000400cfu, 0x00000007u, 0x0000000du, 0x0000000cu, 0x0003003eu,
    0x00000009u, 0x0000000du, 0x0004003du, 0x00000007u, 0x0000000fu, 0x0000000bu, 0x000400d0u,
    0x00000007u, 0x00000010u, 0x0000000fu, 0x0003003eu, 0x0000000eu, 0x00000010u, 0x0004003du,
    0x00000007u, 0x00000012u, 0x00000009u, 0x0004003du, 0x00000007u, 0x00000013u, 0x0000000eu,
    0x0007000cu, 0x00000007u, 0x00000014u, 0x00000001u, 0x00000044u, 0x00000012u, 0x00000013u,
    0x0003003eu, 0x00000011u, 0x00000014u, 0x0004003du, 0x00000007u, 0x00000017u, 0x00000011u,
    0x0004003du, 0x00000007u, 0x00000018u, 0x00000011u, 0x00050094u, 0x00000006u, 0x00000019u,
    0x00000017u, 0x00000018u, 0x0003003eu, 0x00000016u, 0x00000019u, 0x0004003du, 0x00000006u,
    0x0000001bu, 0x00000016u, 0x000500bau, 0x0000001du, 0x0000001eu, 0x0000001bu, 0x0000001cu,
    0x000300f7u, 0x00000021u, 0x00000000u, 0x000400fau, 0x0000001eu, 0x00000020u, 0x00000026u,
    0x000200f8u, 0x00000020u, 0x0004003du, 0x00000007u, 0x00000022u, 0x00000011u, 0x0004003du,
    0x00000006u, 0x00000023u, 0x00000016u, 0x0006000cu, 0x00000006u, 0x00000024u, 0x00000001u,
    0x00000020u, 0x00000023u, 0x0005008eu, 0x00000007u, 0x00000025u, 0x00000022u, 0x00000024u,
    0x0003003eu, 0x0000001fu, 0x00000025u, 0x000200f9u, 0x00000021u, 0x000200f8u, 0x00000026u,
    0x0003003eu, 0x0000001fu, 0x00000029u, 0x000200f9u, 0x00000021u, 0x000200f8u, 0x00000021u,
    0x0004003du, 0x00000007u, 0x0000002au, 0x0000001fu, 0x0003003eu, 0x0000001au, 0x0000002au,
    0x0003003eu, 0x0000002bu, 0x0000002fu, 0x0004003du, 0x00000007u, 0x00000033u, 0x0000001au,
    0x0004003du, 0x00000007u, 0x00000034u, 0x0000002bu, 0x00050094u, 0x00000006u, 0x00000035u,
    0x00000033u, 0x00000034u, 0x0006000cu, 0x00000006u, 0x00000036u, 0x00000001u, 0x00000004u,
    0x00000035u, 0x00050085u, 0x00000006u, 0x00000037u, 0x00000032u, 0x00000036u, 0x00050081u,
    0x00000006u, 0x00000038u, 0x00000031u, 0x00000037u, 0x0003003eu, 0x00000030u, 0x00000038u,
    0x0004003du, 0x00000006u, 0x0000003du, 0x00000030u, 0x0005008eu, 0x00000007u, 0x0000003eu,
    0x0000003cu, 0x0000003du, 0x0003003eu, 0x00000039u, 0x0000003eu, 0x0004003du, 0x00000007u,
    0x00000042u, 0x00000039u, 0x00050051u, 0x00000006u, 0x00000043u, 0x00000042u, 0x00000000u,
    0x00050051u, 0x00000006u, 0x00000044u, 0x00000042u, 0x00000001u, 0x00050051u, 0x00000006u,
    0x00000045u, 0x00000042u, 0x00000002u, 0x00070050u, 0x0000003fu, 0x00000046u, 0x00000043u,
    0x00000044u, 0x00000045u, 0x00000028u, 0x0003003eu, 0x00000041u, 0x00000046u, 0x000100fdu,
    0x00010038u,
};

// Vulkan 1.0 validated GLSL 450: private world Lambert lighting with
// current validated/normalized Firestorm sun direction in 80-byte push data.
// NOT a full world renderer; no user framebuffer or real material maps.
static constexpr uint32_t ogl_dev_world_sunlight_frag_spv[] = {
    0x07230203u, 0x00010000u, 0x0008000bu, 0x00000049u, 0x00000000u, 0x00020011u, 0x00000001u,
    0x0006000bu, 0x00000001u, 0x4c534c47u, 0x6474732eu, 0x3035342eu, 0x00000000u, 0x0003000eu,
    0x00000000u, 0x00000001u, 0x0007000fu, 0x00000004u, 0x00000004u, 0x6e69616du, 0x00000000u,
    0x0000000bu, 0x0000003fu, 0x00030010u, 0x00000004u, 0x00000007u, 0x00030003u, 0x00000002u,
    0x000001c2u, 0x00040005u, 0x00000004u, 0x6e69616du, 0x00000000u, 0x00030005u, 0x00000009u,
    0x00007864u, 0x00060005u, 0x0000000bu, 0x6c726f77u, 0x6f705f64u, 0x69746973u, 0x00006e6fu,
    0x00030005u, 0x0000000eu, 0x00007964u, 0x00050005u, 0x00000011u, 0x63736e75u, 0x64656c61u,
    0x00000000u, 0x00030005u, 0x00000016u, 0x0000326eu, 0x00040005u, 0x0000001au, 0x6d726f6eu,
    0x00006c61u, 0x00040005u, 0x0000002bu, 0x66666964u, 0x00657375u, 0x00060005u, 0x00000031u,
    0x656d6143u, 0x6e416172u, 0x6e755364u, 0x00000000u, 0x00070006u, 0x00000031u, 0x00000000u,
    0x6c726f77u, 0x6f745f64u, 0x696c635fu, 0x00000070u, 0x00070006u, 0x00000031u, 0x00000001u,
    0x5f6e7573u, 0x65726964u, 0x6f697463u, 0x0000006eu, 0x00040005u, 0x00000033u, 0x656d6163u,
    0x00006172u, 0x00050005u, 0x0000003fu, 0x5f74756fu, 0x6f6c6f63u, 0x00000072u, 0x00040047u,
    0x0000000bu, 0x0000001eu, 0x00000000u, 0x00030047u, 0x00000031u, 0x00000002u, 0x00040048u,
    0x00000031u, 0x00000000u, 0x00000005u, 0x00050048u, 0x00000031u, 0x00000000u, 0x00000007u,
    0x00000010u, 0x00050048u, 0x00000031u, 0x00000000u, 0x00000023u, 0x00000000u, 0x00050048u,
    0x00000031u, 0x00000001u, 0x00000023u, 0x00000040u, 0x00040047u, 0x0000003fu, 0x0000001eu,
    0x00000000u, 0x00020013u, 0x00000002u, 0x00030021u, 0x00000003u, 0x00000002u, 0x00030016u,
    0x00000006u, 0x00000020u, 0x00040017u, 0x00000007u, 0x00000006u, 0x00000003u, 0x00040020u,
    0x00000008u, 0x00000007u, 0x00000007u, 0x00040020u, 0x0000000au, 0x00000001u, 0x00000007u,
    0x0004003bu, 0x0000000au, 0x0000000bu, 0x00000001u, 0x00040020u, 0x00000015u, 0x00000007u,
    0x00000006u, 0x0004002bu, 0x00000006u, 0x0000001cu, 0x2b8cbcccu, 0x00020014u, 0x0000001du,
    0x0004002bu, 0x00000006u, 0x00000027u, 0x00000000u, 0x0004002bu, 0x00000006u, 0x00000028u,
    0x3f800000u, 0x0006002cu, 0x00000007u, 0x00000029u, 0x00000027u, 0x00000027u, 0x00000028u,
    0x0004002bu, 0x00000006u, 0x0000002cu, 0x3e4ccccdu, 0x0004002bu, 0x00000006u, 0x0000002du,
    0x3f4ccccdu, 0x00040017u, 0x0000002fu, 0x00000006u, 0x00000004u, 0x00040018u, 0x00000030u,
    0x0000002fu, 0x00000004u, 0x0004001eu, 0x00000031u, 0x00000030u, 0x0000002fu, 0x00040020u,
    0x00000032u, 0x00000009u, 0x00000031u, 0x0004003bu, 0x00000032u, 0x00000033u, 0x00000009u,
    0x00040015u, 0x00000034u, 0x00000020u, 0x00000001u, 0x0004002bu, 0x00000034u, 0x00000035u,
    0x00000001u, 0x00040020u, 0x00000036u, 0x00000009u, 0x0000002fu, 0x00040020u, 0x0000003eu,
    0x00000003u, 0x0000002fu, 0x0004003bu, 0x0000003eu, 0x0000003fu, 0x00000003u, 0x0004002bu,
    0x00000006u, 0x00000040u, 0x3f47ae14u, 0x0004002bu, 0x00000006u, 0x00000041u, 0x3ec7ae14u,
    0x0006002cu, 0x00000007u, 0x00000042u, 0x0000002cu, 0x00000040u, 0x00000041u, 0x00050036u,
    0x00000002u, 0x00000004u, 0x00000000u, 0x00000003u, 0x000200f8u, 0x00000005u, 0x0004003bu,
    0x00000008u, 0x00000009u, 0x00000007u, 0x0004003bu, 0x00000008u, 0x0000000eu, 0x00000007u,
    0x0004003bu, 0x00000008u, 0x00000011u, 0x00000007u, 0x0004003bu, 0x00000015u, 0x00000016u,
    0x00000007u, 0x0004003bu, 0x00000008u, 0x0000001au, 0x00000007u, 0x0004003bu, 0x00000008u,
    0x0000001fu, 0x00000007u, 0x0004003bu, 0x00000015u, 0x0000002bu, 0x00000007u, 0x0004003du,
    0x00000007u, 0x0000000cu, 0x0000000bu, 0x000400cfu, 0x00000007u, 0x0000000du, 0x0000000cu,
    0x0003003eu, 0x00000009u, 0x0000000du, 0x0004003du, 0x00000007u, 0x0000000fu, 0x0000000bu,
    0x000400d0u, 0x00000007u, 0x00000010u, 0x0000000fu, 0x0003003eu, 0x0000000eu, 0x00000010u,
    0x0004003du, 0x00000007u, 0x00000012u, 0x00000009u, 0x0004003du, 0x00000007u, 0x00000013u,
    0x0000000eu, 0x0007000cu, 0x00000007u, 0x00000014u, 0x00000001u, 0x00000044u, 0x00000012u,
    0x00000013u, 0x0003003eu, 0x00000011u, 0x00000014u, 0x0004003du, 0x00000007u, 0x00000017u,
    0x00000011u, 0x0004003du, 0x00000007u, 0x00000018u, 0x00000011u, 0x00050094u, 0x00000006u,
    0x00000019u, 0x00000017u, 0x00000018u, 0x0003003eu, 0x00000016u, 0x00000019u, 0x0004003du,
    0x00000006u, 0x0000001bu, 0x00000016u, 0x000500bau, 0x0000001du, 0x0000001eu, 0x0000001bu,
    0x0000001cu, 0x000300f7u, 0x00000021u, 0x00000000u, 0x000400fau, 0x0000001eu, 0x00000020u,
    0x00000026u, 0x000200f8u, 0x00000020u, 0x0004003du, 0x00000007u, 0x00000022u, 0x00000011u,
    0x0004003du, 0x00000006u, 0x00000023u, 0x00000016u, 0x0006000cu, 0x00000006u, 0x00000024u,
    0x00000001u, 0x00000020u, 0x00000023u, 0x0005008eu, 0x00000007u, 0x00000025u, 0x00000022u,
    0x00000024u, 0x0003003eu, 0x0000001fu, 0x00000025u, 0x000200f9u, 0x00000021u, 0x000200f8u,
    0x00000026u, 0x0003003eu, 0x0000001fu, 0x00000029u, 0x000200f9u, 0x00000021u, 0x000200f8u,
    0x00000021u, 0x0004003du, 0x00000007u, 0x0000002au, 0x0000001fu, 0x0003003eu, 0x0000001au,
    0x0000002au, 0x0004003du, 0x00000007u, 0x0000002eu, 0x0000001au, 0x00050041u, 0x00000036u,
    0x00000037u, 0x00000033u, 0x00000035u, 0x0004003du, 0x0000002fu, 0x00000038u, 0x00000037u,
    0x0008004fu, 0x00000007u, 0x00000039u, 0x00000038u, 0x00000038u, 0x00000000u, 0x00000001u,
    0x00000002u, 0x00050094u, 0x00000006u, 0x0000003au, 0x0000002eu, 0x00000039u, 0x0006000cu,
    0x00000006u, 0x0000003bu, 0x00000001u, 0x00000004u, 0x0000003au, 0x00050085u, 0x00000006u,
    0x0000003cu, 0x0000002du, 0x0000003bu, 0x00050081u, 0x00000006u, 0x0000003du, 0x0000002cu,
    0x0000003cu, 0x0003003eu, 0x0000002bu, 0x0000003du, 0x0004003du, 0x00000006u, 0x00000043u,
    0x0000002bu, 0x0005008eu, 0x00000007u, 0x00000044u, 0x00000042u, 0x00000043u, 0x00050051u,
    0x00000006u, 0x00000045u, 0x00000044u, 0x00000000u, 0x00050051u, 0x00000006u, 0x00000046u,
    0x00000044u, 0x00000001u, 0x00050051u, 0x00000006u, 0x00000047u, 0x00000044u, 0x00000002u,
    0x00070050u, 0x0000002fu, 0x00000048u, 0x00000045u, 0x00000046u, 0x00000047u, 0x00000028u,
    0x0003003eu, 0x0000003fu, 0x00000048u, 0x000100fdu, 0x00010038u,
};

// Vulkan 1.0 validated OpenSim Face RGB tint shader, private GPU.
static constexpr uint32_t ogl_dev_world_face_tint_vert_spv[] = {
    0x07230203u, 0x00010000u, 0x0008000bu, 0x00000029u, 0x00000000u, 0x00020011u, 0x00000001u,
    0x0006000bu, 0x00000001u, 0x4c534c47u, 0x6474732eu, 0x3035342eu, 0x00000000u, 0x0003000eu,
    0x00000000u, 0x00000001u, 0x000a000fu, 0x00000000u, 0x00000004u, 0x6e69616du, 0x00000000u,
    0x0000000du, 0x00000019u, 0x00000024u, 0x00000026u, 0x00000027u, 0x00030003u, 0x00000002u,
    0x000001c2u, 0x00040005u, 0x00000004u, 0x6e69616du, 0x00000000u, 0x00060005u, 0x0000000bu,
    0x505f6c67u, 0x65567265u, 0x78657472u, 0x00000000u, 0x00060006u, 0x0000000bu, 0x00000000u,
    0x505f6c67u, 0x7469736fu, 0x006e6f69u, 0x00070006u, 0x0000000bu, 0x00000001u, 0x505f6c67u,
    0x746e696fu, 0x657a6953u, 0x00000000u, 0x00070006u, 0x0000000bu, 0x00000002u, 0x435f6c67u,
    0x4470696cu, 0x61747369u, 0x0065636eu, 0x00070006u, 0x0000000bu, 0x00000003u, 0x435f6c67u,
    0x446c6c75u, 0x61747369u, 0x0065636eu, 0x00030005u, 0x0000000du, 0x00000000u, 0x00040005u,
    0x00000011u, 0x656d6143u, 0x00006172u, 0x00070006u, 0x00000011u, 0x00000000u, 0x6c726f77u,
    0x6f745f64u, 0x696c635fu, 0x00000070u, 0x00070006u, 0x00000011u, 0x00000001u, 0x5f6e7573u,
    0x65726964u, 0x6f697463u, 0x0000006eu, 0x00040005u, 0x00000013u, 0x656d6163u, 0x00006172u,
    0x00080005u, 0x00000019u, 0x6c726f77u, 0x6f705f64u, 0x69746973u, 0x695f6e6fu, 0x7475706eu,
    0x00000000u, 0x00060005u, 0x00000024u, 0x6c726f77u, 0x6f705f64u, 0x69746973u, 0x00006e6fu,
    0x00050005u, 0x00000026u, 0x65636166u, 0x6267725fu, 0x00000000u, 0x00060005u, 0x00000027u,
    0x65636166u, 0x6267725fu, 0x706e695fu, 0x00007475u, 0x00030047u, 0x0000000bu, 0x00000002u,
    0x00050048u, 0x0000000bu, 0x00000000u, 0x0000000bu, 0x00000000u, 0x00050048u, 0x0000000bu,
    0x00000001u, 0x0000000bu, 0x00000001u, 0x00050048u, 0x0000000bu, 0x00000002u, 0x0000000bu,
    0x00000003u, 0x00050048u, 0x0000000bu, 0x00000003u, 0x0000000bu, 0x00000004u, 0x00030047u,
    0x00000011u, 0x00000002u, 0x00040048u, 0x00000011u, 0x00000000u, 0x00000005u, 0x00050048u,
    0x00000011u, 0x00000000u, 0x00000007u, 0x00000010u, 0x00050048u, 0x00000011u, 0x00000000u,
    0x00000023u, 0x00000000u, 0x00050048u, 0x00000011u, 0x00000001u, 0x00000023u, 0x00000040u,
    0x00040047u, 0x00000019u, 0x0000001eu, 0x00000000u, 0x00040047u, 0x00000024u, 0x0000001eu,
    0x00000000u, 0x00040047u, 0x00000026u, 0x0000001eu, 0x00000001u, 0x00040047u, 0x00000027u,
    0x0000001eu, 0x00000001u, 0x00020013u, 0x00000002u, 0x00030021u, 0x00000003u, 0x00000002u,
    0x00030016u, 0x00000006u, 0x00000020u, 0x00040017u, 0x00000007u, 0x00000006u, 0x00000004u,
    0x00040015u, 0x00000008u, 0x00000020u, 0x00000000u, 0x0004002bu, 0x00000008u, 0x00000009u,
    0x00000001u, 0x0004001cu, 0x0000000au, 0x00000006u, 0x00000009u, 0x0006001eu, 0x0000000bu,
    0x00000007u, 0x00000006u, 0x0000000au, 0x0000000au, 0x00040020u, 0x0000000cu, 0x00000003u,
    0x0000000bu, 0x0004003bu, 0x0000000cu, 0x0000000du, 0x00000003u, 0x00040015u, 0x0000000eu,
    0x00000020u, 0x00000001u, 0x0004002bu, 0x0000000eu, 0x0000000fu, 0x00000000u, 0x00040018u,
    0x00000010u, 0x00000007u, 0x00000004u, 0x0004001eu, 0x00000011u, 0x00000010u, 0x00000007u,
    0x00040020u, 0x00000012u, 0x00000009u, 0x00000011u, 0x0004003bu, 0x00000012u, 0x00000013u,
    0x00000009u, 0x00040020u, 0x00000014u, 0x00000009u, 0x00000010u, 0x00040017u, 0x00000017u,
    0x00000006u, 0x00000003u, 0x00040020u, 0x00000018u, 0x00000001u, 0x00000017u, 0x0004003bu,
    0x00000018u, 0x00000019u, 0x00000001u, 0x0004002bu, 0x00000006u, 0x0000001bu, 0x3f800000u,
    0x00040020u, 0x00000021u, 0x00000003u, 0x00000007u, 0x00040020u, 0x00000023u, 0x00000003u,
    0x00000017u, 0x0004003bu, 0x00000023u, 0x00000024u, 0x00000003u, 0x0004003bu, 0x00000023u,
    0x00000026u, 0x00000003u, 0x0004003bu, 0x00000018u, 0x00000027u, 0x00000001u, 0x00050036u,
    0x00000002u, 0x00000004u, 0x00000000u, 0x00000003u, 0x000200f8u, 0x00000005u, 0x00050041u,
    0x00000014u, 0x00000015u, 0x00000013u, 0x0000000fu, 0x0004003du, 0x00000010u, 0x00000016u,
    0x00000015u, 0x0004003du, 0x00000017u, 0x0000001au, 0x00000019u, 0x00050051u, 0x00000006u,
    0x0000001cu, 0x0000001au, 0x00000000u, 0x00050051u, 0x00000006u, 0x0000001du, 0x0000001au,
    0x00000001u, 0x00050051u, 0x00000006u, 0x0000001eu, 0x0000001au, 0x00000002u, 0x00070050u,
    0x00000007u, 0x0000001fu, 0x0000001cu, 0x0000001du, 0x0000001eu, 0x0000001bu, 0x00050091u,
    0x00000007u, 0x00000020u, 0x00000016u, 0x0000001fu, 0x00050041u, 0x00000021u, 0x00000022u,
    0x0000000du, 0x0000000fu, 0x0003003eu, 0x00000022u, 0x00000020u, 0x0004003du, 0x00000017u,
    0x00000025u, 0x00000019u, 0x0003003eu, 0x00000024u, 0x00000025u, 0x0004003du, 0x00000017u,
    0x00000028u, 0x00000027u, 0x0003003eu, 0x00000026u, 0x00000028u, 0x000100fdu, 0x00010038u,
};

// Vulkan 1.0 validated OpenSim Face RGB tint shader, private GPU.
static constexpr uint32_t ogl_dev_world_face_tint_frag_spv[] = {
    0x07230203u, 0x00010000u, 0x0008000bu, 0x0000004au, 0x00000000u, 0x00020011u, 0x00000001u,
    0x0006000bu, 0x00000001u, 0x4c534c47u, 0x6474732eu, 0x3035342eu, 0x00000000u, 0x0003000eu,
    0x00000000u, 0x00000001u, 0x0008000fu, 0x00000004u, 0x00000004u, 0x6e69616du, 0x00000000u,
    0x0000000bu, 0x0000003eu, 0x0000003fu, 0x00030010u, 0x00000004u, 0x00000007u, 0x00030003u,
    0x00000002u, 0x000001c2u, 0x00040005u, 0x00000004u, 0x6e69616du, 0x00000000u, 0x00030005u,
    0x00000009u, 0x00007864u, 0x00060005u, 0x0000000bu, 0x6c726f77u, 0x6f705f64u, 0x69746973u,
    0x00006e6fu, 0x00030005u, 0x0000000eu, 0x00007964u, 0x00030005u, 0x00000011u, 0x0000006eu,
    0x00030005u, 0x00000016u, 0x0000326eu, 0x00040005u, 0x0000002au, 0x6867696cu, 0x00000074u,
    0x00040005u, 0x00000030u, 0x656d6143u, 0x00006172u, 0x00070006u, 0x00000030u, 0x00000000u,
    0x6c726f77u, 0x6f745f64u, 0x696c635fu, 0x00000070u, 0x00070006u, 0x00000030u, 0x00000001u,
    0x5f6e7573u, 0x65726964u, 0x6f697463u, 0x0000006eu, 0x00040005u, 0x00000032u, 0x656d6163u,
    0x00006172u, 0x00060005u, 0x0000003eu, 0x7074756fu, 0x635f7475u, 0x726f6c6fu, 0x00000000u,
    0x00050005u, 0x0000003fu, 0x65636166u, 0x6267725fu, 0x00000000u, 0x00040047u, 0x0000000bu,
    0x0000001eu, 0x00000000u, 0x00030047u, 0x00000030u, 0x00000002u, 0x00040048u, 0x00000030u,
    0x00000000u, 0x00000005u, 0x00050048u, 0x00000030u, 0x00000000u, 0x00000007u, 0x00000010u,
    0x00050048u, 0x00000030u, 0x00000000u, 0x00000023u, 0x00000000u, 0x00050048u, 0x00000030u,
    0x00000001u, 0x00000023u, 0x00000040u, 0x00040047u, 0x0000003eu, 0x0000001eu, 0x00000000u,
    0x00040047u, 0x0000003fu, 0x0000001eu, 0x00000001u, 0x00020013u, 0x00000002u, 0x00030021u,
    0x00000003u, 0x00000002u, 0x00030016u, 0x00000006u, 0x00000020u, 0x00040017u, 0x00000007u,
    0x00000006u, 0x00000003u, 0x00040020u, 0x00000008u, 0x00000007u, 0x00000007u, 0x00040020u,
    0x0000000au, 0x00000001u, 0x00000007u, 0x0004003bu, 0x0000000au, 0x0000000bu, 0x00000001u,
    0x00040020u, 0x00000015u, 0x00000007u, 0x00000006u, 0x0004002bu, 0x00000006u, 0x0000001bu,
    0x2b8cbcccu, 0x00020014u, 0x0000001cu, 0x0004002bu, 0x00000006u, 0x00000026u, 0x00000000u,
    0x0004002bu, 0x00000006u, 0x00000027u, 0x3f800000u, 0x0006002cu, 0x00000007u, 0x00000028u,
    0x00000026u, 0x00000026u, 0x00000027u, 0x0004002bu, 0x00000006u, 0x0000002bu, 0x3e4ccccdu,
    0x0004002bu, 0x00000006u, 0x0000002cu, 0x3f4ccccdu, 0x00040017u, 0x0000002eu, 0x00000006u,
    0x00000004u, 0x00040018u, 0x0000002fu, 0x0000002eu, 0x00000004u, 0x0004001eu, 0x00000030u,
    0x0000002fu, 0x0000002eu, 0x00040020u, 0x00000031u, 0x00000009u, 0x00000030u, 0x0004003bu,
    0x00000031u, 0x00000032u, 0x00000009u, 0x00040015u, 0x00000033u, 0x00000020u, 0x00000001u,
    0x0004002bu, 0x00000033u, 0x00000034u, 0x00000001u, 0x00040020u, 0x00000035u, 0x00000009u,
    0x0000002eu, 0x00040020u, 0x0000003du, 0x00000003u, 0x0000002eu, 0x0004003bu, 0x0000003du,
    0x0000003eu, 0x00000003u, 0x0004003bu, 0x0000000au, 0x0000003fu, 0x00000001u, 0x0006002cu,
    0x00000007u, 0x00000041u, 0x00000026u, 0x00000026u, 0x00000026u, 0x0006002cu, 0x00000007u,
    0x00000042u, 0x00000027u, 0x00000027u, 0x00000027u, 0x00050036u, 0x00000002u, 0x00000004u,
    0x00000000u, 0x00000003u, 0x000200f8u, 0x00000005u, 0x0004003bu, 0x00000008u, 0x00000009u,
    0x00000007u, 0x0004003bu, 0x00000008u, 0x0000000eu, 0x00000007u, 0x0004003bu, 0x00000008u,
    0x00000011u, 0x00000007u, 0x0004003bu, 0x00000015u, 0x00000016u, 0x00000007u, 0x0004003bu,
    0x00000008u, 0x0000001eu, 0x00000007u, 0x0004003bu, 0x00000015u, 0x0000002au, 0x00000007u,
    0x0004003du, 0x00000007u, 0x0000000cu, 0x0000000bu, 0x000400cfu, 0x00000007u, 0x0000000du,
    0x0000000cu, 0x0003003eu, 0x00000009u, 0x0000000du, 0x0004003du, 0x00000007u, 0x0000000fu,
    0x0000000bu, 0x000400d0u, 0x00000007u, 0x00000010u, 0x0000000fu, 0x0003003eu, 0x0000000eu,
    0x00000010u, 0x0004003du, 0x00000007u, 0x00000012u, 0x00000009u, 0x0004003du, 0x00000007u,
    0x00000013u, 0x0000000eu, 0x0007000cu, 0x00000007u, 0x00000014u, 0x00000001u, 0x00000044u,
    0x00000012u, 0x00000013u, 0x0003003eu, 0x00000011u, 0x00000014u, 0x0004003du, 0x00000007u,
    0x00000017u, 0x00000011u, 0x0004003du, 0x00000007u, 0x00000018u, 0x00000011u, 0x00050094u,
    0x00000006u, 0x00000019u, 0x00000017u, 0x00000018u, 0x0003003eu, 0x00000016u, 0x00000019u,
    0x0004003du, 0x00000006u, 0x0000001au, 0x00000016u, 0x000500bau, 0x0000001cu, 0x0000001du,
    0x0000001au, 0x0000001bu, 0x000300f7u, 0x00000020u, 0x00000000u, 0x000400fau, 0x0000001du,
    0x0000001fu, 0x00000025u, 0x000200f8u, 0x0000001fu, 0x0004003du, 0x00000007u, 0x00000021u,
    0x00000011u, 0x0004003du, 0x00000006u, 0x00000022u, 0x00000016u, 0x0006000cu, 0x00000006u,
    0x00000023u, 0x00000001u, 0x00000020u, 0x00000022u, 0x0005008eu, 0x00000007u, 0x00000024u,
    0x00000021u, 0x00000023u, 0x0003003eu, 0x0000001eu, 0x00000024u, 0x000200f9u, 0x00000020u,
    0x000200f8u, 0x00000025u, 0x0003003eu, 0x0000001eu, 0x00000028u, 0x000200f9u, 0x00000020u,
    0x000200f8u, 0x00000020u, 0x0004003du, 0x00000007u, 0x00000029u, 0x0000001eu, 0x0003003eu,
    0x00000011u, 0x00000029u, 0x0004003du, 0x00000007u, 0x0000002du, 0x00000011u, 0x00050041u,
    0x00000035u, 0x00000036u, 0x00000032u, 0x00000034u, 0x0004003du, 0x0000002eu, 0x00000037u,
    0x00000036u, 0x0008004fu, 0x00000007u, 0x00000038u, 0x00000037u, 0x00000037u, 0x00000000u,
    0x00000001u, 0x00000002u, 0x00050094u, 0x00000006u, 0x00000039u, 0x0000002du, 0x00000038u,
    0x0006000cu, 0x00000006u, 0x0000003au, 0x00000001u, 0x00000004u, 0x00000039u, 0x00050085u,
    0x00000006u, 0x0000003bu, 0x0000002cu, 0x0000003au, 0x00050081u, 0x00000006u, 0x0000003cu,
    0x0000002bu, 0x0000003bu, 0x0003003eu, 0x0000002au, 0x0000003cu, 0x0004003du, 0x00000007u,
    0x00000040u, 0x0000003fu, 0x0008000cu, 0x00000007u, 0x00000043u, 0x00000001u, 0x0000002bu,
    0x00000040u, 0x00000041u, 0x00000042u, 0x0004003du, 0x00000006u, 0x00000044u, 0x0000002au,
    0x0005008eu, 0x00000007u, 0x00000045u, 0x00000043u, 0x00000044u, 0x00050051u, 0x00000006u,
    0x00000046u, 0x00000045u, 0x00000000u, 0x00050051u, 0x00000006u, 0x00000047u, 0x00000045u,
    0x00000001u, 0x00050051u, 0x00000006u, 0x00000048u, 0x00000045u, 0x00000002u, 0x00070050u,
    0x0000002eu, 0x00000049u, 0x00000046u, 0x00000047u, 0x00000048u, 0x00000027u, 0x0003003eu,
    0x0000003eu, 0x00000049u, 0x000100fdu, 0x00010038u,
};

// Vulkan 1.0 verified UV CHECKER test; NOT an uploaded OpenSim texture.
static constexpr uint32_t ogl_dev_world_uv_checker_vert_spv[] = {
    0x07230203u, 0x00010000u, 0x0008000bu, 0x0000002fu, 0x00000000u, 0x00020011u, 0x00000001u,
    0x0006000bu, 0x00000001u, 0x4c534c47u, 0x6474732eu, 0x3035342eu, 0x00000000u, 0x0003000eu,
    0x00000000u, 0x00000001u, 0x000c000fu, 0x00000000u, 0x00000004u, 0x6e69616du, 0x00000000u,
    0x0000000du, 0x00000019u, 0x00000024u, 0x00000026u, 0x00000027u, 0x0000002bu, 0x0000002du,
    0x00030003u, 0x00000002u, 0x000001c2u, 0x00040005u, 0x00000004u, 0x6e69616du, 0x00000000u,
    0x00060005u, 0x0000000bu, 0x505f6c67u, 0x65567265u, 0x78657472u, 0x00000000u, 0x00060006u,
    0x0000000bu, 0x00000000u, 0x505f6c67u, 0x7469736fu, 0x006e6f69u, 0x00070006u, 0x0000000bu,
    0x00000001u, 0x505f6c67u, 0x746e696fu, 0x657a6953u, 0x00000000u, 0x00070006u, 0x0000000bu,
    0x00000002u, 0x435f6c67u, 0x4470696cu, 0x61747369u, 0x0065636eu, 0x00070006u, 0x0000000bu,
    0x00000003u, 0x435f6c67u, 0x446c6c75u, 0x61747369u, 0x0065636eu, 0x00030005u, 0x0000000du,
    0x00000000u, 0x00040005u, 0x00000011u, 0x656d6143u, 0x00006172u, 0x00070006u, 0x00000011u,
    0x00000000u, 0x6c726f77u, 0x6f745f64u, 0x696c635fu, 0x00000070u, 0x00070006u, 0x00000011u,
    0x00000001u, 0x5f6e7573u, 0x65726964u, 0x6f697463u, 0x0000006eu, 0x00040005u, 0x00000013u,
    0x656d6163u, 0x00006172u, 0x00080005u, 0x00000019u, 0x6c726f77u, 0x6f705f64u, 0x69746973u,
    0x695f6e6fu, 0x7475706eu, 0x00000000u, 0x00060005u, 0x00000024u, 0x6c726f77u, 0x6f705f64u,
    0x69746973u, 0x00006e6fu, 0x00050005u, 0x00000026u, 0x65636166u, 0x6267725fu, 0x00000000u,
    0x00060005u, 0x00000027u, 0x65636166u, 0x6267725fu, 0x706e695fu, 0x00007475u, 0x00040005u,
    0x0000002bu, 0x65636166u, 0x0076755fu, 0x00060005u, 0x0000002du, 0x65636166u, 0x5f76755fu,
    0x75706e69u, 0x00000074u, 0x00030047u, 0x0000000bu, 0x00000002u, 0x00050048u, 0x0000000bu,
    0x00000000u, 0x0000000bu, 0x00000000u, 0x00050048u, 0x0000000bu, 0x00000001u, 0x0000000bu,
    0x00000001u, 0x00050048u, 0x0000000bu, 0x00000002u, 0x0000000bu, 0x00000003u, 0x00050048u,
    0x0000000bu, 0x00000003u, 0x0000000bu, 0x00000004u, 0x00030047u, 0x00000011u, 0x00000002u,
    0x00040048u, 0x00000011u, 0x00000000u, 0x00000005u, 0x00050048u, 0x00000011u, 0x00000000u,
    0x00000007u, 0x00000010u, 0x00050048u, 0x00000011u, 0x00000000u, 0x00000023u, 0x00000000u,
    0x00050048u, 0x00000011u, 0x00000001u, 0x00000023u, 0x00000040u, 0x00040047u, 0x00000019u,
    0x0000001eu, 0x00000000u, 0x00040047u, 0x00000024u, 0x0000001eu, 0x00000000u, 0x00040047u,
    0x00000026u, 0x0000001eu, 0x00000001u, 0x00040047u, 0x00000027u, 0x0000001eu, 0x00000001u,
    0x00040047u, 0x0000002bu, 0x0000001eu, 0x00000002u, 0x00040047u, 0x0000002du, 0x0000001eu,
    0x00000002u, 0x00020013u, 0x00000002u, 0x00030021u, 0x00000003u, 0x00000002u, 0x00030016u,
    0x00000006u, 0x00000020u, 0x00040017u, 0x00000007u, 0x00000006u, 0x00000004u, 0x00040015u,
    0x00000008u, 0x00000020u, 0x00000000u, 0x0004002bu, 0x00000008u, 0x00000009u, 0x00000001u,
    0x0004001cu, 0x0000000au, 0x00000006u, 0x00000009u, 0x0006001eu, 0x0000000bu, 0x00000007u,
    0x00000006u, 0x0000000au, 0x0000000au, 0x00040020u, 0x0000000cu, 0x00000003u, 0x0000000bu,
    0x0004003bu, 0x0000000cu, 0x0000000du, 0x00000003u, 0x00040015u, 0x0000000eu, 0x00000020u,
    0x00000001u, 0x0004002bu, 0x0000000eu, 0x0000000fu, 0x00000000u, 0x00040018u, 0x00000010u,
    0x00000007u, 0x00000004u, 0x0004001eu, 0x00000011u, 0x00000010u, 0x00000007u, 0x00040020u,
    0x00000012u, 0x00000009u, 0x00000011u, 0x0004003bu, 0x00000012u, 0x00000013u, 0x00000009u,
    0x00040020u, 0x00000014u, 0x00000009u, 0x00000010u, 0x00040017u, 0x00000017u, 0x00000006u,
    0x00000003u, 0x00040020u, 0x00000018u, 0x00000001u, 0x00000017u, 0x0004003bu, 0x00000018u,
    0x00000019u, 0x00000001u, 0x0004002bu, 0x00000006u, 0x0000001bu, 0x3f800000u, 0x00040020u,
    0x00000021u, 0x00000003u, 0x00000007u, 0x00040020u, 0x00000023u, 0x00000003u, 0x00000017u,
    0x0004003bu, 0x00000023u, 0x00000024u, 0x00000003u, 0x0004003bu, 0x00000023u, 0x00000026u,
    0x00000003u, 0x0004003bu, 0x00000018u, 0x00000027u, 0x00000001u, 0x00040017u, 0x00000029u,
    0x00000006u, 0x00000002u, 0x00040020u, 0x0000002au, 0x00000003u, 0x00000029u, 0x0004003bu,
    0x0000002au, 0x0000002bu, 0x00000003u, 0x00040020u, 0x0000002cu, 0x00000001u, 0x00000029u,
    0x0004003bu, 0x0000002cu, 0x0000002du, 0x00000001u, 0x00050036u, 0x00000002u, 0x00000004u,
    0x00000000u, 0x00000003u, 0x000200f8u, 0x00000005u, 0x00050041u, 0x00000014u, 0x00000015u,
    0x00000013u, 0x0000000fu, 0x0004003du, 0x00000010u, 0x00000016u, 0x00000015u, 0x0004003du,
    0x00000017u, 0x0000001au, 0x00000019u, 0x00050051u, 0x00000006u, 0x0000001cu, 0x0000001au,
    0x00000000u, 0x00050051u, 0x00000006u, 0x0000001du, 0x0000001au, 0x00000001u, 0x00050051u,
    0x00000006u, 0x0000001eu, 0x0000001au, 0x00000002u, 0x00070050u, 0x00000007u, 0x0000001fu,
    0x0000001cu, 0x0000001du, 0x0000001eu, 0x0000001bu, 0x00050091u, 0x00000007u, 0x00000020u,
    0x00000016u, 0x0000001fu, 0x00050041u, 0x00000021u, 0x00000022u, 0x0000000du, 0x0000000fu,
    0x0003003eu, 0x00000022u, 0x00000020u, 0x0004003du, 0x00000017u, 0x00000025u, 0x00000019u,
    0x0003003eu, 0x00000024u, 0x00000025u, 0x0004003du, 0x00000017u, 0x00000028u, 0x00000027u,
    0x0003003eu, 0x00000026u, 0x00000028u, 0x0004003du, 0x00000029u, 0x0000002eu, 0x0000002du,
    0x0003003eu, 0x0000002bu, 0x0000002eu, 0x000100fdu, 0x00010038u,
};

// Vulkan 1.0 verified UV CHECKER test; NOT an uploaded OpenSim texture.
static constexpr uint32_t ogl_dev_world_uv_checker_frag_spv[] = {
    0x07230203u, 0x00010000u, 0x0008000bu, 0x00000068u, 0x00000000u, 0x00020011u, 0x00000001u,
    0x0006000bu, 0x00000001u, 0x4c534c47u, 0x6474732eu, 0x3035342eu, 0x00000000u, 0x0003000eu,
    0x00000000u, 0x00000001u, 0x0009000fu, 0x00000004u, 0x00000004u, 0x6e69616du, 0x00000000u,
    0x0000000bu, 0x0000003du, 0x0000005au, 0x0000005bu, 0x00030010u, 0x00000004u, 0x00000007u,
    0x00030003u, 0x00000002u, 0x000001c2u, 0x00040005u, 0x00000004u, 0x6e69616du, 0x00000000u,
    0x00030005u, 0x00000009u, 0x0000006eu, 0x00060005u, 0x0000000bu, 0x6c726f77u, 0x6f705f64u,
    0x69746973u, 0x00006e6fu, 0x00030005u, 0x00000012u, 0x0000326eu, 0x00040005u, 0x00000026u,
    0x6867696cu, 0x00000074u, 0x00040005u, 0x0000002cu, 0x656d6143u, 0x00006172u, 0x00070006u,
    0x0000002cu, 0x00000000u, 0x6c726f77u, 0x6f745f64u, 0x696c635fu, 0x00000070u, 0x00070006u,
    0x0000002cu, 0x00000001u, 0x5f6e7573u, 0x65726964u, 0x6f697463u, 0x0000006eu, 0x00040005u,
    0x0000002eu, 0x656d6163u, 0x00006172u, 0x00040005u, 0x0000003bu, 0x656c6974u, 0x00000000u,
    0x00040005u, 0x0000003du, 0x65636166u, 0x0076755fu, 0x00040005u, 0x00000043u, 0x63656863u,
    0x0072656bu, 0x00050005u, 0x0000004eu, 0x63656863u, 0x5f72656bu, 0x00626772u, 0x00060005u,
    0x0000005au, 0x7074756fu, 0x635f7475u, 0x726f6c6fu, 0x00000000u, 0x00050005u, 0x0000005bu,
    0x65636166u, 0x6267725fu, 0x00000000u, 0x00040047u, 0x0000000bu, 0x0000001eu, 0x00000000u,
    0x00030047u, 0x0000002cu, 0x00000002u, 0x00040048u, 0x0000002cu, 0x00000000u, 0x00000005u,
    0x00050048u, 0x0000002cu, 0x00000000u, 0x00000007u, 0x00000010u, 0x00050048u, 0x0000002cu,
    0x00000000u, 0x00000023u, 0x00000000u, 0x00050048u, 0x0000002cu, 0x00000001u, 0x00000023u,
    0x00000040u, 0x00040047u, 0x0000003du, 0x0000001eu, 0x00000002u, 0x00040047u, 0x0000005au,
    0x0000001eu, 0x00000000u, 0x00040047u, 0x0000005bu, 0x0000001eu, 0x00000001u, 0x00020013u,
    0x00000002u, 0x00030021u, 0x00000003u, 0x00000002u, 0x00030016u, 0x00000006u, 0x00000020u,
    0x00040017u, 0x00000007u, 0x00000006u, 0x00000003u, 0x00040020u, 0x00000008u, 0x00000007u,
    0x00000007u, 0x00040020u, 0x0000000au, 0x00000001u, 0x00000007u, 0x0004003bu, 0x0000000au,
    0x0000000bu, 0x00000001u, 0x00040020u, 0x00000011u, 0x00000007u, 0x00000006u, 0x0004002bu,
    0x00000006u, 0x00000017u, 0x2b8cbcccu, 0x00020014u, 0x00000018u, 0x0004002bu, 0x00000006u,
    0x00000022u, 0x00000000u, 0x0004002bu, 0x00000006u, 0x00000023u, 0x3f800000u, 0x0006002cu,
    0x00000007u, 0x00000024u, 0x00000022u, 0x00000022u, 0x00000023u, 0x0004002bu, 0x00000006u,
    0x00000027u, 0x3e4ccccdu, 0x0004002bu, 0x00000006u, 0x00000028u, 0x3f4ccccdu, 0x00040017u,
    0x0000002au, 0x00000006u, 0x00000004u, 0x00040018u, 0x0000002bu, 0x0000002au, 0x00000004u,
    0x0004001eu, 0x0000002cu, 0x0000002bu, 0x0000002au, 0x00040020u, 0x0000002du, 0x00000009u,
    0x0000002cu, 0x0004003bu, 0x0000002du, 0x0000002eu, 0x00000009u, 0x00040015u, 0x0000002fu,
    0x00000020u, 0x00000001u, 0x0004002bu, 0x0000002fu, 0x00000030u, 0x00000001u, 0x00040020u,
    0x00000031u, 0x00000009u, 0x0000002au, 0x00040017u, 0x00000039u, 0x00000006u, 0x00000002u,
    0x00040020u, 0x0000003au, 0x00000007u, 0x00000039u, 0x00040020u, 0x0000003cu, 0x00000001u,
    0x00000039u, 0x0004003bu, 0x0000003cu, 0x0000003du, 0x00000001u, 0x0004002bu, 0x00000006u,
    0x00000040u, 0x40800000u, 0x00040015u, 0x00000044u, 0x00000020u, 0x00000000u, 0x0004002bu,
    0x00000044u, 0x00000045u, 0x00000000u, 0x0004002bu, 0x00000044u, 0x00000048u, 0x00000001u,
    0x0004002bu, 0x00000006u, 0x0000004cu, 0x40000000u, 0x0004002bu, 0x00000006u, 0x0000004fu,
    0x3f70a3d7u, 0x0004002bu, 0x00000006u, 0x00000050u, 0x3df5c28fu, 0x0004002bu, 0x00000006u,
    0x00000051u, 0x3db851ecu, 0x0006002cu, 0x00000007u, 0x00000052u, 0x0000004fu, 0x00000050u,
    0x00000051u, 0x0004002bu, 0x00000006u, 0x00000053u, 0x3f6b851fu, 0x0004002bu, 0x00000006u,
    0x00000054u, 0x3e23d70au, 0x0006002cu, 0x00000007u, 0x00000055u, 0x00000050u, 0x00000053u,
    0x00000054u, 0x00040020u, 0x00000059u, 0x00000003u, 0x0000002au, 0x0004003bu, 0x00000059u,
    0x0000005au, 0x00000003u, 0x0004003bu, 0x0000000au, 0x0000005bu, 0x00000001u, 0x0006002cu,
    0x00000007u, 0x0000005du, 0x00000022u, 0x00000022u, 0x00000022u, 0x0006002cu, 0x00000007u,
    0x0000005eu, 0x00000023u, 0x00000023u, 0x00000023u, 0x00050036u, 0x00000002u, 0x00000004u,
    0x00000000u, 0x00000003u, 0x000200f8u, 0x00000005u, 0x0004003bu, 0x00000008u, 0x00000009u,
    0x00000007u, 0x0004003bu, 0x00000011u, 0x00000012u, 0x00000007u, 0x0004003bu, 0x00000008u,
    0x0000001au, 0x00000007u, 0x0004003bu, 0x00000011u, 0x00000026u, 0x00000007u, 0x0004003bu,
    0x0000003au, 0x0000003bu, 0x00000007u, 0x0004003bu, 0x00000011u, 0x00000043u, 0x00000007u,
    0x0004003bu, 0x00000008u, 0x0000004eu, 0x00000007u, 0x0004003du, 0x00000007u, 0x0000000cu,
    0x0000000bu, 0x000400cfu, 0x00000007u, 0x0000000du, 0x0000000cu, 0x0004003du, 0x00000007u,
    0x0000000eu, 0x0000000bu, 0x000400d0u, 0x00000007u, 0x0000000fu, 0x0000000eu, 0x0007000cu,
    0x00000007u, 0x00000010u, 0x00000001u, 0x00000044u, 0x0000000du, 0x0000000fu, 0x0003003eu,
    0x00000009u, 0x00000010u, 0x0004003du, 0x00000007u, 0x00000013u, 0x00000009u, 0x0004003du,
    0x00000007u, 0x00000014u, 0x00000009u, 0x00050094u, 0x00000006u, 0x00000015u, 0x00000013u,
    0x00000014u, 0x0003003eu, 0x00000012u, 0x00000015u, 0x0004003du, 0x00000006u, 0x00000016u,
    0x00000012u, 0x000500bau, 0x00000018u, 0x00000019u, 0x00000016u, 0x00000017u, 0x000300f7u,
    0x0000001cu, 0x00000000u, 0x000400fau, 0x00000019u, 0x0000001bu, 0x00000021u, 0x000200f8u,
    0x0000001bu, 0x0004003du, 0x00000007u, 0x0000001du, 0x00000009u, 0x0004003du, 0x00000006u,
    0x0000001eu, 0x00000012u, 0x0006000cu, 0x00000006u, 0x0000001fu, 0x00000001u, 0x00000020u,
    0x0000001eu, 0x0005008eu, 0x00000007u, 0x00000020u, 0x0000001du, 0x0000001fu, 0x0003003eu,
    0x0000001au, 0x00000020u, 0x000200f9u, 0x0000001cu, 0x000200f8u, 0x00000021u, 0x0003003eu,
    0x0000001au, 0x00000024u, 0x000200f9u, 0x0000001cu, 0x000200f8u, 0x0000001cu, 0x0004003du,
    0x00000007u, 0x00000025u, 0x0000001au, 0x0003003eu, 0x00000009u, 0x00000025u, 0x0004003du,
    0x00000007u, 0x00000029u, 0x00000009u, 0x00050041u, 0x00000031u, 0x00000032u, 0x0000002eu,
    0x00000030u, 0x0004003du, 0x0000002au, 0x00000033u, 0x00000032u, 0x0008004fu, 0x00000007u,
    0x00000034u, 0x00000033u, 0x00000033u, 0x00000000u, 0x00000001u, 0x00000002u, 0x00050094u,
    0x00000006u, 0x00000035u, 0x00000029u, 0x00000034u, 0x0006000cu, 0x00000006u, 0x00000036u,
    0x00000001u, 0x00000004u, 0x00000035u, 0x00050085u, 0x00000006u, 0x00000037u, 0x00000028u,
    0x00000036u, 0x00050081u, 0x00000006u, 0x00000038u, 0x00000027u, 0x00000037u, 0x0003003eu,
    0x00000026u, 0x00000038u, 0x0004003du, 0x00000039u, 0x0000003eu, 0x0000003du, 0x0006000cu,
    0x00000039u, 0x0000003fu, 0x00000001u, 0x0000000au, 0x0000003eu, 0x0005008eu, 0x00000039u,
    0x00000041u, 0x0000003fu, 0x00000040u, 0x0006000cu, 0x00000039u, 0x00000042u, 0x00000001u,
    0x00000008u, 0x00000041u, 0x0003003eu, 0x0000003bu, 0x00000042u, 0x00050041u, 0x00000011u,
    0x00000046u, 0x0000003bu, 0x00000045u, 0x0004003du, 0x00000006u, 0x00000047u, 0x00000046u,
    0x00050041u, 0x00000011u, 0x00000049u, 0x0000003bu, 0x00000048u, 0x0004003du, 0x00000006u,
    0x0000004au, 0x00000049u, 0x00050081u, 0x00000006u, 0x0000004bu, 0x00000047u, 0x0000004au,
    0x0005008du, 0x00000006u, 0x0000004du, 0x0000004bu, 0x0000004cu, 0x0003003eu, 0x00000043u,
    0x0000004du, 0x0004003du, 0x00000006u, 0x00000056u, 0x00000043u, 0x00060050u, 0x00000007u,
    0x00000057u, 0x00000056u, 0x00000056u, 0x00000056u, 0x0008000cu, 0x00000007u, 0x00000058u,
    0x00000001u, 0x0000002eu, 0x00000052u, 0x00000055u, 0x00000057u, 0x0003003eu, 0x0000004eu,
    0x00000058u, 0x0004003du, 0x00000007u, 0x0000005cu, 0x0000005bu, 0x0008000cu, 0x00000007u,
    0x0000005fu, 0x00000001u, 0x0000002bu, 0x0000005cu, 0x0000005du, 0x0000005eu, 0x0004003du,
    0x00000007u, 0x00000060u, 0x0000004eu, 0x00050085u, 0x00000007u, 0x00000061u, 0x0000005fu,
    0x00000060u, 0x0004003du, 0x00000006u, 0x00000062u, 0x00000026u, 0x0005008eu, 0x00000007u,
    0x00000063u, 0x00000061u, 0x00000062u, 0x00050051u, 0x00000006u, 0x00000064u, 0x00000063u,
    0x00000000u, 0x00050051u, 0x00000006u, 0x00000065u, 0x00000063u, 0x00000001u, 0x00050051u,
    0x00000006u, 0x00000066u, 0x00000063u, 0x00000002u, 0x00070050u, 0x0000002au, 0x00000067u,
    0x00000064u, 0x00000065u, 0x00000066u, 0x00000023u, 0x0003003eu, 0x0000005au, 0x00000067u,
    0x000100fdu, 0x00010038u,
};

// Vulkan 1.0 valid SPIR-V: sampled RGBA8 VkImage, UV binding2,
// fragment-only descriptor set=0 binding=0; private opt-in world GPU.
static constexpr uint32_t ogl_dev_world_rgba_sample_frag_spv[] = {
    0x07230203u, 0x00010000u, 0x0008000bu, 0x00000055u, 0x00000000u, 0x00020011u, 0x00000001u,
    0x0006000bu, 0x00000001u, 0x4c534c47u, 0x6474732eu, 0x3035342eu, 0x00000000u, 0x0003000eu,
    0x00000000u, 0x00000001u, 0x0009000fu, 0x00000004u, 0x00000004u, 0x6e69616du, 0x00000000u,
    0x0000000bu, 0x00000042u, 0x00000046u, 0x00000047u, 0x00030010u, 0x00000004u, 0x00000007u,
    0x00030003u, 0x00000002u, 0x000001c2u, 0x00040005u, 0x00000004u, 0x6e69616du, 0x00000000u,
    0x00040005u, 0x00000009u, 0x6d726f6eu, 0x00006c61u, 0x00060005u, 0x0000000bu, 0x6c726f77u,
    0x6f705f64u, 0x69746973u, 0x00006e6fu, 0x00030005u, 0x00000012u, 0x0000326eu, 0x00040005u,
    0x00000026u, 0x6867696cu, 0x00000074u, 0x00040005u, 0x0000002cu, 0x656d6143u, 0x00006172u,
    0x00070006u, 0x0000002cu, 0x00000000u, 0x6c726f77u, 0x6f745f64u, 0x696c635fu, 0x00000070u,
    0x00070006u, 0x0000002cu, 0x00000001u, 0x5f6e7573u, 0x65726964u, 0x6f697463u, 0x0000006eu,
    0x00040005u, 0x0000002eu, 0x656d6163u, 0x00006172u, 0x00040005u, 0x0000003au, 0x65786574u,
    0x0000006cu, 0x00060005u, 0x0000003eu, 0x74786574u, 0x5f657275u, 0x61626772u, 0x00000000u,
    0x00040005u, 0x00000042u, 0x65636166u, 0x0076755fu, 0x00060005u, 0x00000046u, 0x7074756fu,
    0x635f7475u, 0x726f6c6fu, 0x00000000u, 0x00050005u, 0x00000047u, 0x65636166u, 0x6267725fu,
    0x00000000u, 0x00040047u, 0x0000000bu, 0x0000001eu, 0x00000000u, 0x00030047u, 0x0000002cu,
    0x00000002u, 0x00040048u, 0x0000002cu, 0x00000000u, 0x00000005u, 0x00050048u, 0x0000002cu,
    0x00000000u, 0x00000007u, 0x00000010u, 0x00050048u, 0x0000002cu, 0x00000000u, 0x00000023u,
    0x00000000u, 0x00050048u, 0x0000002cu, 0x00000001u, 0x00000023u, 0x00000040u, 0x00040047u,
    0x0000003eu, 0x00000021u, 0x00000000u, 0x00040047u, 0x0000003eu, 0x00000022u, 0x00000000u,
    0x00040047u, 0x00000042u, 0x0000001eu, 0x00000002u, 0x00040047u, 0x00000046u, 0x0000001eu,
    0x00000000u, 0x00040047u, 0x00000047u, 0x0000001eu, 0x00000001u, 0x00020013u, 0x00000002u,
    0x00030021u, 0x00000003u, 0x00000002u, 0x00030016u, 0x00000006u, 0x00000020u, 0x00040017u,
    0x00000007u, 0x00000006u, 0x00000003u, 0x00040020u, 0x00000008u, 0x00000007u, 0x00000007u,
    0x00040020u, 0x0000000au, 0x00000001u, 0x00000007u, 0x0004003bu, 0x0000000au, 0x0000000bu,
    0x00000001u, 0x00040020u, 0x00000011u, 0x00000007u, 0x00000006u, 0x0004002bu, 0x00000006u,
    0x00000017u, 0x2b8cbcccu, 0x00020014u, 0x00000018u, 0x0004002bu, 0x00000006u, 0x00000022u,
    0x00000000u, 0x0004002bu, 0x00000006u, 0x00000023u, 0x3f800000u, 0x0006002cu, 0x00000007u,
    0x00000024u, 0x00000022u, 0x00000022u, 0x00000023u, 0x0004002bu, 0x00000006u, 0x00000027u,
    0x3e4ccccdu, 0x0004002bu, 0x00000006u, 0x00000028u, 0x3f4ccccdu, 0x00040017u, 0x0000002au,
    0x00000006u, 0x00000004u, 0x00040018u, 0x0000002bu, 0x0000002au, 0x00000004u, 0x0004001eu,
    0x0000002cu, 0x0000002bu, 0x0000002au, 0x00040020u, 0x0000002du, 0x00000009u, 0x0000002cu,
    0x0004003bu, 0x0000002du, 0x0000002eu, 0x00000009u, 0x00040015u, 0x0000002fu, 0x00000020u,
    0x00000001u, 0x0004002bu, 0x0000002fu, 0x00000030u, 0x00000001u, 0x00040020u, 0x00000031u,
    0x00000009u, 0x0000002au, 0x00040020u, 0x00000039u, 0x00000007u, 0x0000002au, 0x00090019u,
    0x0000003bu, 0x00000006u, 0x00000001u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000001u,
    0x00000000u, 0x0003001bu, 0x0000003cu, 0x0000003bu, 0x00040020u, 0x0000003du, 0x00000000u,
    0x0000003cu, 0x0004003bu, 0x0000003du, 0x0000003eu, 0x00000000u, 0x00040017u, 0x00000040u,
    0x00000006u, 0x00000002u, 0x00040020u, 0x00000041u, 0x00000001u, 0x00000040u, 0x0004003bu,
    0x00000041u, 0x00000042u, 0x00000001u, 0x00040020u, 0x00000045u, 0x00000003u, 0x0000002au,
    0x0004003bu, 0x00000045u, 0x00000046u, 0x00000003u, 0x0004003bu, 0x0000000au, 0x00000047u,
    0x00000001u, 0x0006002cu, 0x00000007u, 0x00000049u, 0x00000022u, 0x00000022u, 0x00000022u,
    0x0006002cu, 0x00000007u, 0x0000004au, 0x00000023u, 0x00000023u, 0x00000023u, 0x00050036u,
    0x00000002u, 0x00000004u, 0x00000000u, 0x00000003u, 0x000200f8u, 0x00000005u, 0x0004003bu,
    0x00000008u, 0x00000009u, 0x00000007u, 0x0004003bu, 0x00000011u, 0x00000012u, 0x00000007u,
    0x0004003bu, 0x00000008u, 0x0000001au, 0x00000007u, 0x0004003bu, 0x00000011u, 0x00000026u,
    0x00000007u, 0x0004003bu, 0x00000039u, 0x0000003au, 0x00000007u, 0x0004003du, 0x00000007u,
    0x0000000cu, 0x0000000bu, 0x000400cfu, 0x00000007u, 0x0000000du, 0x0000000cu, 0x0004003du,
    0x00000007u, 0x0000000eu, 0x0000000bu, 0x000400d0u, 0x00000007u, 0x0000000fu, 0x0000000eu,
    0x0007000cu, 0x00000007u, 0x00000010u, 0x00000001u, 0x00000044u, 0x0000000du, 0x0000000fu,
    0x0003003eu, 0x00000009u, 0x00000010u, 0x0004003du, 0x00000007u, 0x00000013u, 0x00000009u,
    0x0004003du, 0x00000007u, 0x00000014u, 0x00000009u, 0x00050094u, 0x00000006u, 0x00000015u,
    0x00000013u, 0x00000014u, 0x0003003eu, 0x00000012u, 0x00000015u, 0x0004003du, 0x00000006u,
    0x00000016u, 0x00000012u, 0x000500bau, 0x00000018u, 0x00000019u, 0x00000016u, 0x00000017u,
    0x000300f7u, 0x0000001cu, 0x00000000u, 0x000400fau, 0x00000019u, 0x0000001bu, 0x00000021u,
    0x000200f8u, 0x0000001bu, 0x0004003du, 0x00000007u, 0x0000001du, 0x00000009u, 0x0004003du,
    0x00000006u, 0x0000001eu, 0x00000012u, 0x0006000cu, 0x00000006u, 0x0000001fu, 0x00000001u,
    0x00000020u, 0x0000001eu, 0x0005008eu, 0x00000007u, 0x00000020u, 0x0000001du, 0x0000001fu,
    0x0003003eu, 0x0000001au, 0x00000020u, 0x000200f9u, 0x0000001cu, 0x000200f8u, 0x00000021u,
    0x0003003eu, 0x0000001au, 0x00000024u, 0x000200f9u, 0x0000001cu, 0x000200f8u, 0x0000001cu,
    0x0004003du, 0x00000007u, 0x00000025u, 0x0000001au, 0x0003003eu, 0x00000009u, 0x00000025u,
    0x0004003du, 0x00000007u, 0x00000029u, 0x00000009u, 0x00050041u, 0x00000031u, 0x00000032u,
    0x0000002eu, 0x00000030u, 0x0004003du, 0x0000002au, 0x00000033u, 0x00000032u, 0x0008004fu,
    0x00000007u, 0x00000034u, 0x00000033u, 0x00000033u, 0x00000000u, 0x00000001u, 0x00000002u,
    0x00050094u, 0x00000006u, 0x00000035u, 0x00000029u, 0x00000034u, 0x0006000cu, 0x00000006u,
    0x00000036u, 0x00000001u, 0x00000004u, 0x00000035u, 0x00050085u, 0x00000006u, 0x00000037u,
    0x00000028u, 0x00000036u, 0x00050081u, 0x00000006u, 0x00000038u, 0x00000027u, 0x00000037u,
    0x0003003eu, 0x00000026u, 0x00000038u, 0x0004003du, 0x0000003cu, 0x0000003fu, 0x0000003eu,
    0x0004003du, 0x00000040u, 0x00000043u, 0x00000042u, 0x00050057u, 0x0000002au, 0x00000044u,
    0x0000003fu, 0x00000043u, 0x0003003eu, 0x0000003au, 0x00000044u, 0x0004003du, 0x00000007u,
    0x00000048u, 0x00000047u, 0x0008000cu, 0x00000007u, 0x0000004bu, 0x00000001u, 0x0000002bu,
    0x00000048u, 0x00000049u, 0x0000004au, 0x0004003du, 0x0000002au, 0x0000004cu, 0x0000003au,
    0x0008004fu, 0x00000007u, 0x0000004du, 0x0000004cu, 0x0000004cu, 0x00000000u, 0x00000001u,
    0x00000002u, 0x00050085u, 0x00000007u, 0x0000004eu, 0x0000004bu, 0x0000004du, 0x0004003du,
    0x00000006u, 0x0000004fu, 0x00000026u, 0x0005008eu, 0x00000007u, 0x00000050u, 0x0000004eu,
    0x0000004fu, 0x00050051u, 0x00000006u, 0x00000051u, 0x00000050u, 0x00000000u, 0x00050051u,
    0x00000006u, 0x00000052u, 0x00000050u, 0x00000001u, 0x00050051u, 0x00000006u, 0x00000053u,
    0x00000050u, 0x00000002u, 0x00070050u, 0x0000002au, 0x00000054u, 0x00000051u, 0x00000052u,
    0x00000053u, 0x00000023u, 0x0003003eu, 0x00000046u, 0x00000054u, 0x000100fdu, 0x00010038u,
};

// Vulkan 1.0 validated P05 material-slot GPU atlas (4 independent RGBA tiles).
static constexpr uint32_t ogl_dev_world_material_atlas_vert_spv[] = {
    0x07230203u, 0x00010000u, 0x0008000bu, 0x00000036u, 0x00000000u, 0x00020011u, 0x00000001u,
    0x0006000bu, 0x00000001u, 0x4c534c47u, 0x6474732eu, 0x3035342eu, 0x00000000u, 0x0003000eu,
    0x00000000u, 0x00000001u, 0x000e000fu, 0x00000000u, 0x00000004u, 0x6e69616du, 0x00000000u,
    0x0000000du, 0x00000019u, 0x00000024u, 0x00000026u, 0x00000027u, 0x0000002bu, 0x0000002du,
    0x00000030u, 0x00000032u, 0x00030003u, 0x00000002u, 0x000001c2u, 0x00040005u, 0x00000004u,
    0x6e69616du, 0x00000000u, 0x00060005u, 0x0000000bu, 0x505f6c67u, 0x65567265u, 0x78657472u,
    0x00000000u, 0x00060006u, 0x0000000bu, 0x00000000u, 0x505f6c67u, 0x7469736fu, 0x006e6f69u,
    0x00070006u, 0x0000000bu, 0x00000001u, 0x505f6c67u, 0x746e696fu, 0x657a6953u, 0x00000000u,
    0x00070006u, 0x0000000bu, 0x00000002u, 0x435f6c67u, 0x4470696cu, 0x61747369u, 0x0065636eu,
    0x00070006u, 0x0000000bu, 0x00000003u, 0x435f6c67u, 0x446c6c75u, 0x61747369u, 0x0065636eu,
    0x00030005u, 0x0000000du, 0x00000000u, 0x00040005u, 0x00000011u, 0x656d6143u, 0x00006172u,
    0x00070006u, 0x00000011u, 0x00000000u, 0x6c726f77u, 0x6f745f64u, 0x696c635fu, 0x00000070u,
    0x00070006u, 0x00000011u, 0x00000001u, 0x5f6e7573u, 0x65726964u, 0x6f697463u, 0x0000006eu,
    0x00040005u, 0x00000013u, 0x656d6163u, 0x00006172u, 0x00080005u, 0x00000019u, 0x6c726f77u,
    0x6f705f64u, 0x69746973u, 0x695f6e6fu, 0x7475706eu, 0x00000000u, 0x00060005u, 0x00000024u,
    0x6c726f77u, 0x6f705f64u, 0x69746973u, 0x00006e6fu, 0x00050005u, 0x00000026u, 0x65636166u,
    0x6267725fu, 0x00000000u, 0x00060005u, 0x00000027u, 0x65636166u, 0x6267725fu, 0x706e695fu,
    0x00007475u, 0x00040005u, 0x0000002bu, 0x65636166u, 0x0076755fu, 0x00060005u, 0x0000002du,
    0x65636166u, 0x5f76755fu, 0x75706e69u, 0x00000074u, 0x00060005u, 0x00000030u, 0x6574616du,
    0x6c616972u, 0x6f6c735fu, 0x00000074u, 0x00070005u, 0x00000032u, 0x6574616du, 0x6c616972u,
    0x6f6c735fu, 0x6e695f74u, 0x00747570u, 0x00030047u, 0x0000000bu, 0x00000002u, 0x00050048u,
    0x0000000bu, 0x00000000u, 0x0000000bu, 0x00000000u, 0x00050048u, 0x0000000bu, 0x00000001u,
    0x0000000bu, 0x00000001u, 0x00050048u, 0x0000000bu, 0x00000002u, 0x0000000bu, 0x00000003u,
    0x00050048u, 0x0000000bu, 0x00000003u, 0x0000000bu, 0x00000004u, 0x00030047u, 0x00000011u,
    0x00000002u, 0x00040048u, 0x00000011u, 0x00000000u, 0x00000005u, 0x00050048u, 0x00000011u,
    0x00000000u, 0x00000007u, 0x00000010u, 0x00050048u, 0x00000011u, 0x00000000u, 0x00000023u,
    0x00000000u, 0x00050048u, 0x00000011u, 0x00000001u, 0x00000023u, 0x00000040u, 0x00040047u,
    0x00000019u, 0x0000001eu, 0x00000000u, 0x00040047u, 0x00000024u, 0x0000001eu, 0x00000000u,
    0x00040047u, 0x00000026u, 0x0000001eu, 0x00000001u, 0x00040047u, 0x00000027u, 0x0000001eu,
    0x00000001u, 0x00040047u, 0x0000002bu, 0x0000001eu, 0x00000002u, 0x00040047u, 0x0000002du,
    0x0000001eu, 0x00000002u, 0x00030047u, 0x00000030u, 0x0000000eu, 0x00040047u, 0x00000030u,
    0x0000001eu, 0x00000003u, 0x00040047u, 0x00000032u, 0x0000001eu, 0x00000003u, 0x00020013u,
    0x00000002u, 0x00030021u, 0x00000003u, 0x00000002u, 0x00030016u, 0x00000006u, 0x00000020u,
    0x00040017u, 0x00000007u, 0x00000006u, 0x00000004u, 0x00040015u, 0x00000008u, 0x00000020u,
    0x00000000u, 0x0004002bu, 0x00000008u, 0x00000009u, 0x00000001u, 0x0004001cu, 0x0000000au,
    0x00000006u, 0x00000009u, 0x0006001eu, 0x0000000bu, 0x00000007u, 0x00000006u, 0x0000000au,
    0x0000000au, 0x00040020u, 0x0000000cu, 0x00000003u, 0x0000000bu, 0x0004003bu, 0x0000000cu,
    0x0000000du, 0x00000003u, 0x00040015u, 0x0000000eu, 0x00000020u, 0x00000001u, 0x0004002bu,
    0x0000000eu, 0x0000000fu, 0x00000000u, 0x00040018u, 0x00000010u, 0x00000007u, 0x00000004u,
    0x0004001eu, 0x00000011u, 0x00000010u, 0x00000007u, 0x00040020u, 0x00000012u, 0x00000009u,
    0x00000011u, 0x0004003bu, 0x00000012u, 0x00000013u, 0x00000009u, 0x00040020u, 0x00000014u,
    0x00000009u, 0x00000010u, 0x00040017u, 0x00000017u, 0x00000006u, 0x00000003u, 0x00040020u,
    0x00000018u, 0x00000001u, 0x00000017u, 0x0004003bu, 0x00000018u, 0x00000019u, 0x00000001u,
    0x0004002bu, 0x00000006u, 0x0000001bu, 0x3f800000u, 0x00040020u, 0x00000021u, 0x00000003u,
    0x00000007u, 0x00040020u, 0x00000023u, 0x00000003u, 0x00000017u, 0x0004003bu, 0x00000023u,
    0x00000024u, 0x00000003u, 0x0004003bu, 0x00000023u, 0x00000026u, 0x00000003u, 0x0004003bu,
    0x00000018u, 0x00000027u, 0x00000001u, 0x00040017u, 0x00000029u, 0x00000006u, 0x00000002u,
    0x00040020u, 0x0000002au, 0x00000003u, 0x00000029u, 0x0004003bu, 0x0000002au, 0x0000002bu,
    0x00000003u, 0x00040020u, 0x0000002cu, 0x00000001u, 0x00000029u, 0x0004003bu, 0x0000002cu,
    0x0000002du, 0x00000001u, 0x00040020u, 0x0000002fu, 0x00000003u, 0x0000000eu, 0x0004003bu,
    0x0000002fu, 0x00000030u, 0x00000003u, 0x00040020u, 0x00000031u, 0x00000001u, 0x00000006u,
    0x0004003bu, 0x00000031u, 0x00000032u, 0x00000001u, 0x00050036u, 0x00000002u, 0x00000004u,
    0x00000000u, 0x00000003u, 0x000200f8u, 0x00000005u, 0x00050041u, 0x00000014u, 0x00000015u,
    0x00000013u, 0x0000000fu, 0x0004003du, 0x00000010u, 0x00000016u, 0x00000015u, 0x0004003du,
    0x00000017u, 0x0000001au, 0x00000019u, 0x00050051u, 0x00000006u, 0x0000001cu, 0x0000001au,
    0x00000000u, 0x00050051u, 0x00000006u, 0x0000001du, 0x0000001au, 0x00000001u, 0x00050051u,
    0x00000006u, 0x0000001eu, 0x0000001au, 0x00000002u, 0x00070050u, 0x00000007u, 0x0000001fu,
    0x0000001cu, 0x0000001du, 0x0000001eu, 0x0000001bu, 0x00050091u, 0x00000007u, 0x00000020u,
    0x00000016u, 0x0000001fu, 0x00050041u, 0x00000021u, 0x00000022u, 0x0000000du, 0x0000000fu,
    0x0003003eu, 0x00000022u, 0x00000020u, 0x0004003du, 0x00000017u, 0x00000025u, 0x00000019u,
    0x0003003eu, 0x00000024u, 0x00000025u, 0x0004003du, 0x00000017u, 0x00000028u, 0x00000027u,
    0x0003003eu, 0x00000026u, 0x00000028u, 0x0004003du, 0x00000029u, 0x0000002eu, 0x0000002du,
    0x0003003eu, 0x0000002bu, 0x0000002eu, 0x0004003du, 0x00000006u, 0x00000033u, 0x00000032u,
    0x0006000cu, 0x00000006u, 0x00000034u, 0x00000001u, 0x00000001u, 0x00000033u, 0x0004006eu,
    0x0000000eu, 0x00000035u, 0x00000034u, 0x0003003eu, 0x00000030u, 0x00000035u, 0x000100fdu,
    0x00010038u,
};

// Vulkan 1.0 validated P05 material-slot GPU atlas (4 independent RGBA tiles).
// P05/8: Vulkan 1.0 validated 4x4 / 64px tile HQ material atlas shader.
static constexpr uint32_t ogl_dev_world_material_atlas_hq_frag_spv[] = {
    0x07230203u, 0x00010000u, 0x0008000bu, 0x00000077u, 0x00000000u, 0x00020011u, 0x00000001u,
    0x0006000bu, 0x00000001u, 0x4c534c47u, 0x6474732eu, 0x3035342eu, 0x00000000u, 0x0003000eu,
    0x00000000u, 0x00000001u, 0x000a000fu, 0x00000004u, 0x00000004u, 0x6e69616du, 0x00000000u,
    0x0000000bu, 0x0000003du, 0x00000049u, 0x00000069u, 0x0000006au, 0x00030010u, 0x00000004u,
    0x00000007u, 0x00030003u, 0x00000002u, 0x000001c2u, 0x00040005u, 0x00000004u, 0x6e69616du,
    0x00000000u, 0x00040005u, 0x00000009u, 0x6d726f6eu, 0x00006c61u, 0x00060005u, 0x0000000bu,
    0x6c726f77u, 0x6f705f64u, 0x69746973u, 0x00006e6fu, 0x00030005u, 0x00000012u, 0x0000326eu,
    0x00040005u, 0x00000026u, 0x6867696cu, 0x00000074u, 0x00040005u, 0x0000002cu, 0x656d6143u,
    0x00006172u, 0x00070006u, 0x0000002cu, 0x00000000u, 0x6c726f77u, 0x6f745f64u, 0x696c635fu,
    0x00000070u, 0x00070006u, 0x0000002cu, 0x00000001u, 0x5f6e7573u, 0x65726964u, 0x6f697463u,
    0x0000006eu, 0x00040005u, 0x0000002eu, 0x656d6163u, 0x00006172u, 0x00040005u, 0x0000003bu,
    0x65786574u, 0x0000006cu, 0x00040005u, 0x0000003du, 0x65636166u, 0x0076755fu, 0x00040005u,
    0x00000047u, 0x746f6c73u, 0x00000000u, 0x00060005u, 0x00000049u, 0x6574616du, 0x6c616972u,
    0x6f6c735fu, 0x00000074u, 0x00040005u, 0x0000004eu, 0x65646e69u, 0x00000078u, 0x00050005u,
    0x00000057u, 0x616c7461u, 0x76755f73u, 0x00000000u, 0x00040005u, 0x0000005fu, 0x66666964u,
    0x00657375u, 0x00060005u, 0x00000063u, 0x74786574u, 0x5f657275u, 0x61626772u, 0x00000000u,
    0x00060005u, 0x00000069u, 0x7074756fu, 0x635f7475u, 0x726f6c6fu, 0x00000000u, 0x00050005u,
    0x0000006au, 0x65636166u, 0x6267725fu, 0x00000000u, 0x00040047u, 0x0000000bu, 0x0000001eu,
    0x00000000u, 0x00030047u, 0x0000002cu, 0x00000002u, 0x00040048u, 0x0000002cu, 0x00000000u,
    0x00000005u, 0x00050048u, 0x0000002cu, 0x00000000u, 0x00000007u, 0x00000010u, 0x00050048u,
    0x0000002cu, 0x00000000u, 0x00000023u, 0x00000000u, 0x00050048u, 0x0000002cu, 0x00000001u,
    0x00000023u, 0x00000040u, 0x00040047u, 0x0000003du, 0x0000001eu, 0x00000002u, 0x00030047u,
    0x00000049u, 0x0000000eu, 0x00040047u, 0x00000049u, 0x0000001eu, 0x00000003u, 0x00040047u,
    0x00000063u, 0x00000021u, 0x00000000u, 0x00040047u, 0x00000063u, 0x00000022u, 0x00000000u,
    0x00040047u, 0x00000069u, 0x0000001eu, 0x00000000u, 0x00040047u, 0x0000006au, 0x0000001eu,
    0x00000001u, 0x00020013u, 0x00000002u, 0x00030021u, 0x00000003u, 0x00000002u, 0x00030016u,
    0x00000006u, 0x00000020u, 0x00040017u, 0x00000007u, 0x00000006u, 0x00000003u, 0x00040020u,
    0x00000008u, 0x00000007u, 0x00000007u, 0x00040020u, 0x0000000au, 0x00000001u, 0x00000007u,
    0x0004003bu, 0x0000000au, 0x0000000bu, 0x00000001u, 0x00040020u, 0x00000011u, 0x00000007u,
    0x00000006u, 0x0004002bu, 0x00000006u, 0x00000017u, 0x2b8cbcccu, 0x00020014u, 0x00000018u,
    0x0004002bu, 0x00000006u, 0x00000022u, 0x00000000u, 0x0004002bu, 0x00000006u, 0x00000023u,
    0x3f800000u, 0x0006002cu, 0x00000007u, 0x00000024u, 0x00000022u, 0x00000022u, 0x00000023u,
    0x0004002bu, 0x00000006u, 0x00000027u, 0x3e4ccccdu, 0x0004002bu, 0x00000006u, 0x00000028u,
    0x3f4ccccdu, 0x00040017u, 0x0000002au, 0x00000006u, 0x00000004u, 0x00040018u, 0x0000002bu,
    0x0000002au, 0x00000004u, 0x0004001eu, 0x0000002cu, 0x0000002bu, 0x0000002au, 0x00040020u,
    0x0000002du, 0x00000009u, 0x0000002cu, 0x0004003bu, 0x0000002du, 0x0000002eu, 0x00000009u,
    0x00040015u, 0x0000002fu, 0x00000020u, 0x00000001u, 0x0004002bu, 0x0000002fu, 0x00000030u,
    0x00000001u, 0x00040020u, 0x00000031u, 0x00000009u, 0x0000002au, 0x00040017u, 0x00000039u,
    0x00000006u, 0x00000002u, 0x00040020u, 0x0000003au, 0x00000007u, 0x00000039u, 0x00040020u,
    0x0000003cu, 0x00000001u, 0x00000039u, 0x0004003bu, 0x0000003cu, 0x0000003du, 0x00000001u,
    0x0004002bu, 0x00000006u, 0x00000040u, 0x42800000u, 0x0004002bu, 0x00000006u, 0x00000043u,
    0x3f000000u, 0x0005002cu, 0x00000039u, 0x00000044u, 0x00000043u, 0x00000043u, 0x00040020u,
    0x00000046u, 0x00000007u, 0x0000002fu, 0x00040020u, 0x00000048u, 0x00000001u, 0x0000002fu,
    0x0004003bu, 0x00000048u, 0x00000049u, 0x00000001u, 0x0004002bu, 0x0000002fu, 0x0000004bu,
    0x00000000u, 0x0004002bu, 0x0000002fu, 0x0000004cu, 0x0000000fu, 0x0004002bu, 0x0000002fu,
    0x00000050u, 0x00000004u, 0x0004002bu, 0x00000006u, 0x0000005cu, 0x43800000u, 0x00090019u,
    0x00000060u, 0x00000006u, 0x00000001u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000001u,
    0x00000000u, 0x0003001bu, 0x00000061u, 0x00000060u, 0x00040020u, 0x00000062u, 0x00000000u,
    0x00000061u, 0x0004003bu, 0x00000062u, 0x00000063u, 0x00000000u, 0x00040020u, 0x00000068u,
    0x00000003u, 0x0000002au, 0x0004003bu, 0x00000068u, 0x00000069u, 0x00000003u, 0x0004003bu,
    0x0000000au, 0x0000006au, 0x00000001u, 0x0006002cu, 0x00000007u, 0x0000006cu, 0x00000022u,
    0x00000022u, 0x00000022u, 0x0006002cu, 0x00000007u, 0x0000006du, 0x00000023u, 0x00000023u,
    0x00000023u, 0x00050036u, 0x00000002u, 0x00000004u, 0x00000000u, 0x00000003u, 0x000200f8u,
    0x00000005u, 0x0004003bu, 0x00000008u, 0x00000009u, 0x00000007u, 0x0004003bu, 0x00000011u,
    0x00000012u, 0x00000007u, 0x0004003bu, 0x00000008u, 0x0000001au, 0x00000007u, 0x0004003bu,
    0x00000011u, 0x00000026u, 0x00000007u, 0x0004003bu, 0x0000003au, 0x0000003bu, 0x00000007u,
    0x0004003bu, 0x00000046u, 0x00000047u, 0x00000007u, 0x0004003bu, 0x0000003au, 0x0000004eu,
    0x00000007u, 0x0004003bu, 0x0000003au, 0x00000057u, 0x00000007u, 0x0004003bu, 0x00000008u,
    0x0000005fu, 0x00000007u, 0x0004003du, 0x00000007u, 0x0000000cu, 0x0000000bu, 0x000400cfu,
    0x00000007u, 0x0000000du, 0x0000000cu, 0x0004003du, 0x00000007u, 0x0000000eu, 0x0000000bu,
    0x000400d0u, 0x00000007u, 0x0000000fu, 0x0000000eu, 0x0007000cu, 0x00000007u, 0x00000010u,
    0x00000001u, 0x00000044u, 0x0000000du, 0x0000000fu, 0x0003003eu, 0x00000009u, 0x00000010u,
    0x0004003du, 0x00000007u, 0x00000013u, 0x00000009u, 0x0004003du, 0x00000007u, 0x00000014u,
    0x00000009u, 0x00050094u, 0x00000006u, 0x00000015u, 0x00000013u, 0x00000014u, 0x0003003eu,
    0x00000012u, 0x00000015u, 0x0004003du, 0x00000006u, 0x00000016u, 0x00000012u, 0x000500bau,
    0x00000018u, 0x00000019u, 0x00000016u, 0x00000017u, 0x000300f7u, 0x0000001cu, 0x00000000u,
    0x000400fau, 0x00000019u, 0x0000001bu, 0x00000021u, 0x000200f8u, 0x0000001bu, 0x0004003du,
    0x00000007u, 0x0000001du, 0x00000009u, 0x0004003du, 0x00000006u, 0x0000001eu, 0x00000012u,
    0x0006000cu, 0x00000006u, 0x0000001fu, 0x00000001u, 0x00000020u, 0x0000001eu, 0x0005008eu,
    0x00000007u, 0x00000020u, 0x0000001du, 0x0000001fu, 0x0003003eu, 0x0000001au, 0x00000020u,
    0x000200f9u, 0x0000001cu, 0x000200f8u, 0x00000021u, 0x0003003eu, 0x0000001au, 0x00000024u,
    0x000200f9u, 0x0000001cu, 0x000200f8u, 0x0000001cu, 0x0004003du, 0x00000007u, 0x00000025u,
    0x0000001au, 0x0003003eu, 0x00000009u, 0x00000025u, 0x0004003du, 0x00000007u, 0x00000029u,
    0x00000009u, 0x00050041u, 0x00000031u, 0x00000032u, 0x0000002eu, 0x00000030u, 0x0004003du,
    0x0000002au, 0x00000033u, 0x00000032u, 0x0008004fu, 0x00000007u, 0x00000034u, 0x00000033u,
    0x00000033u, 0x00000000u, 0x00000001u, 0x00000002u, 0x00050094u, 0x00000006u, 0x00000035u,
    0x00000029u, 0x00000034u, 0x0006000cu, 0x00000006u, 0x00000036u, 0x00000001u, 0x00000004u,
    0x00000035u, 0x00050085u, 0x00000006u, 0x00000037u, 0x00000028u, 0x00000036u, 0x00050081u,
    0x00000006u, 0x00000038u, 0x00000027u, 0x00000037u, 0x0003003eu, 0x00000026u, 0x00000038u,
    0x0004003du, 0x00000039u, 0x0000003eu, 0x0000003du, 0x0006000cu, 0x00000039u, 0x0000003fu,
    0x00000001u, 0x0000000au, 0x0000003eu, 0x0005008eu, 0x00000039u, 0x00000041u, 0x0000003fu,
    0x00000040u, 0x0006000cu, 0x00000039u, 0x00000042u, 0x00000001u, 0x00000008u, 0x00000041u,
    0x00050081u, 0x00000039u, 0x00000045u, 0x00000042u, 0x00000044u, 0x0003003eu, 0x0000003bu,
    0x00000045u, 0x0004003du, 0x0000002fu, 0x0000004au, 0x00000049u, 0x0008000cu, 0x0000002fu,
    0x0000004du, 0x00000001u, 0x0000002du, 0x0000004au, 0x0000004bu, 0x0000004cu, 0x0003003eu,
    0x00000047u, 0x0000004du, 0x0004003du, 0x0000002fu, 0x0000004fu, 0x00000047u, 0x0005008bu,
    0x0000002fu, 0x00000051u, 0x0000004fu, 0x00000050u, 0x0004006fu, 0x00000006u, 0x00000052u,
    0x00000051u, 0x0004003du, 0x0000002fu, 0x00000053u, 0x00000047u, 0x00050087u, 0x0000002fu,
    0x00000054u, 0x00000053u, 0x00000050u, 0x0004006fu, 0x00000006u, 0x00000055u, 0x00000054u,
    0x00050050u, 0x00000039u, 0x00000056u, 0x00000052u, 0x00000055u, 0x0003003eu, 0x0000004eu,
    0x00000056u, 0x0004003du, 0x00000039u, 0x00000058u, 0x0000004eu, 0x0005008eu, 0x00000039u,
    0x00000059u, 0x00000058u, 0x00000040u, 0x0004003du, 0x00000039u, 0x0000005au, 0x0000003bu,
    0x00050081u, 0x00000039u, 0x0000005bu, 0x00000059u, 0x0000005au, 0x00050050u, 0x00000039u,
    0x0000005du, 0x0000005cu, 0x0000005cu, 0x00050088u, 0x00000039u, 0x0000005eu, 0x0000005bu,
    0x0000005du, 0x0003003eu, 0x00000057u, 0x0000005eu, 0x0004003du, 0x00000061u, 0x00000064u,
    0x00000063u, 0x0004003du, 0x00000039u, 0x00000065u, 0x00000057u, 0x00050057u, 0x0000002au,
    0x00000066u, 0x00000064u, 0x00000065u, 0x0008004fu, 0x00000007u, 0x00000067u, 0x00000066u,
    0x00000066u, 0x00000000u, 0x00000001u, 0x00000002u, 0x0003003eu, 0x0000005fu, 0x00000067u,
    0x0004003du, 0x00000007u, 0x0000006bu, 0x0000006au, 0x0008000cu, 0x00000007u, 0x0000006eu,
    0x00000001u, 0x0000002bu, 0x0000006bu, 0x0000006cu, 0x0000006du, 0x0004003du, 0x00000007u,
    0x0000006fu, 0x0000005fu, 0x00050085u, 0x00000007u, 0x00000070u, 0x0000006eu, 0x0000006fu,
    0x0004003du, 0x00000006u, 0x00000071u, 0x00000026u, 0x0005008eu, 0x00000007u, 0x00000072u,
    0x00000070u, 0x00000071u, 0x00050051u, 0x00000006u, 0x00000073u, 0x00000072u, 0x00000000u,
    0x00050051u, 0x00000006u, 0x00000074u, 0x00000072u, 0x00000001u, 0x00050051u, 0x00000006u,
    0x00000075u, 0x00000072u, 0x00000002u, 0x00070050u, 0x0000002au, 0x00000076u, 0x00000073u,
    0x00000074u, 0x00000075u, 0x00000023u, 0x0003003eu, 0x00000069u, 0x00000076u, 0x000100fdu,
    0x00010038u,
};

// P05/7: Vulkan 1.0 validated 4x4 opaque material atlas sampling shader.
static constexpr uint32_t ogl_dev_world_material_atlas_16_frag_spv[] = {
    0x07230203u, 0x00010000u, 0x0008000bu, 0x00000077u, 0x00000000u, 0x00020011u, 0x00000001u,
    0x0006000bu, 0x00000001u, 0x4c534c47u, 0x6474732eu, 0x3035342eu, 0x00000000u, 0x0003000eu,
    0x00000000u, 0x00000001u, 0x000a000fu, 0x00000004u, 0x00000004u, 0x6e69616du, 0x00000000u,
    0x0000000bu, 0x0000003du, 0x00000049u, 0x00000069u, 0x0000006au, 0x00030010u, 0x00000004u,
    0x00000007u, 0x00030003u, 0x00000002u, 0x000001c2u, 0x00040005u, 0x00000004u, 0x6e69616du,
    0x00000000u, 0x00040005u, 0x00000009u, 0x6d726f6eu, 0x00006c61u, 0x00060005u, 0x0000000bu,
    0x6c726f77u, 0x6f705f64u, 0x69746973u, 0x00006e6fu, 0x00030005u, 0x00000012u, 0x0000326eu,
    0x00040005u, 0x00000026u, 0x6867696cu, 0x00000074u, 0x00040005u, 0x0000002cu, 0x656d6143u,
    0x00006172u, 0x00070006u, 0x0000002cu, 0x00000000u, 0x6c726f77u, 0x6f745f64u, 0x696c635fu,
    0x00000070u, 0x00070006u, 0x0000002cu, 0x00000001u, 0x5f6e7573u, 0x65726964u, 0x6f697463u,
    0x0000006eu, 0x00040005u, 0x0000002eu, 0x656d6163u, 0x00006172u, 0x00040005u, 0x0000003bu,
    0x65786574u, 0x0000006cu, 0x00040005u, 0x0000003du, 0x65636166u, 0x0076755fu, 0x00040005u,
    0x00000047u, 0x746f6c73u, 0x00000000u, 0x00060005u, 0x00000049u, 0x6574616du, 0x6c616972u,
    0x6f6c735fu, 0x00000074u, 0x00040005u, 0x0000004eu, 0x65646e69u, 0x00000078u, 0x00050005u,
    0x00000057u, 0x616c7461u, 0x76755f73u, 0x00000000u, 0x00040005u, 0x0000005fu, 0x66666964u,
    0x00657375u, 0x00060005u, 0x00000063u, 0x74786574u, 0x5f657275u, 0x61626772u, 0x00000000u,
    0x00060005u, 0x00000069u, 0x7074756fu, 0x635f7475u, 0x726f6c6fu, 0x00000000u, 0x00050005u,
    0x0000006au, 0x65636166u, 0x6267725fu, 0x00000000u, 0x00040047u, 0x0000000bu, 0x0000001eu,
    0x00000000u, 0x00030047u, 0x0000002cu, 0x00000002u, 0x00040048u, 0x0000002cu, 0x00000000u,
    0x00000005u, 0x00050048u, 0x0000002cu, 0x00000000u, 0x00000007u, 0x00000010u, 0x00050048u,
    0x0000002cu, 0x00000000u, 0x00000023u, 0x00000000u, 0x00050048u, 0x0000002cu, 0x00000001u,
    0x00000023u, 0x00000040u, 0x00040047u, 0x0000003du, 0x0000001eu, 0x00000002u, 0x00030047u,
    0x00000049u, 0x0000000eu, 0x00040047u, 0x00000049u, 0x0000001eu, 0x00000003u, 0x00040047u,
    0x00000063u, 0x00000021u, 0x00000000u, 0x00040047u, 0x00000063u, 0x00000022u, 0x00000000u,
    0x00040047u, 0x00000069u, 0x0000001eu, 0x00000000u, 0x00040047u, 0x0000006au, 0x0000001eu,
    0x00000001u, 0x00020013u, 0x00000002u, 0x00030021u, 0x00000003u, 0x00000002u, 0x00030016u,
    0x00000006u, 0x00000020u, 0x00040017u, 0x00000007u, 0x00000006u, 0x00000003u, 0x00040020u,
    0x00000008u, 0x00000007u, 0x00000007u, 0x00040020u, 0x0000000au, 0x00000001u, 0x00000007u,
    0x0004003bu, 0x0000000au, 0x0000000bu, 0x00000001u, 0x00040020u, 0x00000011u, 0x00000007u,
    0x00000006u, 0x0004002bu, 0x00000006u, 0x00000017u, 0x2b8cbcccu, 0x00020014u, 0x00000018u,
    0x0004002bu, 0x00000006u, 0x00000022u, 0x00000000u, 0x0004002bu, 0x00000006u, 0x00000023u,
    0x3f800000u, 0x0006002cu, 0x00000007u, 0x00000024u, 0x00000022u, 0x00000022u, 0x00000023u,
    0x0004002bu, 0x00000006u, 0x00000027u, 0x3e4ccccdu, 0x0004002bu, 0x00000006u, 0x00000028u,
    0x3f4ccccdu, 0x00040017u, 0x0000002au, 0x00000006u, 0x00000004u, 0x00040018u, 0x0000002bu,
    0x0000002au, 0x00000004u, 0x0004001eu, 0x0000002cu, 0x0000002bu, 0x0000002au, 0x00040020u,
    0x0000002du, 0x00000009u, 0x0000002cu, 0x0004003bu, 0x0000002du, 0x0000002eu, 0x00000009u,
    0x00040015u, 0x0000002fu, 0x00000020u, 0x00000001u, 0x0004002bu, 0x0000002fu, 0x00000030u,
    0x00000001u, 0x00040020u, 0x00000031u, 0x00000009u, 0x0000002au, 0x00040017u, 0x00000039u,
    0x00000006u, 0x00000002u, 0x00040020u, 0x0000003au, 0x00000007u, 0x00000039u, 0x00040020u,
    0x0000003cu, 0x00000001u, 0x00000039u, 0x0004003bu, 0x0000003cu, 0x0000003du, 0x00000001u,
    0x0004002bu, 0x00000006u, 0x00000040u, 0x41800000u, 0x0004002bu, 0x00000006u, 0x00000043u,
    0x3f000000u, 0x0005002cu, 0x00000039u, 0x00000044u, 0x00000043u, 0x00000043u, 0x00040020u,
    0x00000046u, 0x00000007u, 0x0000002fu, 0x00040020u, 0x00000048u, 0x00000001u, 0x0000002fu,
    0x0004003bu, 0x00000048u, 0x00000049u, 0x00000001u, 0x0004002bu, 0x0000002fu, 0x0000004bu,
    0x00000000u, 0x0004002bu, 0x0000002fu, 0x0000004cu, 0x0000000fu, 0x0004002bu, 0x0000002fu,
    0x00000050u, 0x00000004u, 0x0004002bu, 0x00000006u, 0x0000005cu, 0x42800000u, 0x00090019u,
    0x00000060u, 0x00000006u, 0x00000001u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000001u,
    0x00000000u, 0x0003001bu, 0x00000061u, 0x00000060u, 0x00040020u, 0x00000062u, 0x00000000u,
    0x00000061u, 0x0004003bu, 0x00000062u, 0x00000063u, 0x00000000u, 0x00040020u, 0x00000068u,
    0x00000003u, 0x0000002au, 0x0004003bu, 0x00000068u, 0x00000069u, 0x00000003u, 0x0004003bu,
    0x0000000au, 0x0000006au, 0x00000001u, 0x0006002cu, 0x00000007u, 0x0000006cu, 0x00000022u,
    0x00000022u, 0x00000022u, 0x0006002cu, 0x00000007u, 0x0000006du, 0x00000023u, 0x00000023u,
    0x00000023u, 0x00050036u, 0x00000002u, 0x00000004u, 0x00000000u, 0x00000003u, 0x000200f8u,
    0x00000005u, 0x0004003bu, 0x00000008u, 0x00000009u, 0x00000007u, 0x0004003bu, 0x00000011u,
    0x00000012u, 0x00000007u, 0x0004003bu, 0x00000008u, 0x0000001au, 0x00000007u, 0x0004003bu,
    0x00000011u, 0x00000026u, 0x00000007u, 0x0004003bu, 0x0000003au, 0x0000003bu, 0x00000007u,
    0x0004003bu, 0x00000046u, 0x00000047u, 0x00000007u, 0x0004003bu, 0x0000003au, 0x0000004eu,
    0x00000007u, 0x0004003bu, 0x0000003au, 0x00000057u, 0x00000007u, 0x0004003bu, 0x00000008u,
    0x0000005fu, 0x00000007u, 0x0004003du, 0x00000007u, 0x0000000cu, 0x0000000bu, 0x000400cfu,
    0x00000007u, 0x0000000du, 0x0000000cu, 0x0004003du, 0x00000007u, 0x0000000eu, 0x0000000bu,
    0x000400d0u, 0x00000007u, 0x0000000fu, 0x0000000eu, 0x0007000cu, 0x00000007u, 0x00000010u,
    0x00000001u, 0x00000044u, 0x0000000du, 0x0000000fu, 0x0003003eu, 0x00000009u, 0x00000010u,
    0x0004003du, 0x00000007u, 0x00000013u, 0x00000009u, 0x0004003du, 0x00000007u, 0x00000014u,
    0x00000009u, 0x00050094u, 0x00000006u, 0x00000015u, 0x00000013u, 0x00000014u, 0x0003003eu,
    0x00000012u, 0x00000015u, 0x0004003du, 0x00000006u, 0x00000016u, 0x00000012u, 0x000500bau,
    0x00000018u, 0x00000019u, 0x00000016u, 0x00000017u, 0x000300f7u, 0x0000001cu, 0x00000000u,
    0x000400fau, 0x00000019u, 0x0000001bu, 0x00000021u, 0x000200f8u, 0x0000001bu, 0x0004003du,
    0x00000007u, 0x0000001du, 0x00000009u, 0x0004003du, 0x00000006u, 0x0000001eu, 0x00000012u,
    0x0006000cu, 0x00000006u, 0x0000001fu, 0x00000001u, 0x00000020u, 0x0000001eu, 0x0005008eu,
    0x00000007u, 0x00000020u, 0x0000001du, 0x0000001fu, 0x0003003eu, 0x0000001au, 0x00000020u,
    0x000200f9u, 0x0000001cu, 0x000200f8u, 0x00000021u, 0x0003003eu, 0x0000001au, 0x00000024u,
    0x000200f9u, 0x0000001cu, 0x000200f8u, 0x0000001cu, 0x0004003du, 0x00000007u, 0x00000025u,
    0x0000001au, 0x0003003eu, 0x00000009u, 0x00000025u, 0x0004003du, 0x00000007u, 0x00000029u,
    0x00000009u, 0x00050041u, 0x00000031u, 0x00000032u, 0x0000002eu, 0x00000030u, 0x0004003du,
    0x0000002au, 0x00000033u, 0x00000032u, 0x0008004fu, 0x00000007u, 0x00000034u, 0x00000033u,
    0x00000033u, 0x00000000u, 0x00000001u, 0x00000002u, 0x00050094u, 0x00000006u, 0x00000035u,
    0x00000029u, 0x00000034u, 0x0006000cu, 0x00000006u, 0x00000036u, 0x00000001u, 0x00000004u,
    0x00000035u, 0x00050085u, 0x00000006u, 0x00000037u, 0x00000028u, 0x00000036u, 0x00050081u,
    0x00000006u, 0x00000038u, 0x00000027u, 0x00000037u, 0x0003003eu, 0x00000026u, 0x00000038u,
    0x0004003du, 0x00000039u, 0x0000003eu, 0x0000003du, 0x0006000cu, 0x00000039u, 0x0000003fu,
    0x00000001u, 0x0000000au, 0x0000003eu, 0x0005008eu, 0x00000039u, 0x00000041u, 0x0000003fu,
    0x00000040u, 0x0006000cu, 0x00000039u, 0x00000042u, 0x00000001u, 0x00000008u, 0x00000041u,
    0x00050081u, 0x00000039u, 0x00000045u, 0x00000042u, 0x00000044u, 0x0003003eu, 0x0000003bu,
    0x00000045u, 0x0004003du, 0x0000002fu, 0x0000004au, 0x00000049u, 0x0008000cu, 0x0000002fu,
    0x0000004du, 0x00000001u, 0x0000002du, 0x0000004au, 0x0000004bu, 0x0000004cu, 0x0003003eu,
    0x00000047u, 0x0000004du, 0x0004003du, 0x0000002fu, 0x0000004fu, 0x00000047u, 0x0005008bu,
    0x0000002fu, 0x00000051u, 0x0000004fu, 0x00000050u, 0x0004006fu, 0x00000006u, 0x00000052u,
    0x00000051u, 0x0004003du, 0x0000002fu, 0x00000053u, 0x00000047u, 0x00050087u, 0x0000002fu,
    0x00000054u, 0x00000053u, 0x00000050u, 0x0004006fu, 0x00000006u, 0x00000055u, 0x00000054u,
    0x00050050u, 0x00000039u, 0x00000056u, 0x00000052u, 0x00000055u, 0x0003003eu, 0x0000004eu,
    0x00000056u, 0x0004003du, 0x00000039u, 0x00000058u, 0x0000004eu, 0x0005008eu, 0x00000039u,
    0x00000059u, 0x00000058u, 0x00000040u, 0x0004003du, 0x00000039u, 0x0000005au, 0x0000003bu,
    0x00050081u, 0x00000039u, 0x0000005bu, 0x00000059u, 0x0000005au, 0x00050050u, 0x00000039u,
    0x0000005du, 0x0000005cu, 0x0000005cu, 0x00050088u, 0x00000039u, 0x0000005eu, 0x0000005bu,
    0x0000005du, 0x0003003eu, 0x00000057u, 0x0000005eu, 0x0004003du, 0x00000061u, 0x00000064u,
    0x00000063u, 0x0004003du, 0x00000039u, 0x00000065u, 0x00000057u, 0x00050057u, 0x0000002au,
    0x00000066u, 0x00000064u, 0x00000065u, 0x0008004fu, 0x00000007u, 0x00000067u, 0x00000066u,
    0x00000066u, 0x00000000u, 0x00000001u, 0x00000002u, 0x0003003eu, 0x0000005fu, 0x00000067u,
    0x0004003du, 0x00000007u, 0x0000006bu, 0x0000006au, 0x0008000cu, 0x00000007u, 0x0000006eu,
    0x00000001u, 0x0000002bu, 0x0000006bu, 0x0000006cu, 0x0000006du, 0x0004003du, 0x00000007u,
    0x0000006fu, 0x0000005fu, 0x00050085u, 0x00000007u, 0x00000070u, 0x0000006eu, 0x0000006fu,
    0x0004003du, 0x00000006u, 0x00000071u, 0x00000026u, 0x0005008eu, 0x00000007u, 0x00000072u,
    0x00000070u, 0x00000071u, 0x00050051u, 0x00000006u, 0x00000073u, 0x00000072u, 0x00000000u,
    0x00050051u, 0x00000006u, 0x00000074u, 0x00000072u, 0x00000001u, 0x00050051u, 0x00000006u,
    0x00000075u, 0x00000072u, 0x00000002u, 0x00070050u, 0x0000002au, 0x00000076u, 0x00000073u,
    0x00000074u, 0x00000075u, 0x00000023u, 0x0003003eu, 0x00000069u, 0x00000076u, 0x000100fdu,
    0x00010038u,
};

static constexpr uint32_t ogl_dev_world_material_atlas_frag_spv[] = {
    0x07230203u, 0x00010000u, 0x0008000bu, 0x00000077u, 0x00000000u, 0x00020011u, 0x00000001u,
    0x0006000bu, 0x00000001u, 0x4c534c47u, 0x6474732eu, 0x3035342eu, 0x00000000u, 0x0003000eu,
    0x00000000u, 0x00000001u, 0x000a000fu, 0x00000004u, 0x00000004u, 0x6e69616du, 0x00000000u,
    0x0000000bu, 0x0000003du, 0x00000049u, 0x00000069u, 0x0000006au, 0x00030010u, 0x00000004u,
    0x00000007u, 0x00030003u, 0x00000002u, 0x000001c2u, 0x00040005u, 0x00000004u, 0x6e69616du,
    0x00000000u, 0x00040005u, 0x00000009u, 0x6d726f6eu, 0x00006c61u, 0x00060005u, 0x0000000bu,
    0x6c726f77u, 0x6f705f64u, 0x69746973u, 0x00006e6fu, 0x00030005u, 0x00000012u, 0x0000326eu,
    0x00040005u, 0x00000026u, 0x6867696cu, 0x00000074u, 0x00040005u, 0x0000002cu, 0x656d6143u,
    0x00006172u, 0x00070006u, 0x0000002cu, 0x00000000u, 0x6c726f77u, 0x6f745f64u, 0x696c635fu,
    0x00000070u, 0x00070006u, 0x0000002cu, 0x00000001u, 0x5f6e7573u, 0x65726964u, 0x6f697463u,
    0x0000006eu, 0x00040005u, 0x0000002eu, 0x656d6163u, 0x00006172u, 0x00040005u, 0x0000003bu,
    0x65786574u, 0x0000006cu, 0x00040005u, 0x0000003du, 0x65636166u, 0x0076755fu, 0x00040005u,
    0x00000047u, 0x746f6c73u, 0x00000000u, 0x00060005u, 0x00000049u, 0x6574616du, 0x6c616972u,
    0x6f6c735fu, 0x00000074u, 0x00040005u, 0x0000004eu, 0x65646e69u, 0x00000078u, 0x00050005u,
    0x00000057u, 0x616c7461u, 0x76755f73u, 0x00000000u, 0x00040005u, 0x0000005fu, 0x66666964u,
    0x00657375u, 0x00060005u, 0x00000063u, 0x74786574u, 0x5f657275u, 0x61626772u, 0x00000000u,
    0x00060005u, 0x00000069u, 0x7074756fu, 0x635f7475u, 0x726f6c6fu, 0x00000000u, 0x00050005u,
    0x0000006au, 0x65636166u, 0x6267725fu, 0x00000000u, 0x00040047u, 0x0000000bu, 0x0000001eu,
    0x00000000u, 0x00030047u, 0x0000002cu, 0x00000002u, 0x00040048u, 0x0000002cu, 0x00000000u,
    0x00000005u, 0x00050048u, 0x0000002cu, 0x00000000u, 0x00000007u, 0x00000010u, 0x00050048u,
    0x0000002cu, 0x00000000u, 0x00000023u, 0x00000000u, 0x00050048u, 0x0000002cu, 0x00000001u,
    0x00000023u, 0x00000040u, 0x00040047u, 0x0000003du, 0x0000001eu, 0x00000002u, 0x00030047u,
    0x00000049u, 0x0000000eu, 0x00040047u, 0x00000049u, 0x0000001eu, 0x00000003u, 0x00040047u,
    0x00000063u, 0x00000021u, 0x00000000u, 0x00040047u, 0x00000063u, 0x00000022u, 0x00000000u,
    0x00040047u, 0x00000069u, 0x0000001eu, 0x00000000u, 0x00040047u, 0x0000006au, 0x0000001eu,
    0x00000001u, 0x00020013u, 0x00000002u, 0x00030021u, 0x00000003u, 0x00000002u, 0x00030016u,
    0x00000006u, 0x00000020u, 0x00040017u, 0x00000007u, 0x00000006u, 0x00000003u, 0x00040020u,
    0x00000008u, 0x00000007u, 0x00000007u, 0x00040020u, 0x0000000au, 0x00000001u, 0x00000007u,
    0x0004003bu, 0x0000000au, 0x0000000bu, 0x00000001u, 0x00040020u, 0x00000011u, 0x00000007u,
    0x00000006u, 0x0004002bu, 0x00000006u, 0x00000017u, 0x2b8cbcccu, 0x00020014u, 0x00000018u,
    0x0004002bu, 0x00000006u, 0x00000022u, 0x00000000u, 0x0004002bu, 0x00000006u, 0x00000023u,
    0x3f800000u, 0x0006002cu, 0x00000007u, 0x00000024u, 0x00000022u, 0x00000022u, 0x00000023u,
    0x0004002bu, 0x00000006u, 0x00000027u, 0x3e4ccccdu, 0x0004002bu, 0x00000006u, 0x00000028u,
    0x3f4ccccdu, 0x00040017u, 0x0000002au, 0x00000006u, 0x00000004u, 0x00040018u, 0x0000002bu,
    0x0000002au, 0x00000004u, 0x0004001eu, 0x0000002cu, 0x0000002bu, 0x0000002au, 0x00040020u,
    0x0000002du, 0x00000009u, 0x0000002cu, 0x0004003bu, 0x0000002du, 0x0000002eu, 0x00000009u,
    0x00040015u, 0x0000002fu, 0x00000020u, 0x00000001u, 0x0004002bu, 0x0000002fu, 0x00000030u,
    0x00000001u, 0x00040020u, 0x00000031u, 0x00000009u, 0x0000002au, 0x00040017u, 0x00000039u,
    0x00000006u, 0x00000002u, 0x00040020u, 0x0000003au, 0x00000007u, 0x00000039u, 0x00040020u,
    0x0000003cu, 0x00000001u, 0x00000039u, 0x0004003bu, 0x0000003cu, 0x0000003du, 0x00000001u,
    0x0004002bu, 0x00000006u, 0x00000040u, 0x42000000u, 0x0004002bu, 0x00000006u, 0x00000043u,
    0x3f000000u, 0x0005002cu, 0x00000039u, 0x00000044u, 0x00000043u, 0x00000043u, 0x00040020u,
    0x00000046u, 0x00000007u, 0x0000002fu, 0x00040020u, 0x00000048u, 0x00000001u, 0x0000002fu,
    0x0004003bu, 0x00000048u, 0x00000049u, 0x00000001u, 0x0004002bu, 0x0000002fu, 0x0000004bu,
    0x00000000u, 0x0004002bu, 0x0000002fu, 0x0000004cu, 0x00000003u, 0x0004002bu, 0x0000002fu,
    0x00000050u, 0x00000002u, 0x0004002bu, 0x00000006u, 0x0000005cu, 0x42800000u, 0x00090019u,
    0x00000060u, 0x00000006u, 0x00000001u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000001u,
    0x00000000u, 0x0003001bu, 0x00000061u, 0x00000060u, 0x00040020u, 0x00000062u, 0x00000000u,
    0x00000061u, 0x0004003bu, 0x00000062u, 0x00000063u, 0x00000000u, 0x00040020u, 0x00000068u,
    0x00000003u, 0x0000002au, 0x0004003bu, 0x00000068u, 0x00000069u, 0x00000003u, 0x0004003bu,
    0x0000000au, 0x0000006au, 0x00000001u, 0x0006002cu, 0x00000007u, 0x0000006cu, 0x00000022u,
    0x00000022u, 0x00000022u, 0x0006002cu, 0x00000007u, 0x0000006du, 0x00000023u, 0x00000023u,
    0x00000023u, 0x00050036u, 0x00000002u, 0x00000004u, 0x00000000u, 0x00000003u, 0x000200f8u,
    0x00000005u, 0x0004003bu, 0x00000008u, 0x00000009u, 0x00000007u, 0x0004003bu, 0x00000011u,
    0x00000012u, 0x00000007u, 0x0004003bu, 0x00000008u, 0x0000001au, 0x00000007u, 0x0004003bu,
    0x00000011u, 0x00000026u, 0x00000007u, 0x0004003bu, 0x0000003au, 0x0000003bu, 0x00000007u,
    0x0004003bu, 0x00000046u, 0x00000047u, 0x00000007u, 0x0004003bu, 0x0000003au, 0x0000004eu,
    0x00000007u, 0x0004003bu, 0x0000003au, 0x00000057u, 0x00000007u, 0x0004003bu, 0x00000008u,
    0x0000005fu, 0x00000007u, 0x0004003du, 0x00000007u, 0x0000000cu, 0x0000000bu, 0x000400cfu,
    0x00000007u, 0x0000000du, 0x0000000cu, 0x0004003du, 0x00000007u, 0x0000000eu, 0x0000000bu,
    0x000400d0u, 0x00000007u, 0x0000000fu, 0x0000000eu, 0x0007000cu, 0x00000007u, 0x00000010u,
    0x00000001u, 0x00000044u, 0x0000000du, 0x0000000fu, 0x0003003eu, 0x00000009u, 0x00000010u,
    0x0004003du, 0x00000007u, 0x00000013u, 0x00000009u, 0x0004003du, 0x00000007u, 0x00000014u,
    0x00000009u, 0x00050094u, 0x00000006u, 0x00000015u, 0x00000013u, 0x00000014u, 0x0003003eu,
    0x00000012u, 0x00000015u, 0x0004003du, 0x00000006u, 0x00000016u, 0x00000012u, 0x000500bau,
    0x00000018u, 0x00000019u, 0x00000016u, 0x00000017u, 0x000300f7u, 0x0000001cu, 0x00000000u,
    0x000400fau, 0x00000019u, 0x0000001bu, 0x00000021u, 0x000200f8u, 0x0000001bu, 0x0004003du,
    0x00000007u, 0x0000001du, 0x00000009u, 0x0004003du, 0x00000006u, 0x0000001eu, 0x00000012u,
    0x0006000cu, 0x00000006u, 0x0000001fu, 0x00000001u, 0x00000020u, 0x0000001eu, 0x0005008eu,
    0x00000007u, 0x00000020u, 0x0000001du, 0x0000001fu, 0x0003003eu, 0x0000001au, 0x00000020u,
    0x000200f9u, 0x0000001cu, 0x000200f8u, 0x00000021u, 0x0003003eu, 0x0000001au, 0x00000024u,
    0x000200f9u, 0x0000001cu, 0x000200f8u, 0x0000001cu, 0x0004003du, 0x00000007u, 0x00000025u,
    0x0000001au, 0x0003003eu, 0x00000009u, 0x00000025u, 0x0004003du, 0x00000007u, 0x00000029u,
    0x00000009u, 0x00050041u, 0x00000031u, 0x00000032u, 0x0000002eu, 0x00000030u, 0x0004003du,
    0x0000002au, 0x00000033u, 0x00000032u, 0x0008004fu, 0x00000007u, 0x00000034u, 0x00000033u,
    0x00000033u, 0x00000000u, 0x00000001u, 0x00000002u, 0x00050094u, 0x00000006u, 0x00000035u,
    0x00000029u, 0x00000034u, 0x0006000cu, 0x00000006u, 0x00000036u, 0x00000001u, 0x00000004u,
    0x00000035u, 0x00050085u, 0x00000006u, 0x00000037u, 0x00000028u, 0x00000036u, 0x00050081u,
    0x00000006u, 0x00000038u, 0x00000027u, 0x00000037u, 0x0003003eu, 0x00000026u, 0x00000038u,
    0x0004003du, 0x00000039u, 0x0000003eu, 0x0000003du, 0x0006000cu, 0x00000039u, 0x0000003fu,
    0x00000001u, 0x0000000au, 0x0000003eu, 0x0005008eu, 0x00000039u, 0x00000041u, 0x0000003fu,
    0x00000040u, 0x0006000cu, 0x00000039u, 0x00000042u, 0x00000001u, 0x00000008u, 0x00000041u,
    0x00050081u, 0x00000039u, 0x00000045u, 0x00000042u, 0x00000044u, 0x0003003eu, 0x0000003bu,
    0x00000045u, 0x0004003du, 0x0000002fu, 0x0000004au, 0x00000049u, 0x0008000cu, 0x0000002fu,
    0x0000004du, 0x00000001u, 0x0000002du, 0x0000004au, 0x0000004bu, 0x0000004cu, 0x0003003eu,
    0x00000047u, 0x0000004du, 0x0004003du, 0x0000002fu, 0x0000004fu, 0x00000047u, 0x0005008bu,
    0x0000002fu, 0x00000051u, 0x0000004fu, 0x00000050u, 0x0004006fu, 0x00000006u, 0x00000052u,
    0x00000051u, 0x0004003du, 0x0000002fu, 0x00000053u, 0x00000047u, 0x00050087u, 0x0000002fu,
    0x00000054u, 0x00000053u, 0x00000050u, 0x0004006fu, 0x00000006u, 0x00000055u, 0x00000054u,
    0x00050050u, 0x00000039u, 0x00000056u, 0x00000052u, 0x00000055u, 0x0003003eu, 0x0000004eu,
    0x00000056u, 0x0004003du, 0x00000039u, 0x00000058u, 0x0000004eu, 0x0005008eu, 0x00000039u,
    0x00000059u, 0x00000058u, 0x00000040u, 0x0004003du, 0x00000039u, 0x0000005au, 0x0000003bu,
    0x00050081u, 0x00000039u, 0x0000005bu, 0x00000059u, 0x0000005au, 0x00050050u, 0x00000039u,
    0x0000005du, 0x0000005cu, 0x0000005cu, 0x00050088u, 0x00000039u, 0x0000005eu, 0x0000005bu,
    0x0000005du, 0x0003003eu, 0x00000057u, 0x0000005eu, 0x0004003du, 0x00000061u, 0x00000064u,
    0x00000063u, 0x0004003du, 0x00000039u, 0x00000065u, 0x00000057u, 0x00050057u, 0x0000002au,
    0x00000066u, 0x00000064u, 0x00000065u, 0x0008004fu, 0x00000007u, 0x00000067u, 0x00000066u,
    0x00000066u, 0x00000000u, 0x00000001u, 0x00000002u, 0x0003003eu, 0x0000005fu, 0x00000067u,
    0x0004003du, 0x00000007u, 0x0000006bu, 0x0000006au, 0x0008000cu, 0x00000007u, 0x0000006eu,
    0x00000001u, 0x0000002bu, 0x0000006bu, 0x0000006cu, 0x0000006du, 0x0004003du, 0x00000007u,
    0x0000006fu, 0x0000005fu, 0x00050085u, 0x00000007u, 0x00000070u, 0x0000006eu, 0x0000006fu,
    0x0004003du, 0x00000006u, 0x00000071u, 0x00000026u, 0x0005008eu, 0x00000007u, 0x00000072u,
    0x00000070u, 0x00000071u, 0x00050051u, 0x00000006u, 0x00000073u, 0x00000072u, 0x00000000u,
    0x00050051u, 0x00000006u, 0x00000074u, 0x00000072u, 0x00000001u, 0x00050051u, 0x00000006u,
    0x00000075u, 0x00000072u, 0x00000002u, 0x00070050u, 0x0000002au, 0x00000076u, 0x00000073u,
    0x00000074u, 0x00000075u, 0x00000023u, 0x0003003eu, 0x00000069u, 0x00000076u, 0x000100fdu,
    0x00010038u,
};

// Depth QA shader: near red, far green (actual GPU gl_FragCoord.z).
static constexpr uint32_t ogl_dev_depth_frag_spv[] = {
    0x07230203u, 0x00010000u, 0x0008000bu, 0x00000020u, 0x00000000u, 0x00020011u, 0x00000001u,
    0x0006000bu, 0x00000001u, 0x4c534c47u, 0x6474732eu, 0x3035342eu, 0x00000000u, 0x0003000eu,
    0x00000000u, 0x00000001u, 0x0007000fu, 0x00000004u, 0x00000004u, 0x6e69616du, 0x00000000u,
    0x00000009u, 0x0000000bu, 0x00030010u, 0x00000004u, 0x00000007u, 0x00030003u, 0x00000002u,
    0x000001c2u, 0x00040005u, 0x00000004u, 0x6e69616du, 0x00000000u, 0x00040005u, 0x00000009u,
    0x6f6c6f63u, 0x00000072u, 0x00060005u, 0x0000000bu, 0x465f6c67u, 0x43676172u, 0x64726f6fu,
    0x00000000u, 0x00040047u, 0x00000009u, 0x0000001eu, 0x00000000u, 0x00040047u, 0x0000000bu,
    0x0000000bu, 0x0000000fu, 0x00020013u, 0x00000002u, 0x00030021u, 0x00000003u, 0x00000002u,
    0x00030016u, 0x00000006u, 0x00000020u, 0x00040017u, 0x00000007u, 0x00000006u, 0x00000004u,
    0x00040020u, 0x00000008u, 0x00000003u, 0x00000007u, 0x0004003bu, 0x00000008u, 0x00000009u,
    0x00000003u, 0x00040020u, 0x0000000au, 0x00000001u, 0x00000007u, 0x0004003bu, 0x0000000au,
    0x0000000bu, 0x00000001u, 0x00040015u, 0x0000000cu, 0x00000020u, 0x00000000u, 0x0004002bu,
    0x0000000cu, 0x0000000du, 0x00000002u, 0x00040020u, 0x0000000eu, 0x00000001u, 0x00000006u,
    0x0004002bu, 0x00000006u, 0x00000011u, 0x3f000000u, 0x00020014u, 0x00000012u, 0x0004002bu,
    0x00000006u, 0x00000014u, 0x3f6e147bu, 0x0004002bu, 0x00000006u, 0x00000015u, 0x3e428f5cu,
    0x0004002bu, 0x00000006u, 0x00000016u, 0x3e19999au, 0x0004002bu, 0x00000006u, 0x00000017u,
    0x3f800000u, 0x0007002cu, 0x00000007u, 0x00000018u, 0x00000014u, 0x00000015u, 0x00000016u,
    0x00000017u, 0x0004002bu, 0x00000006u, 0x00000019u, 0x3da3d70au, 0x0004002bu, 0x00000006u,
    0x0000001au, 0x3f47ae14u, 0x0004002bu, 0x00000006u, 0x0000001bu, 0x3e23d70au, 0x0007002cu,
    0x00000007u, 0x0000001cu, 0x00000019u, 0x0000001au, 0x0000001bu, 0x00000017u, 0x00040017u,
    0x0000001du, 0x00000012u, 0x00000004u, 0x00050036u, 0x00000002u, 0x00000004u, 0x00000000u,
    0x00000003u, 0x000200f8u, 0x00000005u, 0x00050041u, 0x0000000eu, 0x0000000fu, 0x0000000bu,
    0x0000000du, 0x0004003du, 0x00000006u, 0x00000010u, 0x0000000fu, 0x000500b8u, 0x00000012u,
    0x00000013u, 0x00000010u, 0x00000011u, 0x00070050u, 0x0000001du, 0x0000001eu, 0x00000013u,
    0x00000013u, 0x00000013u, 0x00000013u, 0x000600a9u, 0x00000007u, 0x0000001fu, 0x0000001eu,
    0x00000018u, 0x0000001cu, 0x0003003eu, 0x00000009u, 0x0000001fu, 0x000100fdu, 0x00010038u,
};

ogl_log_fn logger = nullptr;
// P05/3 developer-only exclusive X11 swapchain-present acceptance probe.
// Its caller is either the isolated native X11 QA window, or the real SDL
// viewer window strictly BEFORE GL context creation. It must NEVER run
// while OpenGL owns the window and is NOT a world renderer.
uint32_t present_exclusive_native_test_frame(uint32_t gl_released) {
#if defined(OGL_HAS_XLIB_WSI)
    const char* flag=std::getenv("OGL_VULKAN_EXCLUSIVE_PRESENT_PROBE");
    if (!flag || std::strcmp(flag,"1")!=0 || gl_released!=1 ||
        !world_device.swapchain_probe_ready ||
        !world_device.device || !world_device.queue || !world_device.swapchain ||
        world_device.swapchain_format==VK_FORMAT_UNDEFINED ||
        !world_device.swapchain_extent.width ||
        !world_device.swapchain_extent.height ||
        !world_device.device_proc) return 0;
    // Only isolated native QA can request up to sixteen sequential frames.
    // The Viewer never calls this ABI and does not request frame-loop mode.
    const char* loop=std::getenv("OGL_VULKAN_EXCLUSIVE_FRAME_LOOP");
    const bool multi_frame=loop && std::strcmp(loop,"1")==0;
    constexpr uint32_t max_native_frames=16;
    if (world_device.exclusive_present_used &&
        (!multi_frame || world_device.exclusive_present_count==0 ||
         world_device.exclusive_present_count>=max_native_frames))
        return 0;
    const uint32_t sequence=world_device.exclusive_present_count;
    const char* raster_env=std::getenv("OGL_VULKAN_EXCLUSIVE_RASTER_PROBE");
    const bool raster=raster_env && std::strcmp(raster_env,"1")==0;
    const char* depth_env=std::getenv("OGL_VULKAN_EXCLUSIVE_DEPTH_PROBE");
    const bool depth_test=depth_env && std::strcmp(depth_env,"1")==0;
    // A separate optical negative control uses the same Z-based fragment
    // colors WITHOUT a depth attachment. Far geometry submitted last must
    // overwrite near geometry. This never enables depth for real world frames.
    const char* depth_color_env=std::getenv(
        "OGL_VULKAN_EXCLUSIVE_DEPTH_COLOR_PROBE");
    const bool depth_color=depth_color_env &&
                            std::strcmp(depth_color_env,"1")==0;
    if ((depth_test || depth_color) && !raster) return 0;
    if (depth_test && depth_color) return 0; // controls remain exclusive
    const char* world_env=std::getenv("OGL_VULKAN_EXCLUSIVE_WORLD_PROBE");
    const bool world_projection=world_env && std::strcmp(world_env,"1")==0;
    if (world_projection &&
        (!raster || !world_device.world_camera_ready ||
         std::fabs(world_device.world_camera_aspect-
             float(world_device.swapchain_extent.width)/
             float(world_device.swapchain_extent.height))>0.01f))
        return 0;
    // A geometry pipeline must never silently degrade to solid-color clear.
    // Existing OpenSim vertex/index VkBuffers may be used only after a
    // validated clip-space diagnostic upload, not as world coordinates.
    if (raster && (!world_device.mesh_vertices || !world_device.mesh_indices ||
                   world_device.mesh_triangle_count==0 ||
                   world_device.mesh_triangle_count>OGL_RTX_MAX_SCENE_TRIANGLES))
        return 0;
    auto fn=[&](const char* name) {
        return world_device.device_proc(world_device.device,name);
    };
#define OGL_PRESENT_FN(name) auto name=reinterpret_cast<PFN_##name>(fn(#name))
    OGL_PRESENT_FN(vkGetSwapchainImagesKHR);
    OGL_PRESENT_FN(vkAcquireNextImageKHR);
    OGL_PRESENT_FN(vkQueuePresentKHR);
    OGL_PRESENT_FN(vkCreateSemaphore);
    OGL_PRESENT_FN(vkDestroySemaphore);
    OGL_PRESENT_FN(vkCreateFence);
    OGL_PRESENT_FN(vkDestroyFence);
    OGL_PRESENT_FN(vkWaitForFences);
    OGL_PRESENT_FN(vkCreateImageView);
    OGL_PRESENT_FN(vkDestroyImageView);
    OGL_PRESENT_FN(vkCreateRenderPass);
    OGL_PRESENT_FN(vkDestroyRenderPass);
    OGL_PRESENT_FN(vkCreateFramebuffer);
    OGL_PRESENT_FN(vkDestroyFramebuffer);
    OGL_PRESENT_FN(vkCreateCommandPool);
    OGL_PRESENT_FN(vkDestroyCommandPool);
    OGL_PRESENT_FN(vkAllocateCommandBuffers);
    OGL_PRESENT_FN(vkBeginCommandBuffer);
    OGL_PRESENT_FN(vkCmdBeginRenderPass);
    OGL_PRESENT_FN(vkCmdEndRenderPass);
    OGL_PRESENT_FN(vkEndCommandBuffer);
    OGL_PRESENT_FN(vkQueueSubmit);
    OGL_PRESENT_FN(vkQueueWaitIdle);
    OGL_PRESENT_FN(vkCreateShaderModule);
    OGL_PRESENT_FN(vkDestroyShaderModule);
    OGL_PRESENT_FN(vkCreatePipelineLayout);
    OGL_PRESENT_FN(vkDestroyPipelineLayout);
    OGL_PRESENT_FN(vkCreateGraphicsPipelines);
    OGL_PRESENT_FN(vkDestroyPipeline);
    OGL_PRESENT_FN(vkCmdBindPipeline);
    OGL_PRESENT_FN(vkCmdBindVertexBuffers);
    OGL_PRESENT_FN(vkCmdBindIndexBuffer);
    OGL_PRESENT_FN(vkCmdDrawIndexed);
    OGL_PRESENT_FN(vkCmdPushConstants);
    OGL_PRESENT_FN(vkCreateImage);
    OGL_PRESENT_FN(vkDestroyImage);
    OGL_PRESENT_FN(vkGetImageMemoryRequirements);
    OGL_PRESENT_FN(vkAllocateMemory);
    OGL_PRESENT_FN(vkFreeMemory);
    OGL_PRESENT_FN(vkBindImageMemory);
#undef OGL_PRESENT_FN
    if (!vkGetSwapchainImagesKHR || !vkAcquireNextImageKHR || !vkQueuePresentKHR ||
        !vkCreateSemaphore || !vkDestroySemaphore ||
        !vkCreateFence || !vkDestroyFence || !vkWaitForFences ||
        !vkCreateImageView || !vkDestroyImageView ||
        !vkCreateRenderPass || !vkDestroyRenderPass ||
        !vkCreateFramebuffer || !vkDestroyFramebuffer ||
        !vkCreateCommandPool || !vkDestroyCommandPool ||
        !vkAllocateCommandBuffers || !vkBeginCommandBuffer ||
        !vkCmdBeginRenderPass || !vkCmdEndRenderPass ||
        !vkEndCommandBuffer || !vkQueueSubmit || !vkQueueWaitIdle) return 0;
    if (raster && (!vkCreateShaderModule || !vkDestroyShaderModule ||
                   !vkCreatePipelineLayout || !vkDestroyPipelineLayout ||
                   !vkCreateGraphicsPipelines || !vkDestroyPipeline ||
                   !vkCmdBindPipeline || !vkCmdBindVertexBuffers ||
                   !vkCmdBindIndexBuffer || !vkCmdDrawIndexed))
        return 0;
    if (world_projection && !vkCmdPushConstants) return 0;
    const auto get_mem_properties=reinterpret_cast<
        PFN_vkGetPhysicalDeviceMemoryProperties>(
        world_device.instance_proc(world_device.instance,
            "vkGetPhysicalDeviceMemoryProperties"));
    const auto get_format_properties=reinterpret_cast<
        PFN_vkGetPhysicalDeviceFormatProperties>(
        world_device.instance_proc(world_device.instance,
            "vkGetPhysicalDeviceFormatProperties"));
    if (depth_test && (!vkCreateImage || !vkDestroyImage ||
                       !vkGetImageMemoryRequirements || !vkAllocateMemory ||
                       !vkFreeMemory || !vkBindImageMemory ||
                       !get_mem_properties || !get_format_properties))
        return 0;

    // A rejected frame is never retried against an acquired swapchain image.
    world_device.exclusive_present_used=true;
    VkSemaphore acquired=VK_NULL_HANDLE,rendered=VK_NULL_HANDLE;
    VkFence submit_fence=VK_NULL_HANDLE;
    VkImageView view=VK_NULL_HANDLE;
    VkRenderPass pass=VK_NULL_HANDLE;
    VkFramebuffer fb=VK_NULL_HANDLE;
    VkCommandPool pool=VK_NULL_HANDLE;
    VkShaderModule vertex_shader=VK_NULL_HANDLE,fragment_shader=VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout=VK_NULL_HANDLE;
    VkPipeline graphics_pipeline=VK_NULL_HANDLE;
    VkImage depth_image=VK_NULL_HANDLE;
    VkImageView depth_view=VK_NULL_HANDLE;
    VkDeviceMemory depth_memory=VK_NULL_HANDLE;
    VkFormat depth_format=VK_FORMAT_UNDEFINED;
    bool in_flight=false;
    auto cleanup=[&]() {
        if (graphics_pipeline) vkDestroyPipeline(
            world_device.device,graphics_pipeline,nullptr);
        if (pipeline_layout) vkDestroyPipelineLayout(
            world_device.device,pipeline_layout,nullptr);
        if (vertex_shader) vkDestroyShaderModule(
            world_device.device,vertex_shader,nullptr);
        if (fragment_shader) vkDestroyShaderModule(
            world_device.device,fragment_shader,nullptr);
        if (submit_fence) vkDestroyFence(world_device.device,submit_fence,nullptr);
        if (pool) vkDestroyCommandPool(world_device.device,pool,nullptr);
        if (fb) vkDestroyFramebuffer(world_device.device,fb,nullptr);
        if (pass) vkDestroyRenderPass(world_device.device,pass,nullptr);
        if (depth_view) vkDestroyImageView(
            world_device.device,depth_view,nullptr);
        if (depth_image) vkDestroyImage(
            world_device.device,depth_image,nullptr);
        if (depth_memory) vkFreeMemory(
            world_device.device,depth_memory,nullptr);
        if (view) vkDestroyImageView(world_device.device,view,nullptr);
        if (acquired) vkDestroySemaphore(world_device.device,acquired,nullptr);
        if (rendered) vkDestroySemaphore(world_device.device,rendered,nullptr);
    };
    auto run=[&]() -> bool {
        uint32_t count=0;
        if (vkGetSwapchainImagesKHR(world_device.device,world_device.swapchain,
                                   &count,nullptr)!=VK_SUCCESS ||
            count<2 || count>16) return false;
        std::vector<VkImage> images(count);
        if (vkGetSwapchainImagesKHR(world_device.device,world_device.swapchain,
                                   &count,images.data())!=VK_SUCCESS ||
            count<2) return false;
        VkSemaphoreCreateInfo si{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        if (vkCreateSemaphore(world_device.device,&si,nullptr,&acquired)!=VK_SUCCESS ||
            !acquired ||
            vkCreateSemaphore(world_device.device,&si,nullptr,&rendered)!=VK_SUCCESS ||
            !rendered) return false;
        uint32_t idx=UINT32_MAX;
        const auto acquire=vkAcquireNextImageKHR(world_device.device,
            world_device.swapchain,2000000000ULL,acquired,VK_NULL_HANDLE,&idx);
        if ((acquire!=VK_SUCCESS && acquire!=VK_SUBOPTIMAL_KHR) ||
            idx>=count) return false;

        VkImageViewCreateInfo iv{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        iv.image=images[idx];
        iv.viewType=VK_IMAGE_VIEW_TYPE_2D;
        iv.format=world_device.swapchain_format;
        iv.subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;
        iv.subresourceRange.levelCount=1;
        iv.subresourceRange.layerCount=1;
        if (vkCreateImageView(world_device.device,&iv,nullptr,&view)!=VK_SUCCESS ||
            !view) return false;

        // Dedicated depth image for ONLY the exclusive Vulkan dev probe.
        // No OpenGL depth/texture resources are borrowed or modified.
        if (depth_test) {
            for (VkFormat candidate: {VK_FORMAT_D32_SFLOAT,
                                      VK_FORMAT_D16_UNORM}) {
                VkFormatProperties format_props{};
                get_format_properties(world_device.physical_device,
                                      candidate,&format_props);
                if (format_props.optimalTilingFeatures &
                    VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                    depth_format=candidate;
                    break;
                }
            }
            if (depth_format==VK_FORMAT_UNDEFINED) return false;
            VkImageCreateInfo image_info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
            image_info.imageType=VK_IMAGE_TYPE_2D;
            image_info.format=depth_format;
            image_info.extent={world_device.swapchain_extent.width,
                               world_device.swapchain_extent.height,1};
            image_info.mipLevels=1;
            image_info.arrayLayers=1;
            image_info.samples=VK_SAMPLE_COUNT_1_BIT;
            image_info.tiling=VK_IMAGE_TILING_OPTIMAL;
            image_info.usage=VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            image_info.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
            image_info.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;
            if (vkCreateImage(world_device.device,&image_info,nullptr,
                              &depth_image)!=VK_SUCCESS || !depth_image)
                return false;
            VkMemoryRequirements requirements{};
            vkGetImageMemoryRequirements(world_device.device,depth_image,
                                         &requirements);
            VkPhysicalDeviceMemoryProperties memory_properties{};
            get_mem_properties(world_device.physical_device,
                               &memory_properties);
            uint32_t memory_index=UINT32_MAX;
            for (uint32_t i=0;i<memory_properties.memoryTypeCount && i<32;++i) {
                if (!(requirements.memoryTypeBits&(1u<<i))) continue;
                if (memory_index==UINT32_MAX) memory_index=i;
                if (memory_properties.memoryTypes[i].propertyFlags &
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
                    memory_index=i;
                    break;
                }
            }
            if (memory_index==UINT32_MAX) return false;
            VkMemoryAllocateInfo allocation{
                VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
            allocation.allocationSize=requirements.size;
            allocation.memoryTypeIndex=memory_index;
            if (vkAllocateMemory(world_device.device,&allocation,nullptr,
                                 &depth_memory)!=VK_SUCCESS || !depth_memory)
                return false;
            if (vkBindImageMemory(world_device.device,depth_image,
                                  depth_memory,0)!=VK_SUCCESS)
                return false;
            VkImageViewCreateInfo dvi{
                VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            dvi.image=depth_image;
            dvi.viewType=VK_IMAGE_VIEW_TYPE_2D;
            dvi.format=depth_format;
            dvi.subresourceRange.aspectMask=VK_IMAGE_ASPECT_DEPTH_BIT;
            dvi.subresourceRange.levelCount=1;
            dvi.subresourceRange.layerCount=1;
            if (vkCreateImageView(world_device.device,&dvi,nullptr,
                                  &depth_view)!=VK_SUCCESS || !depth_view)
                return false;
        }

        VkAttachmentDescription attachment{};
        attachment.format=world_device.swapchain_format;
        attachment.samples=VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment.storeOp=VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;
        attachment.finalLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        VkAttachmentDescription depth_attachment{};
        depth_attachment.format=depth_format;
        depth_attachment.samples=VK_SAMPLE_COUNT_1_BIT;
        depth_attachment.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth_attachment.storeOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depth_attachment.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;
        depth_attachment.finalLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        const VkAttachmentDescription attachments[2]={attachment,depth_attachment};
        VkAttachmentReference ref{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        VkAttachmentReference depth_ref{
            1,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount=1;
        subpass.pColorAttachments=&ref;
        subpass.pDepthStencilAttachment=depth_test ? &depth_ref : nullptr;
        VkRenderPassCreateInfo pi{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        pi.attachmentCount=depth_test ? 2u : 1u;
        pi.pAttachments=attachments;
        pi.subpassCount=1;
        pi.pSubpasses=&subpass;
        if (vkCreateRenderPass(world_device.device,&pi,nullptr,&pass)!=VK_SUCCESS ||
            !pass) return false;
        VkFramebufferCreateInfo fbi{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        fbi.renderPass=pass;
        const VkImageView framebuffer_views[2]={view,depth_view};
        fbi.attachmentCount=depth_test ? 2u : 1u;
        fbi.pAttachments=framebuffer_views;
        fbi.width=world_device.swapchain_extent.width;
        fbi.height=world_device.swapchain_extent.height;
        fbi.layers=1;
        if (vkCreateFramebuffer(world_device.device,&fbi,nullptr,&fb)!=VK_SUCCESS ||
            !fb) return false;

        // Optional genuine rasterization of indexed vertex data. The
        // normal single/16-frame clear tests use no shaders/pipelines.
        if (raster) {
            VkShaderModuleCreateInfo vertex_info{
                VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
            vertex_info.codeSize=world_projection
                ? sizeof(ogl_dev_world_vert_spv) : sizeof(ogl_dev_vert_spv);
            vertex_info.pCode=world_projection
                ? ogl_dev_world_vert_spv : ogl_dev_vert_spv;
            if (vkCreateShaderModule(world_device.device,&vertex_info,nullptr,
                                     &vertex_shader)!=VK_SUCCESS || !vertex_shader)
                return false;
            VkShaderModuleCreateInfo fragment_info{
                VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
            fragment_info.codeSize=(depth_test || depth_color)
                ? sizeof(ogl_dev_depth_frag_spv) : sizeof(ogl_dev_frag_spv);
            fragment_info.pCode=(depth_test || depth_color)
                ? ogl_dev_depth_frag_spv : ogl_dev_frag_spv;
            if (vkCreateShaderModule(world_device.device,&fragment_info,nullptr,
                                     &fragment_shader)!=VK_SUCCESS || !fragment_shader)
                return false;
            VkPipelineShaderStageCreateInfo stages[2]{};
            stages[0].sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stages[0].stage=VK_SHADER_STAGE_VERTEX_BIT;
            stages[0].module=vertex_shader;
            stages[0].pName="main";
            stages[1].sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stages[1].stage=VK_SHADER_STAGE_FRAGMENT_BIT;
            stages[1].module=fragment_shader;
            stages[1].pName="main";
            VkVertexInputBindingDescription vertex_binding{};
            vertex_binding.binding=0;
            vertex_binding.stride=sizeof(float)*3;
            vertex_binding.inputRate=VK_VERTEX_INPUT_RATE_VERTEX;
            VkVertexInputAttributeDescription vertex_attribute{};
            vertex_attribute.location=0;
            vertex_attribute.binding=0;
            vertex_attribute.format=VK_FORMAT_R32G32B32_SFLOAT;
            vertex_attribute.offset=0;
            VkPipelineVertexInputStateCreateInfo vertex_input{
                VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
            vertex_input.vertexBindingDescriptionCount=1;
            vertex_input.pVertexBindingDescriptions=&vertex_binding;
            vertex_input.vertexAttributeDescriptionCount=1;
            vertex_input.pVertexAttributeDescriptions=&vertex_attribute;
            VkPipelineInputAssemblyStateCreateInfo assembly{
                VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
            assembly.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            VkViewport viewport{};
            viewport.width=static_cast<float>(world_device.swapchain_extent.width);
            viewport.height=static_cast<float>(world_device.swapchain_extent.height);
            viewport.minDepth=0.f;
            viewport.maxDepth=1.f;
            VkRect2D scissor{};
            scissor.extent=world_device.swapchain_extent;
            VkPipelineViewportStateCreateInfo viewport_state{
                VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
            viewport_state.viewportCount=1;
            viewport_state.pViewports=&viewport;
            viewport_state.scissorCount=1;
            viewport_state.pScissors=&scissor;
            VkPipelineRasterizationStateCreateInfo raster_state{
                VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
            raster_state.polygonMode=VK_POLYGON_MODE_FILL;
            raster_state.cullMode=VK_CULL_MODE_NONE;
            raster_state.frontFace=VK_FRONT_FACE_COUNTER_CLOCKWISE;
            raster_state.lineWidth=1.f;
            VkPipelineMultisampleStateCreateInfo multisample{
                VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
            multisample.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
            VkPipelineDepthStencilStateCreateInfo depth_state{
                VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
            depth_state.depthTestEnable=VK_TRUE;
            depth_state.depthWriteEnable=VK_TRUE;
            depth_state.depthCompareOp=VK_COMPARE_OP_LESS;
            VkPipelineColorBlendAttachmentState color_attachment{};
            color_attachment.colorWriteMask=VK_COLOR_COMPONENT_R_BIT |
                VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                VK_COLOR_COMPONENT_A_BIT;
            VkPipelineColorBlendStateCreateInfo blend{
                VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
            blend.attachmentCount=1;
            blend.pAttachments=&color_attachment;
            VkPushConstantRange camera_range{};
            camera_range.stageFlags=VK_SHADER_STAGE_VERTEX_BIT;
            camera_range.offset=0;
            camera_range.size=sizeof(float)*16;
            VkPipelineLayoutCreateInfo layout_info{
                VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
            layout_info.pushConstantRangeCount=world_projection ? 1u : 0u;
            layout_info.pPushConstantRanges=world_projection
                ? &camera_range : nullptr;
            if (vkCreatePipelineLayout(world_device.device,&layout_info,nullptr,
                                       &pipeline_layout)!=VK_SUCCESS ||
                !pipeline_layout)
                return false;
            VkGraphicsPipelineCreateInfo graphics_info{
                VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
            graphics_info.stageCount=2;
            graphics_info.pStages=stages;
            graphics_info.pVertexInputState=&vertex_input;
            graphics_info.pInputAssemblyState=&assembly;
            graphics_info.pViewportState=&viewport_state;
            graphics_info.pRasterizationState=&raster_state;
            graphics_info.pMultisampleState=&multisample;
            graphics_info.pDepthStencilState=depth_test ? &depth_state : nullptr;
            graphics_info.pColorBlendState=&blend;
            graphics_info.layout=pipeline_layout;
            graphics_info.renderPass=pass;
            graphics_info.subpass=0;
            if (vkCreateGraphicsPipelines(world_device.device,VK_NULL_HANDLE,
                1,&graphics_info,nullptr,&graphics_pipeline)!=VK_SUCCESS ||
                !graphics_pipeline)
                return false;
        }

        VkCommandPoolCreateInfo pooli{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        pooli.queueFamilyIndex=world_device.queue_family;
        pooli.flags=VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        if (vkCreateCommandPool(world_device.device,&pooli,nullptr,&pool)!=VK_SUCCESS ||
            !pool) return false;
        VkCommandBuffer cmd=VK_NULL_HANDLE;
        VkCommandBufferAllocateInfo ai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        ai.commandPool=pool;
        ai.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        ai.commandBufferCount=1;
        if (vkAllocateCommandBuffers(world_device.device,&ai,&cmd)!=VK_SUCCESS ||
            !cmd) return false;
        VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        bi.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        if (vkBeginCommandBuffer(cmd,&bi)!=VK_SUCCESS) return false;
        VkClearValue clear{};
        // Alternating clear colors prove that consecutive GPU frames are
        // distinct and avoid accidentally treating a stale image as progress.
        clear.color.float32[0]=(sequence%2)==0 ? 0.08f : 0.65f;
        clear.color.float32[1]=(sequence%2)==0 ? 0.21f : 0.35f;
        clear.color.float32[2]=(sequence%2)==0 ? 0.45f : 0.12f;
        clear.color.float32[3]=1.f;
        VkClearValue clears[2]{clear,{}};
        clears[1].depthStencil.depth=1.f;
        VkRenderPassBeginInfo rbi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        rbi.renderPass=pass;
        rbi.framebuffer=fb;
        rbi.renderArea.extent=world_device.swapchain_extent;
        rbi.clearValueCount=depth_test ? 2u : 1u;
        rbi.pClearValues=clears;
        vkCmdBeginRenderPass(cmd,&rbi,VK_SUBPASS_CONTENTS_INLINE);
        if (raster) {
            vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,
                              graphics_pipeline);
            if (world_projection)
                vkCmdPushConstants(cmd,pipeline_layout,
                    VK_SHADER_STAGE_VERTEX_BIT,0,
                    sizeof(float)*16,world_device.world_to_clip.data());
            const VkDeviceSize vertex_offset=0;
            vkCmdBindVertexBuffers(cmd,0,1,&world_device.mesh_vertices,
                                   &vertex_offset);
            vkCmdBindIndexBuffer(cmd,world_device.mesh_indices,0,
                                 VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(cmd,world_device.mesh_triangle_count*3u,
                             1,0,0,0);
        }
        vkCmdEndRenderPass(cmd);
        if (vkEndCommandBuffer(cmd)!=VK_SUCCESS) return false;

        VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        if (vkCreateFence(world_device.device,&fi,nullptr,&submit_fence)!=VK_SUCCESS ||
            !submit_fence) return false;
        VkPipelineStageFlags stage=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo sub{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        sub.waitSemaphoreCount=1;
        sub.pWaitSemaphores=&acquired;
        sub.pWaitDstStageMask=&stage;
        sub.commandBufferCount=1;
        sub.pCommandBuffers=&cmd;
        sub.signalSemaphoreCount=1;
        sub.pSignalSemaphores=&rendered;
        if (vkQueueSubmit(world_device.queue,1,&sub,submit_fence)!=VK_SUCCESS)
            return false;
        in_flight=true;
        if (vkWaitForFences(world_device.device,1,&submit_fence,VK_TRUE,
                            2000000000ULL)!=VK_SUCCESS) return false;
        in_flight=false;
        VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        present.waitSemaphoreCount=1;
        present.pWaitSemaphores=&rendered;
        present.swapchainCount=1;
        present.pSwapchains=&world_device.swapchain;
        present.pImageIndices=&idx;
        // A present attempt may retain semaphore/image references even if
        // it reports OUT_OF_DATE or SUBOPTIMAL. Quarantine the resources
        // until the native QA queue has completed all submitted work.
        in_flight=true;
        const auto presented=vkQueuePresentKHR(world_device.queue,&present);
        // Vulkan 1.1 present-completion lacks a bounded wait. This exists
        // in the isolated QA subprocess, or the explicit pre-GL developer
        // startup probe. Do NOT use in an interactive renderer frame loop.
        if (vkQueueWaitIdle(world_device.queue)!=VK_SUCCESS) return false;
        in_flight=false;
        if (presented!=VK_SUCCESS && presented!=VK_SUBOPTIMAL_KHR)
            return false;
        if (logger)
            logger(1,depth_test
                ? "P05 Vulkan X11 GPU depth-buffer occlusion: depth test LESS with front/back fragment colors; dev only."
                : (world_projection
                    ? "P05 Vulkan X11 camera-projected indexed triangle: real GPU mat4 push constants and vkCmdDrawIndexed; dev only."
                    : (raster
                        ? "P05 Vulkan X11 indexed triangle raster: real VkGraphicsPipeline and vkCmdDrawIndexed; dev only."
                        : "P05 exclusive Vulkan X11: real swapchain acquire, clear and present; no concurrent OpenGL.")));
        return true;
    };
    const bool success=run();
    if (!in_flight) cleanup(); // do not free GPU resources still in flight
    if (success) ++world_device.exclusive_present_count;
    return success ? 1u : 0u;
#else
    (void)gl_released;
    return 0;
#endif
}

// P05/3 real persistent swapchain CLEAR path, developer-only and strictly
// pre-GL. Unlike the legacy draw diagnostic, the VkRenderPass, framebuffer
// per swapchain image, VkCommandPool, two command buffers, two acquired-image
// semaphores, two fences and image-indexed render-finished semaphores are
// created ONCE per attach and reused over multiple GPU frames.
//
// This is a WSI/frame ownership milestone, NOT the OpenSim world engine.
// Vulkan 1.0 provides no fence for present completion, hence render-finished
// semaphores belong to swapchain IMAGE indices and are reused only after
// reacquiring that image. One vkQueueWaitIdle is allowed on detach to retire
// outstanding presentations before destroying those semaphores.
uint32_t present_persistent_clear_frame(uint32_t gl_released) {
#if defined(OGL_HAS_XLIB_WSI)
    const char* enabled=std::getenv(
        "OGL_VULKAN_EXCLUSIVE_PERSISTENT_CLEAR_PROBE");
    const char* world_enabled=std::getenv(
        "OGL_VULKAN_EXCLUSIVE_PERSISTENT_WORLD_PROBE");
    const bool world=world_enabled && std::strcmp(world_enabled,"1")==0;
    if (world) {
        // Mesh/camera/graphics/depth flags MUST all be independently on.
        // No fallback to a solid-color clear when any input is missing.
        auto flagged=[](const char* name) {
            const char* value=std::getenv(name);
            return value && std::strcmp(value,"1")==0;
        };
        if (!flagged("OGL_VULKAN_EXCLUSIVE_RASTER_PROBE") ||
            !flagged("OGL_VULKAN_EXCLUSIVE_WORLD_PROBE") ||
            !flagged("OGL_VULKAN_EXCLUSIVE_DEPTH_PROBE") ||
            !flagged("OGL_VULKAN_WORLD_MESH_UPLOAD") ||
            !flagged("OGL_VULKAN_WORLD_CAMERA_PROBE") ||
            !world_device.world_camera_ready ||
            world_device.camera_mesh_generation!=world_device.mesh_generation ||
            !world_device.mesh_vertices || !world_device.mesh_indices ||
            !world_device.mesh_triangle_count ||
            world_device.mesh_triangle_count>OGL_RTX_MAX_SCENE_TRIANGLES ||
            !world_device.swapchain_extent.height ||
            std::fabs(world_device.world_camera_aspect-
                float(world_device.swapchain_extent.width)/
                float(world_device.swapchain_extent.height))>0.01f)
            return 0;
    }
    if (!enabled || std::strcmp(enabled,"1")!=0 ||
        gl_released!=1 || !world_device.swapchain_probe_ready ||
        !world_device.device || !world_device.queue ||
        !world_device.swapchain || !world_device.device_proc ||
        world_device.exclusive_present_used)
        return 0;
    auto& p=world_device.persistent;
    // A separately enabled native GPU acceptance run exercises 32 frames
    // on ONE VkSwapchain to expose fence/semaphore reuse beyond the usual
    // 16-frame QA ceiling. The normal pre-GL path remains capped at 16.
    if (p.blocked || (p.initialized && p.frame_count>=p.max_frames) ||
        (p.initialized && p.world_depth!=world)) return 0;
    auto proc=[&](const char* name) {
        return world_device.device_proc(world_device.device,name);
    };
#define OGL_PERSIST_FN(name) auto name=reinterpret_cast<PFN_##name>(proc(#name))
    OGL_PERSIST_FN(vkGetSwapchainImagesKHR);
    OGL_PERSIST_FN(vkCreateImageView);
    OGL_PERSIST_FN(vkCreateRenderPass);
    OGL_PERSIST_FN(vkCreateFramebuffer);
    OGL_PERSIST_FN(vkCreateCommandPool);
    OGL_PERSIST_FN(vkAllocateCommandBuffers);
    OGL_PERSIST_FN(vkCreateSemaphore);
    OGL_PERSIST_FN(vkCreateFence);
    OGL_PERSIST_FN(vkWaitForFences);
    OGL_PERSIST_FN(vkResetFences);
    OGL_PERSIST_FN(vkResetCommandBuffer);
    OGL_PERSIST_FN(vkBeginCommandBuffer);
    OGL_PERSIST_FN(vkCmdBeginRenderPass);
    OGL_PERSIST_FN(vkCmdEndRenderPass);
    OGL_PERSIST_FN(vkEndCommandBuffer);
    OGL_PERSIST_FN(vkAcquireNextImageKHR);
    OGL_PERSIST_FN(vkQueueSubmit);
    OGL_PERSIST_FN(vkQueuePresentKHR);
    OGL_PERSIST_FN(vkCreateImage);
    OGL_PERSIST_FN(vkGetImageMemoryRequirements);
    OGL_PERSIST_FN(vkAllocateMemory);
    OGL_PERSIST_FN(vkBindImageMemory);
    OGL_PERSIST_FN(vkCreateShaderModule);
    OGL_PERSIST_FN(vkCreatePipelineLayout);
    OGL_PERSIST_FN(vkCreateGraphicsPipelines);
    OGL_PERSIST_FN(vkCmdBindPipeline);
    OGL_PERSIST_FN(vkCmdBindVertexBuffers);
    OGL_PERSIST_FN(vkCmdBindIndexBuffer);
    OGL_PERSIST_FN(vkCmdDrawIndexed);
    OGL_PERSIST_FN(vkCmdPushConstants);
#undef OGL_PERSIST_FN
    if (!vkGetSwapchainImagesKHR || !vkCreateImageView ||
        !vkCreateRenderPass || !vkCreateFramebuffer ||
        !vkCreateCommandPool || !vkAllocateCommandBuffers ||
        !vkCreateSemaphore || !vkCreateFence || !vkWaitForFences ||
        !vkResetFences || !vkResetCommandBuffer ||
        !vkBeginCommandBuffer || !vkCmdBeginRenderPass ||
        !vkCmdEndRenderPass || !vkEndCommandBuffer ||
        !vkAcquireNextImageKHR || !vkQueueSubmit || !vkQueuePresentKHR)
        return 0;
    auto get_format_properties=reinterpret_cast<
        PFN_vkGetPhysicalDeviceFormatProperties>(
        world_device.instance_proc(world_device.instance,
            "vkGetPhysicalDeviceFormatProperties"));
    auto get_mem_properties=reinterpret_cast<
        PFN_vkGetPhysicalDeviceMemoryProperties>(
        world_device.instance_proc(world_device.instance,
            "vkGetPhysicalDeviceMemoryProperties"));
    if (world && (!vkCreateImage || !vkGetImageMemoryRequirements ||
        !vkAllocateMemory || !vkBindImageMemory ||
        !vkCreateShaderModule || !vkCreatePipelineLayout ||
        !vkCreateGraphicsPipelines || !vkCmdBindPipeline ||
        !vkCmdBindVertexBuffers || !vkCmdBindIndexBuffer ||
        !vkCmdDrawIndexed || !vkCmdPushConstants ||
        !get_format_properties || !get_mem_properties))
        return 0;

    if (!p.initialized) {
        auto init=[&]() -> bool {
            uint32_t count=0;
            if (vkGetSwapchainImagesKHR(world_device.device,
                    world_device.swapchain,&count,nullptr)!=VK_SUCCESS ||
                count<2 || count>16) return false;
            // The active renderer must use the same verified image set as
            // passive WSI attach: count-only success is insufficient.
            const uint32_t expected_count=count;
            std::vector<VkImage> images(expected_count,VK_NULL_HANDLE);
            if (vkGetSwapchainImagesKHR(world_device.device,
                    world_device.swapchain,&count,images.data())!=VK_SUCCESS ||
                count!=expected_count ||
                std::any_of(images.begin(),images.end(),
                    [](VkImage image){ return image==VK_NULL_HANDLE; }))
                return false;
            p.image_count=expected_count;
            p.world_depth=world;
            if (world) {
                for (VkFormat format:{VK_FORMAT_D32_SFLOAT,
                                     VK_FORMAT_D16_UNORM}) {
                    VkFormatProperties properties{};
                    get_format_properties(world_device.physical_device,
                        format,&properties);
                    if (properties.optimalTilingFeatures &
                        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                        p.depth_format=format; break;
                    }
                }
                if (p.depth_format==VK_FORMAT_UNDEFINED) return false;
                p.depth_images.resize(count,VK_NULL_HANDLE);
                p.depth_memories.resize(count,VK_NULL_HANDLE);
                p.depth_views.resize(count,VK_NULL_HANDLE);
            }
            p.views.resize(count,VK_NULL_HANDLE);
            p.framebuffers.resize(count,VK_NULL_HANDLE);
            p.presented_by_image.resize(count,VK_NULL_HANDLE);
            p.image_in_flight.resize(count,VK_NULL_HANDLE);
            VkAttachmentDescription color{};
            color.format=world_device.swapchain_format;
            color.samples=VK_SAMPLE_COUNT_1_BIT;
            color.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;
            color.storeOp=VK_ATTACHMENT_STORE_OP_STORE;
            color.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            color.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
            color.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;
            color.finalLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            VkAttachmentDescription depth_attachment{};
            depth_attachment.format=p.depth_format;
            depth_attachment.samples=VK_SAMPLE_COUNT_1_BIT;
            depth_attachment.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;
            depth_attachment.storeOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depth_attachment.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            depth_attachment.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depth_attachment.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;
            depth_attachment.finalLayout=
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            const VkAttachmentDescription attachments[2]{
                color,depth_attachment};
            VkAttachmentReference ref{0,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
            VkAttachmentReference depth_ref{1,
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
            VkSubpassDescription subpass{};
            subpass.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount=1;
            subpass.pColorAttachments=&ref;
            subpass.pDepthStencilAttachment=world ? &depth_ref : nullptr;
            VkRenderPassCreateInfo ri{
                VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
            ri.attachmentCount=world ? 2u : 1u;
            ri.pAttachments=attachments;
            ri.subpassCount=1;
            ri.pSubpasses=&subpass;
            if (vkCreateRenderPass(world_device.device,&ri,nullptr,
                                   &p.pass)!=VK_SUCCESS || !p.pass)
                return false;
            VkSemaphoreCreateInfo si{
                VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
            for (uint32_t i=0;i<count;++i) {
                VkImageViewCreateInfo vi{
                    VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
                vi.image=images[i];
                vi.viewType=VK_IMAGE_VIEW_TYPE_2D;
                vi.format=world_device.swapchain_format;
                vi.subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;
                vi.subresourceRange.levelCount=1;
                vi.subresourceRange.layerCount=1;
                if (vkCreateImageView(world_device.device,&vi,nullptr,
                                      &p.views[i])!=VK_SUCCESS ||
                    !p.views[i]) return false;
                if (world) {
                    // Each acquired swapchain image gets its OWN depth
                    // attachment, so frames in flight cannot race Z writes.
                    VkImageCreateInfo di{
                        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
                    di.imageType=VK_IMAGE_TYPE_2D;
                    di.format=p.depth_format;
                    di.extent={world_device.swapchain_extent.width,
                               world_device.swapchain_extent.height,1};
                    di.mipLevels=1; di.arrayLayers=1;
                    di.samples=VK_SAMPLE_COUNT_1_BIT;
                    di.tiling=VK_IMAGE_TILING_OPTIMAL;
                    di.usage=VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
                    di.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
                    if (vkCreateImage(world_device.device,&di,nullptr,
                                      &p.depth_images[i])!=VK_SUCCESS ||
                        !p.depth_images[i]) return false;
                    VkMemoryRequirements req{};
                    vkGetImageMemoryRequirements(world_device.device,
                        p.depth_images[i],&req);
                    VkPhysicalDeviceMemoryProperties properties{};
                    get_mem_properties(world_device.physical_device,
                        &properties);
                    uint32_t type=UINT32_MAX;
                    for (uint32_t m=0;m<properties.memoryTypeCount &&
                         m<32;++m) {
                        if (!(req.memoryTypeBits&(1u<<m))) continue;
                        if (type==UINT32_MAX) type=m;
                        if (properties.memoryTypes[m].propertyFlags &
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
                            type=m;break;
                        }
                    }
                    if (type==UINT32_MAX) return false;
                    VkMemoryAllocateInfo alloc{
                        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
                    alloc.allocationSize=req.size;
                    alloc.memoryTypeIndex=type;
                    if (vkAllocateMemory(world_device.device,&alloc,nullptr,
                                         &p.depth_memories[i])!=VK_SUCCESS ||
                        !p.depth_memories[i]) return false;
                    if (vkBindImageMemory(world_device.device,
                        p.depth_images[i],p.depth_memories[i],0)!=VK_SUCCESS)
                        return false;
                    VkImageViewCreateInfo dv{
                        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
                    dv.image=p.depth_images[i];
                    dv.viewType=VK_IMAGE_VIEW_TYPE_2D;
                    dv.format=p.depth_format;
                    dv.subresourceRange.aspectMask=VK_IMAGE_ASPECT_DEPTH_BIT;
                    dv.subresourceRange.levelCount=1;
                    dv.subresourceRange.layerCount=1;
                    if (vkCreateImageView(world_device.device,&dv,nullptr,
                                          &p.depth_views[i])!=VK_SUCCESS ||
                        !p.depth_views[i]) return false;
                }
                const VkImageView framebuffer_views[2]{
                    p.views[i],world ? p.depth_views[i] : VK_NULL_HANDLE};
                VkFramebufferCreateInfo fi{
                    VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
                fi.renderPass=p.pass;
                fi.attachmentCount=world ? 2u : 1u;
                fi.pAttachments=framebuffer_views;
                fi.width=world_device.swapchain_extent.width;
                fi.height=world_device.swapchain_extent.height;
                fi.layers=1;
                if (vkCreateFramebuffer(world_device.device,&fi,nullptr,
                                        &p.framebuffers[i])!=VK_SUCCESS ||
                    !p.framebuffers[i]) return false;
                if (vkCreateSemaphore(world_device.device,&si,nullptr,
                                      &p.presented_by_image[i])!=VK_SUCCESS ||
                    !p.presented_by_image[i]) return false;
            }
        // Optional genuine rasterization of indexed vertex data. The
        // normal single/16-frame clear tests use no shaders/pipelines.
        if (world) {
            VkShaderModuleCreateInfo vertex_info{
                VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
            vertex_info.codeSize=true
                ? sizeof(ogl_dev_world_vert_spv) : sizeof(ogl_dev_vert_spv);
            vertex_info.pCode=true
                ? ogl_dev_world_vert_spv : ogl_dev_vert_spv;
            if (vkCreateShaderModule(world_device.device,&vertex_info,nullptr,
                                     &p.vertex_shader)!=VK_SUCCESS || !p.vertex_shader)
                return false;
            VkShaderModuleCreateInfo fragment_info{
                VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
            fragment_info.codeSize=true
                ? sizeof(ogl_dev_depth_frag_spv) : sizeof(ogl_dev_frag_spv);
            fragment_info.pCode=true
                ? ogl_dev_depth_frag_spv : ogl_dev_frag_spv;
            if (vkCreateShaderModule(world_device.device,&fragment_info,nullptr,
                                     &p.fragment_shader)!=VK_SUCCESS || !p.fragment_shader)
                return false;
            VkPipelineShaderStageCreateInfo stages[2]{};
            stages[0].sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stages[0].stage=VK_SHADER_STAGE_VERTEX_BIT;
            stages[0].module=p.vertex_shader;
            stages[0].pName="main";
            stages[1].sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stages[1].stage=VK_SHADER_STAGE_FRAGMENT_BIT;
            stages[1].module=p.fragment_shader;
            stages[1].pName="main";
            VkVertexInputBindingDescription vertex_binding{};
            vertex_binding.binding=0;
            vertex_binding.stride=sizeof(float)*3;
            vertex_binding.inputRate=VK_VERTEX_INPUT_RATE_VERTEX;
            VkVertexInputAttributeDescription vertex_attribute{};
            vertex_attribute.location=0;
            vertex_attribute.binding=0;
            vertex_attribute.format=VK_FORMAT_R32G32B32_SFLOAT;
            vertex_attribute.offset=0;
            VkPipelineVertexInputStateCreateInfo vertex_input{
                VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
            vertex_input.vertexBindingDescriptionCount=1;
            vertex_input.pVertexBindingDescriptions=&vertex_binding;
            vertex_input.vertexAttributeDescriptionCount=1;
            vertex_input.pVertexAttributeDescriptions=&vertex_attribute;
            VkPipelineInputAssemblyStateCreateInfo assembly{
                VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
            assembly.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            VkViewport viewport{};
            viewport.width=static_cast<float>(world_device.swapchain_extent.width);
            viewport.height=static_cast<float>(world_device.swapchain_extent.height);
            viewport.minDepth=0.f;
            viewport.maxDepth=1.f;
            VkRect2D scissor{};
            scissor.extent=world_device.swapchain_extent;
            VkPipelineViewportStateCreateInfo viewport_state{
                VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
            viewport_state.viewportCount=1;
            viewport_state.pViewports=&viewport;
            viewport_state.scissorCount=1;
            viewport_state.pScissors=&scissor;
            VkPipelineRasterizationStateCreateInfo raster_state{
                VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
            raster_state.polygonMode=VK_POLYGON_MODE_FILL;
            raster_state.cullMode=VK_CULL_MODE_NONE;
            raster_state.frontFace=VK_FRONT_FACE_COUNTER_CLOCKWISE;
            raster_state.lineWidth=1.f;
            VkPipelineMultisampleStateCreateInfo multisample{
                VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
            multisample.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
            VkPipelineDepthStencilStateCreateInfo depth_state{
                VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
            depth_state.depthTestEnable=VK_TRUE;
            depth_state.depthWriteEnable=VK_TRUE;
            depth_state.depthCompareOp=VK_COMPARE_OP_LESS;
            VkPipelineColorBlendAttachmentState color_attachment{};
            color_attachment.colorWriteMask=VK_COLOR_COMPONENT_R_BIT |
                VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                VK_COLOR_COMPONENT_A_BIT;
            VkPipelineColorBlendStateCreateInfo blend{
                VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
            blend.attachmentCount=1;
            blend.pAttachments=&color_attachment;
            VkPushConstantRange camera_range{};
            camera_range.stageFlags=VK_SHADER_STAGE_VERTEX_BIT;
            camera_range.offset=0;
            camera_range.size=sizeof(float)*16;
            VkPipelineLayoutCreateInfo layout_info{
                VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
            layout_info.pushConstantRangeCount=1u;
            layout_info.pPushConstantRanges=true
                ? &camera_range : nullptr;
            if (vkCreatePipelineLayout(world_device.device,&layout_info,nullptr,
                                       &p.pipeline_layout)!=VK_SUCCESS ||
                !p.pipeline_layout)
                return false;
            VkGraphicsPipelineCreateInfo graphics_info{
                VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
            graphics_info.stageCount=2;
            graphics_info.pStages=stages;
            graphics_info.pVertexInputState=&vertex_input;
            graphics_info.pInputAssemblyState=&assembly;
            graphics_info.pViewportState=&viewport_state;
            graphics_info.pRasterizationState=&raster_state;
            graphics_info.pMultisampleState=&multisample;
            graphics_info.pDepthStencilState=&depth_state;
            graphics_info.pColorBlendState=&blend;
            graphics_info.layout=p.pipeline_layout;
            graphics_info.renderPass=p.pass;
            graphics_info.subpass=0;
            if (vkCreateGraphicsPipelines(world_device.device,VK_NULL_HANDLE,
                1,&graphics_info,nullptr,&p.pipeline)!=VK_SUCCESS ||
                !p.pipeline)
                return false;
        }


            VkCommandPoolCreateInfo ci{
                VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
            ci.queueFamilyIndex=world_device.queue_family;
            ci.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            if (vkCreateCommandPool(world_device.device,&ci,nullptr,
                                    &p.pool)!=VK_SUCCESS || !p.pool)
                return false;
            VkCommandBufferAllocateInfo ai{
                VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
            ai.commandPool=p.pool;
            ai.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            ai.commandBufferCount=2;
            if (vkAllocateCommandBuffers(world_device.device,&ai,
                                         p.commands.data())!=VK_SUCCESS)
                return false;
            VkFenceCreateInfo fence_info{
                VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
            fence_info.flags=VK_FENCE_CREATE_SIGNALED_BIT;
            for (uint32_t i=0;i<2;++i) {
                if (vkCreateSemaphore(world_device.device,&si,nullptr,
                                      &p.image_acquired[i])!=VK_SUCCESS ||
                    !p.image_acquired[i]) return false;
                if (vkCreateFence(world_device.device,&fence_info,nullptr,
                                  &p.submit_fences[i])!=VK_SUCCESS ||
                    !p.submit_fences[i]) return false;
            }
            return true;
        };
        if (!init()) {
            world_device.release_persistent_frames();
            return 0;
        }
        // Only the native X11 stress harness can request 32 frames.
        // Freeze that decision once per initialized swapchain, so changing
        // process flags cannot extend a live presentation sequence.
        const char* long_run=std::getenv("OGL_VULKAN_PERSISTENT_LONG_RUN_PROBE");
        const char* native_test=std::getenv("OGL_VULKAN_PRESENT_TEST_X11");
        p.max_frames=long_run && std::strcmp(long_run,"1")==0 &&
            native_test && std::strcmp(native_test,"1")==0 ? 32u : 16u;
        p.initialized=true;
        ++world_device.persistent_builds;
    }

    if (p.frame_count>=p.max_frames) return 0;
    // A borrowed SDL/X11 window can resize without first detaching this
    // developer-only swapchain. Never submit to a known stale fixed extent.
    // The caller must detach/recreate; this path deliberately cannot recover
    // ownership behind SDL's back. A variable extent remains driver-owned.
    // Even when WSI reports a variable currentExtent (UINT32_MAX), the
    // actual SDL-owned X11 drawable must still match this swapchain.
    // Its borrowed XID is valid only while SDL retains window ownership.
    XWindowAttributes drawable{};
    if (!world_device.borrowed_xlib_display ||
        !world_device.borrowed_xlib_window ||
        !XGetWindowAttributes(world_device.borrowed_xlib_display,
                              world_device.borrowed_xlib_window,&drawable) ||
        drawable.map_state!=IsViewable ||
        drawable.width<=0 || drawable.height<=0 ||
        static_cast<uint32_t>(drawable.width)!=world_device.swapchain_extent.width ||
        static_cast<uint32_t>(drawable.height)!=world_device.swapchain_extent.height) {
        p.blocked=true;
        return 0;
    }
    const auto get_live_caps=reinterpret_cast<
        PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>(
            world_device.instance_proc(world_device.instance,
                "vkGetPhysicalDeviceSurfaceCapabilitiesKHR"));
    VkSurfaceCapabilitiesKHR live_caps{};
    if (!get_live_caps ||
        get_live_caps(world_device.physical_device,world_device.surface,
                      &live_caps)!=VK_SUCCESS ||
        !(live_caps.supportedUsageFlags&VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) ||
        !(live_caps.supportedCompositeAlpha&world_device.swapchain_composite_alpha) ||
        !live_caps.maxImageArrayLayers ||
        !live_caps.minImageCount ||
        p.image_count<live_caps.minImageCount ||
        (live_caps.maxImageCount && p.image_count>live_caps.maxImageCount) ||
        !(live_caps.supportedTransforms&world_device.swapchain_transform) ||
        live_caps.currentTransform!=world_device.swapchain_transform ||
        !live_caps.minImageExtent.width || !live_caps.minImageExtent.height ||
        live_caps.minImageExtent.width>live_caps.maxImageExtent.width ||
        live_caps.minImageExtent.height>live_caps.maxImageExtent.height ||
        world_device.swapchain_extent.width<live_caps.minImageExtent.width ||
        world_device.swapchain_extent.height<live_caps.minImageExtent.height ||
        world_device.swapchain_extent.width>live_caps.maxImageExtent.width ||
        world_device.swapchain_extent.height>live_caps.maxImageExtent.height ||
        ((live_caps.currentExtent.width==UINT32_MAX) !=
         (live_caps.currentExtent.height==UINT32_MAX)) ||
        (live_caps.currentExtent.width!=UINT32_MAX &&
         (live_caps.currentExtent.width!=world_device.swapchain_extent.width ||
          live_caps.currentExtent.height!=world_device.swapchain_extent.height))) {
        p.blocked=true;
        return 0;
    }
    // A valid extent alone does not guarantee that this swapchain's color
    // format and color space are STILL advertised for this Vulkan surface.
    // Validate the pair as one unit; a format with a different color space
    // must never be accepted as an equivalent attachment.
    const auto get_live_formats=reinterpret_cast<
        PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>(
            world_device.instance_proc(world_device.instance,
                "vkGetPhysicalDeviceSurfaceFormatsKHR"));
    uint32_t live_format_count=0;
    if (!get_live_formats ||
        get_live_formats(world_device.physical_device,world_device.surface,
                         &live_format_count,nullptr)!=VK_SUCCESS ||
        !live_format_count || live_format_count>256) {
        p.blocked=true;
        return 0;
    }
    std::vector<VkSurfaceFormatKHR> live_formats(live_format_count);
    const uint32_t expected_live_formats=live_format_count;
    if (get_live_formats(world_device.physical_device,world_device.surface,
                         &live_format_count,live_formats.data())!=VK_SUCCESS ||
        live_format_count!=expected_live_formats ||
        !std::any_of(live_formats.begin(),live_formats.end(),
            [expected_live_formats](const VkSurfaceFormatKHR& available) {
                // Vulkan allows a sole VK_FORMAT_UNDEFINED to mean that
                // the app may retain its concrete format in this color space.
                return (available.format==world_device.swapchain_format ||
                    (expected_live_formats==1 &&
                     available.format==VK_FORMAT_UNDEFINED)) &&
                    available.colorSpace==world_device.swapchain_color_space;
            })) {
        p.blocked=true;
        return 0;
    }
    // Native QA can simulate a surface losing queue/FIFO permission
    // AFTER a real GPU present. This is never an SDL/UI setting.
    const char* lost_native_qa=std::getenv("OGL_VULKAN_PRESENT_TEST_X11");
    const char* lost_present_qa=std::getenv(
        "OGL_VULKAN_PERSISTENT_PRESENT_SUPPORT_LOST_TEST_X11");
    if (p.frame_count>0 && lost_native_qa &&
        std::strcmp(lost_native_qa,"1")==0 && lost_present_qa &&
        std::strcmp(lost_present_qa,"1")==0) {
        p.blocked=true;
        return 0;
    }
    // The FIFO present queue and surface support were valid at ATTACH,
    // but a window-system transition can invalidate that capability while
    // our pre-GL developer swapchain remains borrowed from SDL. Never
    // acquire or submit with an unsupported present queue/mode.
    const auto live_present_support=reinterpret_cast<
        PFN_vkGetPhysicalDeviceSurfaceSupportKHR>(
            world_device.instance_proc(world_device.instance,
                "vkGetPhysicalDeviceSurfaceSupportKHR"));
    const auto live_present_modes=reinterpret_cast<
        PFN_vkGetPhysicalDeviceSurfacePresentModesKHR>(
            world_device.instance_proc(world_device.instance,
                "vkGetPhysicalDeviceSurfacePresentModesKHR"));
    VkBool32 queue_still_supported=VK_FALSE;
    uint32_t live_mode_count=0;
    if (!live_present_support || !live_present_modes ||
        live_present_support(world_device.physical_device,
            world_device.queue_family,world_device.surface,
            &queue_still_supported)!=VK_SUCCESS ||
        queue_still_supported!=VK_TRUE ||
        live_present_modes(world_device.physical_device,world_device.surface,
            &live_mode_count,nullptr)!=VK_SUCCESS ||
        !live_mode_count || live_mode_count>256) {
        p.blocked=true;
        return 0;
    }
    std::vector<VkPresentModeKHR> active_modes(live_mode_count);
    const uint32_t expected_modes=live_mode_count;
    if (live_present_modes(world_device.physical_device,world_device.surface,
            &live_mode_count,active_modes.data())!=VK_SUCCESS ||
        live_mode_count!=expected_modes ||
        std::find(active_modes.begin(),active_modes.end(),
                  VK_PRESENT_MODE_FIFO_KHR)==active_modes.end()) {
        p.blocked=true;
        return 0;
    }
    const uint32_t slot=p.frame_count%2u;
    const VkFence slot_fence=p.submit_fences[slot];
    // Reuse a CPU frame slot only after its prior GPU submit completed.
    // Timeouts do not release potentially in-flight command buffers.
    if (vkWaitForFences(world_device.device,1,&slot_fence,
                        VK_TRUE,2000000000ULL)!=VK_SUCCESS) {
        p.blocked=true;
        return 0;
    }
    p.submit_pending[slot]=false;
    uint32_t image_index=UINT32_MAX;
    const VkResult acquired=vkAcquireNextImageKHR(world_device.device,
        world_device.swapchain,2000000000ULL,
        p.image_acquired[slot],VK_NULL_HANDLE,&image_index);
    if ((acquired!=VK_SUCCESS && acquired!=VK_SUBOPTIMAL_KHR) ||
        image_index>=p.image_count) {
        p.blocked=true;
        return 0;
    }
    // VK_SUBOPTIMAL_KHR can still return a signaled acquire semaphore.
    // Complete that frame rather than abandoning the acquired image, but
    // require a new swapchain before accepting another GPU submission.
    const bool acquire_suboptimal=acquired==VK_SUBOPTIMAL_KHR;
    const VkFence image_fence=p.image_in_flight[image_index];
    if (image_fence && image_fence!=slot_fence &&
        vkWaitForFences(world_device.device,1,&image_fence,
                        VK_TRUE,2000000000ULL)!=VK_SUCCESS) {
        p.blocked=true;
        return 0;
    }
    const VkCommandBuffer cmd=p.commands[slot];
    if (vkResetCommandBuffer(cmd,0)!=VK_SUCCESS) {
        p.blocked=true;
        return 0;
    }
    VkCommandBufferBeginInfo bi{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    bi.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (vkBeginCommandBuffer(cmd,&bi)!=VK_SUCCESS) {
        p.blocked=true;
        return 0;
    }
    VkClearValue clear{};
    clear.color.float32[0]=(p.frame_count%2)==0 ? 0.08f : 0.65f;
    clear.color.float32[1]=(p.frame_count%2)==0 ? 0.21f : 0.35f;
    clear.color.float32[2]=(p.frame_count%2)==0 ? 0.45f : 0.12f;
    clear.color.float32[3]=1.f;
    VkRenderPassBeginInfo rbi{
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    rbi.renderPass=p.pass;
    rbi.framebuffer=p.framebuffers[image_index];
    rbi.renderArea.extent=world_device.swapchain_extent;
    VkClearValue clears[2]{clear,{}};
    clears[1].depthStencil.depth=1.f;
    rbi.clearValueCount=world ? 2u : 1u;
    rbi.pClearValues=clears;
    vkCmdBeginRenderPass(cmd,&rbi,VK_SUBPASS_CONTENTS_INLINE);
    if (world) {
        vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,p.pipeline);
        vkCmdPushConstants(cmd,p.pipeline_layout,VK_SHADER_STAGE_VERTEX_BIT,
            0,sizeof(float)*16,world_device.world_to_clip.data());
        const VkDeviceSize offset=0;
        vkCmdBindVertexBuffers(cmd,0,1,&world_device.mesh_vertices,&offset);
        vkCmdBindIndexBuffer(cmd,world_device.mesh_indices,0,
                             VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmd,world_device.mesh_triangle_count*3u,1,0,0,0);
    }
    vkCmdEndRenderPass(cmd);
    if (vkEndCommandBuffer(cmd)!=VK_SUCCESS ||
        vkResetFences(world_device.device,1,&slot_fence)!=VK_SUCCESS) {
        p.blocked=true;
        return 0;
    }
    VkPipelineStageFlags stage=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.waitSemaphoreCount=1;
    submit.pWaitSemaphores=&p.image_acquired[slot];
    submit.pWaitDstStageMask=&stage;
    submit.commandBufferCount=1;
    submit.pCommandBuffers=&cmd;
    submit.signalSemaphoreCount=1;
    // Render-complete is PER SWAPCHAIN IMAGE, never per CPU frame slot.
    submit.pSignalSemaphores=&p.presented_by_image[image_index];
    if (vkQueueSubmit(world_device.queue,1,&submit,slot_fence)!=VK_SUCCESS) {
        p.blocked=true;
        return 0;
    }
    p.submitted_any=true;
    p.submit_pending[slot]=true;
    if (world) p.submitted_mesh_generations[slot]=world_device.mesh_generation;
    p.image_in_flight[image_index]=slot_fence;
    VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present.waitSemaphoreCount=1;
    present.pWaitSemaphores=&p.presented_by_image[image_index];
    present.swapchainCount=1;
    present.pSwapchains=&world_device.swapchain;
    present.pImageIndices=&image_index;
    const VkResult presented=vkQueuePresentKHR(world_device.queue,&present);
    if (presented!=VK_SUCCESS && presented!=VK_SUBOPTIMAL_KHR) {
        p.blocked=true;
        return 0;
    }
    ++p.frame_count;
    ++world_device.persistent_total_presents;
    // This fault injection happens only AFTER an actual successful GPU
    // present, under two independent native-X11 QA permissions. Never
    // simulate a successful present or affect normal SDL/GL startup.
    const char* native_qa=std::getenv("OGL_VULKAN_PRESENT_TEST_X11");
    const char* force_suboptimal=std::getenv(
        "OGL_VULKAN_PERSISTENT_SUBOPTIMAL_TEST_X11");
    const bool injected_suboptimal=native_qa &&
        std::strcmp(native_qa,"1")==0 && force_suboptimal &&
        std::strcmp(force_suboptimal,"1")==0;
    if (acquire_suboptimal || presented==VK_SUBOPTIMAL_KHR ||
        injected_suboptimal) p.blocked=true;
    // Crucially NO vkQueueWaitIdle in the frame function.
    return 1;
#else
    (void)gl_released;
    return 0;
#endif
}

// P05/3 foundation: exercise the REAL Vulkan graphics queue with two
// independent primary command buffers and completion fences. NO swapchain
// acquisition/presentation, render passes, shaders or OpenGL resources.
// This diagnostic cannot satisfy the full Vulkan world-frame acceptance gate.
uint32_t submit_world_sync_probe() {
    // If a real driver reports a lost/stalled submission, do not reissue
    // work. The tiny probe resources remain quarantined until device
    // teardown rather than blocking the GL viewer on vkQueueWaitIdle.
    static bool disabled_after_gpu_timeout=false;
    if (disabled_after_gpu_timeout) return 0;
    const char* enabled=std::getenv("OGL_VULKAN_FRAME_SYNC_PROBE");
    if (!enabled || std::strcmp(enabled,"1")!=0 ||
        !world_device.device || !world_device.queue ||
        world_device.queue_family==UINT32_MAX || !world_device.device_proc)
        return 0;
    const auto proc=[&](const char* name) {
        return world_device.device_proc(world_device.device,name);
    };
    const auto create_pool=reinterpret_cast<PFN_vkCreateCommandPool>(
        proc("vkCreateCommandPool"));
    const auto destroy_pool=reinterpret_cast<PFN_vkDestroyCommandPool>(
        proc("vkDestroyCommandPool"));
    const auto alloc=reinterpret_cast<PFN_vkAllocateCommandBuffers>(
        proc("vkAllocateCommandBuffers"));
    const auto begin=reinterpret_cast<PFN_vkBeginCommandBuffer>(
        proc("vkBeginCommandBuffer"));
    const auto end=reinterpret_cast<PFN_vkEndCommandBuffer>(
        proc("vkEndCommandBuffer"));
    const auto create_fence=reinterpret_cast<PFN_vkCreateFence>(
        proc("vkCreateFence"));
    const auto destroy_fence=reinterpret_cast<PFN_vkDestroyFence>(
        proc("vkDestroyFence"));
    const auto submit=reinterpret_cast<PFN_vkQueueSubmit>(
        proc("vkQueueSubmit"));
    const auto wait=reinterpret_cast<PFN_vkWaitForFences>(
        proc("vkWaitForFences"));
    if (!create_pool || !destroy_pool || !alloc || !begin || !end ||
        !create_fence || !destroy_fence || !submit || !wait)
        return 0;
    VkCommandPool pool=VK_NULL_HANDLE;
    std::array<VkCommandBuffer,2> commands{};
    std::array<VkFence,2> fences{};
    VkCommandPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pool_info.queueFamilyIndex=world_device.queue_family;
    pool_info.flags=VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    if (create_pool(world_device.device,&pool_info,nullptr,&pool)!=VK_SUCCESS ||
        !pool)
        return 0;
    auto cleanup=[&]() {
        for (VkFence fence : fences)
            if (fence) destroy_fence(world_device.device,fence,nullptr);
        destroy_pool(world_device.device,pool,nullptr);
    };
    VkCommandBufferAllocateInfo alloc_info{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    alloc_info.commandPool=pool;
    alloc_info.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount=static_cast<uint32_t>(commands.size());
    if (alloc(world_device.device,&alloc_info,commands.data())!=VK_SUCCESS) {
        cleanup(); return 0;
    }
    VkFenceCreateInfo fence_info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    for (size_t i=0;i<fences.size();++i) {
        if (create_fence(world_device.device,&fence_info,nullptr,
                         &fences[i])!=VK_SUCCESS || !fences[i]) {
            cleanup(); return 0;
        }
    }
    // Empty command buffers deliberately validate queue submission and
    // independent GPU fence completion without exposing uninitialized images.
    for (auto command : commands) {
        VkCommandBufferBeginInfo begin_info{
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        begin_info.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        if (begin(command,&begin_info)!=VK_SUCCESS ||
            end(command)!=VK_SUCCESS) {
            cleanup(); return 0;
        }
    }
    // Real dual-frame-in-flight foundation: both commands are submitted
    // before waiting on either GPU fence. They are distinct command buffers
    // and fences, queued in submission order to one real graphics VkQueue.
    // NO vkQueueWaitIdle or per-submit CPU fence wait; this is NOT a
    // swapchain renderer and does not render a visible world.
    uint32_t submitted=0;
    world_device.sync_last_submitted=0;
    world_device.sync_last_completed=0;
    for (size_t i=0;i<commands.size();++i) {
        VkSubmitInfo info{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        info.commandBufferCount=1;
        info.pCommandBuffers=&commands[i];
        if (submit(world_device.queue,1,&info,fences[i])!=VK_SUCCESS)
            break;
        ++submitted;
        world_device.sync_max_outstanding=(std::max)(
            world_device.sync_max_outstanding,submitted);
    }
    world_device.sync_last_submitted=submitted;
    if (submitted) {
        // One bounded, wait-all fence operation for all queued GPU work.
        // If any fence remains pending, destroying its pool/semaphore would
        // violate Vulkan ownership rules. The device teardown owns those.
        const auto completed=wait(world_device.device,submitted,
                                  fences.data(),VK_TRUE,2000000000ULL);
        if (completed!=VK_SUCCESS) {
            disabled_after_gpu_timeout=true;
            if (logger)
                logger(2,"P05 Vulkan two-submit fence wait failed or timed out; probe disabled, GPU resources quarantined; GL unaffected.");
            return 0;
        }
        world_device.sync_last_completed=submitted;
    }
    cleanup();
    if (logger && submitted==commands.size())
        logger(1,"P05 Vulkan graphics queue: TWO separate GPU submissions issued BEFORE a single bounded wait-all fence; no queue-idle, no world/present.");
    return submitted==commands.size() ? submitted : 0u;
}

// A developer diagnostic, not a screen capture. The fixed allowlist keeps
// memory and compute bounded (max RGBA 512*288*4 + one depth attachment).
// Typos FAIL CLOSED; SDL and the visible GL viewport are never changed.
VkExtent2D ogl_private_world_offscreen_extent() {
    const char* value=std::getenv("OGL_VULKAN_WORLD_OFFSCREEN_RESOLUTION");
    if (!value || !*value || std::strcmp(value,"32x32")==0) return {32,32};
    if (std::strcmp(value,"128x96")==0) return {128,96};
    if (std::strcmp(value,"256x144")==0) return {256,144};
    if (std::strcmp(value,"512x288")==0) return {512,288};
    return {0,0};
}

// P05/3: verified Vulkan offscreen color frame, isolated from the GL window.
// This is an actual GPU graphics render pass and CPU-verified image, not a
// swapchain acquire/present or an OpenSim world renderer. Developer opt-in.
uint32_t render_offscreen_color_frame(bool scene = false) {
    static bool disabled_after_timeout = false;
    if (disabled_after_timeout) return 0;
    const char* enabled = std::getenv("OGL_VULKAN_OFFSCREEN_FRAME_PROBE");
    // A real Firestorm mesh may be submitted while OpenGL owns its
    // window: this VkImage is PRIVATE and never uses VkSurfaceKHR,
    // vkQueuePresentKHR, SDL or OpenGL's window/context.
    if (scene) {
        const char* allowed=std::getenv("OGL_VULKAN_WORLD_OFFSCREEN_RASTER");
        const char* upload=std::getenv("OGL_VULKAN_WORLD_MESH_UPLOAD");
        const char* camera=std::getenv("OGL_VULKAN_WORLD_CAMERA_PROBE");
        if (!allowed || std::strcmp(allowed,"1")!=0 ||
            !upload || std::strcmp(upload,"1")!=0 ||
            !camera || std::strcmp(camera,"1")!=0 ||
            !world_device.world_camera_ready ||
            world_device.camera_mesh_generation!=world_device.mesh_generation ||
            !world_device.mesh_vertices || !world_device.mesh_indices ||
            !world_device.mesh_triangle_count ||
            world_device.mesh_triangle_count>OGL_RTX_MAX_SCENE_TRIANGLES)
            return 0;
    }
    if (!enabled || std::strcmp(enabled,"1")!=0 ||
        !world_device.device || !world_device.queue ||
        !world_device.physical_device || !world_device.device_proc ||
        !world_device.instance_proc ||
        world_device.queue_family==UINT32_MAX) return 0;

    auto device_fn = [&](const char* name) {
        return world_device.device_proc(world_device.device,name);
    };
    auto instance_fn = [&](const char* name) {
        return world_device.instance_proc(world_device.instance,name);
    };
#define OGL_OFFSCREEN_FN(name) auto name = reinterpret_cast<PFN_##name>(device_fn(#name))
    OGL_OFFSCREEN_FN(vkCreateImage);
    OGL_OFFSCREEN_FN(vkDestroyImage);
    OGL_OFFSCREEN_FN(vkGetImageMemoryRequirements);
    OGL_OFFSCREEN_FN(vkCreateImageView);
    OGL_OFFSCREEN_FN(vkDestroyImageView);
    OGL_OFFSCREEN_FN(vkCreateRenderPass);
    OGL_OFFSCREEN_FN(vkDestroyRenderPass);
    OGL_OFFSCREEN_FN(vkCreateFramebuffer);
    OGL_OFFSCREEN_FN(vkDestroyFramebuffer);
    OGL_OFFSCREEN_FN(vkCreateBuffer);
    OGL_OFFSCREEN_FN(vkDestroyBuffer);
    OGL_OFFSCREEN_FN(vkGetBufferMemoryRequirements);
    OGL_OFFSCREEN_FN(vkAllocateMemory);
    OGL_OFFSCREEN_FN(vkFreeMemory);
    OGL_OFFSCREEN_FN(vkBindImageMemory);
    OGL_OFFSCREEN_FN(vkBindBufferMemory);
    OGL_OFFSCREEN_FN(vkMapMemory);
    OGL_OFFSCREEN_FN(vkUnmapMemory);
    OGL_OFFSCREEN_FN(vkCreateCommandPool);
    OGL_OFFSCREEN_FN(vkDestroyCommandPool);
    OGL_OFFSCREEN_FN(vkAllocateCommandBuffers);
    OGL_OFFSCREEN_FN(vkBeginCommandBuffer);
    OGL_OFFSCREEN_FN(vkEndCommandBuffer);
    OGL_OFFSCREEN_FN(vkCmdBeginRenderPass);
    OGL_OFFSCREEN_FN(vkCmdEndRenderPass);
    OGL_OFFSCREEN_FN(vkCmdPipelineBarrier);
    OGL_OFFSCREEN_FN(vkCmdCopyImageToBuffer);
    OGL_OFFSCREEN_FN(vkCreateFence);
    OGL_OFFSCREEN_FN(vkDestroyFence);
    OGL_OFFSCREEN_FN(vkQueueSubmit);
    OGL_OFFSCREEN_FN(vkWaitForFences);
    OGL_OFFSCREEN_FN(vkResetFences);
    OGL_OFFSCREEN_FN(vkResetCommandBuffer);
    OGL_OFFSCREEN_FN(vkCreateShaderModule);
    OGL_OFFSCREEN_FN(vkDestroyShaderModule);
    OGL_OFFSCREEN_FN(vkCreatePipelineLayout);
    OGL_OFFSCREEN_FN(vkDestroyPipelineLayout);
    OGL_OFFSCREEN_FN(vkCreateGraphicsPipelines);
    OGL_OFFSCREEN_FN(vkDestroyPipeline);
    OGL_OFFSCREEN_FN(vkCmdBindPipeline);
    OGL_OFFSCREEN_FN(vkCmdBindVertexBuffers);
    OGL_OFFSCREEN_FN(vkCmdBindIndexBuffer);
    OGL_OFFSCREEN_FN(vkCmdPushConstants);
    OGL_OFFSCREEN_FN(vkCmdDrawIndexed);
    OGL_OFFSCREEN_FN(vkCreateSampler);
    OGL_OFFSCREEN_FN(vkDestroySampler);
    OGL_OFFSCREEN_FN(vkCreateDescriptorSetLayout);
    OGL_OFFSCREEN_FN(vkDestroyDescriptorSetLayout);
    OGL_OFFSCREEN_FN(vkCreateDescriptorPool);
    OGL_OFFSCREEN_FN(vkDestroyDescriptorPool);
    OGL_OFFSCREEN_FN(vkAllocateDescriptorSets);
    OGL_OFFSCREEN_FN(vkUpdateDescriptorSets);
    OGL_OFFSCREEN_FN(vkCmdBindDescriptorSets);
    OGL_OFFSCREEN_FN(vkGetImageSubresourceLayout);
#undef OGL_OFFSCREEN_FN
    auto get_memory_properties =
        reinterpret_cast<PFN_vkGetPhysicalDeviceMemoryProperties>(
            instance_fn("vkGetPhysicalDeviceMemoryProperties"));
    auto get_format_properties =
        reinterpret_cast<PFN_vkGetPhysicalDeviceFormatProperties>(
            instance_fn("vkGetPhysicalDeviceFormatProperties"));
    if (!vkCreateImage || !vkDestroyImage || !vkGetImageMemoryRequirements ||
        !vkCreateImageView || !vkDestroyImageView || !vkCreateRenderPass ||
        !vkDestroyRenderPass || !vkCreateFramebuffer ||
        !vkDestroyFramebuffer || !vkCreateBuffer || !vkDestroyBuffer ||
        !vkGetBufferMemoryRequirements || !vkAllocateMemory || !vkFreeMemory ||
        !vkBindImageMemory || !vkBindBufferMemory || !vkMapMemory ||
        !vkUnmapMemory || !vkCreateCommandPool || !vkDestroyCommandPool ||
        !vkAllocateCommandBuffers || !vkBeginCommandBuffer ||
        !vkEndCommandBuffer || !vkCmdBeginRenderPass ||
        !vkCmdEndRenderPass || !vkCmdPipelineBarrier ||
        !vkCmdCopyImageToBuffer || !vkCreateFence || !vkDestroyFence ||
        !vkQueueSubmit || !vkWaitForFences ||
        (scene && (!vkResetFences || !vkResetCommandBuffer)) ||
        !get_memory_properties || !get_format_properties) return 0;
    if (scene && (!vkCreateShaderModule || !vkDestroyShaderModule ||
        !vkCreatePipelineLayout || !vkDestroyPipelineLayout ||
        !vkCreateGraphicsPipelines || !vkDestroyPipeline ||
        !vkCmdBindPipeline || !vkCmdBindVertexBuffers ||
        !vkCmdBindIndexBuffer || !vkCmdPushConstants || !vkCmdDrawIndexed))
        return 0;

    const VkExtent2D extent=scene ?
        ogl_private_world_offscreen_extent() : VkExtent2D{32,32};
    // Separate opt-in face-normal shading, never the public Vulkan option.
    // Unknown mode fails closed and does not release the active GPU cache.
    const char* lambert_flag=std::getenv("OGL_VULKAN_WORLD_LAMBERT_PROBE");
    if (scene && lambert_flag && *lambert_flag &&
        std::strcmp(lambert_flag,"0")!=0 &&
        std::strcmp(lambert_flag,"1")!=0)
        return 0;
    const bool lambert_enabled=scene && lambert_flag &&
        std::strcmp(lambert_flag,"1")==0;
    const char* sun_flag=std::getenv("OGL_VULKAN_WORLD_SUN_LIGHT_PROBE");
    if (scene && sun_flag && *sun_flag &&
        std::strcmp(sun_flag,"0")!=0 && std::strcmp(sun_flag,"1")!=0)
        return 0;
    const bool sun_enabled=sun_flag && std::strcmp(sun_flag,"1")==0;
    if (scene && sun_enabled && (!lambert_enabled || !world_device.world_sun_ready))
        return 0;
    const char* tint_flag=std::getenv("OGL_VULKAN_WORLD_FACE_TINT_PROBE");
    if (scene && tint_flag && *tint_flag &&
        std::strcmp(tint_flag,"0")!=0 && std::strcmp(tint_flag,"1")!=0)
        return 0;
    const bool tinted=scene && tint_flag &&
        std::strcmp(tint_flag,"1")==0;
    if (tinted && (!lambert_enabled || !sun_enabled ||
        !world_device.mesh_tint_vertices ||
        world_device.tint_mesh_generation!=world_device.mesh_generation ||
        world_device.tint_vertex_count!=world_device.mesh_triangle_count*3u))
        return 0;
    const char* uv_flag=std::getenv("OGL_VULKAN_WORLD_UV_CHECKER_PROBE");
    if (scene && uv_flag && *uv_flag &&
        std::strcmp(uv_flag,"0")!=0 && std::strcmp(uv_flag,"1")!=0)
        return 0;
    const bool uv_checker=scene && uv_flag && std::strcmp(uv_flag,"1")==0;
    if (uv_checker && (!tinted || !world_device.mesh_uv_vertices ||
        world_device.uv_mesh_generation!=world_device.mesh_generation ||
        world_device.uv_vertex_count!=world_device.mesh_triangle_count*3u))
        return 0;
    const char* rgba_flag=std::getenv("OGL_VULKAN_WORLD_RGBA_IMAGE_PROBE");
    if (scene && rgba_flag && *rgba_flag &&
        std::strcmp(rgba_flag,"0")!=0 && std::strcmp(rgba_flag,"1")!=0)
        return 0;
    const bool sampled=scene && rgba_flag &&
        std::strcmp(rgba_flag,"1")==0;
    if (sampled && (!uv_checker ||
        world_device.rgba_mesh_generation!=world_device.mesh_generation ||
        !world_device.rgba_width || !world_device.rgba_height ||
        world_device.rgba_cpu_texels.size()!=
            size_t(world_device.rgba_width)*world_device.rgba_height*4u))
        return 0;
    if (sampled && (!vkCreateSampler || !vkDestroySampler ||
        !vkCreateDescriptorSetLayout || !vkDestroyDescriptorSetLayout ||
        !vkCreateDescriptorPool || !vkDestroyDescriptorPool ||
        !vkAllocateDescriptorSets || !vkUpdateDescriptorSets ||
        !vkCmdBindDescriptorSets || !vkGetImageSubresourceLayout))
        return 0;
    const char* atlas_flag=std::getenv("OGL_VULKAN_WORLD_MATERIAL_ATLAS_PROBE");
    if (scene && atlas_flag && *atlas_flag &&
        std::strcmp(atlas_flag,"0")!=0 && std::strcmp(atlas_flag,"1")!=0)
        return 0;
    const bool atlas=scene && atlas_flag &&
        std::strcmp(atlas_flag,"1")==0;
    const char* atlas16_flag=std::getenv(
        "OGL_VULKAN_WORLD_MATERIAL_ATLAS_16_PROBE");
    if (scene && atlas16_flag && *atlas16_flag &&
        std::strcmp(atlas16_flag,"0")!=0 &&
        std::strcmp(atlas16_flag,"1")!=0)
        return 0;
    const bool atlas16=scene && atlas16_flag &&
        std::strcmp(atlas16_flag,"1")==0;
    if (atlas16 && !atlas) return 0;
    const char* hq_flag=std::getenv(
        "OGL_VULKAN_WORLD_MATERIAL_ATLAS_HQ_PROBE");
    if (scene && hq_flag && *hq_flag &&
        std::strcmp(hq_flag,"0")!=0 &&
        std::strcmp(hq_flag,"1")!=0)
        return 0;
    const bool atlas_hq=scene && hq_flag &&
        std::strcmp(hq_flag,"1")==0;
    if (atlas_hq && !atlas16) return 0;
    if (atlas && (!sampled ||
        !world_device.mesh_material_slot_vertices ||
        world_device.material_slot_mesh_generation!=world_device.mesh_generation ||
        world_device.material_slot_vertex_count!=world_device.mesh_triangle_count*3u ||
        !world_device.material_slot_count ||
        world_device.material_slot_count>(atlas16 ? 16u : 4u) ||
        world_device.rgba_width!=(atlas_hq ? 256u : 64u) ||
        world_device.rgba_height!=(atlas_hq ? 256u : 64u)))
        return 0;
    const uint32_t shading_mode=atlas_hq ? 8u : (atlas16 ? 7u :
        (atlas ? 6u : (sampled ? 5u :
        (uv_checker ? 4u : (tinted ? 3u :
        (lambert_enabled ? (sun_enabled ? 2u : 1u) : 0u))))));
    const uint32_t width=extent.width, height=extent.height;
    if (!width || !height || width>512 || height>288 ||
        uint64_t(width)*height>512u*288u) return 0;
    const VkDeviceSize readback_bytes=VkDeviceSize(width)*height*4;
    constexpr VkFormat format=VK_FORMAT_R8G8B8A8_UNORM;
    VkFormatProperties fmt{};
    get_format_properties(world_device.physical_device,format,&fmt);
    const auto required_features =
        VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT |
        VK_FORMAT_FEATURE_TRANSFER_SRC_BIT;
    if ((fmt.optimalTilingFeatures & required_features)!=required_features)
        return 0;
    // Vulkan 1.0 linear RGBA image is deliberately bounded and supports
    // direct CPU upload with VkSubresourceLayout.rowPitch. This is the
    // first sampling proof, not portable full asset texture streaming.
    if (sampled && !(fmt.linearTilingFeatures&
                     VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT))
        return 0;

    VkPhysicalDeviceMemoryProperties mem{};
    get_memory_properties(world_device.physical_device,&mem);
    auto choose_memory=[&](uint32_t bits, VkMemoryPropertyFlags wanted) {
        for (uint32_t i=0;i<mem.memoryTypeCount && i<32;++i)
            if ((bits & (1u<<i)) &&
                (mem.memoryTypes[i].propertyFlags & wanted)==wanted)
                return i;
        return UINT32_MAX;
    };
    // The legacy 32x32 clear probe remains transient; only the separate
    // opt-in indexed WORLD raster owns a device-lifetime GPU cache.
    OglOffscreenWorldCache transient;
    auto& resource=scene ? world_device.offscreen_world_cache : transient;
    if (scene && resource.initialized &&
        (resource.extent.width!=width || resource.extent.height!=height ||
         resource.shading_mode!=shading_mode)) {
        world_device.release_offscreen_world_cache();
        // A pending GPU submission is never destroyed or replaced.
        if (resource.initialized || resource.submitted) return 0;
    }
    const bool reuse=scene && resource.initialized;
    auto& color=resource.color;
    auto& depth=resource.depth;
    auto& color_mem=resource.color_mem;
    auto& depth_mem=resource.depth_mem;
    auto& readback_mem=resource.readback_mem;
    auto& depth_view=resource.depth_view;
    auto& depth_format=resource.depth_format;
    auto& pipeline_state=resource;
    auto& view=resource.view;
    auto& render_pass=resource.render_pass;
    auto& framebuffer=resource.framebuffer;
    auto& readback=resource.readback;
    auto& command_pool=resource.command_pool;
    auto& command=resource.command;
    auto& fence=resource.fence;
    auto cleanup=[&]() {
        if (pipeline_state.pipeline && vkDestroyPipeline)
            vkDestroyPipeline(world_device.device,pipeline_state.pipeline,nullptr);
        if (pipeline_state.pipeline_layout && vkDestroyPipelineLayout)
            vkDestroyPipelineLayout(world_device.device,
                pipeline_state.pipeline_layout,nullptr);
        if (pipeline_state.texture_descriptor_pool && vkDestroyDescriptorPool)
            vkDestroyDescriptorPool(world_device.device,
                pipeline_state.texture_descriptor_pool,nullptr);
        if (pipeline_state.texture_set_layout && vkDestroyDescriptorSetLayout)
            vkDestroyDescriptorSetLayout(world_device.device,
                pipeline_state.texture_set_layout,nullptr);
        if (pipeline_state.texture_sampler && vkDestroySampler)
            vkDestroySampler(world_device.device,
                pipeline_state.texture_sampler,nullptr);
        if (pipeline_state.texture_view && vkDestroyImageView)
            vkDestroyImageView(world_device.device,
                pipeline_state.texture_view,nullptr);
        if (pipeline_state.texture_image && vkDestroyImage)
            vkDestroyImage(world_device.device,
                pipeline_state.texture_image,nullptr);
        if (pipeline_state.texture_memory && vkFreeMemory)
            vkFreeMemory(world_device.device,
                pipeline_state.texture_memory,nullptr);
        if (pipeline_state.vertex_shader && vkDestroyShaderModule)
            vkDestroyShaderModule(world_device.device,
                pipeline_state.vertex_shader,nullptr);
        if (pipeline_state.fragment_shader && vkDestroyShaderModule)
            vkDestroyShaderModule(world_device.device,
                pipeline_state.fragment_shader,nullptr);
        if (depth_view) vkDestroyImageView(world_device.device,depth_view,nullptr);
        if (depth) vkDestroyImage(world_device.device,depth,nullptr);
        if (depth_mem) vkFreeMemory(world_device.device,depth_mem,nullptr);
        if (fence) vkDestroyFence(world_device.device,fence,nullptr);
        if (command_pool)
            vkDestroyCommandPool(world_device.device,command_pool,nullptr);
        if (framebuffer)
            vkDestroyFramebuffer(world_device.device,framebuffer,nullptr);
        if (render_pass)
            vkDestroyRenderPass(world_device.device,render_pass,nullptr);
        if (view) vkDestroyImageView(world_device.device,view,nullptr);
        if (color) vkDestroyImage(world_device.device,color,nullptr);
        if (readback) vkDestroyBuffer(world_device.device,readback,nullptr);
        if (color_mem) vkFreeMemory(world_device.device,color_mem,nullptr);
        if (readback_mem) vkFreeMemory(world_device.device,readback_mem,nullptr);
        if (scene && resource.initialized)
            ++world_device.offscreen_world_releases;
        resource={};
    };
    auto frame=[&]() -> uint32_t {
        if (!reuse) {
        VkImageCreateInfo image{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        image.imageType=VK_IMAGE_TYPE_2D;
        image.format=format;
        image.extent={width,height,1};
        image.mipLevels=1;
        image.arrayLayers=1;
        image.samples=VK_SAMPLE_COUNT_1_BIT;
        image.tiling=VK_IMAGE_TILING_OPTIMAL;
        image.usage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                    VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        image.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
        image.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;
        if (vkCreateImage(world_device.device,&image,nullptr,&color)!=VK_SUCCESS ||
            !color) return 0;
        VkMemoryRequirements req{};
        vkGetImageMemoryRequirements(world_device.device,color,&req);
        uint32_t mem_index=choose_memory(req.memoryTypeBits,
                                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (mem_index==UINT32_MAX)
            mem_index=choose_memory(req.memoryTypeBits,0);
        if (mem_index==UINT32_MAX) return 0;
        VkMemoryAllocateInfo alloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        alloc.allocationSize=req.size;
        alloc.memoryTypeIndex=mem_index;
        if (vkAllocateMemory(world_device.device,&alloc,nullptr,
                             &color_mem)!=VK_SUCCESS || !color_mem ||
            vkBindImageMemory(world_device.device,color,color_mem,0)!=VK_SUCCESS)
            return 0;

        VkImageViewCreateInfo iv{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        iv.image=color;
        iv.viewType=VK_IMAGE_VIEW_TYPE_2D;
        iv.format=format;
        iv.subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;
        iv.subresourceRange.levelCount=1;
        iv.subresourceRange.layerCount=1;
        if (vkCreateImageView(world_device.device,&iv,nullptr,&view)!=VK_SUCCESS ||
            !view) return 0;

        if (scene) {
            for (VkFormat candidate:{VK_FORMAT_D32_SFLOAT,
                                     VK_FORMAT_D16_UNORM}) {
                VkFormatProperties candidate_features{};
                get_format_properties(world_device.physical_device,
                    candidate,&candidate_features);
                if (candidate_features.optimalTilingFeatures &
                    VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                    depth_format=candidate;break;
                }
            }
            if (depth_format==VK_FORMAT_UNDEFINED) return 0;
            VkImageCreateInfo di{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
            di.imageType=VK_IMAGE_TYPE_2D;
            di.format=depth_format;di.extent={width,height,1};
            di.mipLevels=1;di.arrayLayers=1;
            di.samples=VK_SAMPLE_COUNT_1_BIT;
            di.tiling=VK_IMAGE_TILING_OPTIMAL;
            di.usage=VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            di.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
            if (vkCreateImage(world_device.device,&di,nullptr,&depth)!=VK_SUCCESS
                || !depth) return 0;
            VkMemoryRequirements dr{};
            vkGetImageMemoryRequirements(world_device.device,depth,&dr);
            uint32_t type=choose_memory(dr.memoryTypeBits,
                                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            if (type==UINT32_MAX) type=choose_memory(dr.memoryTypeBits,0);
            if (type==UINT32_MAX) return 0;
            VkMemoryAllocateInfo da{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
            da.allocationSize=dr.size;da.memoryTypeIndex=type;
            if (vkAllocateMemory(world_device.device,&da,nullptr,
                &depth_mem)!=VK_SUCCESS || !depth_mem ||
                vkBindImageMemory(world_device.device,depth,depth_mem,0)!=VK_SUCCESS)
                return 0;
            VkImageViewCreateInfo dv{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            dv.image=depth;dv.viewType=VK_IMAGE_VIEW_TYPE_2D;
            dv.format=depth_format;
            dv.subresourceRange.aspectMask=VK_IMAGE_ASPECT_DEPTH_BIT;
            dv.subresourceRange.levelCount=1;
            dv.subresourceRange.layerCount=1;
            if (vkCreateImageView(world_device.device,&dv,nullptr,
                &depth_view)!=VK_SUCCESS || !depth_view) return 0;
        }

        VkAttachmentDescription attachment{};
        attachment.format=format;
        attachment.samples=VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment.storeOp=VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;
        attachment.finalLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        VkAttachmentReference ref{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        VkAttachmentDescription depth_attachment{};
        depth_attachment.format=depth_format;
        depth_attachment.samples=VK_SAMPLE_COUNT_1_BIT;
        depth_attachment.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth_attachment.storeOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depth_attachment.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;
        depth_attachment.finalLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        VkAttachmentReference depth_ref{1,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
        VkAttachmentDescription attachments[2]{attachment,depth_attachment};
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount=1;
        subpass.pColorAttachments=&ref;
        subpass.pDepthStencilAttachment=scene ? &depth_ref : nullptr;
        VkRenderPassCreateInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        rp.attachmentCount=scene ? 2u : 1u;
        rp.pAttachments=attachments;
        rp.subpassCount=1;
        rp.pSubpasses=&subpass;
        if (vkCreateRenderPass(world_device.device,&rp,nullptr,
                               &render_pass)!=VK_SUCCESS || !render_pass)
            return 0;
        VkFramebufferCreateInfo fb{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        fb.renderPass=render_pass;
        const VkImageView views[2]{view,depth_view};
        fb.attachmentCount=scene ? 2u : 1u;
        fb.pAttachments=views;
        fb.width=width;
        fb.height=height;
        fb.layers=1;
        if (vkCreateFramebuffer(world_device.device,&fb,nullptr,
                                &framebuffer)!=VK_SUCCESS || !framebuffer)
            return 0;

        VkBufferCreateInfo bi{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bi.size=readback_bytes;
        bi.usage=VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        bi.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
        if (vkCreateBuffer(world_device.device,&bi,nullptr,
                           &readback)!=VK_SUCCESS || !readback)
            return 0;
        vkGetBufferMemoryRequirements(world_device.device,readback,&req);
        mem_index=choose_memory(req.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        if (mem_index==UINT32_MAX) return 0;
        alloc.allocationSize=req.size;
        alloc.memoryTypeIndex=mem_index;
        if (vkAllocateMemory(world_device.device,&alloc,nullptr,
                             &readback_mem)!=VK_SUCCESS || !readback_mem ||
            vkBindBufferMemory(world_device.device,readback,
                               readback_mem,0)!=VK_SUCCESS)
            return 0;

        // Private sampled RGBA8 VkImage: linear tiling + host coherent
        // VkSubresourceLayout for this first Vulkan 1.0 texture proof.
        // No compressed OpenSim assets or user-window presentation.
        if (shading_mode>=5u) {
            VkImageCreateInfo ti{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
            ti.imageType=VK_IMAGE_TYPE_2D;
            ti.format=VK_FORMAT_R8G8B8A8_UNORM;
            ti.extent={world_device.rgba_width,world_device.rgba_height,1};
            ti.mipLevels=1;ti.arrayLayers=1;
            ti.samples=VK_SAMPLE_COUNT_1_BIT;
            ti.tiling=VK_IMAGE_TILING_LINEAR;
            ti.usage=VK_IMAGE_USAGE_SAMPLED_BIT;
            ti.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
            ti.initialLayout=VK_IMAGE_LAYOUT_PREINITIALIZED;
            if (vkCreateImage(world_device.device,&ti,nullptr,
                    &resource.texture_image)!=VK_SUCCESS ||
                !resource.texture_image) return 0;
            VkMemoryRequirements tr{};
            vkGetImageMemoryRequirements(world_device.device,
                resource.texture_image,&tr);
            const uint32_t type=choose_memory(tr.memoryTypeBits,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            if (type==UINT32_MAX) return 0;
            VkMemoryAllocateInfo ma{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
            ma.allocationSize=tr.size;ma.memoryTypeIndex=type;
            if (vkAllocateMemory(world_device.device,&ma,nullptr,
                    &resource.texture_memory)!=VK_SUCCESS ||
                !resource.texture_memory ||
                vkBindImageMemory(world_device.device,
                    resource.texture_image,
                    resource.texture_memory,0)!=VK_SUCCESS)
                return 0;
            VkImageSubresource sub{};
            sub.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;
            VkSubresourceLayout row_layout{};
            vkGetImageSubresourceLayout(world_device.device,
                resource.texture_image,&sub,&row_layout);
            const size_t pitch=size_t(world_device.rgba_width)*4u;
            const size_t rows=world_device.rgba_height;
            if (row_layout.rowPitch<pitch ||
                row_layout.offset>tr.size || !rows ||
                (rows-1u)>(tr.size-row_layout.offset)/row_layout.rowPitch ||
                pitch>tr.size-row_layout.offset-
                    (rows-1u)*row_layout.rowPitch)
                return 0;
            void* texels=nullptr;
            if (vkMapMemory(world_device.device,resource.texture_memory,
                    0,tr.size,0,&texels)!=VK_SUCCESS || !texels)
                return 0;
            for (size_t row=0;row<rows;++row)
                std::memcpy(
                    static_cast<uint8_t*>(texels)+row_layout.offset+
                        row*row_layout.rowPitch,
                    world_device.rgba_cpu_texels.data()+row*pitch,pitch);
            vkUnmapMemory(world_device.device,resource.texture_memory);
            VkImageViewCreateInfo iv{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            iv.image=resource.texture_image;
            iv.viewType=VK_IMAGE_VIEW_TYPE_2D;
            iv.format=ti.format;
            iv.subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;
            iv.subresourceRange.levelCount=1;
            iv.subresourceRange.layerCount=1;
            if (vkCreateImageView(world_device.device,&iv,nullptr,
                    &resource.texture_view)!=VK_SUCCESS ||
                !resource.texture_view) return 0;
            VkSamplerCreateInfo si{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
            si.magFilter=VK_FILTER_NEAREST;
            si.minFilter=VK_FILTER_NEAREST;
            si.mipmapMode=VK_SAMPLER_MIPMAP_MODE_NEAREST;
            si.addressModeU=si.addressModeV=si.addressModeW=
                VK_SAMPLER_ADDRESS_MODE_REPEAT;
            si.maxLod=0.f;
            if (vkCreateSampler(world_device.device,&si,nullptr,
                    &resource.texture_sampler)!=VK_SUCCESS ||
                !resource.texture_sampler) return 0;
            VkDescriptorSetLayoutBinding binding{};
            binding.binding=0;
            binding.descriptorCount=1;
            binding.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            binding.stageFlags=VK_SHADER_STAGE_FRAGMENT_BIT;
            VkDescriptorSetLayoutCreateInfo dsl{
                VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
            dsl.bindingCount=1;dsl.pBindings=&binding;
            if (vkCreateDescriptorSetLayout(world_device.device,&dsl,nullptr,
                    &resource.texture_set_layout)!=VK_SUCCESS ||
                !resource.texture_set_layout) return 0;
            VkDescriptorPoolSize pool_size{};
            pool_size.type=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            pool_size.descriptorCount=1;
            VkDescriptorPoolCreateInfo pi{
                VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
            pi.maxSets=1;pi.poolSizeCount=1;pi.pPoolSizes=&pool_size;
            if (vkCreateDescriptorPool(world_device.device,&pi,nullptr,
                    &resource.texture_descriptor_pool)!=VK_SUCCESS ||
                !resource.texture_descriptor_pool) return 0;
            VkDescriptorSetAllocateInfo ai{
                VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
            ai.descriptorPool=resource.texture_descriptor_pool;
            ai.descriptorSetCount=1;
            ai.pSetLayouts=&resource.texture_set_layout;
            if (vkAllocateDescriptorSets(world_device.device,&ai,
                    &resource.texture_descriptor)!=VK_SUCCESS ||
                !resource.texture_descriptor) return 0;
            VkDescriptorImageInfo image_info{};
            image_info.sampler=resource.texture_sampler;
            image_info.imageView=resource.texture_view;
            image_info.imageLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            write.dstSet=resource.texture_descriptor;
            write.dstBinding=0;
            write.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            write.descriptorCount=1;
            write.pImageInfo=&image_info;
            vkUpdateDescriptorSets(world_device.device,1,&write,0,nullptr);
        }

        pipeline_state.pass=render_pass;
        // Optional genuine rasterization of indexed vertex data. The
        // normal single/16-frame clear tests use no shaders/pipelines.
        if (scene) {
            VkShaderModuleCreateInfo vertex_info{
                VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
            vertex_info.codeSize=shading_mode>=6u
                ? sizeof(ogl_dev_world_material_atlas_vert_spv)
                : (shading_mode>=4u
                ? sizeof(ogl_dev_world_uv_checker_vert_spv)
                : (shading_mode==3u ? sizeof(ogl_dev_world_face_tint_vert_spv)
                  : (shading_mode==1u ? sizeof(ogl_dev_world_lambert_vert_spv)
                                      : sizeof(ogl_dev_world_vert_spv))));
            vertex_info.pCode=shading_mode>=6u
                ? ogl_dev_world_material_atlas_vert_spv
                : (shading_mode>=4u
                ? ogl_dev_world_uv_checker_vert_spv
                : (shading_mode==3u ? ogl_dev_world_face_tint_vert_spv
                  : (shading_mode==1u ? ogl_dev_world_lambert_vert_spv
                                      : ogl_dev_world_vert_spv)));
            if (vkCreateShaderModule(world_device.device,&vertex_info,nullptr,
                                     &pipeline_state.vertex_shader)!=VK_SUCCESS || !pipeline_state.vertex_shader)
                return false;
            VkShaderModuleCreateInfo fragment_info{
                VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
            fragment_info.codeSize=shading_mode==8u
                ? sizeof(ogl_dev_world_material_atlas_hq_frag_spv)
                : (shading_mode==7u
                ? sizeof(ogl_dev_world_material_atlas_16_frag_spv)
                : (shading_mode==6u
                ? sizeof(ogl_dev_world_material_atlas_frag_spv)
                : (shading_mode==5u
                ? sizeof(ogl_dev_world_rgba_sample_frag_spv)
                : (shading_mode==4u
                ? sizeof(ogl_dev_world_uv_checker_frag_spv)
                : (shading_mode==3u ? sizeof(ogl_dev_world_face_tint_frag_spv)
                  : (shading_mode==2u ? sizeof(ogl_dev_world_sunlight_frag_spv)
                    : (shading_mode==1u ? sizeof(ogl_dev_world_lambert_frag_spv)
                                        : sizeof(ogl_dev_depth_frag_spv))))))));
            fragment_info.pCode=shading_mode==8u
                ? ogl_dev_world_material_atlas_hq_frag_spv
                : (shading_mode==7u
                ? ogl_dev_world_material_atlas_16_frag_spv
                : (shading_mode==6u
                ? ogl_dev_world_material_atlas_frag_spv
                : (shading_mode==5u
                ? ogl_dev_world_rgba_sample_frag_spv
                : (shading_mode==4u
                ? ogl_dev_world_uv_checker_frag_spv
                : (shading_mode==3u ? ogl_dev_world_face_tint_frag_spv
                  : (shading_mode==2u ? ogl_dev_world_sunlight_frag_spv
                    : (shading_mode==1u ? ogl_dev_world_lambert_frag_spv
                                        : ogl_dev_depth_frag_spv)))))));
            if (vkCreateShaderModule(world_device.device,&fragment_info,nullptr,
                                     &pipeline_state.fragment_shader)!=VK_SUCCESS || !pipeline_state.fragment_shader)
                return false;
            VkPipelineShaderStageCreateInfo stages[2]{};
            stages[0].sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stages[0].stage=VK_SHADER_STAGE_VERTEX_BIT;
            stages[0].module=pipeline_state.vertex_shader;
            stages[0].pName="main";
            stages[1].sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stages[1].stage=VK_SHADER_STAGE_FRAGMENT_BIT;
            stages[1].module=pipeline_state.fragment_shader;
            stages[1].pName="main";
            VkVertexInputBindingDescription vertex_binding{};
            vertex_binding.binding=0;
            vertex_binding.stride=sizeof(float)*3;
            vertex_binding.inputRate=VK_VERTEX_INPUT_RATE_VERTEX;
            VkVertexInputBindingDescription bindings[4]{
                vertex_binding,vertex_binding,vertex_binding,vertex_binding};
            bindings[1].binding=1;
            bindings[2].binding=2;
            bindings[2].stride=sizeof(float)*2;
            bindings[3].binding=3;
            bindings[3].stride=sizeof(float);
            VkVertexInputAttributeDescription vertex_attribute{};
            vertex_attribute.location=0;
            vertex_attribute.binding=0;
            vertex_attribute.format=VK_FORMAT_R32G32B32_SFLOAT;
            vertex_attribute.offset=0;
            VkVertexInputAttributeDescription attributes[4]{
                vertex_attribute,vertex_attribute,vertex_attribute,vertex_attribute};
            attributes[1].location=1;
            attributes[1].binding=1;
            attributes[2].location=2;
            attributes[2].binding=2;
            attributes[2].format=VK_FORMAT_R32G32_SFLOAT;
            attributes[3].location=3;
            attributes[3].binding=3;
            attributes[3].format=VK_FORMAT_R32_SFLOAT;
            VkPipelineVertexInputStateCreateInfo vertex_input{
                VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
            vertex_input.vertexBindingDescriptionCount=shading_mode>=6u
                ? 4u : (shading_mode>=4u ? 3u : (shading_mode==3u ? 2u : 1u));
            vertex_input.pVertexBindingDescriptions=bindings;
            vertex_input.vertexAttributeDescriptionCount=shading_mode>=6u
                ? 4u : (shading_mode>=4u ? 3u : (shading_mode==3u ? 2u : 1u));
            vertex_input.pVertexAttributeDescriptions=attributes;
            VkPipelineInputAssemblyStateCreateInfo assembly{
                VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
            assembly.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            VkViewport viewport{};
            viewport.width=static_cast<float>(width);
            viewport.height=static_cast<float>(height);
            viewport.minDepth=0.f;
            viewport.maxDepth=1.f;
            VkRect2D scissor{};
            scissor.extent={width,height};
            VkPipelineViewportStateCreateInfo viewport_state{
                VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
            viewport_state.viewportCount=1;
            viewport_state.pViewports=&viewport;
            viewport_state.scissorCount=1;
            viewport_state.pScissors=&scissor;
            VkPipelineRasterizationStateCreateInfo raster_state{
                VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
            raster_state.polygonMode=VK_POLYGON_MODE_FILL;
            raster_state.cullMode=VK_CULL_MODE_NONE;
            raster_state.frontFace=VK_FRONT_FACE_COUNTER_CLOCKWISE;
            raster_state.lineWidth=1.f;
            VkPipelineMultisampleStateCreateInfo multisample{
                VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
            multisample.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
            VkPipelineDepthStencilStateCreateInfo depth_state{
                VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
            depth_state.depthTestEnable=VK_TRUE;
            depth_state.depthWriteEnable=VK_TRUE;
            depth_state.depthCompareOp=VK_COMPARE_OP_LESS;
            VkPipelineColorBlendAttachmentState color_attachment{};
            color_attachment.colorWriteMask=VK_COLOR_COMPONENT_R_BIT |
                VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                VK_COLOR_COMPONENT_A_BIT;
            VkPipelineColorBlendStateCreateInfo blend{
                VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
            blend.attachmentCount=1;
            blend.pAttachments=&color_attachment;
            VkPushConstantRange camera_range{};
            camera_range.stageFlags=shading_mode>=2u
                ? VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT
                : VK_SHADER_STAGE_VERTEX_BIT;
            camera_range.offset=0;
            camera_range.size=shading_mode>=2u ?
                sizeof(float)*20 : sizeof(float)*16;
            VkPipelineLayoutCreateInfo layout_info{
                VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
            layout_info.pushConstantRangeCount=1u;
            layout_info.pPushConstantRanges=&camera_range;
            if (shading_mode>=5u) {
                layout_info.setLayoutCount=1;
                layout_info.pSetLayouts=&resource.texture_set_layout;
            }
            if (vkCreatePipelineLayout(world_device.device,&layout_info,nullptr,
                                       &pipeline_state.pipeline_layout)!=VK_SUCCESS ||
                !pipeline_state.pipeline_layout)
                return false;
            VkGraphicsPipelineCreateInfo graphics_info{
                VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
            graphics_info.stageCount=2;
            graphics_info.pStages=stages;
            graphics_info.pVertexInputState=&vertex_input;
            graphics_info.pInputAssemblyState=&assembly;
            graphics_info.pViewportState=&viewport_state;
            graphics_info.pRasterizationState=&raster_state;
            graphics_info.pMultisampleState=&multisample;
            graphics_info.pDepthStencilState=&depth_state;
            graphics_info.pColorBlendState=&blend;
            graphics_info.layout=pipeline_state.pipeline_layout;
            graphics_info.renderPass=pipeline_state.pass;
            graphics_info.subpass=0;
            if (vkCreateGraphicsPipelines(world_device.device,VK_NULL_HANDLE,
                1,&graphics_info,nullptr,&pipeline_state.pipeline)!=VK_SUCCESS ||
                !pipeline_state.pipeline)
                return false;
        }



        VkCommandPoolCreateInfo pool{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        pool.queueFamilyIndex=world_device.queue_family;
        pool.flags=scene ?
            VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT :
            VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        if (vkCreateCommandPool(world_device.device,&pool,nullptr,
                                &command_pool)!=VK_SUCCESS || !command_pool)
            return 0;
        VkCommandBufferAllocateInfo ca{
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        ca.commandPool=command_pool;
        ca.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        ca.commandBufferCount=1;
        if (vkAllocateCommandBuffers(world_device.device,&ca,
                                     &command)!=VK_SUCCESS || !command)
            return 0;
        } // Build VkImages, Depth, VkPipeline, Readback and Pool only once.
        if (reuse && vkResetCommandBuffer(command,0)!=VK_SUCCESS)
            return 0;
        VkCommandBufferBeginInfo cb{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        cb.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        if (vkBeginCommandBuffer(command,&cb)!=VK_SUCCESS) return 0;
        if (shading_mode>=5u && !reuse) {
            // Host-coherent image upload visibility and initial transition.
            VkImageMemoryBarrier barrier{
                VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
            barrier.srcAccessMask=VK_ACCESS_HOST_WRITE_BIT;
            barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
            barrier.oldLayout=VK_IMAGE_LAYOUT_PREINITIALIZED;
            barrier.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
            barrier.image=resource.texture_image;
            barrier.subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.levelCount=1;
            barrier.subresourceRange.layerCount=1;
            vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_HOST_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,0,0,nullptr,
                0,nullptr,1,&barrier);
        }

        VkClearValue clear{};
        clear.color.float32[0]=0.2f;
        clear.color.float32[1]=0.4f;
        clear.color.float32[2]=0.6f;
        clear.color.float32[3]=1.0f;
        VkRenderPassBeginInfo begin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        begin.renderPass=render_pass;
        begin.framebuffer=framebuffer;
        begin.renderArea.extent={width,height};
        VkClearValue clears[2]{clear,{}};
        clears[1].depthStencil.depth=1.f;
        begin.clearValueCount=scene ? 2u : 1u;
        begin.pClearValues=clears;
        vkCmdBeginRenderPass(command,&begin,VK_SUBPASS_CONTENTS_INLINE);
        if (scene) {
            // Reproject the real validated Firestorm camera from its native
            // viewport aspect to the PRIVATE diagnostic target's aspect.
            // Column-major mat4: indices [0,4,8,12] are projected X.
            // This does not mutate the real viewer camera or GPU mesh.
            auto offscreen_projection=world_device.world_to_clip;
            const float aspect_correction=
                world_device.world_camera_aspect /
                (float(width)/float(height));
            if (!std::isfinite(aspect_correction) || aspect_correction<=0.f)
                return 0;
            for (unsigned col=0;col<4;++col)
                offscreen_projection[col*4]*=aspect_correction;
            vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipeline_state.pipeline);
            if (shading_mode>=5u)
                vkCmdBindDescriptorSets(command,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipeline_state.pipeline_layout,
                    0,1,&resource.texture_descriptor,0,nullptr);
            // Dynamic sun light is a per-frame 80-byte Vulkan push
            // constant; sun changes NEVER rebuild VkPipeline or VkMesh.
            std::array<float,20> world_light_push{};
            std::copy(offscreen_projection.begin(),offscreen_projection.end(),
                      world_light_push.begin());
            if (shading_mode>=2u)
                std::copy(world_device.world_sun_direction.begin(),
                          world_device.world_sun_direction.end(),
                          world_light_push.begin()+16);
            vkCmdPushConstants(command,pipeline_state.pipeline_layout,
                shading_mode>=2u
                    ? VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT
                    : VK_SHADER_STAGE_VERTEX_BIT,
                0,shading_mode>=2u ? sizeof(float)*20 : sizeof(float)*16,
                world_light_push.data());
            const VkDeviceSize offset=0;
            if (shading_mode>=6u) {
                const VkBuffer buffers[4]{world_device.mesh_vertices,
                    world_device.mesh_tint_vertices,world_device.mesh_uv_vertices,
                    world_device.mesh_material_slot_vertices};
                const VkDeviceSize offsets[4]{0,0,0,0};
                vkCmdBindVertexBuffers(command,0,4,buffers,offsets);
            } else if (shading_mode>=4u) {
                const VkBuffer buffers[3]{world_device.mesh_vertices,
                                          world_device.mesh_tint_vertices,
                                          world_device.mesh_uv_vertices};
                const VkDeviceSize offsets[3]{0,0,0};
                vkCmdBindVertexBuffers(command,0,3,buffers,offsets);
            } else if (shading_mode==3u) {
                const VkBuffer buffers[2]{world_device.mesh_vertices,
                                          world_device.mesh_tint_vertices};
                const VkDeviceSize offsets[2]{0,0};
                vkCmdBindVertexBuffers(command,0,2,buffers,offsets);
            } else {
                vkCmdBindVertexBuffers(command,0,1,
                                       &world_device.mesh_vertices,&offset);
            }
            vkCmdBindIndexBuffer(command,world_device.mesh_indices,0,
                                 VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(command,world_device.mesh_triangle_count*3u,
                             1,0,0,0);
        }
        vkCmdEndRenderPass(command);
        VkImageMemoryBarrier color_to_transfer{
            VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        color_to_transfer.srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        color_to_transfer.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
        color_to_transfer.oldLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color_to_transfer.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        color_to_transfer.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
        color_to_transfer.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
        color_to_transfer.image=color;
        color_to_transfer.subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;
        color_to_transfer.subresourceRange.levelCount=1;
        color_to_transfer.subresourceRange.layerCount=1;
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,
            1,&color_to_transfer);
        VkBufferImageCopy copy{};
        copy.imageSubresource.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;
        copy.imageSubresource.layerCount=1;
        copy.imageExtent={width,height,1};
        vkCmdCopyImageToBuffer(command,color,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               readback,1,&copy);
        VkBufferMemoryBarrier transfer_to_host{
            VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
        transfer_to_host.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
        transfer_to_host.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
        transfer_to_host.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
        transfer_to_host.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
        transfer_to_host.buffer=readback;
        transfer_to_host.size=readback_bytes;
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_HOST_BIT,0,0,nullptr,1,&transfer_to_host,
            0,nullptr);
        if (vkEndCommandBuffer(command)!=VK_SUCCESS) return 0;

        if (!reuse) {
            VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
            if (vkCreateFence(world_device.device,&fi,nullptr,&fence)!=VK_SUCCESS ||
                !fence) return 0;
        } else if (vkResetFences(world_device.device,1,&fence)!=VK_SUCCESS) {
            return 0;
        }
        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submit.commandBufferCount=1;
        submit.pCommandBuffers=&command;
        if (vkQueueSubmit(world_device.queue,1,&submit,fence)!=VK_SUCCESS)
            return 0;
        resource.submitted=true;
        const auto result=vkWaitForFences(world_device.device,1,&fence,
                                          VK_TRUE,2000000000ULL);
        if (result!=VK_SUCCESS) {
            // No unbounded QueueWaitIdle and no freeing in-flight GPU data.
            // Diagnostic allocations stay on device until module shutdown.
            disabled_after_timeout=true;
            return 2;
        }
        resource.submitted=false; // VkQueueSubmit completed after fence
        void* pixels=nullptr;
        if (vkMapMemory(world_device.device,readback_mem,0,
                        readback_bytes,0,&pixels)!=VK_SUCCESS || !pixels)
            return 0;
        const auto* rgba=static_cast<const uint8_t*>(pixels);
        const auto matches=[&](size_t offset) {
            return std::abs(int(rgba[offset])-51)<=1 &&
                   std::abs(int(rgba[offset+1])-102)<=1 &&
                   std::abs(int(rgba[offset+2])-153)<=1 &&
                   rgba[offset+3]==255;
        };
        bool correct=matches(0) && matches(4*(size_t(width)*height/2)) &&
                     matches(4*(size_t(width)*height-1));
        if (scene) {
            // The GPU actually drew indexed world geometry. Count every
            // changed pixel without exposing image/region/player data.
            // An all-clear frame is valid if the camera sees no geometry,
            // but never label that as visible geometry.
            uint32_t changed=0, lit=0;
            uint64_t luminance_sum=0, red_sum=0, green_sum=0, blue_sum=0;
            for (uint32_t pixel=0;pixel<width*height;++pixel) {
                const size_t offset=size_t(pixel)*4;
                if (!matches(offset)) {
                    ++changed;
                    const uint32_t rgb_sum=uint32_t(rgba[offset])+
                        uint32_t(rgba[offset+1])+uint32_t(rgba[offset+2]);
                    luminance_sum+=rgb_sum;
                    red_sum+=rgba[offset];
                    green_sum+=rgba[offset+1];
                    blue_sum+=rgba[offset+2];
                    // Developer Lambert palette is green-dominant, while
                    // depth debug red/green remains the default unchanged.
                    if (shading_mode!=0u && rgba[offset+1]>rgba[offset] &&
                        rgba[offset+1]>rgba[offset+2])
                        ++lit;
                }
            }
            world_device.offscreen_world_lit_pixels=lit;
            world_device.offscreen_world_mean_luminance=changed ?
                uint32_t(luminance_sum/changed) : 0u;
            world_device.offscreen_world_mean_red=changed ?
                uint32_t(red_sum/changed) : 0u;
            world_device.offscreen_world_mean_blue=changed ?
                uint32_t(blue_sum/changed) : 0u;
            world_device.offscreen_world_mean_green=changed ?
                uint32_t(green_sum/changed) : 0u;
            world_device.offscreen_world_covered_pixels=changed;
            world_device.offscreen_world_width=width;
            world_device.offscreen_world_height=height;
            ++world_device.offscreen_world_frames;
            correct=true; // a completed GPU raster/readback, NOT visibility
        }
        vkUnmapMemory(world_device.device,readback_mem);
        if (scene && correct) {
            if (reuse) ++world_device.offscreen_world_reuses;
            else {
                resource.extent={width,height};
                resource.shading_mode=shading_mode;
                resource.initialized=true;
                ++world_device.offscreen_world_builds;
            }
        }
        if (logger && correct && !scene)
            logger(1,"P05 real Vulkan offscreen render pass: 32x32 RGBA pixels validated after GPU fence; no swapchain present.");
        return correct ? 1u : 0u;
    };
    uint32_t result=frame();
    // Cached indexed-world objects survive NEW camera and mesh revisions.
    // No GPU queue-wide idle. A submitted-but-unfinished command is
    // quarantined for device shutdown instead of premature GPU release.
    if (result!=2 && (!scene || result!=1)) cleanup();
    return result==1 ? 1u : 0u;
}

// Independent opt-in camera projection for the prospective Vulkan World
// raster path. Uses the SAME exact Firestorm v3 camera ABI as the existing
// frame bridge; does not depend on NVIDIA RTX or on ray tracing availability.
// Fail closed for invalid camera vectors/aspect and invalidate old transforms.
int set_world_projection_camera(const ogl_rtx_frame_state* frame) {
    const char* enabled=std::getenv("OGL_VULKAN_WORLD_CAMERA_PROBE");
    if (!world_device.device || !enabled || std::strcmp(enabled,"1")!=0)
        return 0;
    world_device.world_camera_ready=false;
    world_device.world_camera_aspect=0.f;
    world_device.world_sun_ready=false;
    world_device.world_sun_direction.fill(0.f);
    world_device.camera_mesh_generation=0;
    world_device.world_to_clip.fill(0.f);
    if (!frame || !ogl::valid_camera_frame(*frame)) return 0;
    // The optional dynamic-light path strictly validates a nonzero,
    // bounded direction, including NaN/Inf and accidental zero vectors.
    // Standard Depth/fixed Lambert remain backward compatible.
    const char* dynamic_light=std::getenv("OGL_VULKAN_WORLD_SUN_LIGHT_PROBE");
    const bool sun_requested=dynamic_light && std::strcmp(dynamic_light,"1")==0;
    if (dynamic_light && *dynamic_light &&
        std::strcmp(dynamic_light,"0")!=0 &&
        std::strcmp(dynamic_light,"1")!=0) return 0;
    double sun_length2=0.0;
    for (unsigned axis=0;axis<3;++axis)
        sun_length2+=double(frame->sun_direction[axis])*
                     frame->sun_direction[axis];
    if (!std::isfinite(sun_length2) ||
        (sun_requested && (sun_length2<0.25 || sun_length2>4.0)))
        return 0;
    const float actual_aspect=float(frame->width)/float(frame->height);
    if (std::fabs(actual_aspect-frame->aspect_ratio)>0.15f) return 0;
    if (world_device.swapchain_probe_ready &&
        std::fabs(frame->aspect_ratio-
            float(world_device.swapchain_extent.width)/
            float(world_device.swapchain_extent.height))>0.01f)
        return 0;
    const float* axes[]={frame->camera_forward,frame->camera_right,
                         frame->camera_up};
    for (const float* axis:axes) {
        float norm2=0.f;
        for (unsigned a=0;a<3;++a) norm2+=axis[a]*axis[a];
        if (!std::isfinite(norm2) || std::fabs(norm2-1.f)>0.05f)
            return 0;
    }
    const auto& forward=frame->camera_forward;
    const auto& right=frame->camera_right;
    const auto& up=frame->camera_up;
    const double cross_x=double(right[1])*up[2]-double(right[2])*up[1];
    const double cross_y=double(right[2])*up[0]-double(right[0])*up[2];
    const double cross_z=double(right[0])*up[1]-double(right[1])*up[0];
    const double handedness=cross_x*forward[0]+cross_y*forward[1]+
                            cross_z*forward[2];
    // Firestorm's LLCoordFrame axes are forward=+X, LEFT=+Y, up=+Z.
    // The real viewer supplies right=-getLeftAxis(), so its basis has
    // handedness -1, unlike the earlier synthetic GPU fixtures (+1).
    // Accept either orthonormal sign without flipping the supplied right
    // vector (which would mirror actual Vulkan world pixels). The
    // signed magnitude check still rejects degenerate/skewed bases.
    if (!std::isfinite(handedness) ||
        std::fabs(std::fabs(handedness)-1.0)>0.1)
        return 0;
    const double fy=1.0/std::tan(double(frame->vertical_fov_radians)*0.5);
    const double fx=fy/double(frame->aspect_ratio);
    const double depth=double(frame->far_clip_m)/
                       double(frame->far_clip_m-frame->near_clip_m);
    if (!std::isfinite(fx) || !std::isfinite(fy) ||
        !std::isfinite(depth) || fx<=0 || fy<=0)
        return 0;
    // Vulkan NDC: x,y in [-1,1], depth in [0,1], +screen-Y down.
    // Matrix columns are packed in the order consumed by GLSL mat4.
    // view x = dot(right, world-eye), view y = dot(up, world-eye),
    // positive view z = dot(forward, world-eye).
    std::array<float,16> matrix{};
    for (unsigned axis=0;axis<3;++axis) {
        matrix[axis*4+0]=float(double(right[axis])*fx);
        matrix[axis*4+1]=float(-double(up[axis])*fy);
        matrix[axis*4+2]=float(double(forward[axis])*depth);
        matrix[axis*4+3]=forward[axis];
    }
    double right_eye=0,up_eye=0,forward_eye=0;
    for (unsigned axis=0;axis<3;++axis) {
        right_eye+=double(right[axis])*frame->camera_position[axis];
        up_eye+=double(up[axis])*frame->camera_position[axis];
        forward_eye+=double(forward[axis])*frame->camera_position[axis];
    }
    matrix[12]=float(-right_eye*fx);
    matrix[13]=float(up_eye*fy);
    matrix[14]=float(-(forward_eye+double(frame->near_clip_m))*depth);
    matrix[15]=float(-forward_eye);
    for (float v:matrix)
        if (!std::isfinite(v)) return 0;
    world_device.world_to_clip=matrix;
    if (sun_length2>0.0) {
        const float inv_sun=float(1.0/std::sqrt(sun_length2));
        for (unsigned i=0;i<3;++i)
            world_device.world_sun_direction[i]=frame->sun_direction[i]*inv_sun;
        world_device.world_sun_ready=true;
    }
    world_device.world_camera_aspect=frame->aspect_ratio;
    world_device.camera_mesh_generation=world_device.mesh_generation;
    world_device.world_camera_ready=true;
    ++world_device.world_camera_accepts; // no Vulkan draw/queue work
    return 1;
}

// A genuine Firestorm region transition must invalidate the OLD world
// immediately, even if GPU work from the old region is still in flight.
// This is a separate, opt-in barrier, NOT a generic mesh upload/clear and
// never touches the current OpenGL window. The scene is made non-presentable
// BEFORE any bounded GPU waits, so failures cannot display stale geometry.
int invalidate_live_world_region() {
    const char* permitted=std::getenv("OGL_VULKAN_LIVE_REGION_BARRIER");
    const char* upload=std::getenv("OGL_VULKAN_WORLD_MESH_UPLOAD");
    if (!permitted || std::strcmp(permitted,"1")!=0 ||
        !upload || std::strcmp(upload,"1")!=0 ||
        !world_device.device || !world_device.destroy_buffer ||
        !world_device.free_memory)
        return 0;
    world_device.world_camera_ready=false;
    world_device.world_camera_aspect=0.f;
    world_device.world_sun_ready=false;
    world_device.world_sun_direction.fill(0.f);
    world_device.camera_mesh_generation=0;
    world_device.world_to_clip.fill(0.f);
    auto& frames=world_device.persistent;
    if (frames.blocked) {
        ++world_device.region_barrier_failures;
        return 0;
    }
    if (frames.initialized && frames.submitted_any) {
        auto wait=reinterpret_cast<PFN_vkWaitForFences>(
            world_device.device_proc(world_device.device,"vkWaitForFences"));
        if (!wait) {
            frames.blocked=true;
            ++world_device.region_barrier_failures;
            return 0;
        }
        VkFence unfinished[2]{VK_NULL_HANDLE,VK_NULL_HANDLE};
        uint32_t pending=0;
        for (size_t i=0;i<frames.submit_pending.size();++i)
            if (frames.submit_pending[i] &&
                frames.submitted_mesh_generations[i]==world_device.mesh_generation)
                unfinished[pending++]=frames.submit_fences[i];
        if (pending) {
            if (wait(world_device.device,pending,unfinished,VK_TRUE,
                     2000000000ULL)!=VK_SUCCESS) {
                frames.blocked=true;
                ++world_device.mesh_retire_failures;
                ++world_device.region_barrier_failures;
                return 0; // old GPU buffers stay alive on timeout
            }
            ++world_device.mesh_retire_waits;
            for (size_t i=0;i<frames.submit_pending.size();++i)
                if (frames.submitted_mesh_generations[i]==world_device.mesh_generation)
                    frames.submit_pending[i]=false;
        }
    }
    // Successful retirement: old region geometry cannot be used again.
    world_device.clear_mesh();
    ++world_device.mesh_generation;
    ++world_device.region_barrier_accepts;
    return 1;
}

// P05/4 Vulkan vertex/index buffer staging from the existing real OpenSim
// triangle ABI. Not tied to NVIDIA/RTX/ray queries or to GL rendering.
// Opt-in until a complete Vulkan render pass + shader pipeline exists.
int upload_world_geometry(const ogl_rtx_triangle* triangles, uint32_t count) {
    const char* enabled=std::getenv("OGL_VULKAN_WORLD_MESH_UPLOAD");
    if (!enabled || std::strcmp(enabled,"1")!=0 ||
        !world_device.device || !world_device.device_proc ||
        !world_device.instance_proc || !world_device.physical_device ||
        !world_device.destroy_buffer || !world_device.free_memory ||
        count>OGL_RTX_MAX_SCENE_TRIANGLES || (count && !triangles))
        return 0;
    // Invalid input must not stall the graphics queue or mutate a valid
    // generation. Validate before any GPU fence retirement attempt.
    if (count)
        for (uint32_t i=0;i<count;++i)
            if (!ogl::is_valid_triangle(triangles[i])) return 0;

    // An unchanged snapshot references exactly the SAME GPU vertex/index
    // data and camera. Under its own explicit developer flag we can skip
    // upload, invalidation and GPU fence waits entirely. Compare full
    // bytes, not a lossy fingerprint; hash collisions cannot hide updates.
    const char* dedup=std::getenv("OGL_VULKAN_WORLD_MESH_DEDUP");
    const bool dedup_enabled=dedup && std::strcmp(dedup,"1")==0;
    if (dedup_enabled && count>0 &&
        world_device.mesh_triangle_count==count &&
        world_device.mesh_vertices && world_device.mesh_indices &&
        world_device.mesh_cpu_snapshot.size()==count &&
        std::memcmp(world_device.mesh_cpu_snapshot.data(),triangles,
            size_t(count)*sizeof(ogl_rtx_triangle))==0) {
        ++world_device.mesh_dedup_skips;
        return 1; // no generation/camera change; no GPU work
    }

    // P05 world scene rotation: keep the old VkBuffers alive until ALL
    // queued graphics submissions that reference the old mesh have
    // completed. A separate explicit developer gate permits this bounded
    // GPU-fence handshake, WITHOUT vkQueueWaitIdle or swapping the window.
    // Present only references swapchain images/semaphores, not VkMeshBuffers.
    auto& frames=world_device.persistent;
    if (frames.blocked) return 0;
    // After a SUCCESSFUL region barrier the previous mesh is gone and
    // all its GPU references are retired. The FIRST mesh of the new region
    // may therefore be uploaded without a general rotation permission.
    // If OLD VkBuffers still exist, retain the original strict opt-in.
    if (frames.initialized && frames.submitted_any &&
        (world_device.mesh_vertices || world_device.mesh_indices)) {
        const char* rotation=std::getenv(
            "OGL_VULKAN_PERSISTENT_MESH_ROTATION_PROBE");
        if (!rotation || std::strcmp(rotation,"1")!=0 ||
            !frames.world_depth || frames.blocked)
            return 0;
        const auto wait=reinterpret_cast<PFN_vkWaitForFences>(
            world_device.device_proc(world_device.device,"vkWaitForFences"));
        if (!wait) return 0;
        VkFence unfinished[2]{VK_NULL_HANDLE,VK_NULL_HANDLE};
        uint32_t pending=0;
        for (size_t i=0;i<frames.submit_pending.size();++i) {
            if (frames.submit_pending[i] &&
                frames.submitted_mesh_generations[i]==world_device.mesh_generation)
                unfinished[pending++]=frames.submit_fences[i];
        }
        if (pending) {
            if (wait(world_device.device,pending,unfinished,VK_TRUE,
                     2000000000ULL)!=VK_SUCCESS) {
                // Do not release or overwrite any VkBuffer on GPU timeout.
                // Block further Vulkan presents until device teardown.
                frames.blocked=true;
                ++world_device.mesh_retire_failures;
                return 0;
            }
            ++world_device.mesh_retire_waits;
            for (size_t i=0;i<frames.submit_pending.size();++i)
                if (frames.submitted_mesh_generations[i]==
                    world_device.mesh_generation)
                    frames.submit_pending[i]=false;
        }
    }
    if (count==0) {
        world_device.clear_mesh(); // region/scene cleared, no stale vertices
        // Invalidate the old region's projection on teleport/region loss.
        world_device.world_camera_ready=false;
        world_device.world_camera_aspect=0.f;
        world_device.world_sun_ready=false;
        world_device.world_sun_direction.fill(0.f);
        world_device.camera_mesh_generation=0;
        world_device.world_to_clip.fill(0.f);
        ++world_device.mesh_generation; // explicit empty-region generation
        return 1;
    }
    static_assert(sizeof(ogl_rtx_triangle)==sizeof(float)*9,
        "World triangle ABI must be tightly packed XYZ positions");
    // Allocate the optional CPU shadow BEFORE creating or replacing VkBuffers.
    // Publication is a no-throw swap only after BOTH GPU uploads succeed.
    std::vector<ogl_rtx_triangle> accepted_snapshot;
    if (dedup_enabled)
        accepted_snapshot.assign(triangles,triangles+count);
    const uint32_t vertex_count=count*3u;
    const VkDeviceSize vertex_bytes=VkDeviceSize(count)*sizeof(ogl_rtx_triangle);
    const VkDeviceSize index_bytes=VkDeviceSize(vertex_count)*sizeof(uint32_t);
    std::vector<uint32_t> indices(vertex_count);
    for (uint32_t i=0;i<vertex_count;++i) indices[i]=i;

    const auto get_device=[&](const char* name) {
        return world_device.device_proc(world_device.device,name);
    };
    const auto create=reinterpret_cast<PFN_vkCreateBuffer>(get_device("vkCreateBuffer"));
    const auto get_requirements=reinterpret_cast<PFN_vkGetBufferMemoryRequirements>(
        get_device("vkGetBufferMemoryRequirements"));
    const auto allocate=reinterpret_cast<PFN_vkAllocateMemory>(get_device("vkAllocateMemory"));
    const auto bind=reinterpret_cast<PFN_vkBindBufferMemory>(get_device("vkBindBufferMemory"));
    const auto map=reinterpret_cast<PFN_vkMapMemory>(get_device("vkMapMemory"));
    const auto unmap=reinterpret_cast<PFN_vkUnmapMemory>(get_device("vkUnmapMemory"));
    const auto properties=reinterpret_cast<PFN_vkGetPhysicalDeviceMemoryProperties>(
        world_device.instance_proc(world_device.instance,
                                   "vkGetPhysicalDeviceMemoryProperties"));
    if (!create || !get_requirements || !allocate || !bind ||
        !map || !unmap || !properties)
        return 0;

    VkPhysicalDeviceMemoryProperties gpu_memory{};
    properties(world_device.physical_device,&gpu_memory);
    VkBuffer vertex=VK_NULL_HANDLE,index=VK_NULL_HANDLE;
    VkDeviceMemory vertex_memory=VK_NULL_HANDLE,index_memory=VK_NULL_HANDLE;
    auto dispose=[&]() {
        if (vertex) world_device.destroy_buffer(world_device.device,vertex,nullptr);
        if (index) world_device.destroy_buffer(world_device.device,index,nullptr);
        if (vertex_memory) world_device.free_memory(world_device.device,vertex_memory,nullptr);
        if (index_memory) world_device.free_memory(world_device.device,index_memory,nullptr);
    };
    auto create_upload=[&](const void* data,VkDeviceSize bytes,
                           VkBufferUsageFlags usage,
                           VkBuffer& buffer,VkDeviceMemory& memory) {
        VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        info.size=bytes;
        info.usage=usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        info.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
        if (create(world_device.device,&info,nullptr,&buffer)!=VK_SUCCESS ||
            !buffer)
            return false;
        VkMemoryRequirements req{};
        get_requirements(world_device.device,buffer,&req);
        uint32_t chosen=UINT32_MAX;
        for (uint32_t i=0;i<gpu_memory.memoryTypeCount && i<32;++i) {
            const VkMemoryPropertyFlags flags=gpu_memory.memoryTypes[i].propertyFlags;
            if ((req.memoryTypeBits&(1u<<i)) &&
                (flags&(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) ==
                        (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
                chosen=i;
                break;
            }
        }
        if (chosen==UINT32_MAX || req.size<bytes) return false;
        VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocation.allocationSize=req.size;
        allocation.memoryTypeIndex=chosen;
        if (allocate(world_device.device,&allocation,nullptr,&memory)!=VK_SUCCESS ||
            !memory)
            return false;
        if (bind(world_device.device,buffer,memory,0)!=VK_SUCCESS)
            return false;
        void* mapped=nullptr;
        if (map(world_device.device,memory,0,bytes,0,&mapped)!=VK_SUCCESS ||
            !mapped)
            return false;
        std::memcpy(mapped,data,static_cast<size_t>(bytes));
        unmap(world_device.device,memory);
        return true;
    };

    if (!create_upload(triangles,vertex_bytes,VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                       vertex,vertex_memory) ||
        !create_upload(indices.data(),index_bytes,VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                       index,index_memory)) {
        dispose();
        return 0;
    }
    // Publish only after BOTH GPU buffers are allocated, bound and copied.
    // Never release existing valid scene buffers on partial upload failure.
    world_device.clear_mesh();
    world_device.mesh_vertices=vertex;
    world_device.mesh_indices=index;
    world_device.mesh_vertex_memory=vertex_memory;
    world_device.mesh_index_memory=index_memory;
    world_device.mesh_triangle_count=count;
    // Keep CPU bytes only with the opt-in, no memory cost for standard GL.
    // Never cache a partially uploaded scene.
    if (dedup_enabled)
        world_device.mesh_cpu_snapshot.swap(accepted_snapshot);
    ++world_device.mesh_uploads;
    ++world_device.mesh_generation;
    // Mesh and camera MUST be paired only after another real camera
    // submission. A new snapshot may correspond to a new region/camera.
    world_device.world_camera_ready=false;
    world_device.world_camera_aspect=0.f;
    world_device.world_sun_ready=false;
    world_device.world_sun_direction.fill(0.f);
    world_device.camera_mesh_generation=0;
    world_device.world_to_clip.fill(0.f);
    if (logger)
        logger(1,(std::string("P05 Vulkan vertex/index upload: triangles=")+
                  std::to_string(count)+" vertices="+
                  std::to_string(vertex_count)+
                  "; GPU buffers ready, no world draw.").c_str());
    return 1;
}

// Shared Vulkan staging: bounded host-visible coherent vertex attributes.
// Every allocation failure leaves old GPU RGB/UV buffers untouched.
bool upload_world_attribute_bytes(const void* source,VkDeviceSize bytes,
                                  VkBuffer& buffer,VkDeviceMemory& memory) {
    if (!source || bytes==0 || !world_device.device ||
        !world_device.instance_proc || !world_device.device_proc)
        return false;
    const auto proc=[&](const char* name) {
        return world_device.device_proc(world_device.device,name);
    };
    const auto create=reinterpret_cast<PFN_vkCreateBuffer>(proc("vkCreateBuffer"));
    const auto requirements=reinterpret_cast<PFN_vkGetBufferMemoryRequirements>(
        proc("vkGetBufferMemoryRequirements"));
    const auto alloc=reinterpret_cast<PFN_vkAllocateMemory>(proc("vkAllocateMemory"));
    const auto bind=reinterpret_cast<PFN_vkBindBufferMemory>(proc("vkBindBufferMemory"));
    const auto map=reinterpret_cast<PFN_vkMapMemory>(proc("vkMapMemory"));
    const auto unmap=reinterpret_cast<PFN_vkUnmapMemory>(proc("vkUnmapMemory"));
    const auto props=reinterpret_cast<PFN_vkGetPhysicalDeviceMemoryProperties>(
        world_device.instance_proc(world_device.instance,
                                   "vkGetPhysicalDeviceMemoryProperties"));
    if (!create || !requirements || !alloc || !bind ||
        !map || !unmap || !props) return false;
    buffer=VK_NULL_HANDLE;
    memory=VK_NULL_HANDLE;
    const auto rollback=[&]() {
        if (buffer) world_device.destroy_buffer(world_device.device,buffer,nullptr);
        if (memory) world_device.free_memory(world_device.device,memory,nullptr);
    };
    VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    info.size=bytes;
    info.usage=VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    info.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
    if (create(world_device.device,&info,nullptr,&buffer)!=VK_SUCCESS ||
        !buffer) {
        rollback();
        return false;
    }
    VkMemoryRequirements req{};
    requirements(world_device.device,buffer,&req);
    VkPhysicalDeviceMemoryProperties memory_props{};
    props(world_device.physical_device,&memory_props);
    uint32_t index=UINT32_MAX;
    for (uint32_t i=0;i<memory_props.memoryTypeCount && i<32;++i) {
        const VkMemoryPropertyFlags flags=memory_props.memoryTypes[i].propertyFlags;
        if ((req.memoryTypeBits&(1u<<i)) &&
            (flags&(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) ==
                   (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
            index=i;
            break;
        }
    }
    if (index==UINT32_MAX || req.size<bytes) {
        rollback();
        return false;
    }
    VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocation.allocationSize=req.size;
    allocation.memoryTypeIndex=index;
    if (alloc(world_device.device,&allocation,nullptr,&memory)!=VK_SUCCESS ||
        !memory || bind(world_device.device,buffer,memory,0)!=VK_SUCCESS) {
        rollback();
        return false;
    }
    void* target=nullptr;
    if (map(world_device.device,memory,0,bytes,0,&target)!=VK_SUCCESS ||
        !target) {
        rollback();
        return false;
    }
    std::memcpy(target,source,size_t(bytes));
    unmap(world_device.device,memory);
    return true;
}

// Optional additive material-TINT sidecar for real Firestorm volume faces.
// NOT an image/texture streamer. Exactly one finite opaque RGB triple per
// accepted triangle, replicated into three vertex inputs at binding 1.
// No mesh generation change; tint is coupled to the CURRENT GPU mesh.
// Strict source-generation validation prevents stale colors after teleport.
int upload_world_face_tints(const ogl_rtx_face_tint* tints,uint32_t count) {
    const char* enabled=std::getenv("OGL_VULKAN_WORLD_FACE_TINT_PROBE");
    const char* lambert=std::getenv("OGL_VULKAN_WORLD_LAMBERT_PROBE");
    const char* sun=std::getenv("OGL_VULKAN_WORLD_SUN_LIGHT_PROBE");
    if (!enabled || std::strcmp(enabled,"1")!=0 ||
        !lambert || std::strcmp(lambert,"1")!=0 ||
        !sun || std::strcmp(sun,"1")!=0 ||
        !world_device.device || !world_device.device_proc ||
        !world_device.physical_device || !world_device.instance_proc ||
        !world_device.destroy_buffer || !world_device.free_memory ||
        world_device.offscreen_world_cache.submitted)
        return 0;
    // Explicitly withdraw a rejected/missing face-color source without
    // dropping valid geometry. This invalidates a previous tint revision.
    if (count==0) {
        world_device.clear_face_tints();
        return 1;
    }
    if (!world_device.mesh_vertices || !world_device.mesh_indices ||
        world_device.mesh_triangle_count==0 ||
        count>OGL_RTX_MAX_SCENE_TRIANGLES ||
        count!=world_device.mesh_triangle_count || !tints)
        return 0;
    static_assert(sizeof(ogl_rtx_face_tint)==3*sizeof(float),
                  "Face-tint ABI must have no padding");
    for (uint32_t t=0;t<count;++t)
        for (float rgb:tints[t].rgb)
            if (!std::isfinite(rgb) || rgb<0.f || rgb>1.f)
                return 0; // invalid data cannot mutate previous GPU tint
    // One color per triangle becomes three vertex attributes, aligned with
    // the existing 3*count indexed positions without modifying their ABI.
    std::vector<float> expanded(size_t(count)*9u);
    for (uint32_t t=0;t<count;++t)
        for (unsigned vertex=0;vertex<3;++vertex)
            std::copy(tints[t].rgb,tints[t].rgb+3,
                      expanded.begin()+size_t(t)*9u+vertex*3u);
    const VkDeviceSize bytes=VkDeviceSize(expanded.size())*sizeof(float);
    VkBuffer buffer=VK_NULL_HANDLE;
    VkDeviceMemory memory=VK_NULL_HANDLE;
    if (!upload_world_attribute_bytes(expanded.data(),bytes,
                                      buffer,memory))
        return 0;
    // Existing private GPU frame has already completed its own fence;
    // no other rendering mode references binding 1. Commit atomically.
    world_device.clear_face_tints();
    world_device.mesh_tint_vertices=buffer;
    world_device.mesh_tint_memory=memory;
    world_device.tint_mesh_generation=world_device.mesh_generation;
    world_device.tint_vertex_count=count*3u;
    ++world_device.tint_uploads;
    return 1;
}

// Face UVs: actual OpenSim volume-face texture coordinates, one float2
// for each of three world vertices, independent of RGB/geometry updates.
int upload_world_face_uvs(const ogl_rtx_face_uv* uv_triangles,uint32_t count) {
    const char* uv=std::getenv("OGL_VULKAN_WORLD_UV_CHECKER_PROBE");
    const char* tint=std::getenv("OGL_VULKAN_WORLD_FACE_TINT_PROBE");
    const char* lambert=std::getenv("OGL_VULKAN_WORLD_LAMBERT_PROBE");
    const char* sun=std::getenv("OGL_VULKAN_WORLD_SUN_LIGHT_PROBE");
    if (!uv || std::strcmp(uv,"1")!=0 ||
        !tint || std::strcmp(tint,"1")!=0 ||
        !lambert || std::strcmp(lambert,"1")!=0 ||
        !sun || std::strcmp(sun,"1")!=0 ||
        !world_device.device || !world_device.device_proc ||
        !world_device.destroy_buffer || !world_device.free_memory ||
        world_device.offscreen_world_cache.submitted)
        return 0;
    if (count==0) {
        world_device.clear_face_uvs();
        return 1;
    }
    if (!world_device.mesh_vertices || !world_device.mesh_indices ||
        !world_device.mesh_tint_vertices ||
        world_device.tint_mesh_generation!=world_device.mesh_generation ||
        !uv_triangles || count>OGL_RTX_MAX_SCENE_TRIANGLES ||
        count!=world_device.mesh_triangle_count)
        return 0;
    static_assert(sizeof(ogl_rtx_face_uv)==sizeof(float)*6,
                  "UV ABI must be six tightly packed floats");
    for (uint32_t t=0;t<count;++t)
        for (float coord:uv_triangles[t].uv)
            if (!std::isfinite(coord) || coord<-4096.f ||
                coord>4096.f)
                return 0;
    VkBuffer buffer=VK_NULL_HANDLE;
    VkDeviceMemory memory=VK_NULL_HANDLE;
    if (!upload_world_attribute_bytes(uv_triangles,
        VkDeviceSize(count)*sizeof(ogl_rtx_face_uv),buffer,memory))
        return 0;
    // No private offscreen GPU submission is pending; WSI does not use UV.
    world_device.clear_face_uvs();
    world_device.mesh_uv_vertices=buffer;
    world_device.mesh_uv_memory=memory;
    world_device.uv_mesh_generation=world_device.mesh_generation;
    world_device.uv_vertex_count=count*3u;
    ++world_device.uv_uploads;
    return 1;
}

// P05/6 per-face material slots, strict current-generation binding3.
// A slot is one finite exact integer 0..3 per TRIANGLE, repeated 3x,
// so no primitive can interpolate across atlas texture assets.
int upload_world_material_slots(const ogl_rtx_face_material_slot* slots,
                                uint32_t count,uint32_t tile_count) {
    const auto enabled=[](const char* name) {
        const char* v=std::getenv(name);
        return v && std::strcmp(v,"1")==0;
    };
    if (!enabled("OGL_VULKAN_WORLD_MATERIAL_ATLAS_PROBE") ||
        !enabled("OGL_VULKAN_WORLD_RGBA_IMAGE_PROBE") ||
        !enabled("OGL_VULKAN_WORLD_UV_CHECKER_PROBE") ||
        !enabled("OGL_VULKAN_WORLD_FACE_TINT_PROBE") ||
        !enabled("OGL_VULKAN_WORLD_LAMBERT_PROBE") ||
        !enabled("OGL_VULKAN_WORLD_SUN_LIGHT_PROBE") ||
        !enabled("OGL_VULKAN_WORLD_MESH_UPLOAD") ||
        !enabled("OGL_VULKAN_WORLD_CAMERA_PROBE") ||
        !world_device.device || !world_device.device_proc ||
        !world_device.destroy_buffer || !world_device.free_memory ||
        world_device.offscreen_world_cache.submitted)
        return 0;
    if (enabled("OGL_VULKAN_WORLD_MATERIAL_ATLAS_HQ_PROBE") &&
        !enabled("OGL_VULKAN_WORLD_MATERIAL_ATLAS_16_PROBE"))
        return 0;
    if (!count && !tile_count) {
        world_device.clear_material_slots();
        return 1;
    }
    if (!slots || !count || count>OGL_RTX_MAX_SCENE_TRIANGLES ||
        count!=world_device.mesh_triangle_count ||
        tile_count==0 || tile_count>
            (enabled("OGL_VULKAN_WORLD_MATERIAL_ATLAS_16_PROBE") ? 16u : 4u) ||
        !world_device.mesh_vertices || !world_device.mesh_indices ||
        !world_device.mesh_uv_vertices || !world_device.mesh_tint_vertices ||
        world_device.uv_mesh_generation!=world_device.mesh_generation ||
        world_device.tint_mesh_generation!=world_device.mesh_generation)
        return 0;
    static_assert(sizeof(ogl_rtx_face_material_slot)==sizeof(float)*3,
                  "material slot ABI must have no padding");
    for (uint32_t t=0;t<count;++t) {
        const float value=slots[t].slot[0];
        if (!std::isfinite(value) || value<0.f ||
            value>=float(tile_count) || std::floor(value)!=value)
            return 0;
        for (unsigned v=1;v<3;++v)
            if (slots[t].slot[v]!=value) return 0;
    }
    VkBuffer gpu_buffer=VK_NULL_HANDLE;
    VkDeviceMemory gpu_memory=VK_NULL_HANDLE;
    if (!upload_world_attribute_bytes(slots,
            VkDeviceSize(count)*sizeof(ogl_rtx_face_material_slot),
            gpu_buffer,gpu_memory))
        return 0;
    world_device.clear_material_slots();
    world_device.mesh_material_slot_vertices=gpu_buffer;
    world_device.mesh_material_slot_memory=gpu_memory;
    world_device.material_slot_mesh_generation=world_device.mesh_generation;
    world_device.material_slot_vertex_count=count*3u;
    world_device.material_slot_count=tile_count;
    ++world_device.material_slot_uploads;
    return 1;
}

// First real VkImage+VkSampler source ABI: opaque, decoded RGBA8 pixels,
// NOT a JPEG2000 loader and NOT region-asset mapping. A one-image proof.
int upload_world_rgba_image(const uint8_t* rgba,uint32_t width,uint32_t height) {
    const auto enabled=[](const char* name) {
        const char* value=std::getenv(name);
        return value && std::strcmp(value,"1")==0;
    };
    if (!enabled("OGL_VULKAN_WORLD_RGBA_IMAGE_PROBE") ||
        !enabled("OGL_VULKAN_WORLD_UV_CHECKER_PROBE") ||
        !enabled("OGL_VULKAN_WORLD_FACE_TINT_PROBE") ||
        !enabled("OGL_VULKAN_WORLD_LAMBERT_PROBE") ||
        !enabled("OGL_VULKAN_WORLD_SUN_LIGHT_PROBE") ||
        !enabled("OGL_VULKAN_WORLD_MESH_UPLOAD") ||
        !enabled("OGL_VULKAN_WORLD_CAMERA_PROBE") ||
        !world_device.device || !world_device.device_proc ||
        world_device.offscreen_world_cache.submitted)
        return 0;
    if (!rgba && width==0 && height==0) {
        // Drop the sampled-image cache before any descriptor/image is freed.
        if (world_device.offscreen_world_cache.initialized &&
            world_device.offscreen_world_cache.shading_mode>=5u) {
            world_device.release_offscreen_world_cache();
            if (world_device.offscreen_world_cache.initialized ||
                world_device.offscreen_world_cache.submitted)
                return 0;
        }
        world_device.clear_rgba_image();
        return 1;
    }
    const bool atlas_requested=enabled("OGL_VULKAN_WORLD_MATERIAL_ATLAS_PROBE");
    const bool atlas16_requested=enabled(
        "OGL_VULKAN_WORLD_MATERIAL_ATLAS_16_PROBE");
    const bool hq_requested=enabled(
        "OGL_VULKAN_WORLD_MATERIAL_ATLAS_HQ_PROBE");
    if (hq_requested && (!atlas_requested || !atlas16_requested))
        return 0;
    if (atlas_requested && (width!=(hq_requested ? 256u : 64u) ||
        height!=(hq_requested ? 256u : 64u) ||
        !world_device.mesh_material_slot_vertices ||
        !world_device.material_slot_count ||
        world_device.material_slot_mesh_generation!=world_device.mesh_generation ||
        world_device.material_slot_vertex_count!=world_device.mesh_triangle_count*3u))
        return 0;
    const uint32_t max_source_edge=hq_requested ? 256u : 64u;
    if (!rgba || !width || !height ||
        width>max_source_edge || height>max_source_edge ||
        !world_device.mesh_vertices || !world_device.mesh_indices ||
        !world_device.mesh_tint_vertices || !world_device.mesh_uv_vertices ||
        world_device.tint_mesh_generation!=world_device.mesh_generation ||
        world_device.uv_mesh_generation!=world_device.mesh_generation)
        return 0;
    const size_t bytes=size_t(width)*height*4u; // bounded <=262144
    for (size_t i=3;i<bytes;i+=4)
        if (rgba[i]!=255u) return 0; // alpha blending not implemented
    std::vector<uint8_t> accepted(rgba,rgba+bytes);
    if (world_device.offscreen_world_cache.initialized &&
        world_device.offscreen_world_cache.shading_mode>=5u) {
        world_device.release_offscreen_world_cache();
        if (world_device.offscreen_world_cache.initialized ||
            world_device.offscreen_world_cache.submitted)
            return 0;
    }
    world_device.rgba_cpu_texels.swap(accepted);
    world_device.rgba_width=width;
    world_device.rgba_height=height;
    world_device.rgba_mesh_generation=world_device.mesh_generation;
    ++world_device.rgba_uploads;
    return 1;
}

void gpu_probe_cleanup() {
    probe_run = nullptr;
    probe_camera_run = nullptr;
    probe_depth_run = nullptr;
    probe_dense_run = nullptr;
    if (probe_library) gpu_close(probe_library);
    probe_library = nullptr;
}


ogl_rtx_frame_state latest_frame{};
uint64_t received_frames = 0;
ogl::DynamicScene tracked_objects; // stable per-UUID CPU metadata
std::vector<ogl_rtx_triangle> captured_geometry;
uint64_t geometry_revision = 0;
ogl_rtx_shadow_debug last_shadow_debug{};
bool shadow_debug_valid = false;
ogl_rtx_world_shadow_sample last_world_shadow{};
bool world_shadow_valid = false;
ogl_rtx_depth_debug last_depth_debug{};
ogl::DepthStats last_depth_stats{};
bool depth_debug_valid = false;
void report(std::string message) {
    std::snprintf(status.message, sizeof(status.message), "%s", message.c_str());
}
void detect() {
    status = {};
    status.struct_size = sizeof(status);
    status.status_version = OGL_RTX_STATUS_VERSION;

    VulkanLibrary library = open_vulkan_loader();
    if (!library) { report("Vulkan loader absent; OpenGL fallback."); return; }

    const auto get_proc = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
        lookup_vulkan_symbol(library, "vkGetInstanceProcAddr"));
    if (!get_proc) { report("Vulkan loader symbol unavailable."); close_vulkan_loader(library); return; }
    const auto create = reinterpret_cast<PFN_vkCreateInstance>(
        get_proc(VK_NULL_HANDLE, "vkCreateInstance"));
    if (!create) { report("Vulkan instance creation unavailable."); close_vulkan_loader(library); return; }

    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "OGL RTX capability";
    app.pEngineName = "OGL Viewer";
    app.apiVersion = VK_API_VERSION_1_2;
    VkInstanceCreateInfo init{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    init.pApplicationInfo = &app;
    VkInstance instance = VK_NULL_HANDLE;
    if (create(&init, nullptr, &instance) != VK_SUCCESS || !instance) {
        report("Vulkan 1.2 initialization failed.");
        close_vulkan_loader(library);
        return;
    }

    const auto destroy = reinterpret_cast<PFN_vkDestroyInstance>(
        get_proc(instance, "vkDestroyInstance"));
    const auto enumerate = reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(
        get_proc(instance, "vkEnumeratePhysicalDevices"));
    const auto props = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties>(
        get_proc(instance, "vkGetPhysicalDeviceProperties"));
    const auto extensions = reinterpret_cast<PFN_vkEnumerateDeviceExtensionProperties>(
        get_proc(instance, "vkEnumerateDeviceExtensionProperties"));
    const auto features = reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures2>(
        get_proc(instance, "vkGetPhysicalDeviceFeatures2"));

    if (!destroy || !enumerate || !props || !extensions || !features) {
        report("Vulkan physical device inspection is unavailable.");
    } else {
        uint32_t count = 0;
        if (enumerate(instance, &count, nullptr) == VK_SUCCESS && count) {
            std::vector<VkPhysicalDevice> devices(count);
            if (enumerate(instance, &count, devices.data()) == VK_SUCCESS) {
                // Prefer a capable discrete device when several adapters
                // are present. Never select hardware by vendor/brand name.
                for (unsigned preference = 0; preference < 2 && !status.hardware_ready; ++preference) {
                    for (uint32_t i = 0; i < count; ++i) {
                        VkPhysicalDeviceProperties gpu{};
                        props(devices[i], &gpu);
                        const bool discrete = gpu.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
                        if ((preference == 0) != discrete) continue;
                        status.vulkan_gpu = 1;
                        uint32_t ext_count = 0;
                        if (extensions(devices[i], nullptr, &ext_count, nullptr) != VK_SUCCESS)
                            continue;
                        std::vector<VkExtensionProperties> exts(ext_count);
                        if (extensions(devices[i], nullptr, &ext_count, exts.data()) != VK_SUCCESS)
                            continue;
                        std::set<std::string> names;
                        for (const auto &ext : exts) names.insert(ext.extensionName);
                        const bool accel_ext = names.count(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
                        const bool query_ext = names.count(VK_KHR_RAY_QUERY_EXTENSION_NAME);
                        const bool ray_ext = names.count(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
                        const bool deferred_ext = names.count(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
                        if (!accel_ext || !deferred_ext || (!query_ext && !ray_ext))
                            continue;
                        VkPhysicalDeviceAccelerationStructureFeaturesKHR accel{
                            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};
                        VkPhysicalDeviceRayQueryFeaturesKHR query{
                            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR};
                        VkPhysicalDeviceRayTracingPipelineFeaturesKHR ray{
                            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR};
                        VkPhysicalDeviceBufferDeviceAddressFeatures address{
                            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES};
                        VkPhysicalDeviceFeatures2 root{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
                        // Keep unsupported extension structs out of pNext.
                        root.pNext = &address;
                        address.pNext = &accel;
                        if (query_ext) accel.pNext = &query;
                        if (ray_ext) {
                            if (query_ext) query.pNext = &ray;
                            else accel.pNext = &ray;
                        }
                        features(devices[i], &root);
                        const bool acceleration_ready = accel.accelerationStructure == VK_TRUE &&
                                                         address.bufferDeviceAddress == VK_TRUE;
                        const bool ray_query_ready = query_ext && query.rayQuery == VK_TRUE;
                        const bool ray_pipeline_ready = ray_ext && ray.rayTracingPipeline == VK_TRUE;
                        if (!acceleration_ready || (!ray_query_ready && !ray_pipeline_ready))
                            continue;
                        // Only publish the selected capable adapter's status.
                        status.api_version = gpu.apiVersion;
                        std::snprintf(status.gpu_name, sizeof(status.gpu_name), "%s", gpu.deviceName);
                        status.acceleration_structure = 1;
                        status.ray_query = ray_query_ready;
                        status.ray_tracing_pipeline = ray_pipeline_ready;
                        status.hardware_ready = 1;
                        report("Vulkan RT supported; OpenGL active until RT renderer integration.");
                        break;
                    }
                }
            }
        }
        if (!status.hardware_ready) report("No suitable Vulkan ray tracing GPU found; OpenGL fallback.");
    }
    if (destroy) destroy(instance, nullptr);
    close_vulkan_loader(library);
}

void poll_probe_on_viewer_thread() {
    if (!probe_worker) return;
    ogl::AsyncGpuQueue::Result outcome{};
    if (!probe_worker->try_take(outcome)) return;
    // In depth-guided diagnostic mode the worker owns an immutable frame,
    // mesh and depth snapshot. It is a *labeled, expiring UI preview*, not
    // live world shading. New frame/mesh samples must not cancel this frozen
    // GPU job. Real region changes and newer depth captures still cancel it.
    if (outcome.revision != probe_job_sequence ||
        (!probe_depth_enabled && outcome.geometry_revision != geometry_revision) ||
        captured_geometry.empty())
        return;
    if (outcome.return_code == 0 && outcome.dense) {
        dense_shadow_valid=false;
        last_dense_shadow={};
        if (probe_dense_enabled && outcome.has_depth_grid &&
            outcome.geometry_revision==geometry_revision &&
            ogl::same_rt_frame(outcome.capture_frame,latest_frame)) {
            ogl_rtx_dense_shadow ready{};
            ready.struct_size=sizeof(ready);
            ready.version=OGL_RTX_DENSE_VERSION;
            ready.geometry_revision=outcome.geometry_revision;
            ready.sample_sequence=outcome.revision;
            ready.capture_frame=outcome.capture_frame;
            ready.width=OGL_RTX_DENSE_WIDTH;
            ready.height=OGL_RTX_DENSE_HEIGHT;
            ready.surface_pixels=outcome.surface_pixels;
            ready.matched_pixels=outcome.primary_depth_matches;
            ready.shadow_pixels=outcome.occluded;
            std::copy(outcome.dense_valid_mask.begin(),
                      outcome.dense_valid_mask.end(),std::begin(ready.valid_mask));
            std::copy(outcome.dense_shadow_mask.begin(),
                      outcome.dense_shadow_mask.end(),std::begin(ready.shadow_mask));
            std::copy(outcome.dense_capture_depth.begin(),
                      outcome.dense_capture_depth.end(),
                      std::begin(ready.capture_depth));
            if (ogl::valid_dense_shadow(ready,geometry_revision,latest_frame)) {
                last_dense_shadow=ready;
                dense_shadow_valid=true;
                if (logger) logger(1,"OGL RTX dense source synchronized; "
                                  "128x72 GPU depth matches, no compositing");
            }
        }
        if (logger)
            logger(1,(std::string("OGL RTX dense GPU probe: triangles=")+
                std::to_string(outcome.triangle_count)+
                " rays="+std::to_string(outcome.ray_count)+
                " surface="+std::to_string(outcome.surface_pixels)+
                " primary="+std::to_string(outcome.primary_hits)+
                " matched="+std::to_string(outcome.primary_depth_matches)+
                " shadowed="+std::to_string(outcome.occluded)+
                " revision="+std::to_string(outcome.geometry_revision)+
                " worker_wall_ms="+std::to_string(outcome.wall_ms)+
                "; GPU tile NOT yet composited").c_str());
        return;
    }
    if (outcome.return_code == 0) {
        ogl_rtx_shadow_debug fresh{};
        fresh.struct_size = sizeof(fresh);
        fresh.version = OGL_RTX_SHADOW_DEBUG_VERSION;
        fresh.geometry_revision = outcome.geometry_revision;
        fresh.sample_sequence = outcome.revision;
        fresh.width = 8;
        fresh.height = 8;
        fresh.occluded = outcome.occluded;
        fresh.surface_pixels = outcome.surface_pixels;
        fresh.background_pixels = outcome.background_pixels;
        std::copy(outcome.shadow_mask.begin(), outcome.shadow_mask.end(),
                  std::begin(fresh.pixels));
        std::copy(outcome.surface_mask.begin(), outcome.surface_mask.end(),
                  std::begin(fresh.surface_mask));
        last_shadow_debug = fresh; // publisher is the viewer main thread only
        shadow_debug_valid = true;
        world_shadow_valid = false;
        last_world_shadow = {};
        if (probe_depth_enabled && outcome.has_depth_grid &&
            outcome.world_mask_eligible &&
            outcome.geometry_revision == geometry_revision &&
            ogl::same_rt_frame(outcome.capture_frame,latest_frame)) {
            ogl_rtx_world_shadow_sample ready{};
            ready.struct_size=sizeof(ready);
            ready.version=OGL_RTX_WORLD_SHADOW_VERSION;
            ready.geometry_revision=outcome.geometry_revision;
            ready.sample_sequence=outcome.revision;
            ready.capture_frame=outcome.capture_frame;
            ready.matched_surface_pixels=outcome.primary_depth_matches;
            ready.occluded_pixels=outcome.occluded;
            std::copy(outcome.depth_match_mask.begin(),
                      outcome.depth_match_mask.end(),
                      std::begin(ready.valid_mask));
            std::copy(outcome.shadow_mask.begin(),
                      outcome.shadow_mask.end(),
                      std::begin(ready.occlusion_mask));
            std::copy(outcome.capture_depth.begin(),
                      outcome.capture_depth.end(),
                      std::begin(ready.capture_depth));
            if (ogl::valid_world_shadow_sample(
                ready,geometry_revision,latest_frame)) {
                last_world_shadow=ready;
                world_shadow_valid=true;
                if (logger) logger(1,
                    "OGL RTX world-shadow source synchronized and depth-matched;"
                    " GPU pixels still NOT composited");
            }
        }
        if (logger && probe_depth_enabled && !outcome.world_mask_eligible)
            logger(1,(std::string("OGL RTX world-shadow mask rejected: ") +
                std::to_string(outcome.world_mismatch_pixels) +
                " pixel(s) disagree with depth match; diagnostic GPU "
                "result preserved, no world-shadow source").c_str());
        if (logger)
            logger(1, (std::string("OGL RTX GPU probe: triangles=") +
                std::to_string(outcome.triangle_count) + " rays=" +
                std::to_string(outcome.ray_count) + " occluded=" +
                std::to_string(outcome.occluded) + " revision=" +
                std::to_string(outcome.geometry_revision) + " device_inits=" +
                std::to_string(outcome.gpu_device_creations) + " blas_builds=" +
                std::to_string(outcome.blas_builds) + " blas_refits=" +
                std::to_string(outcome.blas_refits) + " tlas_refits=" +
                std::to_string(outcome.tlas_refits) + " cache_hits=" +
                std::to_string(outcome.cache_hits) +
                (probe_depth_enabled ?
                    " surface_samples=" + std::to_string(outcome.surface_pixels) +
                    " background_samples=" + std::to_string(outcome.background_pixels) +
                    " primary_hits=" + std::to_string(outcome.primary_hits) +
                    " depth_matches=" +
                    std::to_string(outcome.primary_depth_matches) +
                    " primary_closer=" + std::to_string(outcome.primary_closer) +
                    " primary_farther=" + std::to_string(outcome.primary_farther) : "") +
                " worker_wall_ms=" + std::to_string(outcome.wall_ms) +
                "; ASYNC DIAGNOSTIC ONLY no shadow compositing").c_str());
    } else if (logger) {
        logger(1,(std::string("OGL RTX async GPU probe failed; ")
            + "gpu_rc=" + std::to_string(outcome.gpu_return_code)
            + " gpu_success=" + std::to_string(outcome.gpu_reported_success)
            + " outcome_rc=" + std::to_string(outcome.return_code)
            + " gpu_rays=" + std::to_string(outcome.ray_count)
            + " gpu_triangles=" + std::to_string(outcome.triangle_count)
            + " mismatch_pixels=" +
              std::to_string(outcome.world_mismatch_pixels)
            + " dense=" + std::to_string(outcome.dense ? 1 : 0)
            + "; disabled until next start; OpenGL active").c_str());
    }
}
// One scheduler for new camera frames and new mesh snapshots. Throttle
// GPU diagnostics in the opt-in mode; never submit Vulkan on this thread.
void schedule_probe_on_viewer_thread() {
    // Never perform Vulkan calls on this render-thread callback.
    if (probe_worker && received_frames > 0 && !captured_geometry.empty() &&
        (!probe_depth_enabled || depth_debug_valid) &&
        (!probe_dense_enabled || dense_depth_valid) &&
        (geometry_revision != probe_last_revision || probe_view_dirty)) {
        using Clock = std::chrono::steady_clock;
        const auto now = Clock::now();
        if (probe_last_time == Clock::time_point{} ||
            now - probe_last_time >= probe_interval) {
            if (probe_dense_enabled && probe_worker->stats().busy)
                return; // no dense job replacement; finish old worker
            ogl::AsyncGpuQueue::Job job{};
            job.revision = ++probe_job_sequence;
            job.geometry_revision = geometry_revision;
            job.triangles = captured_geometry;
            for (unsigned axis = 0; axis < 3; ++axis) {
                job.sun[axis] = latest_frame.sun_direction[axis];
                job.camera_position[axis] = latest_frame.camera_position[axis];
                job.camera_forward[axis] = latest_frame.camera_forward[axis];
                job.camera_right[axis] = latest_frame.camera_right[axis];
                job.camera_up[axis] = latest_frame.camera_up[axis];
            }
            job.vertical_fov_radians = latest_frame.vertical_fov_radians;
            job.aspect_ratio = latest_frame.aspect_ratio;
            job.near_clip_m = latest_frame.near_clip_m;
            job.far_clip_m = latest_frame.far_clip_m;
            job.viewport_width = latest_frame.width;
            job.viewport_height = latest_frame.height;
            if (probe_dense_enabled) {
                std::copy(std::begin(last_dense_depth.normalized_depth),
                          std::end(last_dense_depth.normalized_depth),
                          job.dense_depth_grid.begin());
                job.has_dense_grid=true;
                job.surface_pixels=static_cast<uint32_t>(
                    std::count_if(job.dense_depth_grid.begin(),
                        job.dense_depth_grid.end(),
                        [](float v){return v<0.999999f;}));
                job.background_pixels=OGL_RTX_DENSE_PIXELS-job.surface_pixels;
            }
            if (probe_depth_enabled) {
                std::copy(std::begin(last_depth_debug.normalized_depth),
                          std::end(last_depth_debug.normalized_depth),
                          job.depth_grid.begin());
                job.has_depth_grid = true;
                job.surface_pixels = last_depth_stats.surface_pixels;
                job.background_pixels = last_depth_stats.background_pixels;
            }
            if (probe_worker->enqueue(std::move(job))) {
                probe_last_time = now;
                probe_last_revision = geometry_revision;
                probe_view_dirty = false;
                if (logger && probe_dense_enabled)
                    logger(1,"OGL RTX dense GPU snapshot queued: 128x72 "
                              "depth-aligned samples; no world compositing");
                if (logger && probe_depth_enabled)
                    logger(1, "OGL RTX depth-guided GPU snapshot queued; "
                        "async test only; no world compositing");
            }
        }
    }

}

}

extern "C" OGL_MODULE_EXPORT const ogl_module_info *ogl_module_get_info() {
    return &info;
}
extern "C" OGL_MODULE_EXPORT int ogl_module_start(const ogl_host_api *host) {
    if (!host || host->struct_size < sizeof(ogl_host_api) ||
        host->abi_version != OGL_MODULE_ABI_VERSION || !host->log_message)
        return 1;
    world_device.close(); // repeated starts never leave an old VkDevice active
    logger = host->log_message;
    latest_frame = {};
    received_frames = 0;
    captured_geometry.clear();
    geometry_revision = 0;
    last_shadow_debug = {};
    shadow_debug_valid = false;
    last_world_shadow = {};
    world_shadow_valid = false;
    last_depth_debug = {};
    last_depth_stats = {};
    depth_debug_valid = false;
    last_dense_depth={};
    last_dense_shadow={};
    dense_depth_valid=false;
    dense_shadow_valid=false;
    dense_busy_depth_skips=0;
    probe_worker.reset(); // join before unloading Vulkan function pointers
    gpu_probe_cleanup();
    probe_opt_in = false;
    probe_camera_enabled = false;
    probe_depth_enabled = false;
    probe_dense_enabled = false;
    probe_last_revision = 0;
    probe_job_sequence = 0;
    probe_view_dirty = false;
    probe_last_time = {};
    probe_interval = std::chrono::milliseconds(60000);
    const char* env = std::getenv("OGL_RTX_SCENE_PROBE");
    probe_opt_in = env && std::strcmp(env, "1") == 0;
    // Explicit opt-in developer test override, never used for normal users.
    const char* delay = probe_opt_in ? std::getenv("OGL_RTX_DEV_INTERVAL_MS") : nullptr;
    if (delay && *delay) {
        char* end = nullptr;
        const unsigned long interval = std::strtoul(delay, &end, 10);
        if (end && *end == '\0' && interval >= 10 && interval <= 60000)
            probe_interval = std::chrono::milliseconds(interval);
    }
    const char* camera_env = std::getenv("OGL_RTX_CAMERA_PROBE");
    probe_camera_enabled = probe_opt_in && camera_env &&
                           std::strcmp(camera_env, "1") == 0;
    const char* depth_env = std::getenv("OGL_RTX_DEPTH_GUIDED_PROBE");
    const char* read_env = std::getenv("OGL_RTX_DEPTH_PROBE");
    probe_depth_enabled = probe_camera_enabled && depth_env && read_env &&
                          std::strcmp(depth_env, "1") == 0 &&
                          std::strcmp(read_env, "1") == 0;
    const char* dense_env=std::getenv("OGL_RTX_DENSE_PREVIEW");
    probe_dense_enabled=probe_camera_enabled && dense_env &&
                        std::strcmp(dense_env,"1")==0;
    if (probe_dense_enabled) {
        probe_depth_enabled=false; // mutually exclusive worker modes
        // Main-thread OpenGL capture is already capped at >=2 seconds.
        // A slightly shorter worker throttle prevents a captured depth
        // frame being dropped on scheduling jitter and then invalidated
        // on the next rendered OpenGL frame.
        probe_interval=std::chrono::milliseconds(1000);
    }
    detect();
    const char* world_env=std::getenv("OGL_VULKAN_WORLD_BOOTSTRAP");
    if (world_env && std::strcmp(world_env,"1")==0) {
        const bool ok=init_world_graphics_device();
        logger(1, ok
            ? "P05 graphics device/queue initialized; surface and world renderer NOT implemented."
            : "P05 graphics device/queue unavailable; preserving OpenGL.");
    }
    // Opt-in is only for developers until we have a non-blocking renderer.
    // No developer/system-specific GPU, user data, or filesystem path.
    if (probe_opt_in && status.hardware_ready && status.ray_query) {
        probe_library = load_gpu_probe();
        if (probe_library) {
            probe_run = reinterpret_cast<ProbeRun>(
                gpu_symbol(probe_library, "ogl_gpu_probe_run"));
            probe_camera_run = reinterpret_cast<CameraProbeRun>(
                gpu_symbol(probe_library, "ogl_gpu_probe_camera_run"));
            probe_depth_run = reinterpret_cast<DepthProbeRun>(
                gpu_symbol(probe_library, "ogl_gpu_probe_depth_run"));
            probe_dense_run = reinterpret_cast<DenseProbeRun>(
                gpu_symbol(probe_library, "ogl_gpu_probe_dense_run"));
        }
        if (!probe_run || (probe_camera_enabled && !probe_camera_run) ||
            (probe_depth_enabled && !probe_depth_run) ||
            (probe_dense_enabled && !probe_dense_run)) {
            if (logger) logger(1, "OGL RTX opt-in GPU probe unavailable; OpenGL unaffected");
        } else {
            // Worker calls Vulkan. Immutable data; no viewer callbacks.
            try {
            probe_worker.reset(new ogl::AsyncGpuQueue([](
                const ogl::AsyncGpuQueue::Job& job) {
                ogl_gpu_dense_result native_dense{};
                native_dense.struct_size=sizeof(native_dense);
                native_dense.version=OGL_GPU_DENSE_VERSION;
                ogl_gpu_probe_result native{};
                native.struct_size = sizeof(native);
                native.version = OGL_GPU_PROBE_VERSION;
                int rc = 1;
                ogl_rtx_frame_state frame{};
                if (probe_camera_enabled && probe_camera_run) {
                    frame.struct_size = sizeof(frame);
                    frame.frame_version = OGL_RTX_FRAME_VERSION;
                    frame.width = job.viewport_width;
                    frame.height = job.viewport_height;
                    for (unsigned axis=0; axis<3; ++axis) {
                        frame.camera_position[axis] = job.camera_position[axis];
                        frame.camera_forward[axis] = job.camera_forward[axis];
                        frame.camera_right[axis] = job.camera_right[axis];
                        frame.camera_up[axis] = job.camera_up[axis];
                        frame.sun_direction[axis] = job.sun[axis];
                    }
                    frame.vertical_fov_radians = job.vertical_fov_radians;
                    frame.aspect_ratio = job.aspect_ratio;
                    frame.near_clip_m = job.near_clip_m;
                    frame.far_clip_m = job.far_clip_m;
                    if (probe_dense_enabled) {
                        rc=job.has_dense_grid && probe_dense_run ?
                            probe_dense_run(job.triangles.data(),
                                static_cast<uint32_t>(job.triangles.size()),
                                &frame,job.dense_depth_grid.data(),
                                trusted_dense_shader,&native_dense) : 1;
                    } else if (probe_depth_enabled) {
                        // Unlike the old scene-only fixture, every depth
                        // sample is an immutable copy from Firestorm's frame.
                        rc = job.has_depth_grid && probe_depth_run ?
                            probe_depth_run(job.triangles.data(),
                                static_cast<uint32_t>(job.triangles.size()),
                                &frame, job.depth_grid.data(),
                                trusted_depth_shader, &native) : 1;
                    } else {
                        rc = probe_camera_run(job.triangles.data(),
                            static_cast<uint32_t>(job.triangles.size()),
                            &frame, trusted_camera_shader, &native);
                    }
                } else {
                    rc = probe_run(job.triangles.data(),
                        static_cast<uint32_t>(job.triangles.size()),
                        job.sun.data(), trusted_shader, &native);
                }
                ogl::AsyncGpuQueue::Result result{};
                const bool dense_mode=job.has_dense_grid;
                result.dense=dense_mode;
                result.return_code=(rc==0 && (dense_mode ?
                    native_dense.success : native.success)) ? 0 : 1;
                if (dense_mode) {
                    result.triangle_count=native_dense.triangle_count;
                    result.ray_count=native_dense.ray_count;
                    result.occluded=native_dense.occluded;
                    result.primary_hits=native_dense.primary_hits;
                    result.primary_depth_matches=native_dense.depth_matches;
                    result.has_depth_grid=true;
                    result.capture_frame=frame;
                    result.dense_capture_depth=job.dense_depth_grid;
                    std::copy(std::begin(native_dense.shadow_mask),
                              std::end(native_dense.shadow_mask),
                              result.dense_shadow_mask.begin());
                    std::copy(std::begin(native_dense.depth_match_mask),
                              std::end(native_dense.depth_match_mask),
                              result.dense_valid_mask.begin());
                    result.surface_pixels=job.surface_pixels;
                    result.background_pixels=job.background_pixels;
                    result.world_mask_eligible=true;
                    result.gpu_return_code=rc;
                    result.gpu_reported_success=native_dense.success;
                    return result;
                }
                result.gpu_return_code=rc;
                result.gpu_reported_success=native.success;
                result.triangle_count = native.triangle_count;
                result.ray_count = native.ray_count;
                result.occluded = native.occluded;
                result.primary_hits = native.primary_hits;
                result.primary_depth_matches = native.primary_depth_matches;
                result.primary_closer = native.primary_closer;
                result.primary_farther = native.primary_farther;
                result.surface_pixels = job.surface_pixels;
                result.background_pixels = job.background_pixels;
                result.has_depth_grid=job.has_depth_grid;
                if (job.has_depth_grid) {
                    result.capture_frame=frame;
                    result.capture_depth=job.depth_grid;
                    std::copy(std::begin(native.depth_match_mask),
                              std::end(native.depth_match_mask),
                              result.depth_match_mask.begin());
                    result.world_mask_eligible=
                        ogl::world_shadow_mask_matches_depth(
                            native.shadow_mask,native.depth_match_mask);
                    for (unsigned i=0;i<64;++i)
                        if (native.shadow_mask[i] &&
                            native.depth_match_mask[i]!=1u)
                            ++result.world_mismatch_pixels;
                    // World-pixel validation is fail-closed, but independent
                    // from native Vulkan diagnostic health. The optional
                    // world source will be rejected without falsely
                    // declaring the Vulkan worker broken.
                }
                if (job.has_depth_grid) {
                    for (unsigned i=0; i<64; ++i) {
                        float distance=0.f;
                        result.surface_mask[i] =
                            ogl::gl_depth_to_view_distance(
                                job.depth_grid[i],job.near_clip_m,
                                job.far_clip_m,distance) ? 1u : 0u;
                    }
                }
                if (native.mask_width != 8 || native.mask_height != 8) {
                    result.return_code = 1;
                } else {
                    std::copy(std::begin(native.shadow_mask),
                              std::end(native.shadow_mask),
                              result.shadow_mask.begin());
                }
                result.gpu_device_creations = native.vulkan_device_creations;
                result.blas_builds = native.blas_builds;
                result.blas_refits = native.blas_refits;
                result.tlas_refits = native.tlas_refits;
                result.cache_hits = native.cache_hits;
                return result;
            }));
            } catch (...) {
                probe_worker.reset();
                if (logger) logger(1,
                    "OGL RTX async worker failed to start; OpenGL unaffected");
            }
        }
    }
    if (probe_opt_in && logger)
        logger(1, (std::string("OGL RTX developer probe configured: worker=") +
            (probe_worker ? "1" : "0") +
            " camera=" + (probe_camera_enabled ? "1" : "0") +
            " depth_guided=" + (probe_depth_enabled ? "1" : "0") +
            " dense=" + (probe_dense_enabled ? "1" : "0") +
            "; diagnostic only").c_str());
    std::string gpu = status.gpu_name[0] ? status.gpu_name : "none";
    logger(1, (std::string("OGL RTX capability: gpu=") + gpu +
        " acceleration=" + std::to_string(status.acceleration_structure) +
        " query=" + std::to_string(status.ray_query) +
        " pipeline=" + std::to_string(status.ray_tracing_pipeline) +
        " hardware_ready=" + std::to_string(status.hardware_ready) +
        " rendering_active=0; " + status.message).c_str());
    return 0; // No RTX hardware is normal: keep the OpenGL renderer.
}
extern "C" OGL_MODULE_EXPORT void ogl_module_stop() {
    world_device.close();
    probe_worker.reset(); // join before unloading GPU helper DLL/SO
    gpu_probe_cleanup();
    probe_opt_in = false;
    probe_camera_enabled = false;
    probe_depth_enabled = false;
    probe_dense_enabled = false;
    last_dense_depth={};last_dense_shadow={};
    dense_depth_valid=false;dense_shadow_valid=false;
    captured_geometry.clear();
    last_shadow_debug = {};
    shadow_debug_valid = false;
    last_world_shadow = {};
    world_shadow_valid = false;
    last_depth_debug = {};
    last_depth_stats = {};
    depth_debug_valid = false;
    logger = nullptr;
}
extern "C" OGL_MODULE_EXPORT int ogl_vulkan_world_upload_triangles(
    const ogl_rtx_triangle* triangles, uint32_t count)
{
    return upload_world_geometry(triangles,count);
}
extern "C" OGL_MODULE_EXPORT int ogl_vulkan_world_upload_face_tints(
    const ogl_rtx_face_tint* tints,uint32_t count)
{
    return upload_world_face_tints(tints,count);
}
extern "C" OGL_MODULE_EXPORT uint64_t ogl_vulkan_world_face_tint_mesh_generation() {
    return world_device.tint_mesh_generation;
}
extern "C" OGL_MODULE_EXPORT uint64_t ogl_vulkan_world_face_tint_uploads() {
    return world_device.tint_uploads;
}
extern "C" OGL_MODULE_EXPORT int ogl_vulkan_world_upload_face_uvs(
    const ogl_rtx_face_uv* uv_triangles,uint32_t count)
{
    return upload_world_face_uvs(uv_triangles,count);
}
extern "C" OGL_MODULE_EXPORT uint64_t ogl_vulkan_world_face_uv_mesh_generation() {
    return world_device.uv_mesh_generation;
}
extern "C" OGL_MODULE_EXPORT uint64_t ogl_vulkan_world_face_uv_uploads() {
    return world_device.uv_uploads;
}
extern "C" OGL_MODULE_EXPORT int ogl_vulkan_world_upload_rgba_image(
    const uint8_t* rgba8,uint32_t width,uint32_t height)
{
    return upload_world_rgba_image(rgba8,width,height);
}
extern "C" OGL_MODULE_EXPORT uint64_t ogl_vulkan_world_rgba_mesh_generation() {
    return world_device.rgba_mesh_generation;
}
extern "C" OGL_MODULE_EXPORT uint64_t ogl_vulkan_world_rgba_uploads() {
    return world_device.rgba_uploads;
}
extern "C" OGL_MODULE_EXPORT int ogl_vulkan_world_upload_material_slots(
    const ogl_rtx_face_material_slot* slots,uint32_t triangle_count,
    uint32_t tile_count)
{
    return upload_world_material_slots(slots,triangle_count,tile_count);
}
extern "C" OGL_MODULE_EXPORT uint64_t ogl_vulkan_world_material_slot_mesh_generation() {
    return world_device.material_slot_mesh_generation;
}
extern "C" OGL_MODULE_EXPORT uint64_t ogl_vulkan_world_material_slot_uploads() {
    return world_device.material_slot_uploads;
}
extern "C" OGL_MODULE_EXPORT int ogl_vulkan_world_set_projection_camera(
    const ogl_rtx_frame_state* frame) {
    return set_world_projection_camera(frame);
}
extern "C" OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_projection_camera_ready() {
    return world_device.world_camera_ready ? 1u : 0u;
}
// 1 only when an actual current Vulkan swapchain is paired with a
// projection from the same viewport aspect. This does not imply a present.
extern "C" OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_camera_matches_swapchain() {
    if (!world_device.world_camera_ready ||
        !world_device.swapchain_probe_ready ||
        !world_device.swapchain_extent.height) return 0;
    return std::fabs(world_device.world_camera_aspect-
        float(world_device.swapchain_extent.width)/
        float(world_device.swapchain_extent.height))<=0.01f ? 1u : 0u;
}
// This is a read-only INPUT handshake, never renderer/present readiness.
// A freshly uploaded mesh invalidates the old camera, so only a subsequent
// camera frame can pair with the new GPU buffers.
extern "C" OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_live_inputs_ready() {
    return world_device.world_camera_ready &&
           world_device.camera_mesh_generation==world_device.mesh_generation &&
           world_device.mesh_triangle_count>0 &&
           world_device.mesh_vertices!=VK_NULL_HANDLE &&
           world_device.mesh_indices!=VK_NULL_HANDLE ? 1u : 0u;
}
extern "C" OGL_MODULE_EXPORT uint64_t ogl_vulkan_world_camera_updates() {
    return world_device.world_camera_accepts;
}
extern "C" OGL_MODULE_EXPORT uint64_t ogl_vulkan_world_mesh_uploads() {
    return world_device.mesh_uploads;
}
// Scene generations distinguish accepted mesh snapshots and region-clear.
// These are CPU/queue lifetime diagnostics, NOT live OpenSim region versions.
extern "C" OGL_MODULE_EXPORT int ogl_vulkan_world_invalidate_region() {
    return invalidate_live_world_region();
}
extern "C" OGL_MODULE_EXPORT uint64_t ogl_vulkan_world_region_barrier_accepts() {
    return world_device.region_barrier_accepts;
}
extern "C" OGL_MODULE_EXPORT uint64_t ogl_vulkan_world_region_barrier_failures() {
    return world_device.region_barrier_failures;
}
extern "C" OGL_MODULE_EXPORT uint64_t ogl_vulkan_world_mesh_generation() {
    return world_device.mesh_generation;
}
extern "C" OGL_MODULE_EXPORT uint64_t ogl_vulkan_world_mesh_dedup_skips() {
    return world_device.mesh_dedup_skips;
}
// Diagnostic only: most recently accepted real-world camera's paired mesh
// generation. Zero means no valid pair after a scene update/region clear.
extern "C" OGL_MODULE_EXPORT uint64_t ogl_vulkan_world_camera_mesh_generation() {
    return world_device.world_camera_ready ?
        world_device.camera_mesh_generation : 0u;
}
extern "C" OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_mesh_retire_waits() {
    return world_device.mesh_retire_waits;
}
extern "C" OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_mesh_retire_failures() {
    return world_device.mesh_retire_failures;
}
extern "C" OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_gpu_triangle_count() {
    return world_device.mesh_triangle_count;
}
extern "C" OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_submit_sync_probe() {
    return submit_world_sync_probe();
}
// Only read-only diagnostic counts; a submitted fence is not a presented frame.
extern "C" OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_sync_max_outstanding() {
    return world_device.sync_max_outstanding;
}
extern "C" OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_sync_last_submitted() {
    return world_device.sync_last_submitted;
}
extern "C" OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_sync_last_completed() {
    return world_device.sync_last_completed;
}
extern "C" OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_render_offscreen_probe() {
    return render_offscreen_color_frame();
}
// Render a genuine Vulkan 32x32 indexed world/depth frame to a private
// VkImage while OpenGL retains exclusive ownership of the Viewer window.
extern "C" OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_render_offscreen_scene() {
    return render_offscreen_color_frame(true);
}
extern "C" OGL_MODULE_EXPORT uint64_t ogl_vulkan_world_offscreen_scene_frames() {
    return world_device.offscreen_world_frames;
}
extern "C" OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_offscreen_scene_pixels() {
    return world_device.offscreen_world_covered_pixels;
}
extern "C" OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_offscreen_scene_width() {
    return world_device.offscreen_world_width;
}
extern "C" OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_offscreen_scene_height() {
    return world_device.offscreen_world_height;
}
extern "C" OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_offscreen_scene_lit_pixels() {
    return world_device.offscreen_world_lit_pixels;
}
extern "C" OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_offscreen_scene_mean_luminance() {
    return world_device.offscreen_world_mean_luminance;
}
extern "C" OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_offscreen_scene_mean_red() {
    return world_device.offscreen_world_mean_red;
}
extern "C" OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_offscreen_scene_mean_green() {
    return world_device.offscreen_world_mean_green;
}
extern "C" OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_offscreen_scene_mean_blue() {
    return world_device.offscreen_world_mean_blue;
}
extern "C" OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_offscreen_cache_builds() {
    return world_device.offscreen_world_builds;
}
extern "C" OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_offscreen_cache_reuses() {
    return world_device.offscreen_world_reuses;
}
extern "C" OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_offscreen_cache_releases() {
    return world_device.offscreen_world_releases;
}
extern "C" OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_offscreen_cache_release() {
    const char* flag=std::getenv("OGL_VULKAN_WORLD_OFFSCREEN_RASTER");
    if (!flag || std::strcmp(flag,"1")!=0 || !world_device.device)
        return 0;
    auto& cache=world_device.offscreen_world_cache;
    if (!cache.initialized || cache.submitted) return 0;
    world_device.release_offscreen_world_cache();
    return cache.initialized ? 0u : 1u;
}
extern "C" OGL_MODULE_EXPORT int ogl_vulkan_world_device_bootstrapped() {
    return world_device.device != VK_NULL_HANDLE &&
           world_device.queue != VK_NULL_HANDLE;
}
extern "C" OGL_MODULE_EXPORT int ogl_vulkan_world_attach_x11_window(
    void* display, uint64_t window, uint32_t width, uint32_t height)
{
    return attach_world_xlib_window(display,window,width,height) ? 1 : 0;
}
extern "C" OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_present_exclusive_test_frame(
    uint32_t gl_released)
{
    return present_exclusive_native_test_frame(gl_released);
}
extern "C" OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_persistent_clear_frame(
    uint32_t gl_released)
{
    return present_persistent_clear_frame(gl_released);
}
extern "C" OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_persistent_frame_count() {
    return world_device.persistent.frame_count;
}
extern "C" OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_persistent_build_count() {
    return world_device.persistent_builds;
}
extern "C" OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_persistent_release_count() {
    return world_device.persistent_releases;
}
extern "C" OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_persistent_teardown_idles() {
    return world_device.persistent_teardown_idles;
}
extern "C" OGL_MODULE_EXPORT uint32_t ogl_vulkan_world_exclusive_present_count() {
    return world_device.exclusive_present_count;
}
extern "C" OGL_MODULE_EXPORT int ogl_vulkan_world_swapchain_probe_ready() {
    return world_device.swapchain_probe_ready ? 1 : 0;
}
extern "C" OGL_MODULE_EXPORT void ogl_vulkan_world_detach_window() {
    world_device.detach_window();
}
extern "C" OGL_MODULE_EXPORT const ogl_rtx_status *ogl_rtx_get_status() {
    return &status;
}
extern "C" OGL_MODULE_EXPORT int ogl_rtx_submit_frame(const ogl_rtx_frame_state *frame) {
    if (!frame || !status.hardware_ready ||
        !ogl::valid_camera_frame(*frame)) return 0;
    // This diagnostic mask is tied to camera and sunlight. Even a
    // completed GPU query becomes obsolete after any of these changes.
    bool view_changed = received_frames > 0 &&
        (latest_frame.width != frame->width ||
         latest_frame.height != frame->height ||
         latest_frame.vertical_fov_radians != frame->vertical_fov_radians ||
         latest_frame.aspect_ratio != frame->aspect_ratio ||
         latest_frame.near_clip_m != frame->near_clip_m ||
         latest_frame.far_clip_m != frame->far_clip_m);
    if (received_frames > 0)
        for (unsigned i=0; i<3; ++i)
            if (latest_frame.sun_direction[i] != frame->sun_direction[i] ||
                latest_frame.camera_position[i] != frame->camera_position[i] ||
                latest_frame.camera_forward[i] != frame->camera_forward[i] ||
                latest_frame.camera_right[i] != frame->camera_right[i] ||
                latest_frame.camera_up[i] != frame->camera_up[i])
                view_changed = true;
    if (view_changed) {
        // Even if a frozen GPU debug job is allowed to finish, never feed
        // a stale camera to an actual world compositor.
        last_world_shadow={};
        world_shadow_valid=false;
        last_dense_shadow={};
        dense_shadow_valid=false;
        probe_view_dirty = true;
        // The depth-guided preview is a frozen frame snapshot. Camera
        // movement must not cancel GPU work in progress, or clear the
        // previous 8-second debug tile. Neither is used for world shading.
        if (!probe_depth_enabled) {
            shadow_debug_valid = false;
            last_shadow_debug = {};
            // Dense mode is costly: finish an in-flight immutable GPU
            // snapshot for diagnostics, but never reuse its shadow pixels
            // against a changed camera (dense_shadow_valid was cleared).
            // New jobs will be admitted after the worker becomes idle.
            if (probe_worker && !probe_dense_enabled)
                probe_worker->invalidate(++probe_job_sequence);
        }
    }
    // A depth tile belongs to exactly one submitted frame. No reuse after
    // camera motion, resize, new lighting or a later rendered frame.
    last_depth_debug = {};
    last_depth_stats = {};
    depth_debug_valid = false;
    dense_depth_valid = false;
    latest_frame = *frame;
    ++received_frames;
    if (probe_worker) {
        schedule_probe_on_viewer_thread();
        if (received_frames % 30 == 0)
            poll_probe_on_viewer_thread();
    }
    // Vulkan diagnostics may run only with explicit developer opt-in,
    // after a valid scene snapshot. No user-visible RT pixels are produced.
    if (received_frames == 1 && logger)
        logger(1, "OGL RTX camera/sun frame bridge connected; no raytraced pixels yet.");
    return 1;
}
extern "C" OGL_MODULE_EXPORT uint64_t ogl_rtx_received_frames(void) {
    return received_frames;
}
extern "C" OGL_MODULE_EXPORT int ogl_rtx_submit_geometry(
    const ogl_rtx_triangle *triangles, uint32_t count)
{
    if (!status.hardware_ready || count > OGL_RTX_MAX_SCENE_TRIANGLES ||
        (count && !triangles)) return 0;
    if (!count) {
        last_world_shadow={};
        world_shadow_valid=false;
        last_dense_shadow={};
        dense_shadow_valid=false;
        // A region change invalidates per-UUID metadata even if no aggregate
        // GPU triangle snapshot was available yet.
        tracked_objects.clear();
        if (!captured_geometry.empty()) {
            captured_geometry.clear();
            ++geometry_revision; // Teleport/region change invalidates old AS.
            last_shadow_debug = {};
            shadow_debug_valid = false;
            probe_view_dirty = false;
            if (probe_worker) probe_worker->invalidate(++probe_job_sequence);
            if (logger)
                logger(1, (std::string("OGL RTX CPU scene cleared revision=") +
                           std::to_string(geometry_revision)).c_str());
        }
        return 1;
    }
    for (uint32_t i = 0; i < count; ++i)
        if (!ogl::is_valid_triangle(triangles[i])) return 0;
    const size_t data_size = size_t(count) * sizeof(ogl_rtx_triangle);
    if (captured_geometry.size() == count &&
        std::memcmp(captured_geometry.data(), triangles, data_size) == 0)
        return 1; // avoid rebuilding BLAS for unchanged scene samples
    const bool first_batch = captured_geometry.empty();
    captured_geometry.assign(triangles, triangles + count);
    last_world_shadow={};
    world_shadow_valid=false;
    last_dense_shadow={};
    dense_shadow_valid=false;
    ++geometry_revision;
    probe_view_dirty = true;
    if (!probe_depth_enabled) {
        last_shadow_debug = {};
        shadow_debug_valid = false;
        // Never cancel an in-flight dense ray query merely because another
        // rotating 8192-triangle snapshot was published. It may complete,
        // but the strict current-revision check forbids stale shading.
        if (probe_worker && !probe_dense_enabled)
            probe_worker->invalidate(++probe_job_sequence);
    }
    // The depth-guided developer preview is bound to a frozen camera+depth
    // frame. Geometry scans may change before Vulkan finishes; do not
    // cancel an already queued snapshot unless the region actually changes.
    schedule_probe_on_viewer_thread();
    poll_probe_on_viewer_thread();
    // Log numbers only, never login data or private scene object names.
    // Keep updates sparse to avoid I/O overhead in populated OpenSim regions.
    // The opt-in GPU depth probe needs exact CPU revision logging;
    // otherwise sparse revision-30 logs can incorrectly look newer/older.
    if (logger && (probe_opt_in || first_batch || geometry_revision % 30 == 0))
        logger(1, (std::string("OGL RTX CPU scene snapshot: triangles=") +
                   std::to_string(count) + " revision=" +
                   std::to_string(geometry_revision) +
                   "; GPU scene bridge test only; no in-viewer raytracing").c_str());
    return 1;
}
extern "C" OGL_MODULE_EXPORT int ogl_rtx_track_object(
    const uint8_t* uuid16,const ogl_rtx_triangle* triangles,uint32_t count,
    const float* affine3x4)
{
    if (!status.hardware_ready || !probe_opt_in || !uuid16 || !affine3x4)
        return 0;
    ogl::ObjectKey id{};
    ogl::Affine3x4 transform{};
    std::copy(uuid16,uuid16+16,id.begin());
    std::copy(affine3x4,affine3x4+12,transform.begin());
    const auto result=tracked_objects.upsert(id,triangles,count,transform);
    return result!=ogl::SceneChange::Invalid ? 1:0;
}
extern "C" OGL_MODULE_EXPORT int ogl_rtx_track_transform(
    const uint8_t* uuid16,const float* affine3x4)
{
    if (!status.hardware_ready || !probe_opt_in || !uuid16 || !affine3x4)
        return 0;
    ogl::ObjectKey id{};
    ogl::Affine3x4 transform{};
    std::copy(uuid16,uuid16+16,id.begin());
    std::copy(affine3x4,affine3x4+12,transform.begin());
    const auto result=tracked_objects.update_transform(id,transform);
    return result!=ogl::SceneChange::Invalid ? 1:0;
}
extern "C" OGL_MODULE_EXPORT int ogl_rtx_forget_object(const uint8_t* uuid16)
{
    if (!uuid16) return 0;
    ogl::ObjectKey id{};
    std::copy(uuid16,uuid16+16,id.begin());
    tracked_objects.erase(id);
    return 1;
}
extern "C" OGL_MODULE_EXPORT uint32_t ogl_rtx_tracked_objects(void)
{
    return static_cast<uint32_t>(tracked_objects.objects());
}
extern "C" OGL_MODULE_EXPORT uint64_t ogl_rtx_object_geometry_generation(void)
{
    return tracked_objects.geometry_generation();
}
extern "C" OGL_MODULE_EXPORT uint64_t ogl_rtx_object_transform_generation(void)
{
    return tracked_objects.transform_generation();
}

extern "C" OGL_MODULE_EXPORT uint32_t ogl_rtx_received_triangles(void) {
    return static_cast<uint32_t>(captured_geometry.size());
}
extern "C" OGL_MODULE_EXPORT uint64_t ogl_rtx_geometry_revision(void) {
    return geometry_revision;
}
extern "C" OGL_MODULE_EXPORT uint32_t ogl_rtx_copy_geometry(
    ogl_rtx_triangle *dest, uint32_t capacity)
{
    const uint32_t count = static_cast<uint32_t>(captured_geometry.size());
    if (!dest || count == 0 || capacity < count) return 0;
    std::memcpy(dest, captured_geometry.data(), size_t(count) * sizeof(*dest));
    return count;
}


extern "C" OGL_MODULE_EXPORT int ogl_rtx_submit_depth_debug(
    const ogl_rtx_depth_debug *sample)
{
    if (!sample || !status.hardware_ready || !received_frames ||
        sample->struct_size != sizeof(*sample) ||
        sample->version != OGL_RTX_DEPTH_DEBUG_VERSION ||
        sample->viewport_width == 0 || sample->viewport_height == 0 ||
        std::fabs((float(sample->viewport_width) / sample->viewport_height) -
                  latest_frame.aspect_ratio) > 0.15f ||
        !ogl::valid_depth_grid(sample->viewport_width, sample->viewport_height,
                               sample->grid_width, sample->grid_height,
                               sample->normalized_depth))
        return 0;
    ogl::DepthStats surface_stats{};
    if (!ogl::summarize_depth_grid(sample->normalized_depth,
            sample->viewport_width,sample->viewport_height,
            latest_frame.near_clip_m,latest_frame.far_clip_m,
            surface_stats))
        return 0;
    last_depth_debug = *sample; // bounded copy, no GPU or file I/O
    last_depth_stats = surface_stats;
    depth_debug_valid = true;
    if (logger && probe_depth_enabled) {
        // Anonymized per-8x8-grid health metrics; no raw pixels or world IDs.
        logger(1, (std::string("OGL RTX depth coverage: surface=") +
            std::to_string(surface_stats.surface_pixels) +
            " background=" + std::to_string(surface_stats.background_pixels) +
            " view_m_min=" + std::to_string(surface_stats.view_distance_min) +
            " view_m_max=" + std::to_string(surface_stats.view_distance_max) +
            "; diagnostic only").c_str());
    }
    if (probe_depth_enabled && probe_worker && !captured_geometry.empty()) {
        // Independent reference from the same copied GL depth and scene.
        // A bounded 64x8192 CPU ray test only once per depth readback
        // (currently throttled to >=60s), never on the public viewer path.
        const auto start=std::chrono::steady_clock::now();
        ogl::SceneDepthAgreement reference{};
        const bool compared=ogl::compare_cpu_scene_to_gl_depth(
            captured_geometry.data(),
            static_cast<uint32_t>(captured_geometry.size()),
            sample->normalized_depth,
            latest_frame.camera_position,latest_frame.camera_forward,
            latest_frame.camera_right,latest_frame.camera_up,
            latest_frame.vertical_fov_radians,latest_frame.aspect_ratio,
            latest_frame.near_clip_m,latest_frame.far_clip_m,reference);
        if (compared && logger) {
            const auto elapsed=std::chrono::duration_cast<
                std::chrono::microseconds>(
                std::chrono::steady_clock::now()-start).count();
            logger(1,(std::string("OGL RTX CPU depth reference: revision=")+
                std::to_string(geometry_revision)+" triangles="+
                std::to_string(captured_geometry.size())+" surface="+
                std::to_string(reference.surface)+" background="+
                std::to_string(reference.background)+" primary_hits="+
                std::to_string(reference.primary_hits)+" depth_matches="+
                std::to_string(reference.matching)+" primary_closer="+
                std::to_string(reference.closer)+" primary_farther="+
                std::to_string(reference.farther)+" ray_samples="+
                std::to_string(reference.ray_samples)+" cpu_us="+
                std::to_string(elapsed)+" depth_near="+
                std::to_string(reference.ranges[0].surface)+"/"+
                std::to_string(reference.ranges[0].primary_hits)+"/"+
                std::to_string(reference.ranges[0].matching)+
                " depth_mid="+
                std::to_string(reference.ranges[1].surface)+"/"+
                std::to_string(reference.ranges[1].primary_hits)+"/"+
                std::to_string(reference.ranges[1].matching)+
                " depth_far="+
                std::to_string(reference.ranges[2].surface)+"/"+
                std::to_string(reference.ranges[2].primary_hits)+"/"+
                std::to_string(reference.ranges[2].matching)+
                "; independent test only, not visible shadows").c_str());
        }
    }
    if (probe_depth_enabled && probe_worker) {
        // A newer valid depth grid supersedes the prior GPU result, even
        // when the camera, sun and mesh revision are unchanged.
        shadow_debug_valid = false;
        last_shadow_debug = {};
        last_world_shadow = {};
        world_shadow_valid = false;
        probe_view_dirty = true;
        probe_worker->invalidate(++probe_job_sequence);
        schedule_probe_on_viewer_thread();
        poll_probe_on_viewer_thread();
    }
    return 1;
}
extern "C" OGL_MODULE_EXPORT int ogl_rtx_copy_depth_debug(
    ogl_rtx_depth_debug *output)
{
    if (!output || output->struct_size != sizeof(*output) ||
        output->version != OGL_RTX_DEPTH_DEBUG_VERSION || !depth_debug_valid)
        return 0;
    *output = last_depth_debug;
    return 1;
}

extern "C" OGL_MODULE_EXPORT int ogl_rtx_submit_dense_depth(
    const ogl_rtx_dense_depth* input) {
    if (!probe_dense_enabled || !probe_worker || !status.hardware_ready ||
        !input || !received_frames || captured_geometry.empty() ||
        input->struct_size!=sizeof(*input) ||
        input->version!=OGL_RTX_DENSE_VERSION ||
        input->width!=OGL_RTX_DENSE_WIDTH ||
        input->height!=OGL_RTX_DENSE_HEIGHT ||
        input->viewport_width!=latest_frame.width ||
        input->viewport_height!=latest_frame.height)
        return 0;
    for (float d:input->normalized_depth)
        if (!std::isfinite(d) || d<0.f || d>1.f) return 0;
    // Receive a finished result BEFORE checking queue occupancy. Dense
    // Vulkan calls may take several seconds on heavily loaded regions;
    // invalidating them every two seconds caused repeated GPU starvation.
    poll_probe_on_viewer_thread();
    if (probe_worker->stats().busy) {
        ++dense_busy_depth_skips;
        if (logger && dense_busy_depth_skips%8u==0u) {
            const auto stats=probe_worker->stats();
            logger(1,(std::string("OGL RTX dense scheduler: attempts=")+
                std::to_string(stats.attempts)+
                " successes="+std::to_string(stats.successes)+
                " busy_depth_skips="+std::to_string(dense_busy_depth_skips)+
                " replacements="+std::to_string(stats.replacements)+
                "; no forced cancellation").c_str());
        }
        // Keep newest live camera/depth separate from the frozen GPU job.
        // A completed result remains diagnostic-only unless its capture
        // pose, depth and geometry still match the actual framebuffer.
        return 1;
    }
    last_dense_depth=*input;
    dense_depth_valid=true;
    last_dense_shadow={};
    dense_shadow_valid=false;
    probe_view_dirty=true;
    schedule_probe_on_viewer_thread();
    poll_probe_on_viewer_thread();
    return 1;
}

extern "C" OGL_MODULE_EXPORT int ogl_rtx_copy_dense_shadow(
    ogl_rtx_dense_shadow* output) {
    if (!output || output->struct_size!=sizeof(*output) ||
        output->version!=OGL_RTX_DENSE_VERSION ||
        !probe_dense_enabled || !dense_shadow_valid ||
        !ogl::valid_dense_shadow(last_dense_shadow,geometry_revision,latest_frame))
        return 0;
    *output=last_dense_shadow;
    return 1;
}

extern "C" OGL_MODULE_EXPORT int ogl_rtx_copy_world_shadow_sample(
    ogl_rtx_world_shadow_sample* output)
{
    if (!output || output->struct_size!=sizeof(*output) ||
        output->version!=OGL_RTX_WORLD_SHADOW_VERSION ||
        !world_shadow_valid || !probe_depth_enabled ||
        !ogl::valid_world_shadow_sample(
            last_world_shadow,geometry_revision,latest_frame))
        return 0;
    *output=last_world_shadow;
    return 1;
}

extern "C" OGL_MODULE_EXPORT int ogl_rtx_copy_shadow_debug(
    ogl_rtx_shadow_debug *output)
{
    if (!output || output->struct_size != sizeof(ogl_rtx_shadow_debug) ||
        output->version != OGL_RTX_SHADOW_DEBUG_VERSION ||
        !shadow_debug_valid ||
        (!probe_depth_enabled &&
         last_shadow_debug.geometry_revision != geometry_revision))
        return 0;
    *output = last_shadow_debug;
    return 1;
}

extern "C" OGL_MODULE_EXPORT uint64_t ogl_rtx_gpu_probe_attempts(void) {
    return probe_worker ? probe_worker->stats().attempts : 0;
}
extern "C" OGL_MODULE_EXPORT uint64_t ogl_rtx_gpu_probe_successes(void) {
    return probe_worker ? probe_worker->stats().successes : 0;
}

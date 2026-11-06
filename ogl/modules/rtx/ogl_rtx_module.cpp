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
#if defined(__linux__) && __has_include(<X11/Xlib.h>)
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
    uint32_t image_count = 0;
    bool submitted_any = false;
    bool blocked = false;
    bool initialized = false;
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
    PFN_vkDestroySurfaceKHR destroy_surface = nullptr;
    PFN_vkDestroySwapchainKHR destroy_swapchain = nullptr;
    bool xlib_surface_extension = false;
    bool swapchain_probe_ready = false;
    bool exclusive_present_used = false;
    uint32_t exclusive_present_count = 0; // bounded, QA-only completed presents
    VkFormat swapchain_format = VK_FORMAT_UNDEFINED;
    VkExtent2D swapchain_extent{};
    OglPersistentClearFrames persistent;
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
    uint32_t mesh_triangle_count = 0;
    // Optional exact byte-for-byte dedup for live captured scene snapshots.
    // Never suppress a real scene change based on a hash collision.
    std::vector<ogl_rtx_triangle> mesh_cpu_snapshot;
    uint64_t mesh_dedup_skips = 0;
    // CPU-generated Vulkan clip projection from a validated Firestorm
    // camera. Exactly 16 column-major floats for the 64-byte push constant.
    std::array<float,16> world_to_clip{};
    bool world_camera_ready = false;
    float world_camera_aspect = 0.f;
    uint64_t world_camera_accepts = 0; // CPU matrix updates, no GPU presents
    uint64_t mesh_uploads = 0; // accepted snapshots, never a draw counter
    // P05/3: bounded GPU queue submission diagnostic. These counters refer
    // to commands submitted BEFORE waiting on their completion fences,
    // NOT to simultaneous GPU execution or completed swapchain presents.
    uint32_t sync_max_outstanding = 0;
    uint32_t sync_last_submitted = 0;
    uint32_t sync_last_completed = 0;
    PFN_vkDestroyBuffer destroy_buffer = nullptr;
    PFN_vkFreeMemory free_memory = nullptr;

    void clear_mesh() {
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
        swapchain_extent = {};
        if (surface && destroy_surface && instance)
            destroy_surface(instance, surface, nullptr);
        surface = VK_NULL_HANDLE;
    }
    void close() {
        detach_window(); // WSI handles must die before VkDevice/VkInstance.
        world_camera_ready = false;
        world_camera_aspect = 0.f;
        camera_mesh_generation = 0;
        world_to_clip.fill(0.f);
        world_camera_accepts = 0;
        mesh_uploads = 0;
        mesh_generation = 0;
        mesh_dedup_skips = 0;
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
    std::vector<VkSurfaceFormatKHR> formats(count);
    if (get_formats(world_device.physical_device,world_device.surface,
                    &count,formats.data())!=VK_SUCCESS || count==0) {
        world_device.detach_window();
        return false;
    }
    VkSurfaceFormatKHR format=formats.front();
    if (count==1 && format.format==VK_FORMAT_UNDEFINED)
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
    std::vector<VkPresentModeKHR> modes(count);
    if (get_modes(world_device.physical_device,world_device.surface,
                  &count,modes.data())!=VK_SUCCESS ||
        std::find(modes.begin(),modes.end(),VK_PRESENT_MODE_FIFO_KHR)==modes.end()) {
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
        extent.width>16384 || extent.height>16384) {
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
    world_device.swapchain_probe_ready=true;
    world_device.swapchain_format=format.format;
    world_device.swapchain_extent=extent;
    // A camera belonging to a DIFFERENT viewport aspect must never
    // render into this swapchain after a resize, even when it is valid
    // for the same region/mesh. A new camera update is required.
    const float swap_aspect=float(extent.width)/float(extent.height);
    if (world_device.world_camera_ready &&
        std::fabs(world_device.world_camera_aspect-swap_aspect)>0.01f) {
        world_device.world_camera_ready=false;
        world_device.world_camera_aspect=0.f;
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
    if (p.blocked || p.frame_count>=16 ||
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
            std::vector<VkImage> images(count);
            if (vkGetSwapchainImagesKHR(world_device.device,
                    world_device.swapchain,&count,images.data())!=VK_SUCCESS ||
                count<2 || count>16) return false;
            images.resize(count);
            p.image_count=count;
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
        p.initialized=true;
        ++world_device.persistent_builds;
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

// P05/3: verified Vulkan offscreen color frame, isolated from the GL window.
// This is an actual GPU graphics render pass and CPU-verified image, not a
// swapchain acquire/present or an OpenSim world renderer. Developer opt-in.
uint32_t render_offscreen_color_frame() {
    static bool disabled_after_timeout = false;
    if (disabled_after_timeout) return 0;
    const char* enabled = std::getenv("OGL_VULKAN_OFFSCREEN_FRAME_PROBE");
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
        !get_memory_properties || !get_format_properties) return 0;

    constexpr uint32_t size=32;
    constexpr VkDeviceSize readback_bytes=size*size*4;
    constexpr VkFormat format=VK_FORMAT_R8G8B8A8_UNORM;
    VkFormatProperties fmt{};
    get_format_properties(world_device.physical_device,format,&fmt);
    const auto required_features =
        VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT |
        VK_FORMAT_FEATURE_TRANSFER_SRC_BIT;
    if ((fmt.optimalTilingFeatures & required_features)!=required_features)
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
    VkImage color=VK_NULL_HANDLE;
    VkDeviceMemory color_mem=VK_NULL_HANDLE, readback_mem=VK_NULL_HANDLE;
    VkImageView view=VK_NULL_HANDLE;
    VkRenderPass render_pass=VK_NULL_HANDLE;
    VkFramebuffer framebuffer=VK_NULL_HANDLE;
    VkBuffer readback=VK_NULL_HANDLE;
    VkCommandPool command_pool=VK_NULL_HANDLE;
    VkFence fence=VK_NULL_HANDLE;
    auto cleanup=[&]() {
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
    };
    auto frame=[&]() -> uint32_t {
        VkImageCreateInfo image{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        image.imageType=VK_IMAGE_TYPE_2D;
        image.format=format;
        image.extent={size,size,1};
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
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount=1;
        subpass.pColorAttachments=&ref;
        VkRenderPassCreateInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        rp.attachmentCount=1;
        rp.pAttachments=&attachment;
        rp.subpassCount=1;
        rp.pSubpasses=&subpass;
        if (vkCreateRenderPass(world_device.device,&rp,nullptr,
                               &render_pass)!=VK_SUCCESS || !render_pass)
            return 0;
        VkFramebufferCreateInfo fb{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        fb.renderPass=render_pass;
        fb.attachmentCount=1;
        fb.pAttachments=&view;
        fb.width=size;
        fb.height=size;
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

        VkCommandPoolCreateInfo pool{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        pool.queueFamilyIndex=world_device.queue_family;
        pool.flags=VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        if (vkCreateCommandPool(world_device.device,&pool,nullptr,
                                &command_pool)!=VK_SUCCESS || !command_pool)
            return 0;
        VkCommandBuffer command=VK_NULL_HANDLE;
        VkCommandBufferAllocateInfo ca{
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        ca.commandPool=command_pool;
        ca.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        ca.commandBufferCount=1;
        if (vkAllocateCommandBuffers(world_device.device,&ca,
                                     &command)!=VK_SUCCESS || !command)
            return 0;
        VkCommandBufferBeginInfo cb{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        cb.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        if (vkBeginCommandBuffer(command,&cb)!=VK_SUCCESS) return 0;

        VkClearValue clear{};
        clear.color.float32[0]=0.2f;
        clear.color.float32[1]=0.4f;
        clear.color.float32[2]=0.6f;
        clear.color.float32[3]=1.0f;
        VkRenderPassBeginInfo begin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        begin.renderPass=render_pass;
        begin.framebuffer=framebuffer;
        begin.renderArea.extent={size,size};
        begin.clearValueCount=1;
        begin.pClearValues=&clear;
        vkCmdBeginRenderPass(command,&begin,VK_SUBPASS_CONTENTS_INLINE);
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
        copy.imageExtent={size,size,1};
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

        VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        if (vkCreateFence(world_device.device,&fi,nullptr,&fence)!=VK_SUCCESS ||
            !fence) return 0;
        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submit.commandBufferCount=1;
        submit.pCommandBuffers=&command;
        if (vkQueueSubmit(world_device.queue,1,&submit,fence)!=VK_SUCCESS)
            return 0;
        const auto result=vkWaitForFences(world_device.device,1,&fence,
                                          VK_TRUE,2000000000ULL);
        if (result!=VK_SUCCESS) {
            // No unbounded QueueWaitIdle and no freeing in-flight GPU data.
            // Diagnostic allocations stay on device until module shutdown.
            disabled_after_timeout=true;
            return 2;
        }
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
        const bool correct=matches(0) && matches(4*(size*size/2)) &&
                           matches(4*(size*size-1));
        vkUnmapMemory(world_device.device,readback_mem);
        if (logger && correct)
            logger(1,"P05 real Vulkan offscreen render pass: 32x32 RGBA pixels validated after GPU fence; no swapchain present.");
        return correct ? 1u : 0u;
    };
    uint32_t result=frame();
    // 2 means a submitted command is still in flight: releasing any of its
    // VkObjects would be invalid. Quarantine, never retry this dev probe.
    if (result!=2) cleanup();
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
    world_device.camera_mesh_generation=0;
    world_device.world_to_clip.fill(0.f);
    if (!frame || !ogl::valid_camera_frame(*frame)) return 0;
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
    if (!std::isfinite(handedness) || handedness<0.9 || handedness>1.1)
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
    world_device.world_camera_aspect=frame->aspect_ratio;
    world_device.camera_mesh_generation=world_device.mesh_generation;
    world_device.world_camera_ready=true;
    ++world_device.world_camera_accepts; // no Vulkan draw/queue work
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
    if (frames.initialized && frames.submitted_any) {
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
    world_device.camera_mesh_generation=0;
    world_device.world_to_clip.fill(0.f);
    if (logger)
        logger(1,(std::string("P05 Vulkan vertex/index upload: triangles=")+
                  std::to_string(count)+" vertices="+
                  std::to_string(vertex_count)+
                  "; GPU buffers ready, no world draw.").c_str());
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

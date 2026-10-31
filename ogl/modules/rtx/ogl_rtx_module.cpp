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
#include <algorithm>
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
GpuProbeLibrary probe_library = nullptr;
ProbeRun probe_run = nullptr;
CameraProbeRun probe_camera_run = nullptr;
DepthProbeRun probe_depth_run = nullptr;
bool probe_depth_enabled = false;
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

void gpu_probe_cleanup() {
    probe_run = nullptr;
    probe_camera_run = nullptr;
    probe_depth_run = nullptr;
    if (probe_library) gpu_close(probe_library);
    probe_library = nullptr;
}

ogl_log_fn logger = nullptr;
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
            + "; disabled until next start; OpenGL active").c_str());
    }
}
// One scheduler for new camera frames and new mesh snapshots. Throttle
// GPU diagnostics in the opt-in mode; never submit Vulkan on this thread.
void schedule_probe_on_viewer_thread() {
    // Never perform Vulkan calls on this render-thread callback.
    if (probe_worker && received_frames > 0 && !captured_geometry.empty() &&
        (!probe_depth_enabled || depth_debug_valid) &&
        (geometry_revision != probe_last_revision || probe_view_dirty)) {
        using Clock = std::chrono::steady_clock;
        const auto now = Clock::now();
        if (probe_last_time == Clock::time_point{} ||
            now - probe_last_time >= probe_interval) {
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
    probe_worker.reset(); // join before unloading Vulkan function pointers
    gpu_probe_cleanup();
    probe_opt_in = false;
    probe_camera_enabled = false;
    probe_depth_enabled = false;
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
    detect();
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
        }
        if (!probe_run || (probe_camera_enabled && !probe_camera_run) ||
            (probe_depth_enabled && !probe_depth_run)) {
            if (logger) logger(1, "OGL RTX opt-in GPU probe unavailable; OpenGL unaffected");
        } else {
            // Worker calls Vulkan. Immutable data; no viewer callbacks.
            try {
            probe_worker.reset(new ogl::AsyncGpuQueue([](
                const ogl::AsyncGpuQueue::Job& job) {
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
                    if (probe_depth_enabled) {
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
                result.return_code = (rc == 0 && native.success) ? 0 : 1;
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
    probe_worker.reset(); // join before unloading GPU helper DLL/SO
    gpu_probe_cleanup();
    probe_opt_in = false;
    probe_camera_enabled = false;
    probe_depth_enabled = false;
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
        probe_view_dirty = true;
        // The depth-guided preview is a frozen frame snapshot. Camera
        // movement must not cancel GPU work in progress, or clear the
        // previous 8-second debug tile. Neither is used for world shading.
        if (!probe_depth_enabled) {
            shadow_debug_valid = false;
            last_shadow_debug = {};
            if (probe_worker) probe_worker->invalidate(++probe_job_sequence);
        }
    }
    // A depth tile belongs to exactly one submitted frame. No reuse after
    // camera motion, resize, new lighting or a later rendered frame.
    last_depth_debug = {};
    last_depth_stats = {};
    depth_debug_valid = false;
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
    ++geometry_revision;
    probe_view_dirty = true;
    if (!probe_depth_enabled) {
        last_shadow_debug = {};
        shadow_debug_valid = false;
        if (probe_worker) probe_worker->invalidate(++probe_job_sequence);
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

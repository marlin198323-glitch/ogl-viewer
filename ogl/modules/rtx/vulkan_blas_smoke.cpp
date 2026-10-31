// OGL public Vulkan BLAS acceptance: build a real one-triangle GPU
// acceleration structure. No image rendering, no OpenSim data, no server.
// No hardware: CTest SKIP (77). GPU/API failure after selection: FAIL (1).
// Real ray-query compute shader tested; OpenGL compositing still pending.
#include <vulkan/vulkan.h>
#include "ogl/ogl_rtx_api.h"
#include "ogl/ogl_gpu_probe_api.h"
#include "ogl/ogl_scene_validation.h"
#include "ogl/ogl_camera_validation.h"
#include "ogl/ogl_depth_validation.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <memory>
#if defined(_WIN32)
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#else
#  include <dlfcn.h>
#endif
#include <set>
#include <string>
#include <vector>

// Test-only module fixture. Production GPU library receives actual data.
#ifndef OGL_GPU_PROBE_LIBRARY
using GeometrySubmit = int (*)(const ogl_rtx_triangle*, uint32_t);
using GeometryCopy = uint32_t (*)(ogl_rtx_triangle*, uint32_t);
using GeometryRevision = uint64_t (*)();

void diagnostic_log(int, const char*) {}

static int load_scene_from_module(const char* path, uint32_t count,
                                  std::vector<ogl_rtx_triangle>& scene) {
#if defined(_WIN32)
    HMODULE module = LoadLibraryExA(path, nullptr,
        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (!module) return 1;
    auto symbol = [&](const char* name) -> void* {
        return reinterpret_cast<void*>(GetProcAddress(module, name));
    };
#else
    void* module = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!module) return 1;
    auto symbol = [&](const char* name) -> void* { return dlsym(module, name); };
#endif
    auto start = reinterpret_cast<ogl_module_start_fn>(symbol("ogl_module_start"));
    auto stop = reinterpret_cast<ogl_module_stop_fn>(symbol("ogl_module_stop"));
    auto submit = reinterpret_cast<GeometrySubmit>(symbol("ogl_rtx_submit_geometry"));
    auto copy = reinterpret_cast<GeometryCopy>(symbol("ogl_rtx_copy_geometry"));
    auto revision = reinterpret_cast<GeometryRevision>(symbol("ogl_rtx_geometry_revision"));
    auto status = reinterpret_cast<const ogl_rtx_status*(*)()>(
        symbol("ogl_rtx_get_status"));
    int code = 1;
    if (start && stop && submit && copy && revision && status) {
        ogl_host_api host{sizeof(host), OGL_MODULE_ABI_VERSION, diagnostic_log};
        if (start(&host) == 0) {
            // A Vulkan-free machine is a legitimate OpenGL client. Preserve
            // CTest SKIP rather than treating it as a broken GPU renderer.
            if (!status()->hardware_ready || !status()->ray_query) {
                code = 77;
            } else {
            std::vector<ogl_rtx_triangle> input(count);
            for (uint32_t i = 0; i < count; ++i) {
                // Mimic the viewer C ABI: one unit triangle followed by
                // many physically separate mesh faces outside the ray mask.
                float x = i ? 8.0f + float(i % 64) * 2.f : 0.f;
                float y = i ? 8.0f + float(i / 64) * 2.f : 0.f;
                input[i].position[0] = x;
                input[i].position[1] = y;
                input[i].position[3] = x + 1.f;
                input[i].position[4] = y;
                input[i].position[6] = x;
                input[i].position[7] = y + 1.f;
            }
            scene.resize(count);
            code = (submit(input.data(), count) == 1 &&
                 revision() == 1 &&
                 copy(scene.data(), count) == count &&
                 std::memcmp(scene.data(), input.data(), count * sizeof(ogl_rtx_triangle)) == 0)
                 ? 0 : 1;
            }
            stop();
        }
    }
#if defined(_WIN32)
    FreeLibrary(module);
#else
    dlclose(module);
#endif
    return code;
}
#endif // !OGL_GPU_PROBE_LIBRARY

struct Buffer {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkDeviceSize size = 0;
};

struct VulkanContext {
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physical = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue queue = VK_NULL_HANDLE;
    uint32_t queue_family = ~0u;
    VkCommandPool pool = VK_NULL_HANDLE;
    VkCommandBuffer command = VK_NULL_HANDLE;
    VkAccelerationStructureKHR blas = VK_NULL_HANDLE;
    VkAccelerationStructureKHR tlas = VK_NULL_HANDLE;
    PFN_vkDestroyAccelerationStructureKHR destroy_blas = nullptr;
    Buffer vertices, scratch, storage;
    Buffer instance_buffer, tlas_scratch, tlas_storage, query_results, depth_input;
    VkDescriptorSetLayout descriptor_layout = VK_NULL_HANDLE;
    VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    VkPipeline ray_pipeline = VK_NULL_HANDLE;
    VkShaderModule shader = VK_NULL_HANDLE;
    // Retain completed BLAS/TLAS when only sunlight direction changes;
    // discard descriptors/compute output before the next dispatch.
    void reset_query() {
        if (device) {
            vkDeviceWaitIdle(device);
            if (ray_pipeline) vkDestroyPipeline(device, ray_pipeline, nullptr);
            if (shader) vkDestroyShaderModule(device, shader, nullptr);
            if (pipeline_layout) vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
            if (descriptor_pool) vkDestroyDescriptorPool(device, descriptor_pool, nullptr);
            if (descriptor_layout) vkDestroyDescriptorSetLayout(device, descriptor_layout, nullptr);
            if (query_results.buffer) vkDestroyBuffer(device, query_results.buffer, nullptr);
            if (query_results.memory) vkFreeMemory(device, query_results.memory, nullptr);
            if (depth_input.buffer) vkDestroyBuffer(device, depth_input.buffer, nullptr);
            if (depth_input.memory) vkFreeMemory(device, depth_input.memory, nullptr);
        }
        ray_pipeline = VK_NULL_HANDLE;
        shader = VK_NULL_HANDLE;
        pipeline_layout = VK_NULL_HANDLE;
        descriptor_pool = VK_NULL_HANDLE;
        descriptor_layout = VK_NULL_HANDLE;
        query_results = {};
        depth_input = {};
    }
    // Task buffers are transient, but the Vulkan device survives scene changes.
    void reset_task() {
        reset_query();
        if (device) {
            if (tlas && destroy_blas) destroy_blas(device, tlas, nullptr);
            if (blas && destroy_blas) destroy_blas(device, blas, nullptr);
            for (const Buffer* b : {&vertices, &scratch, &storage,
                                    &instance_buffer, &tlas_scratch, &tlas_storage}) {
                if (b->buffer) vkDestroyBuffer(device, b->buffer, nullptr);
                if (b->memory) vkFreeMemory(device, b->memory, nullptr);
            }
            if (pool) vkDestroyCommandPool(device, pool, nullptr);
        }
        blas = VK_NULL_HANDLE;
        tlas = VK_NULL_HANDLE;
        pool = VK_NULL_HANDLE;
        command = VK_NULL_HANDLE;
        for (Buffer* b : {&vertices, &scratch, &storage,
                          &instance_buffer, &tlas_scratch, &tlas_storage}) *b = {};
    }
    ~VulkanContext() {
        reset_task();
        if (device) vkDestroyDevice(device, nullptr);
        if (instance) vkDestroyInstance(instance, nullptr);
    }
};

bool allocate_buffer(VulkanContext& c, Buffer& b, VkDeviceSize size,
                     VkBufferUsageFlags usage, VkMemoryPropertyFlags requested,
                     bool device_address) {
    b.size = size;
    VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    info.size = size;
    info.usage = usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(c.device, &info, nullptr, &b.buffer) != VK_SUCCESS)
        return false;
    VkMemoryRequirements requirements{};
    vkGetBufferMemoryRequirements(c.device, b.buffer, &requirements);
    VkPhysicalDeviceMemoryProperties memory{};
    vkGetPhysicalDeviceMemoryProperties(c.physical, &memory);
    uint32_t index = ~0u;
    for (uint32_t i = 0; i < memory.memoryTypeCount; ++i) {
        if ((requirements.memoryTypeBits & (1u << i)) &&
            (memory.memoryTypes[i].propertyFlags & requested) == requested) {
            index = i;
            break;
        }
    }
    if (index == ~0u) return false;

    VkMemoryAllocateFlagsInfo flags{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO};
    flags.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
    VkMemoryAllocateInfo alloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    alloc.allocationSize = requirements.size;
    alloc.memoryTypeIndex = index;
    if (device_address) alloc.pNext = &flags;
    if (vkAllocateMemory(c.device, &alloc, nullptr, &b.memory) != VK_SUCCESS)
        return false;
    return vkBindBufferMemory(c.device, b.buffer, b.memory, 0) == VK_SUCCESS;
}

VkDeviceAddress buffer_address(VulkanContext& c, const Buffer& b) {
    VkBufferDeviceAddressInfo info{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
    info.buffer = b.buffer;
    return vkGetBufferDeviceAddress(c.device, &info);
}

bool has_extension(VkPhysicalDevice gpu, const std::set<std::string>& required) {
    uint32_t count = 0;
    if (vkEnumerateDeviceExtensionProperties(gpu, nullptr, &count, nullptr) != VK_SUCCESS)
        return false;
    std::vector<VkExtensionProperties> extensions(count);
    if (vkEnumerateDeviceExtensionProperties(gpu, nullptr, &count, extensions.data()) != VK_SUCCESS)
        return false;
    std::set<std::string> available;
    for (const auto& e : extensions) available.insert(e.extensionName);
    for (const auto& name : required)
        if (!available.count(name)) return false;
    return true;
}

static int execute_gpu_scene(const char* shader_path,
                             const ogl_rtx_triangle* scene,
                             uint32_t wanted_triangles,
                             const float* sun,
                             const ogl_rtx_frame_state* camera,
                             ogl_gpu_probe_result* result,
                             const float* depth_64,
                             const float* instance_xyz = nullptr) {
    if (!shader_path || !scene || wanted_triangles < 1 ||
        wanted_triangles > OGL_RTX_MAX_SCENE_TRIANGLES) return 1;
    for (uint32_t i = 0; i < wanted_triangles; ++i)
        if (!ogl::is_valid_triangle(scene[i])) return 1;
    if (camera && !ogl::valid_camera_frame(*camera)) return 1;
    if (instance_xyz) for (int i=0;i<3;++i)
        if (!std::isfinite(instance_xyz[i]) || std::fabs(instance_xyz[i])>10000.f)
            return 1;
    if (depth_64 && (!camera ||
        !ogl::valid_depth_grid(camera->width, camera->height, 8, 8,
                               depth_64))) return 1;

    // Ray sampling covers the source scene footprint, and uses the frame's
    // sun direction only for the opt-in live diagnostics. This is a scene
    // probe, NOT a perspective-correct shadow map for the user-facing image.
    struct ScenePush {
        float origin[4]{};
        float step[4]{};
        float direction[4]{};
        float camera_origin[4]{};
        float camera_forward[4]{};
        float camera_right[4]{};
        float camera_up[4]{};
        float clip_range[4]{};
    } push{};
    static_assert(sizeof(ScenePush) == 128, "Vulkan push ABI mismatch");
    float minv[3]{scene[0].position[0],scene[0].position[1],scene[0].position[2]};
    float maxv[3]{minv[0],minv[1],minv[2]};
    for (uint32_t i = 0; i < wanted_triangles; ++i)
        for (unsigned v = 0; v < 9; ++v) {
            minv[v % 3] = std::min(minv[v % 3],scene[i].position[v]);
            maxv[v % 3] = std::max(maxv[v % 3],scene[i].position[v]);
        }
    if (result) {
        // Cover the full snapshot's XY bounding box, with a lower sample
        // plane that casts rays upward toward the OpenSim sun.
        push.origin[0] = minv[0];
        push.origin[1] = minv[1];
        push.origin[2] = minv[2] - 2.f;
        push.step[0] = std::max(0.1f, (maxv[0]-minv[0])/8.f);
        push.step[1] = std::max(0.1f, (maxv[1]-minv[1])/8.f);
        const float dx=maxv[0]-minv[0], dy=maxv[1]-minv[1], dz=maxv[2]-minv[2];
        push.origin[3] = std::max(5.f, 2.5f * std::sqrt(dx*dx+dy*dy+dz*dz)+20.f);
        if (sun && std::isfinite(sun[0]) && std::isfinite(sun[1]) &&
            std::isfinite(sun[2])) {
            const float magnitude = std::sqrt(sun[0]*sun[0]+sun[1]*sun[1]+sun[2]*sun[2]);
            if (std::isfinite(magnitude) && magnitude > 1.e-5f)
                for (unsigned i=0; i<3; ++i) push.direction[i] = sun[i]/magnitude;
        }
    } else {
        // Stable deterministic fixture for the 36-hit/28-miss regression.
        push.origin[0] = 0.f;
        push.origin[1] = 0.f;
        push.origin[2] = -2.f;
        push.origin[3] = 5.f;
        push.step[0] = 1.f/8.f;
        push.step[1] = 1.f/8.f;
    }
    if (push.direction[0] == 0.f && push.direction[1] == 0.f &&
        push.direction[2] == 0.f)
        push.direction[2] = 1.f;
    if (camera) {
        for (unsigned i=0; i<3; ++i)
            push.camera_origin[i] = camera->camera_position[i];
        // The axes already include roll, horizon tilt and the actual
        // viewer field of view. Their values are supplied directly by
        // Firestorm's camera, not inferred from world-up or a fixed 60°.
        const float tan_half_fov = std::tan(camera->vertical_fov_radians * 0.5f);
        push.clip_range[0] = camera->near_clip_m;
        push.clip_range[1] = camera->far_clip_m;
        for (unsigned i=0; i<3; ++i) {
            push.camera_forward[i] = camera->camera_forward[i];
            push.camera_right[i] = camera->camera_right[i] *
                tan_half_fov * camera->aspect_ratio;
            push.camera_up[i] = camera->camera_up[i] * tan_half_fov;
        }
    }
#ifdef OGL_GPU_PROBE_LIBRARY
    // This state is owned by the single GPU worker thread and destroyed when
    // that thread exits, BEFORE the Vulkan SO/DLL is unloaded.
    static thread_local std::unique_ptr<VulkanContext> persistent;
    static thread_local uint32_t device_creations = 0;
    static thread_local uint32_t blas_builds = 0;
    static thread_local uint32_t blas_refits = 0;
    static thread_local uint32_t tlas_refits = 0;
    static thread_local std::array<float,3> cached_instance_xyz{};
    static thread_local uint32_t cache_hits = 0;
    static thread_local std::vector<ogl_rtx_triangle> cached_scene;
    static thread_local std::array<float, 3> cached_sun{{0.f, 0.f, 1.f}};
    static thread_local std::array<float, 18> cached_camera{};
    static thread_local std::string cached_shader;
    static thread_local std::array<float,64> cached_depth{};
    static thread_local bool cached_depth_valid = false;
    static thread_local ogl_gpu_probe_result cached_output{};
    const std::array<float,3> requested_sun{{
        sun ? sun[0] : 0.f,
        sun ? sun[1] : 0.f,
        sun ? sun[2] : 1.f
    }};
    std::array<float,18> requested_camera{};
    if (camera) {
        for (unsigned i=0; i<3; ++i) {
            requested_camera[i] = camera->camera_position[i];
            requested_camera[i+3] = camera->camera_forward[i];
            requested_camera[i+6] = camera->camera_right[i];
            requested_camera[i+9] = camera->camera_up[i];
        }
        requested_camera[12] = camera->vertical_fov_radians;
        requested_camera[13] = camera->aspect_ratio;
        requested_camera[14] = static_cast<float>(camera->width);
        requested_camera[15] = static_cast<float>(camera->height);
        requested_camera[16] = camera->near_clip_m;
        requested_camera[17] = camera->far_clip_m;
    }
    // Clip planes are meaningful even for the older camera-ray shader;
    // readback-mode cache must additionally include every input depth.
    const std::array<float,3> requested_instance{{
        instance_xyz ? instance_xyz[0] : 0.f,
        instance_xyz ? instance_xyz[1] : 0.f,
        instance_xyz ? instance_xyz[2] : 0.f}};
    const bool same_depth = !depth_64 ? !cached_depth_valid :
        cached_depth_valid && std::equal(depth_64, depth_64+64,
                                        cached_depth.begin());
    if (persistent && persistent->device &&
        cached_scene.size() == wanted_triangles &&
        !cached_scene.empty() && cached_sun == requested_sun &&
        cached_camera == requested_camera &&
        same_depth && cached_instance_xyz == requested_instance &&
        cached_shader == shader_path &&
        std::memcmp(cached_scene.data(), scene,
            size_t(wanted_triangles) * sizeof(ogl_rtx_triangle)) == 0) {
        ++cache_hits;
        *result = cached_output;
        result->vulkan_device_creations = device_creations;
        result->blas_builds = blas_builds;
        result->blas_refits = blas_refits;
        result->tlas_refits = tlas_refits;
        result->cache_hits = cache_hits;
        return 0;
    }
    const bool same_topology = persistent && persistent->device &&
        persistent->blas && persistent->tlas &&
        cached_scene.size() == wanted_triangles;
    const bool reuse_geometry = same_topology &&
        std::memcmp(cached_scene.data(), scene,
            size_t(wanted_triangles) * sizeof(ogl_rtx_triangle)) == 0;
    // Vulkan UPDATE requires the SAME vertex/primitive layout and all
    // build-time resources kept alive; first build uses ALLOW_UPDATE.
    // This supports changed vertex positions (animated/deformed geometry),
    // not adding/removing triangles or changing primitive topology.
    const bool refit_geometry = same_topology && !reuse_geometry &&
        persistent->vertices.buffer && persistent->scratch.buffer &&
        persistent->blas && persistent->tlas;
    cached_scene.clear(); // failed calls cannot cache incomplete outputs
    if (!persistent) persistent.reset(new VulkanContext);
    if (persistent->device) {
        if (reuse_geometry || refit_geometry) persistent->reset_query();
        else persistent->reset_task(); // changes in primitive count
    }
    VulkanContext& c = *persistent;
#else
    VulkanContext c;
#endif
    VkPhysicalDeviceProperties chosen{};
    if (!c.device) {
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "OGL universal Vulkan BLAS acceptance";
    app.apiVersion = VK_API_VERSION_1_2;
    VkInstanceCreateInfo instance_info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instance_info.pApplicationInfo = &app;
    VkResult rc = vkCreateInstance(&instance_info, nullptr, &c.instance);
    if (rc == VK_ERROR_INCOMPATIBLE_DRIVER) {
        std::puts("SKIP: no Vulkan driver; OpenGL fallback unaffected");
        return 77;
    }
    if (rc != VK_SUCCESS) {
        std::printf("FAIL: vkCreateInstance=%d\n", rc);
        return 1;
    }

    uint32_t gpu_count = 0;
    if (vkEnumeratePhysicalDevices(c.instance, &gpu_count, nullptr) != VK_SUCCESS ||
        !gpu_count) {
        std::puts("SKIP: no Vulkan GPU");
        return 77;
    }
    std::vector<VkPhysicalDevice> gpus(gpu_count);
    if (vkEnumeratePhysicalDevices(c.instance, &gpu_count, gpus.data()) != VK_SUCCESS)
        return 1;

    const std::set<std::string> required{
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_RAY_QUERY_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
    };
    for (int preference = 0; preference < 2 && !c.physical; ++preference) {
        for (const auto gpu : gpus) {
            VkPhysicalDeviceProperties p{};
            vkGetPhysicalDeviceProperties(gpu, &p);
            if ((p.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) !=
                (preference == 0)) continue;
            if (VK_API_VERSION_MAJOR(p.apiVersion) < 1 ||
                (VK_API_VERSION_MAJOR(p.apiVersion) == 1 &&
                 VK_API_VERSION_MINOR(p.apiVersion) < 2)) continue;
            if (!has_extension(gpu, required)) continue;

            VkPhysicalDeviceRayQueryFeaturesKHR ray{
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR};
            VkPhysicalDeviceAccelerationStructureFeaturesKHR accel{
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};
            VkPhysicalDeviceBufferDeviceAddressFeatures address{
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES};
            VkPhysicalDeviceFeatures2 features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
            features.pNext = &address;
            address.pNext = &accel;
            accel.pNext = &ray;
            vkGetPhysicalDeviceFeatures2(gpu, &features);
            if (!ray.rayQuery || !accel.accelerationStructure ||
                !address.bufferDeviceAddress) continue;

            uint32_t family_count = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(gpu, &family_count, nullptr);
            std::vector<VkQueueFamilyProperties> families(family_count);
            vkGetPhysicalDeviceQueueFamilyProperties(gpu, &family_count, families.data());
            for (uint32_t family = 0; family < family_count; ++family) {
                if (families[family].queueCount &&
                    (families[family].queueFlags & VK_QUEUE_COMPUTE_BIT)) {
                    c.physical = gpu;
                    c.queue_family = family;
                    chosen = p;
                    break;
                }
            }
            if (c.physical) break;
        }
    }
    if (!c.physical) {
        std::puts("SKIP: no Vulkan GPU supports required ray-query/AS features");
        return 77;
    }

    VkPhysicalDeviceRayQueryFeaturesKHR use_ray{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR};
    use_ray.rayQuery = VK_TRUE;
    VkPhysicalDeviceAccelerationStructureFeaturesKHR use_accel{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};
    use_accel.accelerationStructure = VK_TRUE;
    use_accel.pNext = &use_ray;
    VkPhysicalDeviceBufferDeviceAddressFeatures use_address{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES};
    use_address.bufferDeviceAddress = VK_TRUE;
    use_address.pNext = &use_accel;
    VkPhysicalDeviceFeatures2 enabled{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    enabled.pNext = &use_address;
    float priority = 1.f;
    VkDeviceQueueCreateInfo queue_info{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queue_info.queueFamilyIndex = c.queue_family;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = &priority;
    std::array<const char*, 3> exts{
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_RAY_QUERY_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME
    };
    VkDeviceCreateInfo device_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    device_info.pNext = &enabled;
    device_info.queueCreateInfoCount = 1;
    device_info.pQueueCreateInfos = &queue_info;
    device_info.enabledExtensionCount = static_cast<uint32_t>(exts.size());
    device_info.ppEnabledExtensionNames = exts.data();
    if (vkCreateDevice(c.physical, &device_info, nullptr, &c.device) != VK_SUCCESS) {
        std::puts("FAIL: selected RT GPU could not create a Vulkan RT device");
        return 1;
    }
    vkGetDeviceQueue(c.device, c.queue_family, 0, &c.queue);
#ifdef OGL_GPU_PROBE_LIBRARY
    ++device_creations;
#endif
    } else {
        vkGetPhysicalDeviceProperties(c.physical, &chosen);
    }

    auto get_build_sizes = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(
        vkGetDeviceProcAddr(c.device, "vkGetAccelerationStructureBuildSizesKHR"));
    auto create_blas = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(
        vkGetDeviceProcAddr(c.device, "vkCreateAccelerationStructureKHR"));
    auto build_blas = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(
        vkGetDeviceProcAddr(c.device, "vkCmdBuildAccelerationStructuresKHR"));
    auto get_blas_address = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(
        vkGetDeviceProcAddr(c.device, "vkGetAccelerationStructureDeviceAddressKHR"));
    c.destroy_blas = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(
        vkGetDeviceProcAddr(c.device, "vkDestroyAccelerationStructureKHR"));
    if (!get_build_sizes || !create_blas || !build_blas || !get_blas_address ||
        !c.destroy_blas) {
        std::puts("FAIL: Vulkan acceleration structure entrypoints unavailable");
        return 1;
    }

    void* mapped = nullptr;
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &c.command;

#ifdef OGL_GPU_PROBE_LIBRARY
    if (refit_geometry) {
        // Vertex positions changed but the primitive topology/count has
        // not. Preserve BLAS device address and the TLAS instance that
        // refers to it; perform an in-place Vulkan UPDATE (refit).
        const VkDeviceSize bytes = size_t(wanted_triangles) *
                                   sizeof(ogl_rtx_triangle);
        if (bytes > c.vertices.size) return 1;
        void* updated = nullptr;
        if (vkMapMemory(c.device,c.vertices.memory,0,bytes,0,
                        &updated) != VK_SUCCESS) return 1;
        std::memcpy(updated,scene,bytes);
        vkUnmapMemory(c.device,c.vertices.memory);

        VkAccelerationStructureGeometryTrianglesDataKHR deform{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR};
        deform.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        deform.vertexData.deviceAddress = buffer_address(c,c.vertices);
        deform.vertexStride = 3*sizeof(float);
        deform.maxVertex = wanted_triangles*3u-1u;
        deform.indexType = VK_INDEX_TYPE_NONE_KHR;
        if (!deform.vertexData.deviceAddress) return 1;
        VkAccelerationStructureGeometryKHR geo{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
        geo.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        geo.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        geo.geometry.triangles = deform;
        VkAccelerationStructureBuildGeometryInfoKHR update_info{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
        update_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        update_info.flags =
            VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR |
            VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
        update_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
        update_info.srcAccelerationStructure = c.blas;
        update_info.dstAccelerationStructure = c.blas;
        update_info.geometryCount = 1;
        update_info.pGeometries = &geo;
        const uint32_t primitive_count = wanted_triangles;
        VkAccelerationStructureBuildSizesInfoKHR update_sizes{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
        get_build_sizes(c.device,VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                        &update_info,&primitive_count,&update_sizes);
        VkPhysicalDeviceAccelerationStructurePropertiesKHR properties{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR};
        VkPhysicalDeviceProperties2 props{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
        props.pNext = &properties;
        vkGetPhysicalDeviceProperties2(c.physical,&props);
        const VkDeviceSize alignment =
            properties.minAccelerationStructureScratchOffsetAlignment;
        if (!alignment || (alignment & (alignment-1u))) return 1;
        const VkDeviceAddress base = buffer_address(c,c.scratch);
        const VkDeviceAddress aligned = (base+alignment-1u)&~(alignment-1u);
        if (!base || update_sizes.updateScratchSize > c.scratch.size ||
            (aligned-base) + update_sizes.updateScratchSize > c.scratch.size)
            return 1;
        update_info.scratchData.deviceAddress = aligned;
        if (vkResetCommandPool(c.device,c.pool,0) != VK_SUCCESS ||
            vkBeginCommandBuffer(c.command,&begin) != VK_SUCCESS)
            return 1;
        VkAccelerationStructureBuildRangeInfoKHR range{};
        range.primitiveCount = wanted_triangles;
        const VkAccelerationStructureBuildRangeInfoKHR* ranges[]{&range};
        build_blas(c.command,1,&update_info,ranges);
        if (vkEndCommandBuffer(c.command) != VK_SUCCESS ||
            vkQueueSubmit(c.queue,1,&submit,VK_NULL_HANDLE) != VK_SUCCESS ||
            vkQueueWaitIdle(c.queue) != VK_SUCCESS)
            return 1;
        ++blas_refits;
        std::printf("PASS: refitted BLAS in place, %u triangles; retained TLAS instance\n",
                    wanted_triangles);
    }
    if (!reuse_geometry && !refit_geometry) {
#endif
    // Caller supplies the actual validated module scene snapshot.
    // GPU probe and deterministic tests share this exact same path.
    std::vector<ogl_rtx_triangle> triangles(scene, scene + wanted_triangles);
    static_assert(sizeof(ogl_rtx_triangle) == 9 * sizeof(float),
                  "Triangle ABI must be packed xyz floats");
    const VkDeviceSize vertex_bytes = triangles.size() * sizeof(ogl_rtx_triangle);
    if (!allocate_buffer(c, c.vertices, vertex_bytes,
                         VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                         VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         true)) {
        std::puts("FAIL: vertex GPU buffer allocation");
        return 1;
    }
    mapped = nullptr;
    if (vkMapMemory(c.device, c.vertices.memory, 0, vertex_bytes, 0, &mapped) != VK_SUCCESS)
        return 1;
    std::memcpy(mapped, triangles.data(), vertex_bytes);
    vkUnmapMemory(c.device, c.vertices.memory);

    VkAccelerationStructureGeometryTrianglesDataKHR vertices{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR};
    vertices.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    vertices.vertexData.deviceAddress = buffer_address(c, c.vertices);
    vertices.vertexStride = 3 * sizeof(float);
    vertices.maxVertex = wanted_triangles * 3 - 1;
    vertices.indexType = VK_INDEX_TYPE_NONE_KHR;
    if (!vertices.vertexData.deviceAddress) return 1;

    VkAccelerationStructureGeometryKHR geometry{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
    geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    geometry.geometry.triangles = vertices;
    VkAccelerationStructureBuildGeometryInfoKHR build_info{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR |
                       VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
    build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    build_info.geometryCount = 1;
    build_info.pGeometries = &geometry;
    const uint32_t primitive_count = wanted_triangles;
    constexpr uint32_t instance_count = 1;
    VkAccelerationStructureBuildSizesInfoKHR sizes{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    get_build_sizes(c.device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                    &build_info, &primitive_count, &sizes);
    if (!sizes.accelerationStructureSize || !sizes.buildScratchSize) {
        std::puts("FAIL: invalid BLAS size response");
        return 1;
    }
    if (!allocate_buffer(c, c.storage, sizes.accelerationStructureSize,
                         VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, false)) {
        std::puts("FAIL: BLAS storage buffer allocation");
        return 1;
    }
    VkPhysicalDeviceAccelerationStructurePropertiesKHR accel_properties{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR};
    VkPhysicalDeviceProperties2 props2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    props2.pNext = &accel_properties;
    vkGetPhysicalDeviceProperties2(c.physical, &props2);
    const VkDeviceSize alignment = accel_properties.minAccelerationStructureScratchOffsetAlignment;
    if (!alignment || (alignment & (alignment - 1))) return 1;
    if (!allocate_buffer(c, c.scratch,
                         std::max(sizes.buildScratchSize,sizes.updateScratchSize) +
                         alignment,
                         VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                         VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, true)) {
        std::puts("FAIL: BLAS scratch buffer allocation");
        return 1;
    }
    const VkDeviceAddress base = buffer_address(c, c.scratch);
    if (!base) return 1;
    build_info.scratchData.deviceAddress = (base + alignment - 1) & ~(alignment - 1);

    VkAccelerationStructureCreateInfoKHR blas_info{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
    blas_info.buffer = c.storage.buffer;
    blas_info.size = sizes.accelerationStructureSize;
    blas_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    if (create_blas(c.device, &blas_info, nullptr, &c.blas) != VK_SUCCESS)
        return 1;
    build_info.dstAccelerationStructure = c.blas;

    VkCommandPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pool_info.queueFamilyIndex = c.queue_family;
    if (vkCreateCommandPool(c.device, &pool_info, nullptr, &c.pool) != VK_SUCCESS)
        return 1;
    VkCommandBufferAllocateInfo cmd_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cmd_info.commandPool = c.pool;
    cmd_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_info.commandBufferCount = 1;
    if (vkAllocateCommandBuffers(c.device, &cmd_info, &c.command) != VK_SUCCESS)
        return 1;
    if (vkBeginCommandBuffer(c.command, &begin) != VK_SUCCESS) return 1;
    VkAccelerationStructureBuildRangeInfoKHR range{};
    range.primitiveCount = primitive_count;
    const VkAccelerationStructureBuildRangeInfoKHR* ranges[] = {&range};
    build_blas(c.command, 1, &build_info, ranges);
    if (vkEndCommandBuffer(c.command) != VK_SUCCESS) return 1;

    if (vkQueueSubmit(c.queue, 1, &submit, VK_NULL_HANDLE) != VK_SUCCESS ||
        vkQueueWaitIdle(c.queue) != VK_SUCCESS) {
        std::puts("FAIL: GPU BLAS c.command submission");
        return 1;
    }
    VkAccelerationStructureDeviceAddressInfoKHR address_info{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
    address_info.accelerationStructure = c.blas;
    const VkDeviceAddress blas_address = get_blas_address(c.device, &address_info);
    if (!blas_address) {
        std::puts("FAIL: completed BLAS has no GPU device address");
        return 1;
    }
#ifdef OGL_GPU_PROBE_LIBRARY
    ++blas_builds;
#endif
    std::printf("PASS: GPU BLAS built from %u ABI triangles on %s, AS bytes=%llu scratch bytes=%llu\n",
        wanted_triangles, chosen.deviceName,
        static_cast<unsigned long long>(sizes.accelerationStructureSize),
        static_cast<unsigned long long>(sizes.buildScratchSize));

    // Build one TLAS instance referencing the completed BLAS. No shaders
    // are dispatched in this smoke test, so the viewer remains OpenGL.
    VkAccelerationStructureInstanceKHR instance{};
    instance.transform.matrix[0][0] = 1.f;
    instance.transform.matrix[1][1] = 1.f;
    instance.transform.matrix[2][2] = 1.f;
    for (unsigned axis=0;axis<3;++axis)
        instance.transform.matrix[axis][3] =
            instance_xyz ? instance_xyz[axis] : 0.f;
    instance.mask = 0xFF;
    instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    instance.accelerationStructureReference = blas_address;
    if (!allocate_buffer(c, c.instance_buffer, sizeof(instance),
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            true)) {
        std::puts("FAIL: TLAS instance buffer allocation");
        return 1;
    }
    mapped = nullptr;
    if (vkMapMemory(c.device, c.instance_buffer.memory, 0,
                    sizeof(instance), 0, &mapped) != VK_SUCCESS)
        return 1;
    std::memcpy(mapped, &instance, sizeof(instance));
    vkUnmapMemory(c.device, c.instance_buffer.memory);
    VkAccelerationStructureGeometryInstancesDataKHR instance_data{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR};
    instance_data.arrayOfPointers = VK_FALSE;
    instance_data.data.deviceAddress = buffer_address(c, c.instance_buffer);
    if (!instance_data.data.deviceAddress) return 1;
    VkAccelerationStructureGeometryKHR top_geometry{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
    top_geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    top_geometry.geometry.instances = instance_data;
    VkAccelerationStructureBuildGeometryInfoKHR top_build{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    top_build.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    top_build.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR |
                      VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
    top_build.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    top_build.geometryCount = 1;
    top_build.pGeometries = &top_geometry;
    VkAccelerationStructureBuildSizesInfoKHR top_sizes{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    get_build_sizes(c.device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                    &top_build, &instance_count, &top_sizes);
    if (!top_sizes.accelerationStructureSize || !top_sizes.buildScratchSize) {
        std::puts("FAIL: invalid TLAS size response");
        return 1;
    }
    if (!allocate_buffer(c, c.tlas_storage, top_sizes.accelerationStructureSize,
                         VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, false))
        return 1;
    if (!allocate_buffer(c, c.tlas_scratch,
                         std::max(top_sizes.buildScratchSize,top_sizes.updateScratchSize)
                         + alignment,
                         VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                         VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, true))
        return 1;
    const VkDeviceAddress scratch_addr = buffer_address(c, c.tlas_scratch);
    if (!scratch_addr) return 1;
    top_build.scratchData.deviceAddress =
        (scratch_addr + alignment - 1) & ~(alignment - 1);
    VkAccelerationStructureCreateInfoKHR top_info{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
    top_info.buffer = c.tlas_storage.buffer;
    top_info.size = top_sizes.accelerationStructureSize;
    top_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    if (create_blas(c.device, &top_info, nullptr, &c.tlas) != VK_SUCCESS)
        return 1;
    top_build.dstAccelerationStructure = c.tlas;

    // QueueWaitIdle above guarantees BLAS completion before TLAS build.
    // Reset the pool and safely reuse its c.command buffer.
    if (vkResetCommandPool(c.device, c.pool, 0) != VK_SUCCESS)
        return 1;
    if (vkBeginCommandBuffer(c.command, &begin) != VK_SUCCESS)
        return 1;
    // TLAS contains ONE instance regardless of its BLAS triangle count.
    // Reusing BLAS primitiveCount here reads beyond the instance buffer,
    // potentially causing Vulkan DEVICE_LOST on real GPUs.
    VkAccelerationStructureBuildRangeInfoKHR top_range{};
    top_range.primitiveCount = instance_count;
    const VkAccelerationStructureBuildRangeInfoKHR* top_ranges[] = {&top_range};
    build_blas(c.command, 1, &top_build, top_ranges);
    if (vkEndCommandBuffer(c.command) != VK_SUCCESS)
        return 1;
    if (vkQueueSubmit(c.queue, 1, &submit, VK_NULL_HANDLE) != VK_SUCCESS ||
        vkQueueWaitIdle(c.queue) != VK_SUCCESS) {
        std::puts("FAIL: TLAS GPU c.command submission");
        return 1;
    }
    address_info.accelerationStructure = c.tlas;
    if (!get_blas_address(c.device, &address_info)) {
        std::puts("FAIL: completed TLAS has no GPU address");
        return 1;
    }
    std::printf("PASS: GPU TLAS built with 1 BLAS instance, AS bytes=%llu scratch bytes=%llu\n",
        static_cast<unsigned long long>(top_sizes.accelerationStructureSize),
        static_cast<unsigned long long>(top_sizes.buildScratchSize));
#ifdef OGL_GPU_PROBE_LIBRARY
    } else {
        std::puts(refit_geometry ?
            "PASS: preserved TLAS after BLAS in-place refit" :
            "PASS: reusing existing GPU BLAS/TLAS after lighting change");
    }
#endif
#ifdef OGL_GPU_PROBE_LIBRARY
    if ((reuse_geometry || refit_geometry) &&
        cached_instance_xyz != requested_instance) {
        // Instance translation changed: the existing BLAS is untouched.
        // Refresh TLAS in-place using the same one-instance topology.
        VkAccelerationStructureDeviceAddressInfoKHR blas_address_info{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
        blas_address_info.accelerationStructure = c.blas;
        const auto address = get_blas_address(c.device,&blas_address_info);
        if (!address) return 1;
        VkAccelerationStructureInstanceKHR instance{};
        for (unsigned axis=0;axis<3;++axis)
            instance.transform.matrix[axis][axis] = 1.f;
        for (unsigned axis=0;axis<3;++axis)
            instance.transform.matrix[axis][3]=requested_instance[axis];
        instance.mask = 0xff;
        instance.flags =
            VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        instance.accelerationStructureReference = address;
        void* target=nullptr;
        if (vkMapMemory(c.device,c.instance_buffer.memory,0,
                        sizeof(instance),0,&target)!=VK_SUCCESS) return 1;
        std::memcpy(target,&instance,sizeof(instance));
        vkUnmapMemory(c.device,c.instance_buffer.memory);
        VkAccelerationStructureGeometryInstancesDataKHR instances{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR};
        instances.data.deviceAddress=buffer_address(c,c.instance_buffer);
        if (!instances.data.deviceAddress) return 1;
        VkAccelerationStructureGeometryKHR geometry{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
        geometry.geometryType=VK_GEOMETRY_TYPE_INSTANCES_KHR;
        geometry.geometry.instances=instances;
        VkAccelerationStructureBuildGeometryInfoKHR update{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
        update.type=VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        update.flags=VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR |
                     VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
        update.mode=VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
        update.srcAccelerationStructure=c.tlas;
        update.dstAccelerationStructure=c.tlas;
        update.geometryCount=1;
        update.pGeometries=&geometry;
        const uint32_t instance_count=1;
        VkAccelerationStructureBuildSizesInfoKHR sizes{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
        get_build_sizes(c.device,VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                        &update,&instance_count,&sizes);
        VkPhysicalDeviceAccelerationStructurePropertiesKHR properties{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR};
        VkPhysicalDeviceProperties2 properties2{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
        properties2.pNext=&properties;
        vkGetPhysicalDeviceProperties2(c.physical,&properties2);
        // Device addresses are 64-bit. The alignment mask must NOT be
        // computed as a 32-bit inverse, which truncates the GPU address!
        const VkDeviceSize alignment =
            properties.minAccelerationStructureScratchOffsetAlignment;
        if (!alignment || (alignment & (alignment-1u))) return 1;
        const auto base=buffer_address(c,c.tlas_scratch);
        const auto aligned=(base+alignment-1u)&~(alignment-1u);
        if (!base || (aligned-base)+sizes.updateScratchSize>c.tlas_scratch.size) {
            std::printf("FAIL: TLAS refit scratch need=%llu available=%llu align=%llu\n",
                static_cast<unsigned long long>((aligned-base)+sizes.updateScratchSize),
                static_cast<unsigned long long>(c.tlas_scratch.size),
                static_cast<unsigned long long>(alignment));
            return 1;
        }
        update.scratchData.deviceAddress=aligned;
        if (vkResetCommandPool(c.device,c.pool,0)!=VK_SUCCESS ||
            vkBeginCommandBuffer(c.command,&begin)!=VK_SUCCESS) {
            std::puts("FAIL: TLAS refit command pool/reset"); return 1;
        }
        VkAccelerationStructureBuildRangeInfoKHR range{};
        range.primitiveCount=1;
        const VkAccelerationStructureBuildRangeInfoKHR* ranges[]{&range};
        build_blas(c.command,1,&update,ranges);
        if (vkEndCommandBuffer(c.command)!=VK_SUCCESS ||
            vkQueueSubmit(c.queue,1,&submit,VK_NULL_HANDLE)!=VK_SUCCESS ||
            vkQueueWaitIdle(c.queue)!=VK_SUCCESS) {
            std::puts("FAIL: TLAS refit Vulkan queue submission"); return 1;
        }
        ++tlas_refits;
        std::puts("PASS: TLAS instance transform updated in-place, BLAS preserved");
    }
#endif
    // A real Vulkan ray query: one ray hits the triangle, one deliberately
    // misses. Results are returned through a host-visible storage buffer.
    std::ifstream input(shader_path, std::ios::binary | std::ios::ate);
    if (!input) { std::puts("FAIL: ray-query SPIR-V missing"); return 1; }
    const auto shader_bytes = input.tellg();
    if (shader_bytes <= 0 || shader_bytes > 65536 || shader_bytes % 4 != 0)
        return 1;
    std::vector<uint32_t> code(static_cast<size_t>(shader_bytes) / 4);
    input.seekg(0);
    input.read(reinterpret_cast<char*>(code.data()), shader_bytes);
    if (!input || code.empty() || code.front() != 0x07230203u)
        return 1;
    VkShaderModuleCreateInfo shader_info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    shader_info.codeSize = static_cast<size_t>(shader_bytes);
    shader_info.pCode = code.data();
    if (vkCreateShaderModule(c.device, &shader_info, nullptr, &c.shader) != VK_SUCCESS)
        return 1;

    VkDescriptorSetLayoutBinding bindings[3]{};
    bindings[0].binding = 0;
    bindings[0].descriptorCount = 1;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    bindings[1].binding = 1;
    bindings[1].descriptorCount = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    bindings[2].binding = 2;
    bindings[2].descriptorCount = 1;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    VkDescriptorSetLayoutCreateInfo layout_info{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layout_info.bindingCount = depth_64 ? 3 : 2;
    layout_info.pBindings = bindings;
    if (vkCreateDescriptorSetLayout(c.device, &layout_info, nullptr,
                                    &c.descriptor_layout) != VK_SUCCESS)
        return 1;
    VkDescriptorPoolSize pool_sizes[2]{};
    pool_sizes[0] = {VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1};
    pool_sizes[1] = {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                     depth_64 ? 2u : 1u};
    VkDescriptorPoolCreateInfo descriptor_pool_info{
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    descriptor_pool_info.maxSets = 1;
    descriptor_pool_info.poolSizeCount = 2;
    descriptor_pool_info.pPoolSizes = pool_sizes;
    if (vkCreateDescriptorPool(c.device, &descriptor_pool_info, nullptr,
                               &c.descriptor_pool) != VK_SUCCESS)
        return 1;
    VkDescriptorSetAllocateInfo descriptor_alloc{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    descriptor_alloc.descriptorPool = c.descriptor_pool;
    descriptor_alloc.descriptorSetCount = 1;
    descriptor_alloc.pSetLayouts = &c.descriptor_layout;
    VkDescriptorSet descriptor = VK_NULL_HANDLE;
    if (vkAllocateDescriptorSets(c.device, &descriptor_alloc, &descriptor) != VK_SUCCESS)
        return 1;

    constexpr uint32_t result_count = 8u * 8u;
    // Depth-guided shader writes sun visibility, primary BLAS intersection
    // and comparison of primary hit distance against OpenGL depth.
    // The original fixture/camera shaders still write only 64 uint values.
    const uint32_t output_sections = depth_64 ? 4u : 1u;
    const uint32_t result_bytes =
        result_count * output_sections * sizeof(uint32_t);
    if (!allocate_buffer(c, c.query_results, result_bytes,
                         VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, false))
        return 1;
    mapped = nullptr;
    if (vkMapMemory(c.device, c.query_results.memory, 0, result_bytes, 0,
                    &mapped) != VK_SUCCESS)
        return 1;
    std::memset(mapped, 0xFF, result_bytes);
    vkUnmapMemory(c.device, c.query_results.memory);

    VkWriteDescriptorSetAccelerationStructureKHR accel_descriptor{
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR};
    accel_descriptor.accelerationStructureCount = 1;
    accel_descriptor.pAccelerationStructures = &c.tlas;
    VkDescriptorBufferInfo result_descriptor{};
    result_descriptor.buffer = c.query_results.buffer;
    result_descriptor.range = result_bytes;
    VkWriteDescriptorSet writes[3]{};
    for (auto& w : writes) {
        w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        w.dstSet = descriptor;
        w.descriptorCount = 1;
    }
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    writes[0].pNext = &accel_descriptor;
    writes[1].dstBinding = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[1].pBufferInfo = &result_descriptor;
    VkDescriptorBufferInfo depth_descriptor{};
    if (depth_64) {
        const VkDeviceSize depth_bytes = 64 * sizeof(float);
        if (!allocate_buffer(c, c.depth_input, depth_bytes,
                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, false)) return 1;
        void* ptr = nullptr;
        if (vkMapMemory(c.device, c.depth_input.memory, 0, depth_bytes,
                        0, &ptr) != VK_SUCCESS) return 1;
        std::memcpy(ptr, depth_64, depth_bytes);
        vkUnmapMemory(c.device, c.depth_input.memory);
        depth_descriptor.buffer = c.depth_input.buffer;
        depth_descriptor.range = depth_bytes;
        writes[2].dstBinding = 2;
        writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[2].pBufferInfo = &depth_descriptor;
    }
    vkUpdateDescriptorSets(c.device, depth_64 ? 3u : 2u,
                           writes, 0, nullptr);

    VkPipelineLayoutCreateInfo pipeline_layout_info{
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &c.descriptor_layout;
    VkPushConstantRange push_range{};
    push_range.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    push_range.offset = 0;
    push_range.size = sizeof(push);
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &push_range;
    if (vkCreatePipelineLayout(c.device, &pipeline_layout_info, nullptr,
                               &c.pipeline_layout) != VK_SUCCESS)
        return 1;
    VkPipelineShaderStageCreateInfo stage{
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stage.module = c.shader;
    stage.pName = "main";
    VkComputePipelineCreateInfo compute{
        VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    compute.stage = stage;
    compute.layout = c.pipeline_layout;
    if (vkCreateComputePipelines(c.device, VK_NULL_HANDLE, 1, &compute,
                                 nullptr, &c.ray_pipeline) != VK_SUCCESS)
        return 1;
    if (vkResetCommandPool(c.device, c.pool, 0) != VK_SUCCESS)
        return 1;
    if (vkBeginCommandBuffer(c.command, &begin) != VK_SUCCESS)
        return 1;
    vkCmdBindPipeline(c.command, VK_PIPELINE_BIND_POINT_COMPUTE, c.ray_pipeline);
    vkCmdBindDescriptorSets(c.command, VK_PIPELINE_BIND_POINT_COMPUTE,
                            c.pipeline_layout, 0, 1, &descriptor, 0, nullptr);
    vkCmdPushConstants(c.command, c.pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT,
                       0, sizeof(push), &push);
    vkCmdDispatch(c.command, 1, 1, 1);
    if (vkEndCommandBuffer(c.command) != VK_SUCCESS)
        return 1;
    if (vkQueueSubmit(c.queue, 1, &submit, VK_NULL_HANDLE) != VK_SUCCESS ||
        vkQueueWaitIdle(c.queue) != VK_SUCCESS)
        return 1;
    mapped = nullptr;
    if (vkMapMemory(c.device, c.query_results.memory, 0, result_bytes, 0,
                    &mapped) != VK_SUCCESS)
        return 1;
    std::array<uint32_t, result_count * 4u> results{};
    std::memcpy(results.data(), mapped, result_bytes);
    vkUnmapMemory(c.device, c.query_results.memory);
    uint32_t hits = 0;
    for (unsigned y = 0; y < 8; ++y) {
        for (unsigned x = 0; x < 8; ++x) {
            const uint32_t actual = results[y * 8 + x];
            const uint32_t expected = (x + y <= 7) ? 1u : 0u;
            if (actual > 1 || (!result && actual != expected)) {
                std::printf("FAIL: GPU shadow mask (%u,%u) expected=%u actual=%u\n",
                            x, y, expected, actual);
                return 1;
            }
            hits += actual;
        }
    }
    std::printf("PASS: GPU 8x8 ray-query probe; occluded=%u visible=%u\n",
                hits, result_count - hits);
    if (result && depth_64) {
        unsigned primary_hits=0, matches=0, closer=0, farther=0;
        for (uint32_t i=0; i<result_count; ++i) {
            const uint32_t hit = results[result_count+i];
            const uint32_t same = results[2u*result_count+i];
            const uint32_t relation=results[3u*result_count+i];
            if (hit>1 || same>1 || same>hit || relation>3 ||
                (hit==0 && relation!=0) ||
                (hit==1 && relation==0) ||
                (same==1 && relation!=1) ||
                (relation==1 && same!=1))
                return 1;
            primary_hits+=hit;
            matches+=same;
            closer+=relation==2;
            farther+=relation==3;
            result->primary_mask[i]=static_cast<uint8_t>(hit);
            result->depth_match_mask[i]=static_cast<uint8_t>(same);
        }
        if (primary_hits!=matches+closer+farther) return 1;
        result->primary_hits=primary_hits;
        result->primary_depth_matches=matches;
        result->primary_closer=closer;
        result->primary_farther=farther;
        std::printf("PASS: primary hit=%u matched=%u closer=%u farther=%u of 64\n",
                    primary_hits,matches,closer,farther);
    }
    if (result) {
        result->occluded = hits;
        result->unoccluded = result_count - hits;
        result->ray_count = result_count;
        result->triangle_count = wanted_triangles;
        result->mask_width = 8;
        result->mask_height = 8;
        for (size_t i = 0; i < result_count; ++i)
            result->shadow_mask[i] = static_cast<uint8_t>(results[i]);
        result->success = 1;
#ifdef OGL_GPU_PROBE_LIBRARY
        result->vulkan_device_creations = device_creations;
        result->blas_builds = blas_builds;
        result->blas_refits = blas_refits;
        result->tlas_refits = tlas_refits;
        result->cache_hits = cache_hits;
        cached_output = *result;
        cached_scene.assign(scene, scene + wanted_triangles);
        cached_sun = requested_sun;
        cached_camera = requested_camera;
        cached_depth_valid = depth_64 != nullptr;
        if (depth_64) std::copy(depth_64,depth_64+64,cached_depth.begin());
        cached_shader = shader_path;
        cached_instance_xyz = requested_instance;
#endif
    } else if (hits != 36) {
        std::puts("FAIL: deterministic synthetic scene mask mismatch");
        return 1;
    }
    std::puts("Viewer OpenGL unchanged; GPU results NOT composited into world.");
    return 0;
}

#ifdef OGL_GPU_PROBE_LIBRARY
extern "C" OGL_MODULE_EXPORT int ogl_gpu_probe_transform_run(
    const ogl_rtx_triangle* triangles,uint32_t count,
    const float* sun_direction_xyz,const float* instance_position_xyz,
    const char* shader_path,ogl_gpu_probe_result* result)
{
    if (!instance_position_xyz || !result ||
        result->struct_size!=sizeof(*result) ||
        result->version!=OGL_GPU_PROBE_VERSION) return 1;
    ogl_gpu_probe_result fresh{};
    fresh.struct_size=sizeof(fresh);
    fresh.version=OGL_GPU_PROBE_VERSION;
    *result=fresh;
    return execute_gpu_scene(shader_path,triangles,count,sun_direction_xyz,
                             nullptr,result,nullptr,instance_position_xyz);
}
extern "C" OGL_MODULE_EXPORT int ogl_gpu_probe_run(
    const ogl_rtx_triangle* triangles, uint32_t count,
    const float* sun_direction_xyz,
    const char* shader_path, ogl_gpu_probe_result* result)
{
    if (!result || result->struct_size != sizeof(ogl_gpu_probe_result) ||
        result->version != OGL_GPU_PROBE_VERSION)
        return 1;
    ogl_gpu_probe_result fresh{};
    fresh.struct_size = sizeof(fresh);
    fresh.version = OGL_GPU_PROBE_VERSION;
    *result = fresh;
    return execute_gpu_scene(shader_path, triangles, count, sun_direction_xyz, nullptr, result, nullptr);
}
// Camera-visible primary rays followed by secondary sunlight occlusion.
extern "C" OGL_MODULE_EXPORT int ogl_gpu_probe_camera_run(
    const ogl_rtx_triangle* triangles, uint32_t count,
    const ogl_rtx_frame_state* frame,
    const char* shader_path, ogl_gpu_probe_result* result)
{
    if (!result || !frame ||
        result->struct_size != sizeof(ogl_gpu_probe_result) ||
        result->version != OGL_GPU_PROBE_VERSION) return 1;
    ogl_gpu_probe_result fresh{};
    fresh.struct_size = sizeof(fresh);
    fresh.version = OGL_GPU_PROBE_VERSION;
    *result = fresh;
    return execute_gpu_scene(shader_path, triangles, count,
                             frame->sun_direction, frame, result, nullptr);
}
// Matches the Full-View-Depth-Grid captured by Firestorm in one frame.
extern "C" OGL_MODULE_EXPORT int ogl_gpu_probe_depth_run(
    const ogl_rtx_triangle* triangles, uint32_t count,
    const ogl_rtx_frame_state* frame, const float* depth_64,
    const char* shader_path, ogl_gpu_probe_result* result)
{
    if (!result || !frame || !depth_64 ||
        result->struct_size != sizeof(ogl_gpu_probe_result) ||
        result->version != OGL_GPU_PROBE_VERSION) return 1;
    ogl_gpu_probe_result fresh{};
    fresh.struct_size = sizeof(fresh);
    fresh.version = OGL_GPU_PROBE_VERSION;
    *result = fresh;
    return execute_gpu_scene(shader_path, triangles, count,
                             frame->sun_direction, frame, result, depth_64);
}
#else
int main(int argc, char** argv) {
    if (argc != 2 && !(argc == 5 && std::strcmp(argv[2], "--module") == 0)) {
        std::puts("FAIL: use <shader.spv> [--module <ogl_rtx.so/dll> <triangle_count>]");
        return 1;
    }
    uint32_t count = 1;
    if (argc == 5) {
        char* end = nullptr;
        const unsigned long n = std::strtoul(argv[4], &end, 10);
        if (!end || *end || n < 1 || n > OGL_RTX_MAX_SCENE_TRIANGLES) return 1;
        count = static_cast<uint32_t>(n);
    }
    std::vector<ogl_rtx_triangle> triangles(count);
    if (argc == 5) {
        const int load_result = load_scene_from_module(argv[3], count, triangles);
        if (load_result != 0) return load_result;
    } else {
        triangles[0].position[3] = 1.f;
        triangles[0].position[7] = 1.f;
    }
    return execute_gpu_scene(argv[1], triangles.data(), count, nullptr, nullptr, nullptr, nullptr);
}
#endif

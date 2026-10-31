// OGL public Vulkan BLAS acceptance: build a real one-triangle GPU
// acceleration structure. No image rendering, no OpenSim data, no server.
// No hardware: CTest SKIP (77). GPU/API failure after selection: FAIL (1).
// Real ray-query compute shader tested; OpenGL compositing still pending.
#include <vulkan/vulkan.h>
#include "ogl/ogl_rtx_api.h"
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#if defined(_WIN32)
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#else
#  include <dlfcn.h>
#endif
#include <set>
#include <string>
#include <vector>

// The same portable C ABI used by the live OpenSim viewer.
using GeometrySubmit = int (*)(const ogl_rtx_triangle*, uint32_t);
using GeometryCopy = uint32_t (*)(ogl_rtx_triangle*, uint32_t);
using GeometryRevision = uint64_t (*)();

void diagnostic_log(int, const char*) {}

static bool load_scene_from_module(const char* path, uint32_t count,
                                   std::vector<ogl_rtx_triangle>& scene) {
#if defined(_WIN32)
    HMODULE module = LoadLibraryExA(path, nullptr,
        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (!module) return false;
    auto symbol = [&](const char* name) -> void* {
        return reinterpret_cast<void*>(GetProcAddress(module, name));
    };
#else
    void* module = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!module) return false;
    auto symbol = [&](const char* name) -> void* { return dlsym(module, name); };
#endif
    auto start = reinterpret_cast<ogl_module_start_fn>(symbol("ogl_module_start"));
    auto stop = reinterpret_cast<ogl_module_stop_fn>(symbol("ogl_module_stop"));
    auto submit = reinterpret_cast<GeometrySubmit>(symbol("ogl_rtx_submit_geometry"));
    auto copy = reinterpret_cast<GeometryCopy>(symbol("ogl_rtx_copy_geometry"));
    auto revision = reinterpret_cast<GeometryRevision>(symbol("ogl_rtx_geometry_revision"));
    bool ok = false;
    if (start && stop && submit && copy && revision) {
        ogl_host_api host{sizeof(host), OGL_MODULE_ABI_VERSION, diagnostic_log};
        if (start(&host) == 0) {
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
            ok = submit(input.data(), count) == 1 &&
                 revision() == 1 &&
                 copy(scene.data(), count) == count &&
                 std::memcmp(scene.data(), input.data(), count * sizeof(ogl_rtx_triangle)) == 0;
            stop();
        }
    }
#if defined(_WIN32)
    FreeLibrary(module);
#else
    dlclose(module);
#endif
    return ok;
}

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
    VkAccelerationStructureKHR blas = VK_NULL_HANDLE;
    VkAccelerationStructureKHR tlas = VK_NULL_HANDLE;
    PFN_vkDestroyAccelerationStructureKHR destroy_blas = nullptr;
    Buffer vertices, scratch, storage;
    Buffer instance_buffer, tlas_scratch, tlas_storage, query_results;
    VkDescriptorSetLayout descriptor_layout = VK_NULL_HANDLE;
    VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    VkPipeline ray_pipeline = VK_NULL_HANDLE;
    VkShaderModule shader = VK_NULL_HANDLE;
    ~VulkanContext() {
        if (device) {
            vkDeviceWaitIdle(device);
            if (ray_pipeline) vkDestroyPipeline(device, ray_pipeline, nullptr);
            if (shader) vkDestroyShaderModule(device, shader, nullptr);
            if (pipeline_layout) vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
            if (descriptor_pool) vkDestroyDescriptorPool(device, descriptor_pool, nullptr);
            if (descriptor_layout) vkDestroyDescriptorSetLayout(device, descriptor_layout, nullptr);
            if (tlas && destroy_blas) destroy_blas(device, tlas, nullptr);
            if (blas && destroy_blas) destroy_blas(device, blas, nullptr);
            for (const Buffer* b : {&vertices, &scratch, &storage,
                                    &instance_buffer, &tlas_scratch, &tlas_storage,
                                    &query_results}) {
                if (b->buffer) vkDestroyBuffer(device, b->buffer, nullptr);
                if (b->memory) vkFreeMemory(device, b->memory, nullptr);
            }
            if (pool) vkDestroyCommandPool(device, pool, nullptr);
            vkDestroyDevice(device, nullptr);
        }
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

int main(int argc, char** argv) {
    if (argc != 2 && !(argc == 5 && std::strcmp(argv[2], "--module") == 0)) {
        std::puts("FAIL: use <shader.spv> [--module <ogl_rtx.so/dll> <triangle_count>]");
        return 1;
    }
    uint32_t wanted_triangles = 1;
    if (argc == 5) {
        char* end = nullptr;
        const unsigned long n = std::strtoul(argv[4], &end, 10);
        if (!end || *end || n < 1 || n > OGL_RTX_MAX_SCENE_TRIANGLES) {
            std::puts("FAIL: triangle_count must be 1-1024");
            return 1;
        }
        wanted_triangles = static_cast<uint32_t>(n);
    }
    VulkanContext c;
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
    VkPhysicalDeviceProperties chosen{};
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

    // Build from the same validated OGL geometry ABI used by the live viewer.
    // In standalone mode keep the deterministic unit triangle fixture.
    // In module mode prove C ABI snapshot -> Vulkan GPU BLAS -> TLAS -> rays.
    std::vector<ogl_rtx_triangle> triangles(wanted_triangles);
    if (argc == 5) {
        if (!load_scene_from_module(argv[3], wanted_triangles, triangles)) {
            std::puts("FAIL: OGL module scene snapshot transfer");
            return 1;
        }
    } else {
        triangles[0].position[3] = 1.f;
        triangles[0].position[7] = 1.f;
    }
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
    void* mapped = nullptr;
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
    build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
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
    if (!allocate_buffer(c, c.scratch, sizes.buildScratchSize + alignment,
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
    VkCommandBuffer command = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(c.device, &cmd_info, &command) != VK_SUCCESS)
        return 1;
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (vkBeginCommandBuffer(command, &begin) != VK_SUCCESS) return 1;
    VkAccelerationStructureBuildRangeInfoKHR range{};
    range.primitiveCount = primitive_count;
    const VkAccelerationStructureBuildRangeInfoKHR* ranges[] = {&range};
    build_blas(command, 1, &build_info, ranges);
    if (vkEndCommandBuffer(command) != VK_SUCCESS) return 1;

    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &command;
    if (vkQueueSubmit(c.queue, 1, &submit, VK_NULL_HANDLE) != VK_SUCCESS ||
        vkQueueWaitIdle(c.queue) != VK_SUCCESS) {
        std::puts("FAIL: GPU BLAS command submission");
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
    top_build.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
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
    if (!allocate_buffer(c, c.tlas_scratch, top_sizes.buildScratchSize + alignment,
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
    // Reset the pool and safely reuse its command buffer.
    if (vkResetCommandPool(c.device, c.pool, 0) != VK_SUCCESS)
        return 1;
    if (vkBeginCommandBuffer(command, &begin) != VK_SUCCESS)
        return 1;
    // TLAS contains ONE instance even if its BLAS has 1024 triangles.
    // Reusing BLAS primitiveCount here reads beyond the instance buffer,
    // potentially causing Vulkan DEVICE_LOST on real GPUs.
    VkAccelerationStructureBuildRangeInfoKHR top_range{};
    top_range.primitiveCount = instance_count;
    const VkAccelerationStructureBuildRangeInfoKHR* top_ranges[] = {&top_range};
    build_blas(command, 1, &top_build, top_ranges);
    if (vkEndCommandBuffer(command) != VK_SUCCESS)
        return 1;
    if (vkQueueSubmit(c.queue, 1, &submit, VK_NULL_HANDLE) != VK_SUCCESS ||
        vkQueueWaitIdle(c.queue) != VK_SUCCESS) {
        std::puts("FAIL: TLAS GPU command submission");
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
    // A real Vulkan ray query: one ray hits the triangle, one deliberately
    // misses. Results are returned through a host-visible storage buffer.
    std::ifstream input(argv[1], std::ios::binary | std::ios::ate);
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

    VkDescriptorSetLayoutBinding bindings[2]{};
    bindings[0].binding = 0;
    bindings[0].descriptorCount = 1;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    bindings[1].binding = 1;
    bindings[1].descriptorCount = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    VkDescriptorSetLayoutCreateInfo layout_info{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layout_info.bindingCount = 2;
    layout_info.pBindings = bindings;
    if (vkCreateDescriptorSetLayout(c.device, &layout_info, nullptr,
                                    &c.descriptor_layout) != VK_SUCCESS)
        return 1;
    VkDescriptorPoolSize pool_sizes[2]{};
    pool_sizes[0] = {VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1};
    pool_sizes[1] = {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1};
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
    constexpr uint32_t result_bytes = result_count * sizeof(uint32_t);
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
    VkWriteDescriptorSet writes[2]{};
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
    vkUpdateDescriptorSets(c.device, 2, writes, 0, nullptr);

    VkPipelineLayoutCreateInfo pipeline_layout_info{
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &c.descriptor_layout;
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
    if (vkBeginCommandBuffer(command, &begin) != VK_SUCCESS)
        return 1;
    vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_COMPUTE, c.ray_pipeline);
    vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_COMPUTE,
                            c.pipeline_layout, 0, 1, &descriptor, 0, nullptr);
    vkCmdDispatch(command, 1, 1, 1);
    if (vkEndCommandBuffer(command) != VK_SUCCESS)
        return 1;
    if (vkQueueSubmit(c.queue, 1, &submit, VK_NULL_HANDLE) != VK_SUCCESS ||
        vkQueueWaitIdle(c.queue) != VK_SUCCESS)
        return 1;
    mapped = nullptr;
    if (vkMapMemory(c.device, c.query_results.memory, 0, result_bytes, 0,
                    &mapped) != VK_SUCCESS)
        return 1;
    std::array<uint32_t, result_count> results{};
    std::memcpy(results.data(), mapped, result_bytes);
    vkUnmapMemory(c.device, c.query_results.memory);
    uint32_t hits = 0;
    for (unsigned y = 0; y < 8; ++y) {
        for (unsigned x = 0; x < 8; ++x) {
            const uint32_t actual = results[y * 8 + x];
            const uint32_t expected = (x + y <= 7) ? 1u : 0u;
            if (actual != expected) {
                std::printf("FAIL: GPU shadow mask (%u,%u) expected=%u actual=%u\n",
                            x, y, expected, actual);
                return 1;
            }
            hits += actual;
        }
    }
    std::printf("PASS: GPU 8x8 ray-query shadow mask; occluded=%u visible=%u\n",
                hits, result_count - hits);
    if (hits != 36) return 1;
    std::puts("Viewer OpenGL unchanged; shadow mask not yet composited into world.");
    return 0;
}

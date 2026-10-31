#include "ogl/ogl_rtx_api.h"
#include <vulkan/vulkan.h>
#include <dlfcn.h>
#include <cstdio>
#include <set>
#include <string>
#include <vector>

namespace {
ogl_rtx_status status{sizeof(ogl_rtx_status), OGL_RTX_STATUS_VERSION};
const ogl_module_info info{
    sizeof(ogl_module_info), OGL_MODULE_ABI_VERSION,
    "de.nexverse.ogl.rtx", "OGL RTX Capability", "0.1.0"
};
ogl_log_fn logger = nullptr;
ogl_rtx_frame_state latest_frame{};
uint64_t received_frames = 0;
void report(std::string message) {
    std::snprintf(status.message, sizeof(status.message), "%s", message.c_str());
}
void detect() {
    status = {};
    status.struct_size = sizeof(status);
    status.status_version = OGL_RTX_STATUS_VERSION;

    void *library = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);
    if (!library) { report("Vulkan loader absent; OpenGL fallback."); return; }

    const auto get_proc = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
        dlsym(library, "vkGetInstanceProcAddr"));
    if (!get_proc) { report("Vulkan loader symbol unavailable."); dlclose(library); return; }
    const auto create = reinterpret_cast<PFN_vkCreateInstance>(
        get_proc(VK_NULL_HANDLE, "vkCreateInstance"));
    if (!create) { report("Vulkan instance creation unavailable."); dlclose(library); return; }

    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "OGL RTX capability";
    app.pEngineName = "OGL Viewer";
    app.apiVersion = VK_API_VERSION_1_2;
    VkInstanceCreateInfo init{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    init.pApplicationInfo = &app;
    VkInstance instance = VK_NULL_HANDLE;
    if (create(&init, nullptr, &instance) != VK_SUCCESS || !instance) {
        report("Vulkan 1.2 initialization failed.");
        dlclose(library);
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
                for (uint32_t i = 0; i < count; ++i) {
                    VkPhysicalDeviceProperties gpu{};
                    props(devices[i], &gpu);
                    if (gpu.vendorID != 0x10DE) continue; // NVIDIA target
                    status.nvidia_gpu = 1;
                    status.api_version = gpu.apiVersion;
                    std::snprintf(status.gpu_name, sizeof(status.gpu_name), "%s", gpu.deviceName);
                    uint32_t ext_count = 0;
                    if (extensions(devices[i], nullptr, &ext_count, nullptr) != VK_SUCCESS)
                        continue;
                    std::vector<VkExtensionProperties> exts(ext_count);
                    if (extensions(devices[i], nullptr, &ext_count, exts.data()) != VK_SUCCESS)
                        continue;
                    std::set<std::string> names;
                    for (const auto &ext : exts) names.insert(ext.extensionName);
                    bool accel_ext = names.count(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
                    bool query_ext = names.count(VK_KHR_RAY_QUERY_EXTENSION_NAME);
                    bool ray_ext = names.count(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
                    bool deferred_ext = names.count(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
                    if (!accel_ext || !deferred_ext || (!query_ext && !ray_ext)) {
                        report("RTX Vulkan extensions incomplete; OpenGL fallback.");
                        continue;
                    }
                    VkPhysicalDeviceAccelerationStructureFeaturesKHR accel{
                        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};
                    VkPhysicalDeviceRayQueryFeaturesKHR query{
                        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR};
                    VkPhysicalDeviceRayTracingPipelineFeaturesKHR ray{
                        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR};
                    VkPhysicalDeviceFeatures2 root{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
                    root.pNext = &accel;
                    accel.pNext = &query;
                    query.pNext = &ray;
                    features(devices[i], &root);
                    status.acceleration_structure = accel.accelerationStructure == VK_TRUE;
                    status.ray_query = query_ext && query.rayQuery == VK_TRUE;
                    status.ray_tracing_pipeline = ray_ext && ray.rayTracingPipeline == VK_TRUE;
                    status.hardware_ready = status.acceleration_structure &&
                        (status.ray_query || status.ray_tracing_pipeline);
                    if (status.hardware_ready) {
                        report("RTX Vulkan supported; camera/sun frame interface available, no BLAS/TLAS renderer.");
                        break;
                    }
                    report("RTX Vulkan features disabled; OpenGL fallback.");
                }
            }
        }
        if (!status.nvidia_gpu) report("No NVIDIA Vulkan GPU detected; OpenGL fallback.");
    }
    if (destroy) destroy(instance, nullptr);
    dlclose(library);
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
    detect();
    std::string gpu = status.gpu_name[0] ? status.gpu_name : "none";
    logger(1, (std::string("OGL RTX capability: gpu=") + gpu +
        " acceleration=" + std::to_string(status.acceleration_structure) +
        " query=" + std::to_string(status.ray_query) +
        " pipeline=" + std::to_string(status.ray_tracing_pipeline) +
        " hardware_ready=" + std::to_string(status.hardware_ready) +
        " rendering_active=0; " + status.message).c_str());
    return 0; // No RTX hardware is normal: keep the OpenGL renderer.
}
extern "C" OGL_MODULE_EXPORT void ogl_module_stop() { logger = nullptr; }
extern "C" OGL_MODULE_EXPORT const ogl_rtx_status *ogl_rtx_get_status() {
    return &status;
}
extern "C" OGL_MODULE_EXPORT int ogl_rtx_submit_frame(const ogl_rtx_frame_state *frame) {
    if (!frame || frame->struct_size < sizeof(ogl_rtx_frame_state) ||
        frame->frame_version != OGL_RTX_FRAME_VERSION || !status.hardware_ready ||
        frame->width == 0 || frame->height == 0) return 0;
    latest_frame = *frame;
    ++received_frames;
    // Shader dispatch remains disabled: no TLAS/BLAS or scene geometry yet.
    if (received_frames == 1 && logger)
        logger(1, "OGL RTX camera/sun frame bridge connected; no raytraced pixels yet.");
    return 1;
}
extern "C" OGL_MODULE_EXPORT uint64_t ogl_rtx_received_frames(void) {
    return received_frames;
}

// Device-level Vulkan RT readiness proof. No GL compositing or rays yet.
#include <vulkan/vulkan.h>
#include <cstdio>
#include <set>
#include <string>
#include <vector>

int main() {
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "OGL RTX device probe";
    app.apiVersion = VK_API_VERSION_1_2;
    VkInstanceCreateInfo ic{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    ic.pApplicationInfo = &app;
    VkInstance instance = VK_NULL_HANDLE;
    if (vkCreateInstance(&ic, nullptr, &instance) != VK_SUCCESS) return 1;

    uint32_t device_count = 0;
    int exit_code = 77; // Skip on machines without Vulkan RTX.
    if (vkEnumeratePhysicalDevices(instance, &device_count, nullptr) == VK_SUCCESS &&
        device_count) {
        std::vector<VkPhysicalDevice> devices(device_count);
        if (vkEnumeratePhysicalDevices(instance, &device_count, devices.data()) == VK_SUCCESS) {
            for (VkPhysicalDevice gpu : devices) {
                VkPhysicalDeviceProperties props{};
                vkGetPhysicalDeviceProperties(gpu, &props);
                if (props.vendorID != 0x10de) continue;

                uint32_t n = 0;
                if (vkEnumerateDeviceExtensionProperties(gpu, nullptr, &n, nullptr) != VK_SUCCESS)
                    continue;
                std::vector<VkExtensionProperties> ext(n);
                if (vkEnumerateDeviceExtensionProperties(gpu, nullptr, &n, ext.data()) != VK_SUCCESS)
                    continue;
                std::set<std::string> supported;
                for (const auto& x : ext) supported.insert(x.extensionName);
                const char* names[] = {
                    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
                    VK_KHR_RAY_QUERY_EXTENSION_NAME,
                    VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME
                };
                bool extensions_ready = true;
                for (const char* name : names) extensions_ready &= supported.count(name) != 0;
                if (!extensions_ready) continue;

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
                uint32_t family = ~0u;
                for (uint32_t i=0; i<family_count; ++i)
                    if (families[i].queueCount &&
                        (families[i].queueFlags & VK_QUEUE_COMPUTE_BIT)) {
                        family = i; break;
                    }
                if (family == ~0u) continue;

                VkPhysicalDeviceRayQueryFeaturesKHR enable_ray{
                    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR};
                enable_ray.rayQuery = VK_TRUE;
                VkPhysicalDeviceAccelerationStructureFeaturesKHR enable_accel{
                    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};
                enable_accel.accelerationStructure = VK_TRUE;
                enable_accel.pNext = &enable_ray;
                VkPhysicalDeviceBufferDeviceAddressFeatures enable_address{
                    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES};
                enable_address.bufferDeviceAddress = VK_TRUE;
                enable_address.pNext = &enable_accel;
                VkPhysicalDeviceFeatures2 requested{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
                requested.pNext = &enable_address;

                float priority = 1.f;
                VkDeviceQueueCreateInfo queue{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
                queue.queueFamilyIndex = family;
                queue.queueCount = 1;
                queue.pQueuePriorities = &priority;
                VkDeviceCreateInfo dc{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
                dc.pNext = &requested;
                dc.queueCreateInfoCount = 1;
                dc.pQueueCreateInfos = &queue;
                dc.enabledExtensionCount = 3;
                dc.ppEnabledExtensionNames = names;
                VkDevice logical_device = VK_NULL_HANDLE;
                VkResult result = vkCreateDevice(gpu, &dc, nullptr, &logical_device);
                if (result != VK_SUCCESS || !logical_device) {
                    std::printf("FAIL: vkCreateDevice returned %d\n", result);
                    exit_code = 1;
                    break;
                }
                vkDestroyDevice(logical_device, nullptr);
                std::printf("PASS: %s Vulkan RT logical device, acceleration structure + ray query + GPU device address\n",
                            props.deviceName);
                std::puts("Not yet raytraced: BLAS/TLAS, shader dispatch and OpenGL compositing are outstanding.");
                exit_code = 0;
                break;
            }
        }
    }
    vkDestroyInstance(instance, nullptr);
    if (exit_code == 77) std::puts("SKIP: NVIDIA ray query-capable Vulkan GPU unavailable");
    return exit_code;
}

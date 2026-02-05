#include "RenderingComponents.h"
#include "../GAME/GlobalRegistry.h"
#include <set>
#include <vector>
#include <string>
#include <cstring>
#include <stdio.h>

using namespace RENDERING;
using namespace RENDERING::RENDERER_HELPERS::Device;

namespace RENDERING::RENDERER_HELPERS::Device
{
    bool CreateInstanceFor(RendererComponent& rendererComponent)
    {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "StarThread Vulkan";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "StarThreadEngine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
#ifdef NDEBUG
        const bool enableValidationLayers = false;
#else
        const bool enableValidationLayers = true;
#endif
        if (enableValidationLayers)
        {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        static const std::vector<const char*> validationLayers =
        {
            "VK_LAYER_KHRONOS_validation"
        };

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        if (enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else
        {
            createInfo.enabledLayerCount = 0;
        }

        if (vkCreateInstance(&createInfo, nullptr, &rendererComponent.instance) != VK_SUCCESS)
        {
            printf("Failed to create Vulkan instance\n");
            return false;
        }

        return true;
    }

    bool CreateSurfaceFor(RendererComponent& rendererComponent)
    {
        if (glfwCreateWindowSurface(rendererComponent.instance, rendererComponent.window, nullptr, &rendererComponent.surface) != VK_SUCCESS)
        {
            printf("Failed to create window surface\n");
            return false;
        }
        return true;
    }

    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
    {
        QueueFamilyIndices indices;
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int index = 0;
        for (const auto& queueFamily : queueFamilies)
        {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                indices.graphicsFamily = index;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, index, surface, &presentSupport);
            if (presentSupport)
            {
                indices.presentFamily = index;
            }

            if (indices.IsComplete()) break;
            ++index;
        }
        return indices;
    }

    bool PickPhysicalDeviceFor(RendererComponent& rendererComponent)
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(rendererComponent.instance, &deviceCount, nullptr);
        if (deviceCount == 0)
        {
            printf("Failed to find GPUs with Vulkan support\n");
            return false;
        }
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(rendererComponent.instance, &deviceCount, devices.data());

        const std::vector<const char*> deviceExtensions =
        {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        for (const auto& device : devices)
        {
            QueueFamilyIndices indices = FindQueueFamilies(device, rendererComponent.surface);

            // Check for extensions
            uint32_t extensionsCount = 0;
            vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionsCount, nullptr);
            std::vector<VkExtensionProperties> availableExtension(extensionsCount);
            vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionsCount, availableExtension.data());

            std::set<std::string> required(deviceExtensions.begin(), deviceExtensions.end());
            for (const auto& extension : availableExtension)
            {
                required.erase(extension.extensionName);
            }

            if (indices.IsComplete() && required.empty())
            {
                rendererComponent.physicalDevice = device;
                break;
            }
        }

        if (rendererComponent.physicalDevice == VK_NULL_HANDLE)
        {
            printf("Failed to find a suitable GPU\n");
            return false;
        }

        return true;
    }

    bool CreateLogicalDeviceFor(RendererComponent& rendererComponent)
    {
        QueueFamilyIndices indices = FindQueueFamilies(rendererComponent.physicalDevice, rendererComponent.surface);

        std::vector<uint32_t> uniqueQueueFamilies;
        if (indices.graphicsFamily.has_value())
            uniqueQueueFamilies.push_back(indices.graphicsFamily.value());
        if (indices.presentFamily.has_value() && indices.presentFamily != indices.graphicsFamily)
            uniqueQueueFamilies.push_back(indices.presentFamily.value());

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{};
        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.pEnabledFeatures = &deviceFeatures;

        const std::vector<const char*> deviceExtensions =
        {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

#ifdef NDEBUG
        const bool enableValidationLayers = false;
#else
        const bool enableValidationLayers = true;
#endif
        static const std::vector<const char*> validationLayers =
        {
            "VK_LAYER_KHRONOS_validation"
        };

        if (enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else
        {
            createInfo.enabledLayerCount = 0;
        }

        if (vkCreateDevice(rendererComponent.physicalDevice, &createInfo, nullptr, &rendererComponent.device) != VK_SUCCESS)
        {
            printf("Failed to create logical device\n");
            return false;
        }

        vkGetDeviceQueue(rendererComponent.device, indices.graphicsFamily.value(), 0, &rendererComponent.graphicsQueue);
        vkGetDeviceQueue(rendererComponent.device, indices.presentFamily.value(), 0, &rendererComponent.presentQueue);

        return true;
    }
}
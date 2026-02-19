#include "RenderingComponents.h"
#include <vector>
#include <algorithm>
#include <array>

namespace RENDERING::RENDERER_HELPERS::Swapchain
{
    bool CreateSwapchainFor(RendererComponent& rendererComponent)
    {
        VkSurfaceCapabilitiesKHR capabilities;
        VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(rendererComponent.physicalDevice, rendererComponent.surface, &capabilities);
        if (result != VK_SUCCESS)
        {
            printf("vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed: %d\n", result);
            return false;
        }

        uint32_t formatCount = 0;
        result = vkGetPhysicalDeviceSurfaceFormatsKHR(rendererComponent.physicalDevice, rendererComponent.surface, &formatCount, nullptr);
        if (result != VK_SUCCESS || formatCount == 0)
        {
            printf("vkGetPhysicalDeviceSurfaceFormatsKHR failed or returned 0 formats: %d\n", result);
            return false;
        }
        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        result = vkGetPhysicalDeviceSurfaceFormatsKHR(rendererComponent.physicalDevice, rendererComponent.surface, &formatCount, formats.data());
        if (result != VK_SUCCESS)
        {
            printf("vkGetPhysicalDeviceSurfaceFormatsKHR (second) failed: %d\n", result);
            return false;
        }

        uint32_t presentModeCount = 0;
        result = vkGetPhysicalDeviceSurfacePresentModesKHR(rendererComponent.physicalDevice, rendererComponent.surface, &presentModeCount, nullptr);
        if (result != VK_SUCCESS || presentModeCount == 0)
        {
            printf("vkGetPhysicalDeviceSurfacePresentModesKHR failed or returned 0 modes: %d\n", result);
            return false;
        }
        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        result = vkGetPhysicalDeviceSurfacePresentModesKHR(rendererComponent.physicalDevice, rendererComponent.surface, &presentModeCount, presentModes.data());
        if (result != VK_SUCCESS)
        {
            printf("vkGetPhysicalDeviceSurfacePresentModesKHR (second) failed: %d\n", result);
            return false;
        }

        VkSurfaceFormatKHR surfaceFormat = formats[0];
        for (const VkSurfaceFormatKHR& availableFormat : formats)
        {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                surfaceFormat = availableFormat;
                break;
            }
        }

        VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
        for (const VkPresentModeKHR& availablePresentMode : presentModes)
        {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                presentMode = availablePresentMode;
                break;
            }
        }

        VkExtent2D extent;
        if (capabilities.currentExtent.width != UINT32_MAX)
        {
            extent = capabilities.currentExtent;
        }
        else
        {
            int width, height;
            glfwGetFramebufferSize(rendererComponent.window, &width, &height);
            extent.width = static_cast<uint32_t>(width);
            extent.height = static_cast<uint32_t>(height);
            extent.width = (std::max)(capabilities.minImageExtent.width, (std::min)(capabilities.maxImageExtent.width, extent.width));
            extent.height = (std::max)(capabilities.minImageExtent.height, (std::min)(capabilities.maxImageExtent.height, extent.height));
        }

        uint32_t imageCount = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
        {
            imageCount = capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = rendererComponent.surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        
        Device::QueueFamilyIndices indices = FindQueueFamilies(rendererComponent.physicalDevice, rendererComponent.surface);
        uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

        if (indices.graphicsFamily != indices.presentFamily)
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else 
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }

        createInfo.preTransform = capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(rendererComponent.device, &createInfo, nullptr, &rendererComponent.swapchain) != VK_SUCCESS)
        {
            printf("Failed to create swap chain\n");
            return false;
        }

        result = vkGetSwapchainImagesKHR(rendererComponent.device, rendererComponent.swapchain, &imageCount, nullptr);
        if (result != VK_SUCCESS || imageCount == 0)
        {
            printf("vkGetSwapchainImagesKHR failed or returned 0 images: %d\n", result);
            return false;
        }
        rendererComponent.swapchainImages.resize(imageCount);
        result = vkGetSwapchainImagesKHR(rendererComponent.device, rendererComponent.swapchain, &imageCount, rendererComponent.swapchainImages.data());
        if (result != VK_SUCCESS)
        {
            printf("vkGetSwapchainImagesKHR failed: %d\n", result);
            return false;
        }

        rendererComponent.swapchainImageFormat = surfaceFormat.format;
        rendererComponent.swapchainExtent = extent;

        rendererComponent.imagesInFlight.resize(rendererComponent.swapchainImages.size(), VK_NULL_HANDLE);
        return true;
    }

    bool CreateImageViewsFor(RendererComponent& rendererComponent)
    {
        rendererComponent.swapchainImageViews.resize(rendererComponent.swapchainImages.size());

        for (size_t image = 0; image < rendererComponent.swapchainImages.size(); ++image)
        {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = rendererComponent.swapchainImages[image];
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = rendererComponent.swapchainImageFormat;
            viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(rendererComponent.device, &viewInfo, nullptr, &rendererComponent.swapchainImageViews[image]) != VK_SUCCESS)
            {
                printf("Failed to create image views\n");
                return false;
            }
        }

        return true;
    }

    bool CreateFramebuffersFor(RendererComponent& rendererComponent)
    {
        rendererComponent.swapchainFramebuffers.resize(rendererComponent.swapchainImageViews.size());
        for (size_t image = 0; image < rendererComponent.swapchainImageViews.size(); ++image)
        {
            // Attach color + depth
            std::array<VkImageView, 2> attachments = { rendererComponent.swapchainImageViews[image], rendererComponent.depthImageView };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = rendererComponent.renderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = rendererComponent.swapchainExtent.width;
            framebufferInfo.height = rendererComponent.swapchainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(rendererComponent.device, &framebufferInfo, nullptr, &rendererComponent.swapchainFramebuffers[image]) != VK_SUCCESS)
            {
                printf("Failed to create framebuffer\n");
                return false;
            }
        }
        return true;
    }

    void RecreateSwapchainFor(RendererComponent& rendererComponent)
    {
        int width = 0, height = 0;
        glfwGetFramebufferSize(rendererComponent.window, &width, &height);
        while (width == 0 || height == 0)
        {
            glfwGetFramebufferSize(rendererComponent.window, &width, &height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(rendererComponent.device);

        // Destroy old resources
        for (VkFramebuffer framebuffer : rendererComponent.swapchainFramebuffers) vkDestroyFramebuffer(rendererComponent.device, framebuffer, nullptr);
        rendererComponent.swapchainFramebuffers.clear();

        if (!rendererComponent.commandBuffers.empty() && rendererComponent.commandPool != VK_NULL_HANDLE)
        {
            vkFreeCommandBuffers(rendererComponent.device, rendererComponent.commandPool,
                                 static_cast<uint32_t>(rendererComponent.commandBuffers.size()), rendererComponent.commandBuffers.data());
            rendererComponent.commandBuffers.clear();
        }

        if (rendererComponent.graphicsPipeline) { vkDestroyPipeline(rendererComponent.device, rendererComponent.graphicsPipeline, nullptr); rendererComponent.graphicsPipeline = VK_NULL_HANDLE; }
        if (rendererComponent.pipelineLayout) { vkDestroyPipelineLayout(rendererComponent.device, rendererComponent.pipelineLayout, nullptr); rendererComponent.pipelineLayout = VK_NULL_HANDLE; }
        if (rendererComponent.renderPass) { vkDestroyRenderPass(rendererComponent.device, rendererComponent.renderPass, nullptr); rendererComponent.renderPass = VK_NULL_HANDLE; }

        for (VkImageView view : rendererComponent.swapchainImageViews) vkDestroyImageView(rendererComponent.device, view, nullptr);
        rendererComponent.swapchainImageViews.clear();

        if (rendererComponent.swapchain) { vkDestroySwapchainKHR(rendererComponent.device, rendererComponent.swapchain, nullptr); rendererComponent.swapchain = VK_NULL_HANDLE; }
        rendererComponent.swapchainImages.clear();

        // Also destroy depth resources if present
        if (rendererComponent.depthImageView) { vkDestroyImageView(rendererComponent.device, rendererComponent.depthImageView, nullptr); rendererComponent.depthImageView = VK_NULL_HANDLE; }
        if (rendererComponent.depthImage) { vkDestroyImage(rendererComponent.device, rendererComponent.depthImage, nullptr); rendererComponent.depthImage = VK_NULL_HANDLE; }
        if (rendererComponent.depthImageMemory) { vkFreeMemory(rendererComponent.device, rendererComponent.depthImageMemory, nullptr); rendererComponent.depthImageMemory = VK_NULL_HANDLE; }

        // Recreate swapchain and image views
        if (!CreateSwapchainFor(rendererComponent) || !CreateImageViewsFor(rendererComponent))
        {
            printf("Failed to recreate basic swapchain resources\n");
            return;
        }

        // Recreate depth, render pass, pipeline, framebuffers, command pool, command buffers and sync objects
        if (!RENDERING::RENDERER_HELPERS::Depth::CreateDepthResourcesFor(rendererComponent))
        {
            printf("Failed to recreate depth resources\n");
            return;
        }

        if (!RENDERING::RENDERER_HELPERS::Pipeline::CreateRenderPassFor(rendererComponent))
        {
            printf("Failed to recreate render pass\n");
            return;
        }

        if (!RENDERING::RENDERER_HELPERS::Pipeline::CreateGraphicsPipelineFor(rendererComponent))
        {
            printf("Failed to recreate graphics pipeline\n");
            return;
        }

        if (!CreateFramebuffersFor(rendererComponent))
        {
            printf("Failed to recreate framebuffers\n");
            return;
        }

        if (!RENDERING::RENDERER_HELPERS::Pipeline::CreateCommandPoolFor(rendererComponent))
        {
            printf("Failed to recreate command pool\n");
            return;
        }

        if (!RENDERING::RENDERER_HELPERS::Pipeline::CreateCommandBuffersFor(rendererComponent))
        {
            printf("Failed to recreate command buffers\n");
            return;
        }

        if (!RENDERING::RENDERER_HELPERS::Pipeline::CreateSyncObjectsFor(rendererComponent))
        {
            printf("Failed to recreate sync objects\n");
            return;
        }

        // Make imagesInFlight size match new swapchain image count
        rendererComponent.imagesInFlight.assign(rendererComponent.swapchainImages.size(), VK_NULL_HANDLE);

        rendererComponent.framebufferResized = false;
    }
} // namespace RENDERER_HELPERS::Swapchain
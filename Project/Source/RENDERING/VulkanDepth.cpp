#include "RenderingComponents.h"
#include <vector>
#include <stdexcept>

using namespace RENDERING;
using namespace RENDERING::RENDERER_HELPERS::Image;
using namespace RENDERING::RENDERER_HELPERS::Memory;

namespace RENDERING::RENDERER_HELPERS::Depth
{
    VkFormat FindSupportedFormat(RendererComponent& rendererComponent, 
        const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
    {
        for (VkFormat format : candidates)
        {
            VkFormatProperties properties;
            vkGetPhysicalDeviceFormatProperties(rendererComponent.physicalDevice, format, &properties);

            if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features)
            {
                return format;
            }
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features)
            {
                return format;
            }
        }

        throw std::runtime_error("Failed to find supported format");
    }

    bool HasStencilComponent(VkFormat format)
    {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

    VkFormat FindDepthFormat(RendererComponent& rendererComponent)
    {
        return FindSupportedFormat(rendererComponent,
            { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }

    bool CreateDepthResourcesFor(RendererComponent& rendererComponent)
    {
        VkFormat depthFormat = FindDepthFormat(rendererComponent);

        // Create depth image
        CreateImageFor(rendererComponent,
            rendererComponent.swapchainExtent.width,
            rendererComponent.swapchainExtent.height,
            depthFormat,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            rendererComponent.depthImage,
            rendererComponent.depthImageMemory);

        // Create view
        VkImageAspectFlags aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (HasStencilComponent(depthFormat)) aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;

        rendererComponent.depthImageView = CreateImageViewForTextureFor(rendererComponent, rendererComponent.depthImage, depthFormat, aspect);

        // Transition to DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        VkCommandBuffer commandBuffer = BeginSingleTimeCommandsFor(rendererComponent);

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = rendererComponent.depthImage;
        barrier.subresourceRange.aspectMask = aspect;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        EndSingleTimeCommandsFor(rendererComponent, commandBuffer);
        return true;
    }
}
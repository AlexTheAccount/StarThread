#include "RenderingComponents.h"
#include <fstream>
#include <stdexcept>
#include <cstring>

namespace RENDERING::RENDERER_HELPERS
{
    namespace Utilities
    {
        std::vector<char> ReadFile(const std::string& filename)
        {
            std::ifstream file(filename, std::ios::ate | std::ios::binary);
            if (!file.is_open()) return {};

            size_t fileSize = (size_t)file.tellg();
            std::vector<char> buffer(fileSize);
            file.seekg(0);
            file.read(buffer.data(), fileSize);
            file.close();
            return buffer;
        }
    } // namespace Utilities

    namespace Pipeline
    {
        VkShaderModule CreateShaderModuleFor(RendererComponent& rendererComponent, const std::vector<char>& code)
        {
            VkShaderModuleCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createInfo.codeSize = code.size();
            createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

            VkShaderModule shaderModule;
            if (vkCreateShaderModule(rendererComponent.device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create shader module");
            }

            return shaderModule;
        }
    } // namespace Pipeline

    namespace Memory
    {
        uint32_t FindMemoryTypeFor(RendererComponent& rendererComponent, uint32_t typeFilter, VkMemoryPropertyFlags properties)
        {
            VkPhysicalDeviceMemoryProperties memoryProperties;
            vkGetPhysicalDeviceMemoryProperties(rendererComponent.physicalDevice, &memoryProperties);

            for (uint32_t memoryType = 0; memoryType < memoryProperties.memoryTypeCount; ++memoryType) 
            {
                if ((typeFilter & (1u << memoryType)) && (memoryProperties.memoryTypes[memoryType].propertyFlags & properties) == properties) 
                {
                    return memoryType;
                }
            }

            throw std::runtime_error("Failed to find suitable memory type");
        }

        void CreateBufferFor(RendererComponent& rendererComponent, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
        {
            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = size;
            bufferInfo.usage = usage;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            if (vkCreateBuffer(rendererComponent.device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) 
            {
                throw std::runtime_error("Failed to create buffer");
            }

            VkMemoryRequirements memoryRequirements;
            vkGetBufferMemoryRequirements(rendererComponent.device, buffer, &memoryRequirements);

            VkMemoryAllocateInfo allocateInfo{};
            allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocateInfo.allocationSize = memoryRequirements.size;
            allocateInfo.memoryTypeIndex = FindMemoryTypeFor(rendererComponent, memoryRequirements.memoryTypeBits, properties);

            if (vkAllocateMemory(rendererComponent.device, &allocateInfo, nullptr, &bufferMemory) != VK_SUCCESS) 
            {
                throw std::runtime_error("Failed to allocate buffer memory");
            }

            vkBindBufferMemory(rendererComponent.device, buffer, bufferMemory, 0);
        }

        VkCommandBuffer BeginSingleTimeCommandsFor(RendererComponent& rendererComponent)
        {
            VkCommandBufferAllocateInfo allocateInfo{};
            allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocateInfo.commandPool = rendererComponent.commandPool;
            allocateInfo.commandBufferCount = 1;

            VkCommandBuffer commandBuffer;
            vkAllocateCommandBuffers(rendererComponent.device, &allocateInfo, &commandBuffer);

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            vkBeginCommandBuffer(commandBuffer, &beginInfo);
            return commandBuffer;
        }

        void EndSingleTimeCommandsFor(RendererComponent& rendererComponent, VkCommandBuffer commandBuffer)
        {
            vkEndCommandBuffer(commandBuffer);

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffer;

            vkQueueSubmit(rendererComponent.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
            vkQueueWaitIdle(rendererComponent.graphicsQueue);

            vkFreeCommandBuffers(rendererComponent.device, rendererComponent.commandPool, 1, &commandBuffer);
        }

        void CreateVertexAndIndexBuffersFor(RendererComponent& rendererComponent,
            std::vector<FBXVertex> vertices, std::vector<uint32_t> indices,
            VkBuffer& outVertexBuffer, VkDeviceMemory& outVertexMemory,
            VkBuffer& outIndexBuffer, VkDeviceMemory& outIndexMemory, size_t& outIndexCount)
        {
            // null checks
            {
                if (rendererComponent.device == VK_NULL_HANDLE)
                {
                    throw std::runtime_error("CreateVertexAndIndexBuffersFor: rendererComponent.device is null");
                }
                else if (outVertexBuffer != VK_NULL_HANDLE || outVertexMemory != VK_NULL_HANDLE || 
                         outIndexBuffer != VK_NULL_HANDLE || outIndexMemory != VK_NULL_HANDLE)
                {
                    throw std::runtime_error("CreateVertexAndIndexBuffersFor: output buffer or memory parameters must be null handles");
                }
            }

            // Calculate byte sizes
            VkDeviceSize vertexBufferSize = vertices.empty() ? 0 : sizeof(FBXVertex) * vertices.size();
            VkDeviceSize indexBufferSize  = indices.empty()  ? 0 : sizeof(uint32_t) * indices.size();

            if (vertexBufferSize == 0 || indexBufferSize == 0)
            {
                throw std::runtime_error("CreateVertexAndIndexBuffersFor: vertices or indices empty");
            }

            // Create staging vertex buffer
            VkBuffer stagingVertexBuffer = VK_NULL_HANDLE;
            VkDeviceMemory stagingVertexMemory = VK_NULL_HANDLE;
            CreateBufferFor(rendererComponent, vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                            stagingVertexBuffer, stagingVertexMemory);

            // Create staging index buffer
            VkBuffer stagingIndexBuffer = VK_NULL_HANDLE;
            VkDeviceMemory stagingIndexMemory = VK_NULL_HANDLE;
            CreateBufferFor(rendererComponent, indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                            stagingIndexBuffer, stagingIndexMemory);

            // Map and copy vertex data
            void* data = nullptr;
            vkMapMemory(rendererComponent.device, stagingVertexMemory, 0, vertexBufferSize, 0, &data);
            std::memcpy(data, vertices.data(), static_cast<size_t>(vertexBufferSize));
            vkUnmapMemory(rendererComponent.device, stagingVertexMemory);

            // Map and copy index data
            data = nullptr;
            vkMapMemory(rendererComponent.device, stagingIndexMemory, 0, indexBufferSize, 0, &data);
            std::memcpy(data, indices.data(), static_cast<size_t>(indexBufferSize));
            vkUnmapMemory(rendererComponent.device, stagingIndexMemory);

            // Create device-local destination buffers
            CreateBufferFor(rendererComponent, vertexBufferSize,
                            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, outVertexBuffer, outVertexMemory);

            CreateBufferFor(rendererComponent, indexBufferSize,
                            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, outIndexBuffer, outIndexMemory);

            // Copy staging to device local
            VkCommandBuffer commandBuffer = BeginSingleTimeCommandsFor(rendererComponent);

            VkBufferCopy copyRegion{};
            copyRegion.srcOffset = 0;
            copyRegion.dstOffset = 0;
            copyRegion.size = vertexBufferSize;
            vkCmdCopyBuffer(commandBuffer, stagingVertexBuffer, outVertexBuffer, 1, &copyRegion);

            copyRegion.size = indexBufferSize;
            vkCmdCopyBuffer(commandBuffer, stagingIndexBuffer, outIndexBuffer, 1, &copyRegion);

            EndSingleTimeCommandsFor(rendererComponent, commandBuffer);

            // Cleanup staging buffers
            vkDestroyBuffer(rendererComponent.device, stagingVertexBuffer, nullptr);
            vkFreeMemory(rendererComponent.device, stagingVertexMemory, nullptr);
            vkDestroyBuffer(rendererComponent.device, stagingIndexBuffer, nullptr);
            vkFreeMemory(rendererComponent.device, stagingIndexMemory, nullptr);

            // Set returned index count
            outIndexCount = indices.size();
        }
    } // namespace Memory

    namespace Image
    {
        void CreateImageFor(RendererComponent& rendererComponent, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
            VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
        {
            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent.width = width;
            imageInfo.extent.height = height;
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.format = format;
            imageInfo.tiling = tiling;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = usage;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            if (vkCreateImage(rendererComponent.device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create image");
            }

            VkMemoryRequirements memoryRequirements;
            vkGetImageMemoryRequirements(rendererComponent.device, image, &memoryRequirements);

            VkMemoryAllocateInfo allocateInfo{};
            allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocateInfo.allocationSize = memoryRequirements.size;
            allocateInfo.memoryTypeIndex = FindMemoryTypeFor(rendererComponent, memoryRequirements.memoryTypeBits, properties);

            if (vkAllocateMemory(rendererComponent.device, &allocateInfo, nullptr, &imageMemory) != VK_SUCCESS) {
                throw std::runtime_error("Failed to allocate image memory");
            }

            vkBindImageMemory(rendererComponent.device, image, imageMemory, 0);
        }

        VkImageView CreateImageViewForTextureFor(RendererComponent& rendererComponent, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
        {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = image;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = format;
            viewInfo.subresourceRange.aspectMask = aspectFlags;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            VkImageView imageView;
            if (vkCreateImageView(rendererComponent.device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create texture image view");
            }
            return imageView;
        }

        void TransitionImageLayoutFor(RendererComponent& rendererComponent, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout)
        {
            VkCommandBuffer commandBuffer = BeginSingleTimeCommandsFor(rendererComponent);

            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = oldLayout;
            barrier.newLayout = newLayout;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = image;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

            VkPipelineStageFlags sourceStage;
            VkPipelineStageFlags destinationStage;

            if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            }
            else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            }
            else {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = 0;
                sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            }

            vkCmdPipelineBarrier(
                commandBuffer,
                sourceStage,
                destinationStage,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier
            );

            EndSingleTimeCommandsFor(rendererComponent, commandBuffer);
        }

        void CopyBufferToImageFor(RendererComponent& rendererComponent, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
        {
            VkCommandBuffer commandBuffer = BeginSingleTimeCommandsFor(rendererComponent);

            VkBufferImageCopy region{};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;
            region.imageOffset = { 0, 0, 0 };
            region.imageExtent = { width, height, 1 };

            vkCmdCopyBufferToImage(
                commandBuffer,
                buffer,
                image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &region
            );

            EndSingleTimeCommandsFor(rendererComponent, commandBuffer);
        }

        bool CreateTextureFromPixelsFor(RendererComponent& rendererComponent, const void* pixels, uint32_t textureWidth, uint32_t textureHeight, VkFormat format)
        {
            VkDeviceSize imageSize = textureWidth * textureHeight * 4;
            VkBuffer stagingBuffer;
            VkDeviceMemory stagingBufferMemory;
            CreateBufferFor(rendererComponent, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

            void* data;
            vkMapMemory(rendererComponent.device, stagingBufferMemory, 0, imageSize, 0, &data);
            std::memcpy(data, pixels, static_cast<size_t>(imageSize));
            vkUnmapMemory(rendererComponent.device, stagingBufferMemory);

            CreateImageFor(rendererComponent, textureWidth, textureHeight, format, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, rendererComponent.textureImage, rendererComponent.textureImageMemory);

            TransitionImageLayoutFor(rendererComponent, rendererComponent.textureImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            CopyBufferToImageFor(rendererComponent, stagingBuffer, rendererComponent.textureImage, textureWidth, textureHeight);
            TransitionImageLayoutFor(rendererComponent, rendererComponent.textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            vkDestroyBuffer(rendererComponent.device, stagingBuffer, nullptr);
            vkFreeMemory(rendererComponent.device, stagingBufferMemory, nullptr);

            rendererComponent.textureImageView = CreateImageViewForTextureFor(rendererComponent, rendererComponent.textureImage, format, VK_IMAGE_ASPECT_COLOR_BIT);
            return true;
        }
    } // namespace Image
} // namespace RENDERER_HELPERS
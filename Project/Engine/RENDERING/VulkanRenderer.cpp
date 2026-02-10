#include "RenderingComponents.h"
#include <stdexcept>
#include <fstream>
#include <cstring>

using namespace RENDERING;
using namespace RENDERING::RENDERER_HELPERS::Depth;
using namespace RENDERING::RENDERER_HELPERS::Memory;
using namespace RENDERING::RENDERER_HELPERS::Pipeline;
using namespace RENDERING::RENDERER_HELPERS::Swapchain;
using namespace RENDERING::RENDERER_HELPERS::Device;

namespace RENDERING::RENDERER_HELPERS
{
#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

    static RendererComponent* boundRenderer = nullptr;

    void BindRenderer(RendererComponent* renderer) { boundRenderer = renderer; }
    void UnbindRenderer() { boundRenderer = nullptr; }
    RendererComponent* GetBoundRenderer() { return boundRenderer; }
    RendererComponent* GetGlobalRenderer() { return GetBoundRenderer(); }

    void FramebufferResize(GLFWwindow* window, int /*width*/, int /*height*/)
    {
        void* user = glfwGetWindowUserPointer(window);
        if (!user) return;
        RENDERING::RendererComponent* renderer = reinterpret_cast<RENDERING::RendererComponent*>(user);
        if (renderer) 
        {
            renderer->framebufferResized = true;
        }
    }

    // Initialize window for an externally owned RendererComponent
    bool InitializeWindowFor(RendererComponent& rendererComp, uint32_t width, uint32_t height, const char* title)
    {
        if (!glfwInit()) return false;
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        GLFWwindow* window = glfwCreateWindow((int)width, (int)height, title, nullptr, nullptr);
        if (!window) return false;

        rendererComp.window = window;
        glfwSetWindowUserPointer(window, &rendererComp);
        glfwSetFramebufferSizeCallback(window, FramebufferResize);

        return true;
    }

    // uses bound renderer
    bool InitializeWindow(uint32_t width, uint32_t height, const char* title)
    {
        RendererComponent* renderer = GetBoundRenderer();
        if (!renderer)
        {
            printf("InitializeWindow(): no bound RendererComponent. Use InitializeWindowFor(renderer, ...) instead.\n");
            return false;
        }
        return InitializeWindowFor(*renderer, width, height, title);
    }

    // Per-instance draw
    void DrawFrameFor(RendererComponent& rendererComponent)
    {
        // Validate necessary sync structures exist
        if (rendererComponent.imageAvailableSemaphores.empty() ||
            rendererComponent.inFlightFences.empty() ||
            rendererComponent.renderFinishedSemaphores.empty() ||
            rendererComponent.commandBuffers.empty())
        {
            printf("Missing synchronization objects or command buffers\n");
            return;
        }

        if (rendererComponent.currentFrame >= rendererComponent.imageAvailableSemaphores.size() ||
            rendererComponent.currentFrame >= rendererComponent.inFlightFences.size())
        {
            printf("Out-of-range currentFrame=%zu avail=%zu fences=%zu\n",
                rendererComponent.currentFrame, rendererComponent.imageAvailableSemaphores.size(),
                rendererComponent.inFlightFences.size());
            return;
        }

        if (rendererComponent.imageAvailableSemaphores[rendererComponent.currentFrame] == VK_NULL_HANDLE ||
            rendererComponent.inFlightFences[rendererComponent.currentFrame] == VK_NULL_HANDLE)
        {
            printf("Null per-frame sync handle at frame %zu\n", rendererComponent.currentFrame);
            return;
        }

        vkWaitForFences(rendererComponent.device, 1, &rendererComponent.inFlightFences[rendererComponent.currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(rendererComponent.device, rendererComponent.swapchain, UINT64_MAX,
            rendererComponent.imageAvailableSemaphores[rendererComponent.currentFrame], VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            RecreateSwapchainFor(rendererComponent);
            return;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        {
            printf("Failed to acquire swap chain image\n");
            return;
        }

        if (rendererComponent.imagesInFlight.size() > imageIndex && rendererComponent.imagesInFlight[imageIndex] != VK_NULL_HANDLE)
        {
            vkWaitForFences(rendererComponent.device, 1, &rendererComponent.imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
        }

        if (rendererComponent.imagesInFlight.size() > imageIndex)
        {
            rendererComponent.imagesInFlight[imageIndex] = rendererComponent.inFlightFences[rendererComponent.currentFrame];
        }

        vkResetFences(rendererComponent.device, 1, &rendererComponent.inFlightFences[rendererComponent.currentFrame]);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = { rendererComponent.imageAvailableSemaphores[rendererComponent.currentFrame] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &rendererComponent.commandBuffers[imageIndex];

        // Use the render-finished semaphore associated with the acquired image index
        if (imageIndex >= rendererComponent.renderFinishedSemaphores.size())
        {
            printf("Invalid imageIndex %u for renderFinishedSemaphores (size=%zu)\n", imageIndex, rendererComponent.renderFinishedSemaphores.size());
            return;
        }
        VkSemaphore signalSemaphores[] = { rendererComponent.renderFinishedSemaphores[imageIndex] };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(rendererComponent.graphicsQueue, 1, &submitInfo, rendererComponent.inFlightFences[rendererComponent.currentFrame]) != VK_SUCCESS)
        {
            printf("Failed to submit draw command buffer\n");
            return;
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = { rendererComponent.swapchain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;

        result = vkQueuePresentKHR(rendererComponent.presentQueue, &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        {
            RecreateSwapchainFor(rendererComponent);
        }
        else if (result != VK_SUCCESS) {
            printf("Failed to present swap chain image\n");
        }

        rendererComponent.currentFrame = (rendererComponent.currentFrame + 1) % rendererComponent.MAX_FRAMES_IN_FLIGHT;
    }

    void DrawFrame()
    {
        RendererComponent* rendererComponent = GetBoundRenderer();
        if (!rendererComponent) return;
        DrawFrameFor(*rendererComponent);
    }

    void RenderFor(RendererComponent& rendererComponent)
    {
        DrawFrameFor(rendererComponent);
    }

    void Render()
    {
        RendererComponent* rendererComponent = GetBoundRenderer();
        if (!rendererComponent) return;
        DrawFrameFor(*rendererComponent);
    }

    bool InitializeFor(RendererComponent& rendererComponent, uint32_t width, uint32_t height, const char* title)
    {
        BindRenderer(&rendererComponent);

        if (!InitializeWindowFor(rendererComponent, width, height, title))
        {
            return false;
        }

        if (!InitializeVulkanFor(rendererComponent))
        {
            return false;
        }

        return true;
    }

    bool Initialize(uint32_t width, uint32_t height, const char* title)
    {
        RendererComponent* rendererComponent = GetBoundRenderer();
        if (!rendererComponent)
        {
            printf("Initialize(): no bound RendererComponent. Use InitializeFor(renderer, ...) instead.\n");
            return false;
        }
        return InitializeFor(*rendererComponent, width, height, title);
    }

    // Per-instance cleanup
    void CleanupFor(RendererComponent& rendererComponent)
    {
        // Destroy all image-available semaphores
        for (VkSemaphore semaphore : rendererComponent.imageAvailableSemaphores)
        {
            if (semaphore != VK_NULL_HANDLE) vkDestroySemaphore(rendererComponent.device, semaphore, nullptr);
        }
        rendererComponent.imageAvailableSemaphores.clear();

        // Destroy all render-finished semaphores
        for (VkSemaphore semaphore : rendererComponent.renderFinishedSemaphores)
        {
            if (semaphore != VK_NULL_HANDLE) vkDestroySemaphore(rendererComponent.device, semaphore, nullptr);
        }
        rendererComponent.renderFinishedSemaphores.clear();

        // Destroy all fences
        for (VkFence fence : rendererComponent.inFlightFences)
        {
            if (fence != VK_NULL_HANDLE) vkDestroyFence(rendererComponent.device, fence, nullptr);
        }
        rendererComponent.inFlightFences.clear();

        if (rendererComponent.commandPool) vkDestroyCommandPool(rendererComponent.device, rendererComponent.commandPool, nullptr);

        for (VkFramebuffer frameBuffer : rendererComponent.swapchainFramebuffers) vkDestroyFramebuffer(rendererComponent.device, frameBuffer, nullptr);
        if (rendererComponent.graphicsPipeline) vkDestroyPipeline(rendererComponent.device, rendererComponent.graphicsPipeline, nullptr);
        if (rendererComponent.pipelineLayout) vkDestroyPipelineLayout(rendererComponent.device, rendererComponent.pipelineLayout, nullptr);
        if (rendererComponent.renderPass) vkDestroyRenderPass(rendererComponent.device, rendererComponent.renderPass, nullptr);

        for (VkImageView view : rendererComponent.swapchainImageViews) vkDestroyImageView(rendererComponent.device, view, nullptr);
        if (rendererComponent.swapchain) vkDestroySwapchainKHR(rendererComponent.device, rendererComponent.swapchain, nullptr);

        if (rendererComponent.descriptorPool) { vkDestroyDescriptorPool(rendererComponent.device, rendererComponent.descriptorPool, nullptr); rendererComponent.descriptorPool = VK_NULL_HANDLE; }

        if (rendererComponent.vertexBuffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(rendererComponent.device, rendererComponent.vertexBuffer, nullptr);
            rendererComponent.vertexBuffer = VK_NULL_HANDLE;
        }
        if (rendererComponent.vertexBufferMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(rendererComponent.device, rendererComponent.vertexBufferMemory, nullptr);
            rendererComponent.vertexBufferMemory = VK_NULL_HANDLE;
        }

        if (rendererComponent.indexBuffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(rendererComponent.device, rendererComponent.indexBuffer, nullptr);
            rendererComponent.indexBuffer = VK_NULL_HANDLE;
        }
        if (rendererComponent.indexBufferMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(rendererComponent.device, rendererComponent.indexBufferMemory, nullptr);
            rendererComponent.indexBufferMemory = VK_NULL_HANDLE;
        }

        if (rendererComponent.uniformBuffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(rendererComponent.device, rendererComponent.uniformBuffer, nullptr);
            rendererComponent.uniformBuffer = VK_NULL_HANDLE;
        }
        if (rendererComponent.uniformBufferMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(rendererComponent.device, rendererComponent.uniformBufferMemory, nullptr);
            rendererComponent.uniformBufferMemory = VK_NULL_HANDLE;
        }

        if (rendererComponent.device) vkDestroyDevice(rendererComponent.device, nullptr);
        if (rendererComponent.surface) vkDestroySurfaceKHR(rendererComponent.instance, rendererComponent.surface, nullptr);
        if (rendererComponent.instance) vkDestroyInstance(rendererComponent.instance, nullptr);

        if (rendererComponent.textureSampler)
        {
            vkDestroySampler(rendererComponent.device, rendererComponent.textureSampler, nullptr);
            rendererComponent.textureSampler = VK_NULL_HANDLE;
        }

        if (rendererComponent.window)
        {
            glfwDestroyWindow(rendererComponent.window);
            rendererComponent.window = nullptr;
            glfwTerminate();
        }

        // If the bound pointer pointed to this instance, clear it to avoid dangling pointer
        if (GetBoundRenderer() == &rendererComponent) UnbindRenderer();
    }

    void Cleanup()
    {
        RendererComponent* rendererComponent = GetBoundRenderer();
        if (!rendererComponent) return;
        CleanupFor(*rendererComponent);
    }

    void UpdateUniformsFor(RendererComponent& rendererComponent, const void* data, size_t size)
    {
        if (rendererComponent.uniformBuffer == VK_NULL_HANDLE || rendererComponent.uniformBufferMemory == VK_NULL_HANDLE) return;

        VkMemoryRequirements memReq{};
        vkGetBufferMemoryRequirements(rendererComponent.device, rendererComponent.uniformBuffer, &memReq);
        size_t copySize = (size <= memReq.size) ? size : memReq.size;

        void* mapped = nullptr;
        VkResult res = vkMapMemory(rendererComponent.device, rendererComponent.uniformBufferMemory, 0, copySize, 0, &mapped);
        if (res != VK_SUCCESS || mapped == nullptr) {
            printf("vkMapMemory failed when updating UBO: %d\n", res);
            return;
        }
        std::memcpy(mapped, data, copySize);
        vkUnmapMemory(rendererComponent.device, rendererComponent.uniformBufferMemory);
    }

    void UpdateUniforms(const void* data, size_t size)
    {
        RendererComponent* rendererComponent = GetBoundRenderer();
        if (!rendererComponent) return;
        UpdateUniformsFor(*rendererComponent, data, size);
    }

    bool InitializeVulkanFor(RendererComponent& rendererComponent)
    {
        if (!CreateInstanceFor(rendererComponent)) return false;
        if (!CreateSurfaceFor(rendererComponent)) return false;
        if (!PickPhysicalDeviceFor(rendererComponent)) return false;
        if (!CreateLogicalDeviceFor(rendererComponent)) return false;
        if (!CreateSwapchainFor(rendererComponent)) return false;
        if (!CreateImageViewsFor(rendererComponent)) return false;
        if (!CreateRenderPassFor(rendererComponent)) return false;
        if (!CreateCommandPoolFor(rendererComponent)) return false;
        if (!CreateGraphicsPipelineFor(rendererComponent)) return false;
        if (!CreateDepthResourcesFor(rendererComponent)) return false;
        if (!CreateFramebuffersFor(rendererComponent)) return false;
        if (!CreateCommandBuffersFor(rendererComponent)) return false;
        if (!CreateSyncObjectsFor(rendererComponent)) return false;
        return true;
    }

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

    VkShaderModule CreateShaderModuleFor(RendererComponent& rendererComponent, const std::vector<char>& code)
    {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(rendererComponent.device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create shader module!");
        }
        return shaderModule;
    }

    VkShaderModule CreateShaderModuleFor(RendererComponent& rendererComponent, const std::vector<char>& code, bool /*unused*/)
    {
        return CreateShaderModuleFor(rendererComponent, code);
    }

    VkShaderModule CreateShaderModule(const std::vector<char>& code)
    {
        RendererComponent* rendererComponent = GetBoundRenderer();
        if (!rendererComponent) throw std::runtime_error("No renderer for CreateShaderModule");
        return CreateShaderModuleFor(*rendererComponent, code);
    }

    uint32_t FindMemoryTypeFor(RendererComponent& rendererComponent, uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(rendererComponent.physicalDevice, &memProperties);
        for (uint32_t memoryType = 0; memoryType < memProperties.memoryTypeCount; memoryType++) 
        {
            if ((typeFilter & (1 << memoryType)) && (memProperties.memoryTypes[memoryType].propertyFlags & properties) == properties) 
            {
                return memoryType;
            }
        }
        throw std::runtime_error("failed to find suitable memory type!");
    }

    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        RendererComponent* rendererComponent = GetBoundRenderer();
        if (!rendererComponent) throw std::runtime_error("No renderer for FindMemoryType");
        return FindMemoryTypeFor(*rendererComponent, typeFilter, properties);
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
            throw std::runtime_error("failed to create buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(rendererComponent.device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryTypeFor(rendererComponent, memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(rendererComponent.device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) 
        {
            throw std::runtime_error("failed to allocate buffer memory!");
        }

        vkBindBufferMemory(rendererComponent.device, buffer, bufferMemory, 0);
    }

    void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
    {
        RendererComponent* rendererComponent = GetBoundRenderer();
        if (!rendererComponent) throw std::runtime_error("No renderer for CreateBuffer");
        CreateBufferFor(*rendererComponent, size, usage, properties, buffer, bufferMemory);
    }

    VkCommandBuffer BeginSingleTimeCommandsFor(RendererComponent& rendererComponent)
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = rendererComponent.commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(rendererComponent.device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);
        return commandBuffer;
    }

    VkCommandBuffer BeginSingleTimeCommands()
    {
        RendererComponent* rendererComponent = GetBoundRenderer();
        if (!rendererComponent) throw std::runtime_error("No renderer for BeginSingleTimeCommands");
        return BeginSingleTimeCommandsFor(*rendererComponent);
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

    void EndSingleTimeCommands(VkCommandBuffer commandBuffer)
    {
        RendererComponent* rendererComponent = GetBoundRenderer();
        if (!rendererComponent) throw std::runtime_error("No renderer for EndSingleTimeCommands");
        EndSingleTimeCommandsFor(*rendererComponent, commandBuffer);
    }
} // namespace RENDERER_HELPERS
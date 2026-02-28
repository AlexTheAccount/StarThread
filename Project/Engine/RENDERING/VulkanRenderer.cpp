#include "RenderingComponents.h"
#include <stdexcept>
#include <fstream>
#include <cstring>
#include <entt/entt.hpp>
#include "../IMGUI/ImguiComponents.h"
#include "../ENGINE/EngineComponents.h"

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
        {
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
        }

        // Wait for the current frame's in-flight fence to ensure the previous frame has finished before we reuse its resources
        vkWaitForFences(rendererComponent.device, 1, &rendererComponent.inFlightFences[rendererComponent.currentFrame], VK_TRUE, UINT64_MAX);
        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(rendererComponent.device, rendererComponent.swapchain, UINT64_MAX,
            rendererComponent.imageAvailableSemaphores[rendererComponent.currentFrame], VK_NULL_HANDLE, &imageIndex);
        {

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
            rendererComponent.lastImageIndex = imageIndex;

            if (rendererComponent.imagesInFlight.size() > imageIndex && rendererComponent.imagesInFlight[imageIndex] != VK_NULL_HANDLE)
            {
                vkWaitForFences(rendererComponent.device, 1, &rendererComponent.imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
            }

            if (rendererComponent.imagesInFlight.size() > imageIndex)
            {
                rendererComponent.imagesInFlight[imageIndex] = rendererComponent.inFlightFences[rendererComponent.currentFrame];
            }

            vkResetFences(rendererComponent.device, 1, &rendererComponent.inFlightFences[rendererComponent.currentFrame]);
        }

        // record commands for this image
        {
            VkCommandBuffer command = rendererComponent.commandBuffers[imageIndex];

            // Reset the command buffer to the initial state before re-recording
            vkResetCommandBuffer(command, 0);

            VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            VkResult beginResult = vkBeginCommandBuffer(command, &beginInfo);
            if (beginResult != VK_SUCCESS)
            {
                printf("vkBeginCommandBuffer failed for per-frame command: %d\n", beginResult);
                return;
            }

            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = rendererComponent.renderPass;
            renderPassInfo.framebuffer = rendererComponent.swapchainFramebuffers[imageIndex];
            renderPassInfo.renderArea.offset = { 0, 0 };
            renderPassInfo.renderArea.extent = rendererComponent.swapchainExtent;

            // provide clear values for attachments
            std::array<VkClearValue, 2> clearValues{};
            clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
            clearValues[1].depthStencil = { 1.0f, 0 }; // depth clear
            renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            renderPassInfo.pClearValues = clearValues.data();

            vkCmdBeginRenderPass(command, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, rendererComponent.graphicsPipeline);
            if (rendererComponent.descriptorSet != VK_NULL_HANDLE)
            {
                vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    rendererComponent.pipelineLayout, 0, 1,
                    &rendererComponent.descriptorSet, 0, nullptr);
            }
            else
            {
                printf("Warning: no descriptor set bound; draw calls may be invalid\n");
            }
            
            Registry& registry = ENGINE::GlobalRegistry();

            // Query camera components on entities
            auto cameraView = registry.view<RENDERING::Camera>();
            if (!cameraView.empty())
            {
                entt::entity cameraEntity = *cameraView.begin();
                const RENDERING::Camera& camera = cameraView.get<RENDERING::Camera>(cameraEntity);

                // update camera UBO (binding 0 expects View + Projection)
                RENDERING::RENDERER_HELPERS::UpdateCameraUBO(rendererComponent, camera.view, camera.projection);
            }
            else
            {
                // fallback identity:
                printf("No Camera component found; using identity matrices for view and projection\n");
                GW::MATH::GMATRIXF identity = GW::MATH::GIdentityMatrixF;
                RENDERING::RENDERER_HELPERS::UpdateCameraUBO(rendererComponent, identity, identity);
            }

            // iterate entities and draw
            auto view = registry.view<RENDERING::MeshHandle, RENDERING::Transform>();
            for (auto entity : view)
            {
                const auto& [meshHandle, transform] = view.get<RENDERING::MeshHandle, RENDERING::Transform>(entity);

                const auto* mesh = MeshManager::Instance().GetMesh(meshHandle.id);
                if (!mesh || mesh->vertexBuffer == VK_NULL_HANDLE || mesh->indexBuffer == VK_NULL_HANDLE)
                    continue;

                VkBuffer vertexBuffer = mesh->vertexBuffer;
                VkDeviceSize offset = 0;
                vkCmdBindVertexBuffers(command, 0, 1, &vertexBuffer, &offset);
                vkCmdBindIndexBuffer(command, mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

                // update object UBO (binding 1 expects World)
                RENDERING::RENDERER_HELPERS::UpdateObjectUBO(rendererComponent, transform.world);

                vkCmdDrawIndexed(command, static_cast<uint32_t>(mesh->indexCount), 1, 0, 0, 0);
            }

            // Render ImGui into command buffer while it is recording and inside the render pass
            {
                if (UI::ImguiLayer* imguiPtr = registry.ctx().find<UI::ImguiLayer>(); imguiPtr && imguiPtr->IsInitialized())
                {
                    imguiPtr->EndFrame(command);
                }
            }

            vkCmdEndRenderPass(command);
            if (vkEndCommandBuffer(command) != VK_SUCCESS)
            {
                printf("Failed to record command buffer\n");
                return;
            }
        }

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
            vkDeviceWaitIdle(rendererComponent.device);
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
        // Ensure all GPU work is finished before destroying objects
        if (rendererComponent.device != VK_NULL_HANDLE)
        {
            // Wait for all queues on the device to be idle
            vkDeviceWaitIdle(rendererComponent.device);
        }

        // Ensure MeshManager releases any GPU buffers that were created using this device
        MeshManager::Instance().ReleaseAllGpuResources(rendererComponent);

        // Destroy texture sampler before device destruction
        if (rendererComponent.textureSampler)
        {
            vkDestroySampler(rendererComponent.device, rendererComponent.textureSampler, nullptr);
            rendererComponent.textureSampler = VK_NULL_HANDLE;
        }

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

        // Destroy command pool
        if (rendererComponent.commandPool) vkDestroyCommandPool(rendererComponent.device, rendererComponent.commandPool, nullptr);
        rendererComponent.commandPool = VK_NULL_HANDLE;
        rendererComponent.commandBuffers.clear();

        for (VkFramebuffer frameBuffer : rendererComponent.swapchainFramebuffers) vkDestroyFramebuffer(rendererComponent.device, frameBuffer, nullptr);
        rendererComponent.swapchainFramebuffers.clear();

        if (rendererComponent.graphicsPipeline) { vkDestroyPipeline(rendererComponent.device, rendererComponent.graphicsPipeline, nullptr); rendererComponent.graphicsPipeline = VK_NULL_HANDLE; }
        if (rendererComponent.pipelineLayout) { vkDestroyPipelineLayout(rendererComponent.device, rendererComponent.pipelineLayout, nullptr); rendererComponent.pipelineLayout = VK_NULL_HANDLE; }
        if (rendererComponent.renderPass) { vkDestroyRenderPass(rendererComponent.device, rendererComponent.renderPass, nullptr); rendererComponent.renderPass = VK_NULL_HANDLE; }

        for (VkImageView view : rendererComponent.swapchainImageViews) vkDestroyImageView(rendererComponent.device, view, nullptr);
        rendererComponent.swapchainImageViews.clear();

        if (rendererComponent.swapchain) { vkDestroySwapchainKHR(rendererComponent.device, rendererComponent.swapchain, nullptr); rendererComponent.swapchain = VK_NULL_HANDLE; }

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

        // Destroy UBO / buffers 
        if (rendererComponent.cameraUniformBuffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(rendererComponent.device, rendererComponent.cameraUniformBuffer, nullptr);
            rendererComponent.cameraUniformBuffer = VK_NULL_HANDLE;
        }
        if (rendererComponent.cameraUniformBufferMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(rendererComponent.device, rendererComponent.cameraUniformBufferMemory, nullptr);
            rendererComponent.cameraUniformBufferMemory = VK_NULL_HANDLE;
        }

        if (rendererComponent.objectUniformBuffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(rendererComponent.device, rendererComponent.objectUniformBuffer, nullptr);
            rendererComponent.objectUniformBuffer = VK_NULL_HANDLE;
        }
        if (rendererComponent.objectUniformBufferMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(rendererComponent.device, rendererComponent.objectUniformBufferMemory, nullptr);
            rendererComponent.objectUniformBufferMemory = VK_NULL_HANDLE;
        }

        // Destroy texture image, view, and memory 
        if (rendererComponent.textureImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(rendererComponent.device, rendererComponent.textureImageView, nullptr);
            rendererComponent.textureImageView = VK_NULL_HANDLE;
        }
        if (rendererComponent.textureImage != VK_NULL_HANDLE)
        {
            vkDestroyImage(rendererComponent.device, rendererComponent.textureImage, nullptr);
            rendererComponent.textureImage = VK_NULL_HANDLE;
        }
        if (rendererComponent.textureImageMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(rendererComponent.device, rendererComponent.textureImageMemory, nullptr);
            rendererComponent.textureImageMemory = VK_NULL_HANDLE;
        }

        // Destroy depth image, view, and memory 
        if (rendererComponent.depthImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(rendererComponent.device, rendererComponent.depthImageView, nullptr);
            rendererComponent.depthImageView = VK_NULL_HANDLE;
        }
        if (rendererComponent.depthImage != VK_NULL_HANDLE)
        {
            vkDestroyImage(rendererComponent.device, rendererComponent.depthImage, nullptr);
            rendererComponent.depthImage = VK_NULL_HANDLE;
        }
        if (rendererComponent.depthImageMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(rendererComponent.device, rendererComponent.depthImageMemory, nullptr);
            rendererComponent.depthImageMemory = VK_NULL_HANDLE;
        }

        // destroy the device.
        if (rendererComponent.device) { vkDestroyDevice(rendererComponent.device, nullptr); rendererComponent.device = VK_NULL_HANDLE; }

        if (rendererComponent.surface) { vkDestroySurfaceKHR(rendererComponent.instance, rendererComponent.surface, nullptr); rendererComponent.surface = VK_NULL_HANDLE; }
        if (rendererComponent.instance) { vkDestroyInstance(rendererComponent.instance, nullptr); rendererComponent.instance = VK_NULL_HANDLE; }

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
        if (rendererComponent.cameraUniformBuffer == VK_NULL_HANDLE || rendererComponent.cameraUniformBufferMemory == VK_NULL_HANDLE) return;

        VkMemoryRequirements memReq{};
        vkGetBufferMemoryRequirements(rendererComponent.device, rendererComponent.cameraUniformBuffer, &memReq);
        size_t copySize = (size <= memReq.size) ? size : memReq.size;

        void* mapped = nullptr;
        VkResult res = vkMapMemory(rendererComponent.device, rendererComponent.cameraUniformBufferMemory, 0, copySize, 0, &mapped);
        if (res != VK_SUCCESS || mapped == nullptr) {
            printf("vkMapMemory failed when updating UBO: %d\n", res);
            return;
        }
        std::memcpy(mapped, data, copySize);
        vkUnmapMemory(rendererComponent.device, rendererComponent.cameraUniformBufferMemory);
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

    void UpdateCameraUBO(RendererComponent& rendererComponent, const GW::MATH::GMATRIXF& view, const GW::MATH::GMATRIXF& projection)
    {
        struct CameraData { GW::MATH::GMATRIXF View; GW::MATH::GMATRIXF Projection; } cam{ view, projection };
        void* mapped = nullptr;
        vkMapMemory(rendererComponent.device, rendererComponent.cameraUniformBufferMemory, 0, sizeof(cam), 0, &mapped);
        memcpy(mapped, &cam, sizeof(cam));
        vkUnmapMemory(rendererComponent.device, rendererComponent.cameraUniformBufferMemory);
    }

    void UpdateObjectUBO(RendererComponent& rendererComponent, const GW::MATH::GMATRIXF& world)
    {
        void* mapped = nullptr;
        vkMapMemory(rendererComponent.device, rendererComponent.objectUniformBufferMemory, 0, sizeof(world), 0, &mapped);
        memcpy(mapped, &world, sizeof(world));
        vkUnmapMemory(rendererComponent.device, rendererComponent.objectUniformBufferMemory);
    }
} // namespace RENDERER_HELPERS
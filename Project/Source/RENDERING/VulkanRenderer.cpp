#include "RenderingComponents.h"
#include <stdexcept>
#include <fstream>
#include <set>
#include <algorithm>
#include <array>
#include <cstring>
#include "../GAME/GlobalRegistry.h"
using namespace RENDERING::RENDERER_HELPERS::Memory;
using namespace RENDERING::RENDERER_HELPERS::Pipeline;
using namespace RENDERING::RENDERER_HELPERS::Swapchain;
using namespace RENDERING::RENDERER_HELPERS::Device;

using namespace RENDERING;

namespace RENDERER_HELPERS
{
    #ifdef NDEBUG
    const bool enableValidationLayers = false;
    #else
    const bool enableValidationLayers = true;
    #endif

    static RendererComponent* GetGlobalRenderer()
    {
        auto& registry = GlobalRegistry();
        auto view = registry.view<RENDERING::RendererComponent>();
        if (view.begin() == view.end()) return nullptr;
        auto entity = *view.begin();
        return &registry.get<RENDERING::RendererComponent>(entity);
    }

    bool InitializeVulkanFor(RendererComponent& rendererComponent);

    bool Initialize(uint32_t width, uint32_t height, const char* title)
    {
        if (!InitializeWindow(width, height, title)) return false;

        RendererComponent* rendererComponent = GetGlobalRenderer();
        if (!rendererComponent)
        {
            printf("RendererComponent missing after InitializeWindow\n");
            return false;
        }

        if (!InitializeVulkanFor(*rendererComponent)) return false;
        return true;
    }

    void MainLoop()
    {
        RendererComponent* rendererComponent = GetGlobalRenderer();
        if (!rendererComponent)
        {
            printf("No RendererComponent found in GlobalRegistry\n");
            return;
        }

        while (!glfwWindowShouldClose(rendererComponent->window))
        {
            glfwPollEvents();
            DrawFrame();
        }
        if (rendererComponent->device != VK_NULL_HANDLE) vkDeviceWaitIdle(rendererComponent->device);
    }

    void Cleanup()
    {
        RendererComponent* rendererComponent = GetGlobalRenderer();
        if (!rendererComponent) return;

        // Cleanup sync
        for (size_t frame = 0; frame < rendererComponent->MAX_FRAMES_IN_FLIGHT; ++frame)
        {
            if (frame < rendererComponent->imageAvailableSemaphores.size()) vkDestroySemaphore(rendererComponent->device, rendererComponent->imageAvailableSemaphores[frame], nullptr);
            if (frame < rendererComponent->renderFinishedSemaphores.size()) vkDestroySemaphore(rendererComponent->device, rendererComponent->renderFinishedSemaphores[frame], nullptr);
            if (frame < rendererComponent->inFlightFences.size()) vkDestroyFence(rendererComponent->device, rendererComponent->inFlightFences[frame], nullptr);
        }

        if (rendererComponent->commandPool) vkDestroyCommandPool(rendererComponent->device, rendererComponent->commandPool, nullptr);

        for (auto frameBuffer : rendererComponent->swapchainFramebuffers) vkDestroyFramebuffer(rendererComponent->device, frameBuffer, nullptr);
        if (rendererComponent->graphicsPipeline) vkDestroyPipeline(rendererComponent->device, rendererComponent->graphicsPipeline, nullptr);
        if (rendererComponent->pipelineLayout) vkDestroyPipelineLayout(rendererComponent->device, rendererComponent->pipelineLayout, nullptr);
        if (rendererComponent->renderPass) vkDestroyRenderPass(rendererComponent->device, rendererComponent->renderPass, nullptr);

        for (auto view : rendererComponent->swapchainImageViews) vkDestroyImageView(rendererComponent->device, view, nullptr);
        if (rendererComponent->swapchain) vkDestroySwapchainKHR(rendererComponent->device, rendererComponent->swapchain, nullptr);

        if (rendererComponent->descriptorPool) { vkDestroyDescriptorPool(rendererComponent->device, rendererComponent->descriptorPool, nullptr); rendererComponent->descriptorPool = VK_NULL_HANDLE; }

        if (rendererComponent->uniformBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(rendererComponent->device, rendererComponent->uniformBuffer, nullptr);
            rendererComponent->uniformBuffer = VK_NULL_HANDLE;
        }
        if (rendererComponent->uniformBufferMemory != VK_NULL_HANDLE) {
            vkFreeMemory(rendererComponent->device, rendererComponent->uniformBufferMemory, nullptr);
            rendererComponent->uniformBufferMemory = VK_NULL_HANDLE;
        }

        if (rendererComponent->device) vkDestroyDevice(rendererComponent->device, nullptr);
        if (rendererComponent->surface) vkDestroySurfaceKHR(rendererComponent->instance, rendererComponent->surface, nullptr);
        if (rendererComponent->instance) vkDestroyInstance(rendererComponent->instance, nullptr);

        if (rendererComponent->textureSampler)
        {
            vkDestroySampler(rendererComponent->device, rendererComponent->textureSampler, nullptr);
            rendererComponent->textureSampler = VK_NULL_HANDLE;
        }

        if (rendererComponent->window)
        {
            glfwDestroyWindow(rendererComponent->window);
            rendererComponent->window = nullptr;
            glfwTerminate();
        }
    }

    void UpdateUniforms(const void* data, size_t size)
    {
        RendererComponent* rendererComponent = GetGlobalRenderer();
        if (!rendererComponent) return;
        if (rendererComponent->uniformBuffer == VK_NULL_HANDLE || rendererComponent->uniformBufferMemory == VK_NULL_HANDLE) return;

        VkMemoryRequirements memReq{};
        vkGetBufferMemoryRequirements(rendererComponent->device, rendererComponent->uniformBuffer, &memReq);
        size_t copySize = (size <= memReq.size) ? size : memReq.size;

        void* mapped = nullptr;
        VkResult res = vkMapMemory(rendererComponent->device, rendererComponent->uniformBufferMemory, 0, copySize, 0, &mapped);
        if (res != VK_SUCCESS || mapped == nullptr) {
            printf("vkMapMemory failed when updating UBO: %d\n", res);
            return;
        }
        std::memcpy(mapped, data, copySize);
        vkUnmapMemory(rendererComponent->device, rendererComponent->uniformBufferMemory);
    }

    void Render()
    {
        RendererComponent* rendererComponent = GetGlobalRenderer();
        if (!rendererComponent) return;
        DrawFrameFor(*rendererComponent);
    }

    bool InitializeWindow(uint32_t width, uint32_t height, const char* title)
    {
        if (!glfwInit()) return false;
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        GLFWwindow* window = glfwCreateWindow((int)width, (int)height, title, nullptr, nullptr);
        if (!window) return false;

        auto& registry = GlobalRegistry();
        auto view = registry.view<RENDERING::RendererComponent>();
        entt::entity entity;
        if (view.begin() == view.end())
        {
            entity = registry.create();
            registry.emplace<RENDERING::RendererComponent>(entity);
        }
        else
        {
            entity = *view.begin();
        }

        auto& rendererComp = registry.get<RENDERING::RendererComponent>(entity);
        rendererComp.window = window;

        glfwSetWindowUserPointer(window, &rendererComp);
        glfwSetFramebufferSizeCallback(window, FramebufferResize);

        return true;
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
        if (!CreateGraphicsPipelineFor(rendererComponent)) return false;
        if (!CreateFramebuffersFor(rendererComponent)) return false;
        if (!CreateCommandPoolFor(rendererComponent)) return false;
        if (!CreateCommandBuffersFor(rendererComponent)) return false;
        if (!CreateSyncObjectsFor(rendererComponent)) return false;
        return true;
    }

    void DrawFrame()
    {
        RendererComponent* rendererComponent = GetGlobalRenderer();
        if (!rendererComponent) return;
        DrawFrameFor(*rendererComponent);
    }

    void DrawFrameFor(RendererComponent& rendererComponent)
    {
        if (rendererComponent.currentFrame >= rendererComponent.imageAvailableSemaphores.size() ||
            rendererComponent.currentFrame >= rendererComponent.renderFinishedSemaphores.size() ||
            rendererComponent.currentFrame >= rendererComponent.inFlightFences.size()) {
            printf("Out-of-range currentFrame=%zu avail=%zu finish=%zu fences=%zu\n",
                rendererComponent.currentFrame, rendererComponent.imageAvailableSemaphores.size(), 
                rendererComponent.renderFinishedSemaphores.size(), rendererComponent.inFlightFences.size());
            return;
        }
        if (rendererComponent.imageAvailableSemaphores[rendererComponent.currentFrame] == VK_NULL_HANDLE ||
            rendererComponent.renderFinishedSemaphores[rendererComponent.currentFrame] == VK_NULL_HANDLE ||
            rendererComponent.inFlightFences[rendererComponent.currentFrame] == VK_NULL_HANDLE) 
        {
            printf("Null sync handle at frame %zu\n", rendererComponent.currentFrame);
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

        VkSemaphore signalSemaphores[] = { rendererComponent.renderFinishedSemaphores[rendererComponent.currentFrame] };
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

    VkShaderModule CreateShaderModule(const std::vector<char>& code)
    {
        RendererComponent* rendererComponent = GetGlobalRenderer();
        if (!rendererComponent) throw std::runtime_error("No renderer for CreateShaderModule");
        return CreateShaderModuleFor(*rendererComponent, code);
    }

    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        RendererComponent* rendererComponent = GetGlobalRenderer();
        if (!rendererComponent) throw std::runtime_error("No renderer for FindMemoryType");
        return FindMemoryTypeFor(*rendererComponent, typeFilter, properties);
    }

    void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
    {
        RendererComponent* rendererComponent = GetGlobalRenderer();
        if (!rendererComponent) throw std::runtime_error("No renderer for CreateBuffer");
        CreateBufferFor(*rendererComponent, size, usage, properties, buffer, bufferMemory);
    }

    VkCommandBuffer BeginSingleTimeCommands()
    {
        RendererComponent* rendererComponent = GetGlobalRenderer();
        if (!rendererComponent) throw std::runtime_error("No renderer for BeginSingleTimeCommands");
        return BeginSingleTimeCommandsFor(*rendererComponent);
    }

    void EndSingleTimeCommands(VkCommandBuffer commandBuffer)
    {
        RendererComponent* rendererComponent = GetGlobalRenderer();
        if (!rendererComponent) throw std::runtime_error("No renderer for EndSingleTimeCommands");
        EndSingleTimeCommandsFor(*rendererComponent, commandBuffer);
    }

    // Framebuffer resize callback
    void FramebufferResize(GLFWwindow* window, int /*width*/, int /*height*/)
    {
        void* user = glfwGetWindowUserPointer(window);
        if (!user) return;
        auto renderer = reinterpret_cast<RENDERING::RendererComponent*>(user);
        renderer->framebufferResized = true;
    }
}
#include "../IMGUI/ImguiComponents.h"
#include "../RENDERING/RenderingComponents.h"

#include <vulkan/vulkan.h>
#include <vector>
#include <stdio.h>
#include <GLFW/glfw3.h>

void UI::InitializeImgui(entt::registry & registry)
{
    // Define ImGui layer bits
    {
        entt::entity mainMenuEntity = registry.create();
        registry.emplace<UI::UILayerBit>(mainMenuEntity, UI::UILayerBit{ static_cast<uint8_t>(1 << 0), "Main Menu" });
    }
    {
        entt::entity creditsMenuEntity = registry.create();
        registry.emplace<UI::UILayerBit>(creditsMenuEntity, UI::UILayerBit{ static_cast<uint8_t>(1 << 1), "Credits Menu" });
    }

    // Create and store global UI state
    auto& uiState = registry.ctx().emplace<UI::UIState>();
    uiState.visible = true;
    uiState.visibleLayers = UI::UILayer(1 << 0);
    uiState.backgroundVisible = true;
    uiState.uiAcceptsInput = true;

    // Build UI
    UI::BuildMainMenu(registry);
    UI::BuildCreditsMenu(registry);

    // initialize ImGuiLayer
    auto view = registry.view<RENDERING::RendererComponent>();
    if (view.begin() != view.end())
    {
        auto entity = view.front();
        auto& vulkanRenderer = registry.get<RENDERING::RendererComponent>(entity);

        // Place ImguiLayer into registry
        registry.ctx().emplace<ImguiLayer>();

        // Retrieve needed Vulkan objects from RendererComponent
        VkInstance instance = vulkanRenderer.instance;
        VkQueue graphicsQueue = vulkanRenderer.graphicsQueue;
        int graphicsQueueFamily = 0;
        VkPhysicalDevice physicalDevice = vulkanRenderer.physicalDevice;
        VkSurfaceKHR surface = vulkanRenderer.surface;

        // Query the queue family index from the Vulkan surface/device properties
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

        for (uint32_t queueFamilyIndex = 0; queueFamilyIndex < queueFamilyCount; ++queueFamilyIndex)
        {
            VkBool32 presentSupport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndex, surface, &presentSupport);

            if ((queueFamilies[queueFamilyIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT) && presentSupport)
            {
                graphicsQueueFamily = queueFamilyIndex;
                break;
            }
        }
        GLFWwindow* window = vulkanRenderer.window;

        if (!window)
        {
            printf("main::ImguiBehavior: No valid GLFW window available for ImGui initialization.\n");
            return;
        }

        // finish ImGuiLayer initialization
        unsigned int swapchainImageCount = 2;
        if (!vulkanRenderer.swapchainImages.empty())
            swapchainImageCount = static_cast<unsigned int>(vulkanRenderer.swapchainImages.size());
        int minImageCount = static_cast<int>(swapchainImageCount);
        auto& imguiLayer = registry.ctx().get<ImguiLayer>();

        // Create a descriptor pool for ImGui
        VkDescriptorPool imguiDescriptorPool = VK_NULL_HANDLE;
        {
            VkDescriptorPoolSize pool_sizes[] =
            {
                { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,  (uint32_t)(minImageCount * 2) },
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,          (uint32_t)(minImageCount) },
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,          (uint32_t)(minImageCount) }
            };
            VkDescriptorPoolCreateInfo pool_info{};
            pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            pool_info.maxSets = (uint32_t)(minImageCount * 4);
            pool_info.poolSizeCount = (uint32_t)std::size(pool_sizes);
            pool_info.pPoolSizes = pool_sizes;

            VkResult result = vkCreateDescriptorPool(vulkanRenderer.device, &pool_info, nullptr, &imguiDescriptorPool);
            RENDERING::SetDebugName(vulkanRenderer.device, VK_OBJECT_TYPE_DESCRIPTOR_POOL, (uint64_t)imguiDescriptorPool, "ImguiDescriptorPool");
            if (result != VK_SUCCESS)
            {
                printf("vkCreateDescriptorPool failed with %d\n", result);
                imguiDescriptorPool = VK_NULL_HANDLE;
                return;
            }
        }

        // finish
        imguiLayer.Initialize
        (
            registry,
            instance,
            vulkanRenderer.physicalDevice,
            vulkanRenderer.device,
            graphicsQueueFamily,
            graphicsQueue,
            imguiDescriptorPool,
            vulkanRenderer.renderPass,
            minImageCount,
            window
        );

        registry.on_update<RENDERING::RendererComponent>().connect<UI::RenderUI>();
    }
    else
        printf("main::ImguiBehavior: VulkanRenderer not created.\n");
}
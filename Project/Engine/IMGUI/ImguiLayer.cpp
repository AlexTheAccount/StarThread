#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include "../IMGUI/ImguiComponents.h"
#include <stdio.h>

namespace UI
{
    static void ApplyStyle()
    {
        ImVec4* colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.29f, 0.34f, 0.26f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.29f, 0.34f, 0.26f, 1.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.24f, 0.27f, 0.20f, 1.00f);
        colors[ImGuiCol_Border] = ImVec4(0.54f, 0.57f, 0.51f, 0.50f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.14f, 0.16f, 0.11f, 0.52f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.24f, 0.27f, 0.20f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.27f, 0.30f, 0.23f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.30f, 0.34f, 0.26f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.24f, 0.27f, 0.20f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.29f, 0.34f, 0.26f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.24f, 0.27f, 0.20f, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.35f, 0.42f, 0.31f, 1.00f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.28f, 0.32f, 0.24f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.25f, 0.30f, 0.22f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.23f, 0.27f, 0.21f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.35f, 0.42f, 0.31f, 1.00f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.54f, 0.57f, 0.51f, 0.50f);
        colors[ImGuiCol_Button] = ImVec4(0.29f, 0.34f, 0.26f, 0.40f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.35f, 0.42f, 0.31f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.54f, 0.57f, 0.51f, 0.50f);
        colors[ImGuiCol_Header] = ImVec4(0.35f, 0.42f, 0.31f, 1.00f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.35f, 0.42f, 0.31f, 0.6f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.54f, 0.57f, 0.51f, 0.50f);
        colors[ImGuiCol_Separator] = ImVec4(0.14f, 0.16f, 0.11f, 1.00f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.54f, 0.57f, 0.51f, 1.00f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.19f, 0.23f, 0.18f, 0.00f); // grip invis
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.54f, 0.57f, 0.51f, 1.00f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
        colors[ImGuiCol_Tab] = ImVec4(0.35f, 0.42f, 0.31f, 1.00f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.54f, 0.57f, 0.51f, 0.78f);
        colors[ImGuiCol_TabActive] = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.24f, 0.27f, 0.20f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.35f, 0.42f, 0.31f, 1.00f);
        colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(1.00f, 0.78f, 0.28f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(0.73f, 0.67f, 0.24f, 1.00f);
        colors[ImGuiCol_NavHighlight] = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

        ImGuiStyle& style = ImGui::GetStyle();
        style.FrameBorderSize = 1.0f;
        style.WindowRounding = 0.0f;
        style.ChildRounding = 0.0f;
        style.FrameRounding = 0.0f;
        style.PopupRounding = 0.0f;
        style.ScrollbarRounding = 0.0f;
        style.GrabRounding = 0.0f;
        style.TabRounding = 0.0f;

        ImGui::GetIO().FontGlobalScale = 2.f;
    }

    void ImguiLayer::Initialize(VkInstance instance,
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        uint32_t graphicsQueueFamily,
        VkQueue graphicsQueue,
        VkDescriptorPool descriptorPool,
        VkRenderPass renderPass,
        uint32_t minImageCount,
        GLFWwindow* window)
    {
        // Basic validation
        if (device == VK_NULL_HANDLE) 
        {
            printf("ImguiLayer::Initialize: device == VK_NULL_HANDLE\n");
            return;
        }
        if (descriptorPool == VK_NULL_HANDLE) 
        {
            printf("ImguiLayer::Initialize: descriptorPool == VK_NULL_HANDLE\n");
            return;
        }
        if (renderPass == VK_NULL_HANDLE) 
        {
            printf("ImguiLayer::Initialize: renderPass == VK_NULL_HANDLE\n");
            return;
        }

        // Log device and dispatch pointer
        void* deviceDispatch = nullptr;
        if (device) deviceDispatch = *(void**)device;
        printf("ImguiLayer::Initialize: device=%p dispatch=%p descriptorPool=%p renderPass=%p\n",
            (void*)device, deviceDispatch, (void*)descriptorPool, (void*)renderPass);

        // Setup Dear ImGui context
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
        ImGui::StyleColorsDark();

        // Apply style
        ApplyStyle();

        // Setup Platform/Renderer backends
        ImGui_ImplVulkan_InitInfo initInfo = {};
        initInfo.Instance = instance;
        initInfo.PhysicalDevice = physicalDevice;
        initInfo.Device = device;
        initInfo.QueueFamily = graphicsQueueFamily;
        initInfo.Queue = graphicsQueue;
        initInfo.PipelineCache = VK_NULL_HANDLE;
        initInfo.DescriptorPool = descriptorPool;
        initInfo.MinImageCount = minImageCount;
        initInfo.ImageCount = minImageCount;
        initInfo.Allocator = nullptr;
        initInfo.CheckVkResultFn = nullptr;
        initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        initInfo.PipelineInfoMain.RenderPass = renderPass;

        ImGui_ImplVulkan_Init(&initInfo);

        // Initialize GLFW platform backend for Vulkan
        ImGui_ImplGlfw_InitForVulkan(window, true);

        // store the window and Vulkan objects
        this->window = window;
        this->device = device;
        this->descriptorPool = descriptorPool;
        this->renderPass = renderPass;
        this->minImageCount = static_cast<int>(minImageCount);

        initialized = true;

        // Confirm stored values
        void* storedDispatch = nullptr;
        if (this->device) storedDispatch = *(void**)this->device;
        printf("ImguiLayer::Initialize complete: stored device=%p dispatch=%p\n", (void*)this->device, storedDispatch);
    }

    bool ImguiLayer::BeginFrame()
    {
        if (!initialized) 
        {
            printf("ImguiLayer::BeginFrame: ImguiLayer not initialized!\n");
            return false;
        }

        ImGui_ImplGlfw_NewFrame();
        ImGui_ImplVulkan_NewFrame();
        ImGui::NewFrame();
        return true;
    }

    void ImguiLayer::EndFrame(VkCommandBuffer commandBuffer)
    {
        if (!initialized)
        {
            printf("ImguiLayer::EndFrame: ImguiLayer not initialized!\n");
            return;
        }

        ImGui::Render();
        ImDrawData* draw_data = ImGui::GetDrawData();
        if (!draw_data) 
        {
            printf("ImguiLayer::EndFrame: draw_data == nullptr\n");
            return;
        }

        if (commandBuffer == VK_NULL_HANDLE) 
        {
            printf("ImguiLayer::EndFrame: commandBuffer == VK_NULL_HANDLE\n");
            return;
        }

        ImGui_ImplVulkan_RenderDrawData(draw_data, commandBuffer);
    }

    void ImguiLayer::Shutdown()
    {
        if (!initialized)
        {
            printf("ImguiLayer::Shutdown: ImguiLayer not initialized!\n");
            return;
        }

        ImGui_ImplVulkan_Shutdown();
        CleanupVulkanResources();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        initialized = false;
    }

    void ImguiLayer::CleanupVulkanResources()
    {
        if (descriptorPool != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(device, descriptorPool, nullptr);
            descriptorPool = VK_NULL_HANDLE;
        }
    }
}
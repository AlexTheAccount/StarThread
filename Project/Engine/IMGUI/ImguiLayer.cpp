#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include "../IMGUI/ImguiComponents.h"
#include <stdio.h>
#include <filesystem>

using namespace UTILITIES;

namespace UI
{
    // Simple background gradient / subtle starfield drawn each frame.
    static void DrawSpaceBackground()
    {
        ImGuiIO& io = ImGui::GetIO();
        ImDrawList* background = ImGui::GetBackgroundDrawList();
        ImVec2 topLeft = ImVec2(0,0);
        ImVec2 bottomRight = ImVec2(io.DisplaySize.x, io.DisplaySize.y);

        // two-color vertical gradient (deep blue -> near black)
        background->AddRectFilledMultiColor(topLeft, bottomRight,
            IM_COL32(8,18,40,255), IM_COL32(8,18,40,255),
            IM_COL32(2,6,12,255), IM_COL32(2,6,12,255));

        // small star dots for depth
        for (int star = 0; star < 60; ++star)
        {
            float x = (float)(rand() % (int)io.DisplaySize.x);
            float y = (float)(rand() % (int)io.DisplaySize.y);
            uint8_t brightness = (rand() % 80) + 120;
            background->AddCircleFilled(ImVec2(x,y), 0.6f, IM_COL32(brightness, brightness, 255, 120));
        }
    }

    static void ApplyStyle()
    {
        ImVec4* colors = ImGui::GetStyle().Colors;
        // Base
        colors[ImGuiCol_Text] = ImVec4(0.92f, 0.97f, 1.00f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.03f, 0.05f, 0.09f, 1.00f); // deep space
        colors[ImGuiCol_ChildBg] = ImVec4(0.03f, 0.05f, 0.07f, 0.95f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.04f, 0.06f, 0.10f, 0.95f);

        // Accents (neon cyan / magenta)
        ImVec4 neonCyan = ImVec4(0.06f, 0.78f, 0.95f, 1.00f);
        ImVec4 neonMag  = ImVec4(0.84f, 0.20f, 0.95f, 1.00f);
        ImVec4 dimAccent = ImVec4(0.10f, 0.22f, 0.28f, 1.00f);

        colors[ImGuiCol_FrameBg] = ImVec4(0.05f, 0.08f, 0.12f, 0.85f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(neonCyan.x, neonCyan.y, neonCyan.z, 0.12f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(neonCyan.x, neonCyan.y, neonCyan.z, 0.18f);

        colors[ImGuiCol_Button] = ImVec4(dimAccent.x, dimAccent.y, dimAccent.z, 0.35f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(neonCyan.x, neonCyan.y, neonCyan.z, 0.18f);
        colors[ImGuiCol_ButtonActive] = ImVec4(neonCyan.x, neonCyan.y, neonCyan.z, 0.28f);

        colors[ImGuiCol_Header] = ImVec4(0.06f, 0.07f, 0.09f, 0.9f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(neonCyan.x, neonCyan.y, neonCyan.z, 0.12f);
        colors[ImGuiCol_HeaderActive] = ImVec4(neonCyan.x, neonCyan.y, neonCyan.z, 0.22f);

        colors[ImGuiCol_Border] = ImVec4(0.12f, 0.16f, 0.22f, 0.8f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(0,0,0,0); // keep invisible
        colors[ImGuiCol_Tab] = ImVec4(0.05f, 0.07f, 0.09f, 0.9f);
        colors[ImGuiCol_TabHovered] = ImVec4(neonCyan.x, neonCyan.y, neonCyan.z, 0.2f);
        colors[ImGuiCol_TabActive] = ImVec4(neonMag.x, neonMag.y, neonMag.z, 0.95f);

        // Plots and highlights
        colors[ImGuiCol_PlotLines] = ImVec4(0.6f, 0.8f, 0.95f, 0.9f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.95f, 0.6f, 0.15f, 1.0f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(neonCyan.x, neonCyan.y, neonCyan.z, 0.18f);

        // Style variables for a tech look
        ImGuiStyle& style = ImGui::GetStyle();
        style.FrameBorderSize = 1.0f;      // thin frame border for "HUD card" feel
        style.WindowRounding = 4.0f;       // small rounding
        style.ChildRounding = 3.0f;
        style.FrameRounding = 3.0f;
        style.PopupRounding = 3.0f;
        style.ScrollbarRounding = 2.0f;
        style.GrabRounding = 2.0f;
        style.TabRounding = 3.0f;

        style.FramePadding = ImVec2(8, 6);
        style.ItemSpacing = ImVec2(8, 6);

        // Font scale — tweak after you load a custom font
        ImGui::GetIO().FontGlobalScale = 1.0f;
    }

    void ImguiLayer::Initialize(entt::registry& registry,
        VkInstance instance,
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        uint32_t graphicsQueueFamily,
        VkQueue graphicsQueue,
        VkDescriptorPool descriptorPool,
        VkRenderPass renderPass,
        uint32_t minImageCount,
        GLFWwindow* window)
    {
        std::shared_ptr<const GameConfig> config = registry.ctx().get<Config>().gameConfig;
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

        // Setup Dear ImGui context
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
        ImGui::StyleColorsDark();

        // Load a custom font for a sci‑fi feel
        try
        {
            if (config)
            {
                std::string fontPath = config->at("Fonts").at("fontPath").as<std::string>();
                float fontSize = config->at("Fonts").at("fontSize").as<float>();

                // Resolve relative paths
                std::filesystem::path filePath(fontPath);
                if (filePath.is_relative())
                {
                    char exePath[MAX_PATH] = {};
                    GetModuleFileNameA(NULL, exePath, MAX_PATH);
                    std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();
                    std::filesystem::path configDir = exeDir.parent_path().parent_path() / "Config";
                    std::filesystem::path resolved = (configDir / filePath).lexically_normal();
                    fontPath = resolved.string();
                }

                // Log of Helpfulness
                printf("While loading '%s'\n", fontPath.c_str());

                // Check file exists before handing to ImGui 
                if (!std::filesystem::exists(fontPath))
                {
                    printf("ImguiLayer::Initialize: Font file not found: %s\n", fontPath.c_str());
                }
                else
                {
                    ImFont* sciFiFont = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), fontSize);
                    if (sciFiFont)
                    {
                        io.FontDefault = sciFiFont;
                        ImGui::GetIO().FontGlobalScale = 1.0f;
                    }
                    else
                    {
                        printf("ImguiLayer::Initialize: ImGui failed to load font file: %s\n", fontPath.c_str());
                    }
                }
            }
        }
        catch (const std::exception& e)
        {
            printf("ImguiLayer::Initialize: failed to read font from config: %s\n", e.what());
        }

        // Apply style
        ApplyStyle();

        // Initialize GLFW platform backend for Vulkan
        ImGui_ImplGlfw_InitForVulkan(window, true);

        // Setup Vulkan backend init info
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
        initInfo.CheckVkResultFn = [](VkResult err) 
        {
            if (err != VK_SUCCESS) 
            {
                printf("ImGui Vulkan backend VkResult: %d\n", err);
            }
        };
        initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        initInfo.PipelineInfoMain.RenderPass = renderPass;

        // Initialize Vulkan backend
        ImGui_ImplVulkan_Init(&initInfo);

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

        // Draw the space background
        DrawSpaceBackground();

        return true;
    }

    void ImguiLayer::EndFrame(VkCommandBuffer commandBuffer)
    {
        if (!initialized)
        {
            printf("ImguiLayer::EndFrame: ImguiLayer not initialized!\n");
            return;
        }

        ImGui::EndFrame();
        ImGui::Render();
        ImDrawData* drawData = ImGui::GetDrawData();
        if (!drawData)
        {
            printf("ImguiLayer::EndFrame: draw_data == nullptr\n");
            return;
        }

        if (commandBuffer == VK_NULL_HANDLE) 
        {
            printf("ImguiLayer::EndFrame: commandBuffer == VK_NULL_HANDLE\n");
            return;
        }

        ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);
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
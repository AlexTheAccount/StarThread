#include "../Engine/RENDERING/RenderingComponents.h"
#include "../Engine/IMGUI/ImguiComponents.h"
#include "GAME/GameComponents.h"
#include <iostream>
#include <chrono>
#include <entt/entt.hpp>

using namespace GAME;
using namespace RENDERING;
using namespace RENDERING::RENDERER_HELPERS;
using namespace RENDERING::CAMERA_SYSTEM;
using Registry = entt::registry;

int main()
{
    // Seed the rand
    unsigned int time = std::chrono::steady_clock::now().time_since_epoch().count();
    srand(time);

    // store everything related to entities and components in the game registry
    Registry& registry = GAME::GlobalRegistry();
    Registry::entity_type entity = registry.create();

    registry.ctx().emplace<UTILITIES::Config>();
    registry.emplace<RendererComponent>(entity);
    RendererComponent& rendererComponent = registry.get<RendererComponent>(entity);

    // Initialize engine for the renderer instance owned by the game
    if (!InitializeFor(rendererComponent, 800, 600, "StarThread Vulkan"))
    {
        printf("Failed to initialize renderer\n");
        return -1;
    }

    // Create Camera entity
    float fovDegrees = 60.0f;
    const float degreesToRadians = 3.14159265358979323846f / 180.0f;
    float fovRadians = fovDegrees * degreesToRadians;
    float aspectRatio = 16.0f / 9.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;
    entt::entity camera = CreateCamera(registry, fovRadians, aspectRatio, nearPlane, farPlane);

    // Upload model data to GPU
    vkDeviceWaitIdle(rendererComponent.device);
    if (!Pipeline::CreateCommandBuffersFor(rendererComponent)) 
    {
        printf("Failed to create command buffers after model upload\n");
    }

    // Define ImGui layer bits
    const uint8_t MAIN_MENU = 1 << 0;
    const uint8_t CREDITS   = 1 << 1;

    // Create and store global UI state
    auto& uiState = registry.ctx().emplace<UI::UIState>();
    uiState.visible = true;
    uiState.visibleLayers = UI::UILayer(MAIN_MENU);
    uiState.backgroundVisible = true;
    uiState.acceptsInput = true;

    // Build UI
    UI::BuildMainMenu(registry, MAIN_MENU, CREDITS);
    UI::BuildCreditsMenu(registry, CREDITS, MAIN_MENU);

    // Initialize ImGui layer 
    UI::InitializeImgui(registry);

    // game loop
    while (!glfwWindowShouldClose(rendererComponent.window))
    {
        CAMERA_SYSTEM::UpdateCameraAndUpload(registry, camera);

        // update credits scrolling 
        UI::UpdateCredits(registry);

        UI::RenderUI(registry, entity);
        Render();
        glfwPollEvents();
    }

    // Cleanup renderer resources
    CleanupFor(rendererComponent);
    registry.clear();
    return 0;
}
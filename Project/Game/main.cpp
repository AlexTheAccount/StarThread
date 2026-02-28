#include "../Engine/RENDERING/RenderingComponents.h"
#include "../Engine/IMGUI/ImguiComponents.h"
#include "GAME/GameComponents.h"
#include <iostream>
#include <chrono>
#include <entt/entt.hpp>

using namespace RENDERING::RENDERER_HELPERS;
using namespace RENDERING::CAMERA_SYSTEM;

using Registry = entt::registry;

int main()
{
    // All components, tags, and systems are stored in a single registry
    entt::registry& registry = ENGINE::GlobalRegistry();

    // Seed the rand
    unsigned int time = std::chrono::steady_clock::now().time_since_epoch().count();
    srand(time);

    registry.ctx().emplace<UTILITIES::Config>();
    registry.ctx().emplace<UTILITIES::Input>();
    registry.ctx().emplace<RENDERING::ModelManager>();

    // Per-subsystem initializers
    RENDERING::InitializeGraphics(registry); // create windows, surfaces, and renderers
    UI::InitializeImgui(registry);   // initialize ImGui layer
    GAME::InitializeGameplay(registry); // create entities and components for gameplay
    
    // Create Camera entity
    float fovDegrees = 60.0f;
    const float degreesToRadians = 3.14159265358979323846f / 180.0f;
    float fovRadians = fovDegrees * degreesToRadians;
    float aspectRatio = 16.0f / 9.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;
    entt::entity camera = CreateCamera(registry, fovRadians, aspectRatio, nearPlane, farPlane);

    // Main loop runs until all windows close
    GAME::RunMainLoop(registry); // update windows and input

    for (auto entity : registry.view<RENDERING::RendererComponent>())
    {
        RENDERING::RendererComponent& rendererComponent = registry.get<RENDERING::RendererComponent>(entity);

        // free Imgui resources before renderer resources, they depend on the renderer's descriptor pool
        if (auto imguiPtr = registry.ctx().find<UI::ImguiLayer>(); imguiPtr && imguiPtr->IsInitialized())
        {
            imguiPtr->Shutdown();
        }

        CleanupFor(rendererComponent);
        registry.remove<RENDERING::RendererComponent>(entity);
    }

    registry.clear();
    return 0; // now destructors will be called for all components
}
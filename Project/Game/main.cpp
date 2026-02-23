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
    entt::registry registry;

    // Seed the rand
    unsigned int time = std::chrono::steady_clock::now().time_since_epoch().count();
    srand(time);

    registry.ctx().emplace<UTILITIES::Config>();

    registry.ctx().emplace<RENDERING::ModelManager>();

    // Per-subsystem initializers
    RENDERING::InitializeGraphics(registry); // create windows, surfaces, and renderers
    UI::InitializeImgui(registry);   // initialize ImGui layer
    GAME::InitializeGameplay(registry); // create entities and components for gameplay

    // Main loop runs until all windows close
    GAME::RunMainLoop(registry); // update windows and input

    for (auto entity : registry.view<RENDERING::RendererComponent>())
    {
        RENDERING::RendererComponent& rendererComponent = registry.get<RENDERING::RendererComponent>(entity);
        CleanupFor(rendererComponent);
        registry.remove<RENDERING::RendererComponent>(entity);
    }

    registry.clear();
    return 0; // now destructors will be called for all components
}
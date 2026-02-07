#include "../Engine/RENDERING/RenderingComponents.h"
#include "GAME/GlobalRegistry.h"
#include <iostream>
#include <chrono>
#include <entt/entt.hpp>

using namespace RENDERING::RENDERER_HELPERS;

int main()
{
    // Seed the rand
    unsigned int time = std::chrono::steady_clock::now().time_since_epoch().count();
    srand(time);

    // store everything related to entities and components in the game registry
    auto& registry = GlobalRegistry();
    auto entity = registry.create();

    registry.emplace<RENDERING::RendererComponent>(entity);
    auto& rendererComponent = registry.get<RENDERING::RendererComponent>(entity);

    // Initialize engine for the renderer instance owned by the game
    if (!InitializeFor(rendererComponent, 800, 600, "StarThread Vulkan"))
    {
        std::cerr << "Failed to initialize renderer\n";
        return -1;
    }

    // game loop
    bool running = true;
    while (running)
    {
        RenderFor(rendererComponent);

        if (glfwWindowShouldClose(rendererComponent.window))
            running = false;
    }

    // Cleanup renderer resources
    CleanupFor(rendererComponent);
    registry.clear();
    return 0;
}
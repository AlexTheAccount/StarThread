#include "RENDERING/RenderingComponents.h"
#include <GAME/GlobalRegistry.h>
#include <iostream>
#include <chrono>
#include <entt/entt.hpp>

using namespace RENDERING::RENDERER_HELPERS;

int main()
{
    // Seed the rand
    unsigned int time = std::chrono::steady_clock::now().time_since_epoch().count();
    srand(time);

    // store everything related to entities and components in a single registry
    auto& registry = GlobalRegistry();
    auto entity = registry.create();
    registry.emplace<RENDERING::RendererComponent>(entity);
    auto& rendererComponent = registry.get<RENDERING::RendererComponent>(entity);
    //bool InitializeWindowSuccess = InitializeWindow(800, 600, "StarThread Vulkan");
    bool InitializeSuccess = Initialize(800, 600, "StarThread Vulkan");
    
    // game loop
    while (true)
    {
        // update systems
        RenderingLoop();
    }

    // stops game
    // registry.clear();
    // return 0;
}
#include "RENDERING/RenderingComponents.h"
#include <GAME/GlobalRegistry.h>
#include <iostream>
#include <chrono>
#include <entt/entt.hpp>

int main()
{
    // Seed the rand
    unsigned int time = std::chrono::steady_clock::now().time_since_epoch().count();
    srand(time);

    // store everything related to entities and components in a single registry
    auto& registry = GlobalRegistry();
    auto entity = registry.create();
    registry.emplace<RENDERING::RendererComponent>(entity);

    // stops game
    // registry.clear();
    // return 0;
}
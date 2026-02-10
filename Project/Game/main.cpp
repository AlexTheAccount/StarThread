#include "../Engine/RENDERING/RenderingComponents.h"
#include "GAME/GameComponents.h"
#include <iostream>
#include <chrono>
#include <entt/entt.hpp>

using namespace GAME;
using namespace RENDERING;
using namespace RENDERING::RENDERER_HELPERS;
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

    // Create player entity and attach the TestShip model
    entt::entity player = registry.create();
    registry.emplace<Player>(player);
    AttachModelToEntity(registry, player, "TestShip");

    // Recreate / re-record command buffers so draw calls reference the newly-created vertex/index buffers
    if (!Pipeline::CreateCommandBuffersFor(rendererComponent)) 
    {
        printf("Failed to create command buffers after model upload\n");
    }

    // game loop
    while (!glfwWindowShouldClose(rendererComponent.window))
    {
        Render();
        glfwPollEvents();
    }

    // Cleanup renderer resources
    CleanupFor(rendererComponent);
    registry.clear();
    return 0;
}
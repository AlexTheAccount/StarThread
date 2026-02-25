#include "../GAME/GameComponents.h"
#include "../../Engine/RENDERING/RenderingComponents.h"
#include "../../Engine/IMGUI/ImguiComponents.h"
#include <chrono>

void GAME::RunMainLoop(entt::registry& registry)
{
    using clock = std::chrono::steady_clock;

    // Ensure DeltaTime exists
    if (!registry.ctx().contains<UTILITIES::DeltaTime>())
        registry.ctx().emplace<UTILITIES::DeltaTime>();

    UTILITIES::DeltaTime& deltaContext = registry.ctx().get<UTILITIES::DeltaTime>();

    std::chrono::steady_clock::time_point lastTime = clock::now();
    
    while (true)
    {
        // Delta time
        std::chrono::steady_clock::time_point now = clock::now();
        float deltaTime = std::chrono::duration_cast<std::chrono::duration<float>>(now - lastTime).count();
        lastTime = now;
        deltaContext.dtSec = deltaTime;

        // Poll window events
        glfwPollEvents();

        // Game update
        UpdateGameManager(registry);

        // Render / UI per renderer
        bool anyWindowOpen = false;
        for (entt::entity entity : registry.view<RENDERING::RendererComponent>())
        {
            RENDERING::RendererComponent& renderer = registry.get<RENDERING::RendererComponent>(entity);
            if (!renderer.window) continue;

            // If any window is still open, keep running
            if (!glfwWindowShouldClose(renderer.window))
                anyWindowOpen = true;

            // Start ImGui frame (if initialized)
            if (UI::ImguiLayer* imguiPtr = registry.ctx().find<UI::ImguiLayer>(); imguiPtr && imguiPtr->IsInitialized())
            {
                UI::RenderUI(registry, entity);
            }

            // Bind renderer and issue draw for this instance
            RENDERING::RENDERER_HELPERS::BindRenderer(&renderer);
            RENDERING::RENDERER_HELPERS::RenderFor(renderer);
            RENDERING::RENDERER_HELPERS::UnbindRenderer();
        }

        if (!anyWindowOpen)
            break;
    }

    // Wait for GPU to finish for each renderer before returning control to main cleanup
    for (entt::entity entity : registry.view<RENDERING::RendererComponent>())
    {
        RENDERING::RendererComponent& renderer = registry.get<RENDERING::RendererComponent>(entity);
        if (renderer.device != VK_NULL_HANDLE)
            vkDeviceWaitIdle(renderer.device);
    }
}
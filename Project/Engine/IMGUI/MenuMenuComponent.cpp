#include "../IMGUI/ImguiComponents.h"
#include "../RENDERING/RenderingComponents.h"
#include "../../Game/GAME/GameComponents.h"

namespace UI
{
    void BuildMainMenu(entt::registry& registry)
    {
        uint8_t MAIN_MENU = 9;
        uint8_t CREDITS_MENU = 9;

        for (entt::entity entity : registry.view<UI::UILayerBit>())
        {
            UI::UILayerBit& layer = registry.get<UI::UILayerBit>(entity);

            if (layer.name == "Main Menu")
            {
                MAIN_MENU = layer.bit;
            }

            if (layer.name == "Credits Menu")
            {
                CREDITS_MENU = layer.bit;
            }
        }

        if (MAIN_MENU == 9 || CREDITS_MENU == 9)
        {
            printf("Error: Main Menu or Credits Menu layer not found in registry context.\n");
            return;
        }

        UI::UIBuilder builder(UI::UILayer(MAIN_MENU), registry);

        // Centered start button
        builder.Push<UI::Button>
        (
            {
                .size = float2(0.30f, 0.08f),
                .position = float2(0.35f, 0.30f),
                .positionMode = UI::PositionMode::Relative,
                .sizeMode = UI::PositionMode::Relative
            },
            {
                .label = "Start",
                .onClick = [](UI::Button& button, entt::registry& registry, entt::entity)
                {
                    // Transition to Playing
                    GAME::SetGameState(registry, GAME::GameState::Playing);
                }
            }
        );

        // Options button (placeholder)
        builder.Push<UI::Button>
        (
            {
                .size = float2(0.30f, 0.08f),
                .position = float2(0.35f, 0.42f),
                .positionMode = UI::PositionMode::Relative,
                .sizeMode = UI::PositionMode::Relative
            },
            {
                .label = "Options",
                .onClick = [](UI::Button& button, entt::registry& registry, entt::entity)
                {
                    // TODO: options UI
                }
            }
        );

        // Credits button -> enable credits layer
        builder.Push<UI::Button>
        (
            {
                .size = float2(0.30f, 0.08f),
                .position = float2(0.35f, 0.54f),
                .positionMode = UI::PositionMode::Relative,
                .sizeMode = UI::PositionMode::Relative
            },
            {
                .label = "Credits",
                .onClick = [CREDITS_MENU](UI::Button& button, entt::registry& registry, entt::entity)
                {
                    // Show credits layer and hide main menu layer
                    auto& state = registry.ctx().get<UI::UIState>();
                    state.visibleLayers = UI::UILayer(CREDITS_MENU);
                }
            }
        );

        // Quit button
        builder.Push<UI::Button>
        (
            {
                .size = float2(0.30f, 0.08f),
                .position = float2(0.35f, 0.66f),
                .positionMode = UI::PositionMode::Relative,
                .sizeMode = UI::PositionMode::Relative
            },
            {
                .label = "Quit",
                .onClick = [](UI::Button& button, entt::registry& registry, entt::entity)
                {
                    // Find renderer entity to access GLFW window and request close
                    auto view = registry.view<RENDERING::RendererComponent>();
                    if (view.begin() != view.end())
                    {
                        auto rEntity = view.front();
                        auto& renderer = registry.get<RENDERING::RendererComponent>(rEntity);
                        if (renderer.window)
                            glfwSetWindowShouldClose(renderer.window, GLFW_TRUE);
                    }
                }
            }
        );
    }
} // namespace UI
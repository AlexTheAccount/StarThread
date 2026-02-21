#include "GameComponents.h"
#include "../Engine/IMGUI/ImguiComponents.h"
#include "../../Engine/RENDERING/RenderingComponents.h"
#include <cstdio>

using namespace GAME;

namespace GAME
{
    GameState GetGameState(entt::registry& registry)
    {
        if (registry.ctx().contains<GameStateContext>())
            return registry.ctx().get<GameStateContext>().state;
        return GameState::MainMenu;
    }

    void SetGameState(entt::registry& registry, GameState newState)
    {
        if (!registry.ctx().contains<GameStateContext>())
            registry.ctx().emplace<GameStateContext>();
        auto& context = registry.ctx().get<GameStateContext>();
        GameState oldState = context.state;
        if (oldState == newState) return;
        context.state = newState;

        // Ensure UI state exists before toggling
        if (!registry.ctx().contains<UI::UIState>())
            registry.ctx().emplace<UI::UIState>();
        auto& ui = registry.ctx().get<UI::UIState>();

        // Exit / Enter hooks
        switch (newState)
        {
        case GameState::MainMenu:
            // Show UI and accept UI input
            ui.visible = true;
            ui.uiAcceptsInput = true;
            context.gameAcceptsInput = false;
            break;

        case GameState::Playing:
            // Hide UI and let game systems accept input
            ui.visible = false;
            ui.uiAcceptsInput = false;
            context.gameAcceptsInput = true;

            // If the game hasn't been started yet, start it
            {
                auto gmView = registry.view<GameManager>();
                if (gmView.empty())
                {
                    StartGame(registry);
                }

                // cache player entity if present
                auto playerView = registry.view<Player>();
                if (!playerView.empty()) context.player = playerView.front();
            }
            break;

        case GameState::Paused:
            // Show UI or pause overlay and prevent game updates
            ui.visible = true;
            ui.uiAcceptsInput = true;
            context.gameAcceptsInput = false;
            break;

        case GameState::GameOver:
            // show game over UI, stop game updates
            ui.visible = true;
            ui.uiAcceptsInput = true;
            context.gameAcceptsInput = false;
            break;
        }
    }
}
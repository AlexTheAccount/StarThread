#include "../IMGUI/ImguiComponents.h"

namespace UI
{
    struct CreditsLabel
    {
        float initialY = 0.0f;
    };

    void BuildCreditsMenu(entt::registry& registry)
    {
        uint8_t MAIN_MENU;
        uint8_t CREDITS_MENU;

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

        if (MAIN_MENU == 0 || CREDITS_MENU == 0)
        {
            printf("Error: Main Menu or Credits Menu layer not found in registry context.\n");
            return;
        }

        UI::UIBuilder builder(UI::UILayer(CREDITS_MENU), registry);

        constexpr std::array<std::string_view, 14> lines
        {
            "Producer: ",
            "",
            "Developers:",
            "Alexander Swanson",
            "",
            "Music and Sound Effects:",
            "",
            "Thank you for playing!"
        };

        float startY = 1.05f;
        float spacing = 0.07f;

        for (int line = 0; line < lines.size(); ++line)
        {
            auto entity = registry.create();

            UI::GUIItem guiItem;
            guiItem.size = float2(1.0f, 0.06f);
            guiItem.position = float2(0.0f, startY + static_cast<float>(line) * spacing);
            guiItem.positionMode = UI::PositionMode::Relative;
            guiItem.sizeMode = UI::PositionMode::Relative;
            guiItem.state = UI::UIVisibility{ true, UI::UILayer(CREDITS_MENU) };

            registry.emplace<UI::GUIItem>(entity, guiItem);
            registry.emplace<UI::Label>(entity, UI::Label{ (std::string) lines[line], UI::LabelPositioningMode::Center });
            registry.emplace<CreditsLabel>(entity, CreditsLabel{ guiItem.position.y });
        }

        // Back button
        builder.Push<UI::Button>
        (
            {
                .size = float2(0.25f, 0.08f),
                .position = float2(0.01f, 0.85f)
            },
            {
                .label = "Back",
                .onClick = [MAIN_MENU](UI::Button& button, entt::registry& registry, entt::entity)
                {
                    // Return to main menu
                    UI::UIState& state = registry.ctx().get<UI::UIState>();
                    state.visibleLayers = UI::UILayer(MAIN_MENU);

                    // stop credits
                    if (registry.ctx().contains<CreditsState>())
                        registry.ctx().get<CreditsState>().active = false;
                }
            }
        );

        // Create/initialize credits state
        auto& creditsState = registry.ctx().emplace<CreditsState>();
        creditsState.offset = 0.0f;
        creditsState.speed = 0.12f;
        creditsState.lines = lines.size();
        creditsState.active = false;
    }

    void UpdateCredits(entt::registry& registry)
    {
        if (!registry.ctx().contains<CreditsState>())
            return;

        auto& creditsState = registry.ctx().get<CreditsState>();
        if (!creditsState.active)
            return;

        // delta time
        float deltaTime = static_cast<float>(registry.ctx().get<UTILITIES::DeltaTime>().dtSec);

        creditsState.offset += creditsState.speed * deltaTime;

        // Update all credits GUIItems | scroll upward
        auto view = registry.view<CreditsLabel, UI::GUIItem>();
        for (auto entity : view)
        {
            auto& creditsLabel = view.get<CreditsLabel>(entity);
            auto& gui = view.get<UI::GUIItem>(entity);
            gui.position.y = creditsLabel.initialY - creditsState.offset;
        }
    }
} // namespace UI
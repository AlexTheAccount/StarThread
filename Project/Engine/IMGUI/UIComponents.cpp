#include "../IMGUI/ImguiComponents.h"
#include "../ENGINE/EngineComponents.h"
#include "../RENDERING/RenderingComponents.h"
#include <imgui.h>

namespace UI
{
    void DrawLabel(const std::string& label)
    {
        if (!label.empty())
        {
            ImGui::Text(label.c_str());
            ImGui::SameLine();
        }
    }

    // @return Size of the draw area
    float2 SetDrawPosition(const GUIItem& item)
    {
        float2 size = NormalizePosition(item.size, item.sizeMode);
        float2 position = NormalizePosition(item.position, item.positionMode);
        ImGui::SetCursorScreenPos(ImVec2(position.x, position.y));
        return size;
    }

    void DrawButton(entt::registry& registry, entt::entity entity, const GUIItem& item, Button& button)
    {
        // get imgui layer
        auto imguiPtr = registry.ctx().find<ImguiLayer>();
        if (!imguiPtr)
        {
            printf("IMGUI::DrawButton: ImguiLayer not found in registry context.\n");
            return;
        }

        float2 size = SetDrawPosition(item);

        ImGui::BeginDisabled(item.disabled);

        bool clicked = false;
        if (size.x == 0.0f && size.y == 0.0f)
            clicked = ImGui::Button(button.label.c_str());
        else
            clicked = ImGui::Button(button.label.c_str(), ImVec2(size.x, size.y));

        if (clicked)
        {
            registry.ctx().get<ENGINE::SoundBank>().Play("button");
            
            if (button.onClick)
                button.onClick(button, registry, entity);
        }

        if (!imguiPtr->IsInitialized())
            return;

        ImGui::EndDisabled();
    }

    void DrawTextField(entt::registry& registry, entt::entity entity, const GUIItem& item, TextField& field)
    {
        // get imgui layer
        auto imguiPtr = registry.ctx().find<ImguiLayer>();
        if (!imguiPtr)
        {
            printf("IMGUI::DrawTextField: ImguiLayer not found in registry context.\n");
            return;
        }

        float2 size = SetDrawPosition(item);
        DrawLabel(field.label);

        ImGui::BeginDisabled(item.disabled);

        // ## Hides the label.
        std::string inputLabelName = "##" + field.label;
        ImGui::SetNextItemWidth(size.x > 0 ? size.x : -1.f);
        if (ImGui::InputText(inputLabelName.c_str(), field.value.data(), field.value.capacity() + 1, ImGuiInputTextFlags_EnterReturnsTrue));
            if (field.onUpdate) field.onUpdate(field, registry, entity);

        if (!imguiPtr->IsInitialized())
            return;

        ImGui::EndDisabled();
    }

    void DrawSlider(entt::registry& registry, entt::entity entity, const GUIItem& item, FloatSlider& slider)
    {
        // get imgui layer
        auto imguiPtr = registry.ctx().find<ImguiLayer>();
        if (!imguiPtr)
        {
            printf("IMGUI::DrawSlider: ImguiLayer not found in registry context.\n");
            return;
        }

        float2 size = SetDrawPosition(item);
        DrawLabel(slider.label);

        ImGui::BeginDisabled(item.disabled);

        std::string inputLabelName = "##" + slider.label;
        ImGui::SetNextItemWidth(size.x > 0 ? size.x : -1.f);

        bool changed = ImGui::SliderFloat(inputLabelName.c_str(), &slider.value, slider.min, slider.max);
        if (changed)
        {
            if (slider.onUpdate) slider.onUpdate(slider, registry, entity);

            auto& bank = registry.ctx().get<ENGINE::SoundBank>();
            auto it = bank.sounds.find("button");
            if (it != bank.sounds.end())
            {
                bool playing = false;
                it->second->isPlaying(playing);
                if (!playing)
                    it->second->Play();
            }
        }

        if (!imguiPtr->IsInitialized())
            return;

        ImGui::EndDisabled();
    }

    void DrawLabel(entt::registry& registry, const GUIItem& item, Label& label)
    {
        // get imgui layer
        auto imguiPtr = registry.ctx().find<ImguiLayer>();
        if (!imguiPtr)
        {
            printf("IMGUI::DrawLabel: ImguiLayer not found in registry context.\n");
            return;
        }

        float2 size = NormalizePosition(item.size, item.sizeMode);
        float2 position = NormalizePosition(item.position, item.positionMode);
        
        if (label.mode == LabelPositioningMode::Right)
        {
            float width = ImGui::CalcTextSize(label.text.c_str()).x;
            position.x += size.x - width;
        }
        else if (label.mode == LabelPositioningMode::Center)
        {
            float width = ImGui::CalcTextSize(label.text.c_str()).x;
            position.x += (size.x - width) / 2.f;
        }

        if (!imguiPtr->IsInitialized())
            return;

        ImGui::SetCursorScreenPos(ImVec2(position.x, position.y));
        ImGui::Text(label.text.c_str());
    }

    void DrawDropdown(entt::registry& registry, entt::entity entity, const GUIItem& item, Dropdown& dropdown)
    {
        // get imgui layer
        auto imguiPtr = registry.ctx().find<ImguiLayer>();
        if (!imguiPtr)
        {
            printf("IMGUI::DrawDropdown: ImguiLayer not found in registry context.\n");
            return;
        }

        float2 size = SetDrawPosition(item);
        DrawLabel(dropdown.label);

        std::string inputLabelName = "##" + dropdown.label;
        ImGui::SetNextItemWidth(size.x > 0 ? size.x : -1.f);

        if (!dropdown.toString)
        {
            if (ImGui::BeginCombo(inputLabelName.c_str(), "ERROR"))
                ImGui::EndCombo();
            return;
        }

        std::string preview = dropdown.toString(dropdown.index).value_or("ERROR");

        ImGui::BeginDisabled(item.disabled);

        if(!ImGui::BeginCombo(inputLabelName.c_str(), preview.c_str()))
            return ImGui::EndDisabled();

        for (uint8_t i = 0; i < UINT8_MAX; i++)
        {
            std::optional<std::string> asString = dropdown.toString(i);

            if (!asString.has_value())
                break;

            bool isSelected = dropdown.index == i;

            if (ImGui::Selectable(asString.value().c_str(), isSelected))
            {
                uint8_t wasSelected = dropdown.index == i;
                dropdown.index = i;
                if (!wasSelected && dropdown.onUpdate)
                    dropdown.onUpdate(dropdown, registry, entity);
            }

            if (isSelected)
                ImGui::SetItemDefaultFocus();
        }

        if (!imguiPtr->IsInitialized())
            return;

        ImGui::EndCombo();

        ImGui::EndDisabled();
    }

    void DrawCheckbox(entt::registry& registry, entt::entity entity, const GUIItem& item, Checkbox& checkbox)
    {
        // get imgui layer
        auto imguiPtr = registry.ctx().find<ImguiLayer>();
        if (!imguiPtr)
        {
            printf("IMGUI::DrawCheckbox: ImguiLayer not found in registry context.\n");
            return;
        }

        float2 size = SetDrawPosition(item);
        DrawLabel(checkbox.label);

        ImGui::BeginDisabled(item.disabled);

        std::string inputLabelName = "##" + checkbox.label;
        ImGui::SetNextItemWidth(size.x > 0 ? size.x : -1.f);
        if (ImGui::Checkbox(inputLabelName.c_str(), &checkbox.value))
            if (checkbox.onUpdate) checkbox.onUpdate(checkbox, registry, entity);

        if (!imguiPtr->IsInitialized())
            return;

        ImGui::EndDisabled();
    }

    float2 NormalizePosition(float2 a, PositionMode mode)
    {
        if (mode == PositionMode::Absolute) return a;

        return
        {
            a.x * ImGui::GetIO().DisplaySize.x,
            a.y * ImGui::GetIO().DisplaySize.y
        };
    }

    void RenderUI_Internal(entt::registry& registry, entt::entity vkEntity)
    {
        // get imgui layer
        auto imguiPtr = registry.ctx().find<ImguiLayer>();
        if (!imguiPtr)
        {
            printf("IMGUI::RenderUI_Internal: ImguiLayer not found in registry context.\n");
            return;
        }

        UIState& globalState = registry.ctx().get<UIState>();

        ImGuiIO& io = ImGui::GetIO();
        io.WantCaptureKeyboard = globalState.acceptsInput;
        io.ConfigFlags = globalState.acceptsInput ?
            io.ConfigFlags | ImGuiConfigFlags_NavEnableGamepad :
            io.ConfigFlags & ~ImGuiConfigFlags_NavEnableKeyboard;

        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize;
        if (!globalState.backgroundVisible)
            windowFlags |= ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::Begin("MainWindow", 0, windowFlags);

        // gather all buttons into a vector for sorting
        auto view = registry.view<GUIItem>();
        for (auto entity : view)
        {
            auto& item = registry.get<GUIItem>(entity);

            if (!static_cast<bool>(globalState & item.state))
                continue;

            if (auto* button = registry.try_get<Button>(entity))
                DrawButton(registry, entity, item, *button);
            else if (auto* field = registry.try_get<TextField>(entity))
                DrawTextField(registry, entity, item, *field);
            else if (auto* slider = registry.try_get<FloatSlider>(entity))
                DrawSlider(registry, entity, item, *slider);
            else if (auto* label = registry.try_get<Label>(entity))
                DrawLabel(registry, item, *label);
            else if (auto* dropdown = registry.try_get<Dropdown>(entity))
                DrawDropdown(registry, entity, item, *dropdown);
            else if (auto* checkbox = registry.try_get<Checkbox>(entity))
                DrawCheckbox(registry, entity, item, *checkbox);
        }

        if (!imguiPtr->IsInitialized())
            return;

        ImGui::End();
        ImGui::PopStyleVar(1);
    }
    void RenderUI(entt::registry& registry, entt::entity vkEntity)
    {
        // get imgui layer
        auto imguiPtr = registry.ctx().find<ImguiLayer>();
        if (!imguiPtr)
        {
            printf("IMGUI::RenderButtons: ImguiLayer not found in registry context.\n");
            return;
        }

        if (!imguiPtr->IsInitialized())
            return;

        imguiPtr->BeginFrame();

        // ensure context still exists
        if (!ImGui::GetCurrentContext()) 
        {
            printf("RenderUI: ImGui context missing after BeginFrame\n");
            return;
        }
        
        // ensure required ctx entries exist
        auto inputPtr = registry.ctx().find<UTILITIES::Input>();
        if (!inputPtr) 
        {
            printf("RenderUI: Input not found in registry context.\n");
            return;
        }
        auto uiStatePtr = registry.ctx().find<UIState>();
        if (!uiStatePtr) 
        {
            printf("RenderUI: UIState not found in registry context.\n");
            return;
        }

        // read gamepad state from registry context
        auto& input = *inputPtr;
        auto& uiState = *uiStatePtr;

        // controller support
        {
            ImGuiIO& io = ImGui::GetIO();
            io.BackendFlags |= ImGuiBackendFlags_HasGamepad;

            // read current analog/button values
            float left = 0.0f, right = 0.0f, up = 0.0f, down = 0.0f;
            float south = 0.0f, east = 0.0f;

            input.gamePads.GetState(0, G_DPAD_LEFT_BTN, left);
            input.gamePads.GetState(0, G_DPAD_RIGHT_BTN, right);
            input.gamePads.GetState(0, G_DPAD_UP_BTN, up);
            input.gamePads.GetState(0, G_DPAD_DOWN_BTN, down);

            input.gamePads.GetState(0, G_SOUTH_BTN, south);
            input.gamePads.GetState(0, G_EAST_BTN, east);

            // prevents a single press from scrolling across multiple items in one frame
            auto forwardEdge = [&](ImGuiKey key, float currentValue, float& previousValue)
            {
                const float threshold = 0.5f;
                bool currentPressed = currentValue >= threshold;
                bool previousPressed = previousValue >= threshold;

                if (currentPressed != previousPressed)
                {
                    ImGui::GetIO().AddKeyAnalogEvent(key, currentPressed, currentValue);
                }

                previousValue = currentValue;
            };

            forwardEdge(ImGuiKey_GamepadDpadLeft, left, uiState.previousDpadLeft);
            forwardEdge(ImGuiKey_GamepadDpadRight, right, uiState.previousDpadRight);
            forwardEdge(ImGuiKey_GamepadDpadUp, up, uiState.previousDpadUp);
            forwardEdge(ImGuiKey_GamepadDpadDown, down, uiState.previousDpadDown);

            forwardEdge(ImGuiKey_GamepadFaceDown, south, uiState.previousSouth);
            forwardEdge(ImGuiKey_GamepadFaceRight, east, uiState.previousEast);
        }

        if(static_cast<bool>(uiState))
            RenderUI_Internal(registry, vkEntity);

        // retrieve current command buffer and hand to ImguiLayer
        if (!registry.valid(vkEntity) || !registry.all_of<RENDERING::RendererComponent>(vkEntity))
        {
            printf("RenderUI: invalid or missing VulkanRenderer for entity %u\n", vkEntity);
            return;
        }
        auto& vkRenderer = registry.get<RENDERING::RendererComponent>(vkEntity);
        unsigned int currentBuffer = 0;
        if (vkRenderer.commandBuffers.size() > vkRenderer.currentFrame)
            currentBuffer = vkRenderer.currentFrame;
        else
        {
            printf("RenderUI: currentFrame index %zu out of bounds for commandBuffers size %zu\n", 
                vkRenderer.currentFrame, vkRenderer.commandBuffers.size());
            return;
        }
    }
}
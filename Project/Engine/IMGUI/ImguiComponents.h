#pragma once
#include <string>
#include "../UTILITIES/UtilityComponents.h"
#include <entt/entt.hpp>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

using UTILITIES::float2;

namespace UI
{
    ///*** Components ***///
    struct UILayer
    {
        bool layer1 : 1;
        bool layer2 : 1;
        bool layer3 : 1;
        bool layer4 : 1;
        bool layer5 : 1;
        bool layer6 : 1;
        bool layer7 : 1;
        bool layer8 : 1;

        UILayer() : UILayer(0) {}
        explicit UILayer(uint8_t flags) { *this = std::_Bit_cast<UILayer>(flags); }

        explicit operator uint8_t() const noexcept { return std::_Bit_cast<uint8_t>(*this); } // convert to uint8_t for bitwise operations
        explicit operator bool() const noexcept { return static_cast<bool>(static_cast<uint8_t>(*this)); }
        UILayer operator&(UILayer layer) const noexcept { return UILayer(static_cast<uint8_t>(*this) & static_cast<uint8_t>(layer)); }
        UILayer operator|(UILayer layer) const noexcept { return UILayer(static_cast<uint8_t>(*this) | static_cast<uint8_t>(layer)); }
    };

    struct UIVisibility
    {
        bool visible = true;
        UILayer visibleLayers = UILayer(UINT8_MAX);

        explicit operator bool() const noexcept
        {
            return visible && static_cast<bool>(visibleLayers);
        }

        UIVisibility operator&(const UIVisibility visibility) const noexcept
        {
            return
            {
                visible && visibility.visible,
                visibleLayers & visibility.visibleLayers
            };
        }
    };

    struct UIState : UIVisibility
    {
        bool backgroundVisible = true;
        bool uiAcceptsInput = true;

        // Track previous controller axis/button values to perform edge detection
        float previousDpadLeft = 0.0f;
        float previousDpadRight = 0.0f;
        float previousDpadUp = 0.0f;
        float previousDpadDown = 0.0f;
        float previousSouth = 0.0f;
        float previousEast = 0.0f;
    };

    struct CreditsState
    {
        float offset = 0.0f;
        float speed = 0.12f;
        int lines = 0;
        bool active = false;
        bool uiAcceptsInput = true;
    };

    enum class PositionMode
    {
        Relative,
        Absolute
    };

    float2 NormalizePosition(float2 a, PositionMode mode);

    struct GUIItem
    {
        float2 size = 1.0f;
        float2 position = 0.0f;
        PositionMode positionMode = PositionMode::Relative;
        PositionMode sizeMode = PositionMode::Relative;
        bool disabled = false;

        UIVisibility state = {};
    };
    
    template<class Type>
    using UIFunc = std::function<void(Type& type, entt::registry& registry, entt::entity entity)>;

    struct Button
    {
        std::string label = "Button";
        UIFunc<Button> onClick = nullptr;
    };

    struct TextField
    {
        std::string label;
        std::string value;

        UIFunc<TextField> onUpdate;
    };

    enum class LabelPositioningMode : uint8_t
    {
        Left,
        Center,
        Right
    };

    struct Label
    {
        std::string text;
        LabelPositioningMode mode = LabelPositioningMode::Left;
    };

    struct Dropdown
    {
        std::string label;
        uint64_t index = 0;
        
        std::function<std::optional<std::string>(uint64_t)> toString;
        UIFunc<Dropdown> onUpdate;
    };

    template<typename Type>
    struct Slider
    {
        static_assert(std::numeric_limits<Type>::is_specialized, "Type must be a numeric type!");

        std::string label;
        Type min = (std::numeric_limits<Type>::lowest)();
        Type max = (std::numeric_limits<Type>::max)();

        Type value = {};

        UIFunc<Slider<Type>> onUpdate;
    };

    struct Checkbox
    {
        std::string label;
        bool value;

        UIFunc<Checkbox> onUpdate;
    };

    using IntSlider = Slider<int32_t>;
    using FloatSlider = Slider<float>;

    //*** CLASSES ***//
    class UIBuilder
    {
    public:
        UIBuilder(UILayer layer, entt::registry& registry) : layer(layer), registry(&registry) {};

        template<class Type>
        UIBuilder& Push(GUIItem&& guiItem, Type&& item)
        {
            entt::entity entity;
            Push(std::move(guiItem), std::forward<Type>(item), entity);
            return *this;
        }

        template<class Type>
        UIBuilder& Push(GUIItem&& baseItem, Type&& item, entt::entity& outEntity)
        {
            outEntity = registry->create();

            baseItem.state = { true, layer };

            registry->emplace<UI::GUIItem>(outEntity, std::move(baseItem));
            registry->emplace<Type>(outEntity, std::forward<Type>(item));

            return *this;
        }

    private:
        UILayer layer;
        entt::registry* registry;
    };

    class ImguiLayer
    {
    public:
        void Initialize(entt::registry& registry,
            VkInstance instance,
            VkPhysicalDevice physicalDevice,
            VkDevice device,
            uint32_t graphicsQueueFamily,
            VkQueue graphicsQueue,
            VkDescriptorPool descriptorPool,
            VkRenderPass renderPass,
            uint32_t minImageCount,
            GLFWwindow* window);

        bool BeginFrame();
        void EndFrame(VkCommandBuffer commandBuffer);
        void Shutdown();
        void CleanupVulkanResources();
        bool IsInitialized() const { return initialized; }

    private:
        bool initialized{ false };
        int minImageCount{ 2 };

        // GLFW integration for input
        GLFWwindow* window{ nullptr };

        // Vulkan objects
        VkDevice device{ VK_NULL_HANDLE };
        VkDescriptorPool descriptorPool{ VK_NULL_HANDLE };
        VkRenderPass renderPass{ VK_NULL_HANDLE };

    };

    ///*** Functions ***///
    void InitializeImgui(entt::registry& registry);
    void RenderUI(entt::registry& registry, entt::entity vkEntity);

    void BuildMainMenu(entt::registry& registry, uint8_t MAIN_MENU, uint8_t CREDITS);
    void BuildCreditsMenu(entt::registry& registry, uint8_t CREDITS, uint8_t MAIN_MENU);
    void UpdateCredits(entt::registry& registry);

} // namespace IMGUI
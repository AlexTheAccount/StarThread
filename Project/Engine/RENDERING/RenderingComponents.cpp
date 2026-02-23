#include "RenderingComponents.h"

namespace RENDERING
{
    void SetDebugName(VkDevice device, VkObjectType objectType, uint64_t objectHandle, const char* name)
    {
        if (!device || !name) return;
        auto function = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT");
        if (!function) return;

        VkDebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = objectType;
        nameInfo.objectHandle = objectHandle;
        nameInfo.pObjectName = name;
        function(device, &nameInfo);
    }

    void InitializeGraphics(entt::registry& registry)
    {
        // Create an entity that will own the RendererComponent
        entt::entity rendererEntity = registry.create();
        registry.emplace<RendererComponent>(rendererEntity);
        RendererComponent& renderer = registry.get<RendererComponent>(rendererEntity);

        // Default window size and title
        const uint32_t width = 1280;
        const uint32_t height = 720;
        const char* title = "StarThread";

        // Initialize window + Vulkan for this renderer instance | InitializeFor will BindRenderer internally
        if (!RENDERER_HELPERS::InitializeFor(renderer, width, height, title))
        {
            printf("RENDERING::InitializeGraphics: failed to initialize renderer for entity %u.\n", static_cast<uint32_t>(rendererEntity));
            // If initialization fails, remove the component/entity to keep registry clean.
            if (registry.valid(rendererEntity))
            {
                registry.destroy(rendererEntity);
            }
            return;
        }

        // store the renderer entity in the registry context for easy lookup:
        registry.ctx().emplace<entt::entity>(rendererEntity);
    }
}
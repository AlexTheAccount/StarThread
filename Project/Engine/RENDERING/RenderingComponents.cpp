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

    void UpdateTransforms(entt::registry& registry)
    {
        auto transformView = registry.view<RENDERING::Transform>();
        for (auto entity : transformView)
        {
            auto& t = transformView.get<RENDERING::Transform>(entity);
            if (!t.recomputeWorld) continue;

            // collect chain up to root (or until an ancestor doesn't need recompute)
            std::vector<entt::entity> stack;
            entt::entity cur = entity;
            while (cur != entt::null)
            {
                auto& ct = registry.get<RENDERING::Transform>(cur);
                // break if ancestor already up-to-date (and not the starting entity)
                if (!ct.recomputeWorld && cur != entity) break;
                stack.push_back(cur);
                // cycle guard
                if (ct.parent == cur) break;
                cur = ct.parent;
            }

            // compute from root -> leaf
            for (auto it = stack.rbegin(); it != stack.rend(); ++it)
            {
                auto& ct = registry.get<RENDERING::Transform>(*it);
                ct.RecalculateWorld();

                if (ct.parent != entt::null)
                {
                    // multiply parent world into this world (parent * local)
                    auto& pt = registry.get<RENDERING::Transform>(ct.parent);
                    GW::MATH::GMatrix::MultiplyMatrixF(pt.world, ct.world, ct.world);
                }

                // Debug: print world rows for this transform after recalculation
                printf("UpdateTransforms: entity=%u world.row1 = %f %f %f %f\n", static_cast<uint32_t>(*it),
                       ct.world.row1.x, ct.world.row1.y, ct.world.row1.z, ct.world.row1.w);
                printf("UpdateTransforms: entity=%u world.row4 = %f %f %f %f\n", static_cast<uint32_t>(*it),
                       ct.world.row4.x, ct.world.row4.y, ct.world.row4.z, ct.world.row4.w);
            }
        }
    }
}
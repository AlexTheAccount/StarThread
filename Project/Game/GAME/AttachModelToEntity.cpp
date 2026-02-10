#include "GameComponents.h"
#include "../../Engine/RENDERING/RenderingComponents.h"

using namespace UTILITIES;
using namespace RENDERING;
using namespace GAME;

namespace GAME
{
    void AttachModelToEntity(entt::registry& registry, entt::entity entity, const std::string& modelName)
    {
        // Get path from config
        std::shared_ptr<const GameConfig> gameConfig;
        {
            auto* config = registry.ctx().find<Config>();
            if (!config) 
            {
                printf("No GameConfig found in registry context; cannot resolve model path\n");
                return;
            }
            gameConfig = config->gameConfig;
        }

        // Resolve model path
        std::string modelPath = "../Models";
        auto it = gameConfig->entries.find("modelPath");
        if (it != gameConfig->entries.end())
            modelPath = it->second;

        std::string fullFile = modelPath + "/" + modelName + ".fbx";

        // Load into CPU-side MeshManager
        uint32_t meshId = MeshManager::Instance().LoadMesh(modelName, fullFile);

        const MeshResource* mesh = MeshManager::Instance().GetMesh(meshId);
        if (!mesh) return;

        // Ensure a renderer is bound
        RendererComponent* renderer = RENDERER_HELPERS::GetGlobalRenderer();
        if (!renderer)
        {
            printf("No RendererComponent found in GlobalRegistry; cannot attach model to entity\n");
            return;
        }

        // Upload vertex/index data into the renderer GPU buffers
        RENDERER_HELPERS::Memory::CreateVertexAndIndexBuffersFor(*renderer, mesh->vertices, mesh->indices);
        renderer->indexCount = static_cast<size_t>(mesh->indices.size());

        // Attach rendering components to the entity
        registry.emplace_or_replace<RENDERING::MeshHandle>(entity, RENDERING::MeshHandle{ meshId });

        // Provide a basic Material if not present
        if (!registry.any_of<RENDERING::Material>(entity))
            registry.emplace<RENDERING::Material>(entity, RENDERING::Material{});

        // If the entity has no GAME::Transform, give it an identity transform
        if (!registry.any_of<GAME::Transform>(entity))
        {
            GW::MATH::GMATRIXF id = GW::MATH::GIdentityMatrixF;
            registry.emplace<GAME::Transform>(entity, GAME::Transform{ id });
        }
    }
}
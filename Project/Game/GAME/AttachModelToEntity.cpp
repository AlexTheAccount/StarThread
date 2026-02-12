#include "GameComponents.h"
#include "../../Engine/RENDERING/RenderingComponents.h"
#include <filesystem>

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
        std::string modelPath = "Models";
        auto it = gameConfig->entries.find("modelPath");
        if (it != gameConfig->entries.end())
            modelPath = it->second;

        std::filesystem::path filename = std::filesystem::path(modelName).replace_extension(".fbx");
        std::filesystem::path candidatePath = std::filesystem::path(modelPath) / filename;

        // If the file doesn't exist at the candidate path, try searching upward through parent directories for a Models folder
        if (!std::filesystem::exists(candidatePath))
        {
            std::filesystem::path currentPath = std::filesystem::current_path();
            bool found = false;
            for (std::filesystem::path path = currentPath; path.has_parent_path(); path = path.parent_path())
            {
                // Try configured modelPath relative to path
                std::filesystem::path tryPath = path / modelPath / filename;
                if (std::filesystem::exists(tryPath))
                {
                    candidatePath = tryPath;
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                printf("AttachModelToEntity: model file not found. Tried '%s'\n", candidatePath.string().c_str());
                return;
            }
        }

        // Debug: print resolved path
        printf("AttachModelToEntity: loading model '%s' from '%s'\n", modelName.c_str(), candidatePath.string().c_str());

        // Load into CPU-side MeshManager
        uint32_t meshId = MeshManager::Instance().LoadMesh(modelName, candidatePath.string());
        printf("AttachModelToEntity: MeshManager.LoadMesh returned id=%u\n", meshId);

        const MeshResource* mesh = MeshManager::Instance().GetMesh(meshId);
        if (!mesh) 
        {
            printf("AttachModelToEntity: GetMesh returned null for id=%u\n", meshId);
            return;
        }

        printf("AttachModelToEntity: mesh vertex count=%zu index count=%zu\n", mesh->vertices.size(), mesh->indices.size());

        // Ensure a renderer is bound
        RendererComponent* renderer = RENDERER_HELPERS::GetGlobalRenderer();
        if (!renderer)
        {
            printf("No RendererComponent found in GlobalRegistry; cannot attach model to entity\n");
            return;
        }

        // Debug: print vertexBuffer state before upload
        printf("AttachModelToEntity: renderer->vertexBuffer before upload = 0x%llx\n", (unsigned long long)renderer->vertexBuffer);

        // Upload vertex/index data into the renderer GPU buffers
        RENDERER_HELPERS::Memory::CreateVertexAndIndexBuffersFor(*renderer, mesh->vertices, mesh->indices);

        // Debug: print vertexBuffer state after upload
        printf("AttachModelToEntity: renderer->vertexBuffer after upload = 0x%llx\n", (unsigned long long)renderer->vertexBuffer);
        printf("AttachModelToEntity: renderer->indexBuffer after upload = 0x%llx indexCount=%zu\n",
            (unsigned long long)renderer->indexBuffer, renderer->indexCount);

        renderer->indexCount = static_cast<size_t>(mesh->indices.size());

        // Re-record command buffers so they bind the newly created vertex/index buffers
        if (!RENDERING::RENDERER_HELPERS::Pipeline::CreateCommandBuffersFor(*renderer))
        {
            printf("AttachModelToEntity: failed to re-record command buffers after mesh upload\n");
        }

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
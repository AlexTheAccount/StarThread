#include <entt/entt.hpp>
#include "../Engine/RENDERING/RenderingComponents.h"

using namespace RENDERING;
using namespace GW::MATH;

namespace RENDERING::CAMERA_SYSTEM
{
    // Create a camera entity
    entt::entity CreateCamera(entt::registry& registry, float fovRadians, float aspect, float nearZ, float farZ)
    {
        auto camera = registry.create();

        // Set projection matrix
        GW::MATH::GMATRIXF projection = GIdentityMatrixF;

        // build perspective projection matrix
        GMatrix::ProjectionVulkanRHF(fovRadians, aspect, nearZ, farZ, projection);

        registry.emplace<RENDERING::Camera>(camera, RENDERING::Camera{ GIdentityMatrixF, projection });

        // simple spatial representation on the camera 
        RENDERING::Transform transform;
        transform.position = { {0.0f, 0.0f, -6.0f, 0.0f} };
        transform.rotation = { {0.0f, 0.0f, 0.0f, 0.0f} };
        transform.scale = { {1.0f, 1.0f, 1.0f, 0.0f} };
        registry.emplace<RENDERING::Transform>(camera, transform);

        return camera;
    }

    // Per-frame: update camera view from its transform and upload into uniform buffer
    void UpdateCameraAndUpload(entt::registry& registry, entt::entity cameraEntity)
    {
        auto* renderer = RENDERER_HELPERS::GetGlobalRenderer();
        if (!renderer) return;

        auto& camera = registry.get<RENDERING::Camera>(cameraEntity);
        auto& transform = registry.get<RENDERING::Transform>(cameraEntity);

        // Build camera world matrix from transform
        GW::MATH::GMATRIXF world = GIdentityMatrixF;
        GW::MATH::GMATRIXF translate = GIdentityMatrixF;
        GW::MATH::GMatrix::TranslateGlobalF(world, transform.position, translate);
        world = translate;

        // View = inverse(world)
        GW::MATH::GMATRIXF view = GIdentityMatrixF;
        GW::MATH::GMatrix::InverseF(world, view);

        // transpose view and projection for GPU
        GW::MATH::GMATRIXF viewT = GIdentityMatrixF;
        GW::MATH::GMATRIXF projT = GIdentityMatrixF;
        GW::MATH::GMatrix::TransposeF(view, viewT);
        GW::MATH::GMatrix::TransposeF(camera.projection, projT);

        camera.view = view;

        // Prepare GPU cbuffer
        GPU_CBUFFER cbuffer{};
        cbuffer.World = GIdentityMatrixF;
        cbuffer.View = viewT;
        cbuffer.Projection = projT;

        RENDERER_HELPERS::UpdateUniforms(&cbuffer, sizeof(cbuffer));
    }
}
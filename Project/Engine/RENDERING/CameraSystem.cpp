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
        GMATRIXF projection = GIdentityMatrixF;

        printf("Before call: fov=%f aspect=%f near=%f far=%f\n", fovRadians, aspect, nearZ, farZ);
        // build perspective projection matrix
        GMatrix::ProjectionVulkanRHF(fovRadians, aspect, nearZ, farZ, projection);

        // Debug: show camera projection details
        printf("CreateCamera: fov=%f aspect=%f near=%f far=%f\n", fovRadians, aspect, nearZ, farZ);
        printf("CreateCamera: projection.row1 = %f %f %f %f\n",
               projection.row1.x, projection.row1.y, projection.row1.z, projection.row1.w);
        printf("CreateCamera: projection.row4 = %f %f %f %f\n",
               projection.row4.x, projection.row4.y, projection.row4.z, projection.row4.w);

        registry.emplace<RENDERING::Camera>(camera, RENDERING::Camera{ GIdentityMatrixF, projection });

        // simple spatial representation on the camera 
        RENDERING::Transform transform;
        transform.position = { {0.0f, 0.0f, -6.0f, 1.0f} };
        transform.rotation = { {0.0f, 0.0f, 0.0f, 1.0f} };
        transform.scale = { {1.0f, 1.0f, 1.0f, 0.0f} };
        registry.emplace<RENDERING::Transform>(camera, transform);

        return camera;
    }

    // Per-frame: update camera view from its transform and upload into uniform buffer
    void UpdateCameraAndUpload(entt::registry& registry, entt::entity cameraEntity)
    {
        // compute view from transform and store in the camera component regardless of bound renderer
        auto& camera = registry.get<RENDERING::Camera>(cameraEntity);
        auto& transform = registry.get<RENDERING::Transform>(cameraEntity);

        // Debug: show camera transform position before building world matrix
        printf("UpdateCameraAndUpload: camera position = %f %f %f %f recompute=%d\n",
               transform.position.data[0], transform.position.data[1], transform.position.data[2], transform.position.data[3],
               transform.recomputeWorld ? 1 : 0);

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

        // store computed view back into the Camera component so DrawFrameFor sees it
        camera.view = view;

        // Debug: show computed camera matrices
        printf("UpdateCameraAndUpload: computed Camera.view.row1 = %f %f %f %f\n",
               camera.view.row1.x, camera.view.row1.y, camera.view.row1.z, camera.view.row1.w);
        printf("UpdateCameraAndUpload: Camera.proj.row1 = %f %f %f %f\n",
               camera.projection.row1.x, camera.projection.row1.y, camera.projection.row1.z, camera.projection.row1.w);

        // Only upload to GPU if a renderer is bound
        if (RENDERING::RENDERER_HELPERS::GetGlobalRenderer())
        {
            GPU_CBUFFER cbuffer{};
            cbuffer.World = GIdentityMatrixF;
            cbuffer.View = viewT;
            cbuffer.Projection = projT;

            UpdateUniforms(&cbuffer, sizeof(cbuffer));
        }
        else
        {
            printf("UpdateCameraAndUpload: no renderer bound, skipping GPU upload (but camera.view stored in registry)\n");
        }
    }
}
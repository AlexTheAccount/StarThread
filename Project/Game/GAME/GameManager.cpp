#include "GameComponents.h"
#include "../../Engine/UTILITIES/UtilityComponents.h"

using namespace UTILITIES;
using namespace GW::MATH;
using namespace GAME;

void UpdatePlayerComponent(entt::registry& registry, entt::entity entity);

namespace GAME
{
    void UpdateGameManager(entt::registry& registry)
    {
        // if GameOver tag exists, skip update
        if (registry.ctx().contains<GAME::GameOver>())
            return;

        // get config 
        std::shared_ptr<const GameConfig> config = registry.ctx().get<Config>().gameConfig;

        // Get Delta Time
        double& deltaTime = registry.ctx().get<DeltaTime>().dtSec;

        // Recalculate world matrices for any transforms that need it
        RENDERING::UpdateTransforms(registry);

        // Collision Detection System for OBBF Colliders
        auto collisions = registry.view<RENDERING::Transform, RENDERING::MeshCollection>();

        // mark entities with ToDestroy during collision handling,
        for (auto itA = collisions.begin(); itA != collisions.end(); ++itA)
        {
            entt::entity a = *itA;

            // Get collider and transform
            auto colA = collisions.get<RENDERING::MeshCollection>(a).collider;
            auto& transA = collisions.get<RENDERING::Transform>(a);

            // Get and Set Scale
            GVECTORF vecA;
            GMatrix::GetScaleF(transA.world, vecA);
            colA.extent.x *= vecA.x;
            colA.extent.y *= vecA.y;
            colA.extent.z *= vecA.z;

            // Get and Set Location
            GMatrix::VectorXMatrixF(transA.world, colA.center, colA.center);

            // Rotation
            GQUATERNIONF qA;
            GQuaternion::SetByMatrixF(transA.world, qA);
            GQuaternion::MultiplyQuaternionF(qA, colA.rotation, colA.rotation);

            // Compare with every following entity to avoid duplicate checks
            for (auto itB = std::next(itA); itB != collisions.end(); ++itB)
            {
                entt::entity b = *itB;

                auto colB = collisions.get<RENDERING::MeshCollection>(b).collider;
                auto& transB = collisions.get<RENDERING::Transform>(b);

                // Get and Set Scale
                GVECTORF vecB;
                GMatrix::GetScaleF(transB.world, vecB);
                colB.extent.x *= vecB.x;
                colB.extent.y *= vecB.y;
                colB.extent.z *= vecB.z;

                // Get and Set Location
                GMatrix::VectorXMatrixF(transB.world, colB.center, colB.center);

                // Rotation
                GQUATERNIONF qB;
                GQuaternion::SetByMatrixF(transB.world, qB);
                GQuaternion::MultiplyQuaternionF(qB, colB.rotation, colB.rotation);

                // Check Collision
                GCollision::GCollisionCheck result;
                GCollision::TestOBBToOBBF(colA, colB, result);
                if (GCollision::GCollisionCheck::COLLISION == result)
                {
                    // Handle Collision Response (same logic as before)
                    // Bullet to Enemy
                    if (registry.all_of<Bullet>(a) && registry.all_of<Enemy>(b))
                    {
                        auto& enemyHealth = registry.get<Health>(b);
                        enemyHealth.currentHealth--;
                        registry.emplace_or_replace<ToDestroy>(a);
                    }
                    if (registry.all_of<Bullet>(b) && registry.all_of<Enemy>(a))
                    {
                        auto& enemyHealth = registry.get<Health>(a);
                        enemyHealth.currentHealth--;
                        registry.emplace_or_replace<ToDestroy>(b);
                    }

                    // Enemy to Player
                    if (registry.all_of<Enemy>(a) && registry.all_of<Player>(b))
                    {
                        if (!registry.any_of<GAME::InvulnerabilityState>(b))
                        {
                            auto& playerHealth = registry.get<Health>(b);
                            playerHealth.currentHealth--;
                            printf("Player Health: %d\n", playerHealth.currentHealth);
                            registry.emplace<GAME::InvulnerabilityState>(b);
                            registry.get<GAME::InvulnerabilityState>(b).cooldown = (*config).at("Player").at("invulnPeriod").as<int>();
                        }
                    }
                    if (registry.all_of<Enemy>(b) && registry.all_of<Player>(a))
                    {
                        if (!registry.any_of<GAME::InvulnerabilityState>(a))
                        {
                            auto& playerHealth = registry.get<Health>(a);
                            playerHealth.currentHealth--;
                            printf("Player Health: %d\n", playerHealth.currentHealth);
                            registry.emplace<GAME::InvulnerabilityState>(a);
                            registry.get<GAME::InvulnerabilityState>(a).cooldown = (*config).at("Player").at("invulnPeriod").as<int>();
                        }
                    }
                }
            }
        }

        // check the status of all enemy entities that have a Health component.
        auto enemyHealthView = registry.view<Enemy, Health>();
        for (auto enemyEntity : enemyHealthView)
        {
            auto& healthComponent = enemyHealthView.get<Health>(enemyEntity);
            if (healthComponent.currentHealth <= 0)
            {
                registry.emplace_or_replace<ToDestroy>(enemyEntity);
            }
        }

        // Check a view of all entities tagged as Enemy 
        auto enemyView = registry.view<Enemy>();
        if (enemyView.begin() == enemyView.end()) // if that's empty
        {
            if (!registry.ctx().contains<GameOver>())
                registry.ctx().emplace<GameOver>();
            printf("You win, good job!\n");
        }

        // Destroy all the things tagged with ToDestroy (take a snapshot first)
        std::vector<entt::entity> destroySnapshot;
        destroySnapshot.reserve(std::distance(registry.view<ToDestroy>().begin(), registry.view<ToDestroy>().end()));
        for (auto e : registry.view<ToDestroy>()) destroySnapshot.push_back(e);

        for (auto entity : destroySnapshot)
        {
            if (registry.all_of<RENDERING::MeshCollection>(entity))
            {
                auto& meshCollection = registry.get<RENDERING::MeshCollection>(entity);
                for (auto meshEntity : meshCollection.entities)
                {
                    registry.destroy(meshEntity);
                }
            }
            registry.destroy(entity);
        }

        // Update gameplay for player entities before copying transforms to GPU instances
        auto playerView = registry.view<Player, RENDERING::Transform>();
        for (auto playerEntity : playerView)
        {
            UpdatePlayerComponent(registry, playerEntity);
        }

        // system that checks all Player entities 
        auto playerHealthView = registry.view<Player, Health>();
        for (auto playerHealthEntity : playerHealthView)
        {
            auto& healthComponent = playerHealthView.get<Health>(playerHealthEntity);
            if (healthComponent.currentHealth <= 0)
            {
                if (!registry.ctx().contains<GAME::GameOver>())
                    registry.ctx().emplace<GAME::GameOver>();
                printf("You lose, game over\n");
            }
        }

        // update entity Transforms based on their Velocity
        auto velocityView = registry.view<RENDERING::Transform, Velocity>();
        for (auto entity : velocityView)
        {
            auto& transformComponent = velocityView.get<RENDERING::Transform>(entity);
            auto& velocityComponent = velocityView.get<Velocity>(entity);

            transformComponent.world.row4.x += velocityComponent.velocity.x * deltaTime;
            transformComponent.world.row4.y += velocityComponent.velocity.y * deltaTime;
            transformComponent.world.row4.z += velocityComponent.velocity.z * deltaTime;
        }

        // Copy transforms to GPU instances
        auto view = registry.view<RENDERING::Transform, RENDERING::MeshCollection>();
        for (auto entity : view)
        {
            auto& transform = view.get<RENDERING::Transform>(entity);
            auto& meshCollection = view.get<RENDERING::MeshCollection>(entity);
            for (auto meshEntity : meshCollection.entities)
            {
                if (auto vecEntity = registry.try_get<std::vector<RENDERING::GPUInstance>>(meshEntity))
                {
                    for (auto& instance : *vecEntity)
                    {
                        instance.transform = transform.world;
                    }
                }
                else if (auto singleEntity = registry.try_get<RENDERING::GPUInstance>(meshEntity))
                {
                    singleEntity->transform = transform.world;
                }
            }
        }

        // Update camera view/projection from camera transforms and upload to GPU
        auto cameraView = registry.view<RENDERING::Camera>();
        for (auto cameraEntity : cameraView)
        {
            RENDERING::CAMERA_SYSTEM::UpdateCameraAndUpload(registry, cameraEntity);
        }
    }
    
}
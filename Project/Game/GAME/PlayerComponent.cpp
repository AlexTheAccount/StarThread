#include "GameComponents.h"
#include "../../Engine/UTILITIES/UtilityComponents.h"

using namespace UTILITIES;
using namespace GW::MATH;
using namespace GAME;

void UpdatePlayerComponent(entt::registry& registry, entt::entity entity)
{
    // Grab the single Input and DeltaTime components 
    Input& input = registry.ctx().get<Input>();
    DeltaTime& deltaTime = registry.ctx().get<DeltaTime>();

    // Using the speed stat from the .ini file to adjust the Player’s Transform with the WASD keys. 
    std::shared_ptr<const GameConfig> config = registry.ctx().get<Config>().gameConfig;
    float speed = (*config).at("Player").at("speed").as<float>();
    // Only move on the X/Z plane directions
    auto& playerTransform = registry.get<RENDERING::Transform>(entity);
    GW::MATH::GVECTORF translation = { 0.0f, 0.0f, 0.0f };

    auto& gameContext = registry.ctx().get<GAME::GameStateContext>();

    auto* playerFiring = registry.try_get<GAME::FiringState>(entity);
    if (playerFiring)
        if (playerFiring->cooldown == 0.0f)
            playerFiring->cooldown = (*config).at("Player").at("firerate").as<float>();

    // Only accept input if the game state allows it
    if (gameContext.gameAcceptsInput)
    {
        // if the Firing component isn't present
        if (!playerFiring)
        {
            // Read arrow key states and build a directional vector for firing.
            float upState = 0.0f, downState = 0.0f, leftState = 0.0f, rightState = 0.0f;
            +input.immediateInput.GetState(G_KEY_UP, upState);
            +input.immediateInput.GetState(G_KEY_DOWN, downState);
            +input.immediateInput.GetState(G_KEY_LEFT, leftState);
            +input.immediateInput.GetState(G_KEY_RIGHT, rightState);
            // Determine firing direction based on key states
            float fireX = 0.0f, fireZ = 0.0f;
            if (upState != 0.0f)    fireZ += 1.0f;
            if (downState != 0.0f)  fireZ -= 1.0f;
            if (leftState != 0.0f)  fireX -= 1.0f;
            if (rightState != 0.0f) fireX += 1.0f;

            // Only spawn when there is directional input
            if (fireX != 0.0f || fireZ != 0.0f)
            {
                // Normalize direction
                float length = std::sqrt(fireX * fireX + fireZ * fireZ);
                if (length > 0.0f)
                {
                    fireX /= length;
                    fireZ /= length;
                }

                // Spawn a bullet entity
                auto bulletEntity = registry.create();
                registry.emplace<GAME::Bullet>(bulletEntity);
                registry.emplace<RENDERING::Transform>(bulletEntity);
                registry.emplace<GAME::Velocity>(bulletEntity);
                // Set the bullet's initial position and velocity
                    // position
                auto& bulletTransform = registry.get<RENDERING::Transform>(bulletEntity);
                bulletTransform = playerTransform;
                // velocity
                auto& bulletVelocity = registry.get<GAME::Velocity>(bulletEntity);
                bulletVelocity.startSpeed = (*config).at("Bullet").at("speed").as<float>();
                bulletVelocity.velocity =
                {
                    fireX * bulletVelocity.startSpeed,
                    0.0f,
                    fireZ * bulletVelocity.startSpeed
                };

                // make the bullet renderable
                RENDERING::ModelManager* mmPtr = nullptr;
                auto mmView = registry.view<RENDERING::ModelManager>();
                if (!mmView.empty())
                    mmPtr = &registry.get<RENDERING::ModelManager>(mmView.front());
                if (mmPtr)
                {
                    // Add the MeshCollection component to the bullet
                    RENDERING::MeshCollection bulletMC;
                    bulletMC.collectionName = (*config).at("Bullet").at("model").as<std::string>();
                    registry.emplace<RENDERING::MeshCollection>(bulletEntity, bulletMC);

                    // Find the source collection in the ModelManager
                    std::vector<entt::entity> sourceMeshes;
                    for (auto& mc : mmPtr->meshCollections)
                    {
                        if (mc.collectionName == bulletMC.collectionName)
                        {
                            sourceMeshes = mc.entities;
                            break;
                        }
                    }

                    // Instantiate children for each mesh in the collection
                    auto& bulletMeshCollection = registry.get<RENDERING::MeshCollection>(bulletEntity);
                    bulletMeshCollection.collectionName = (*config).at("Bullet").at("model").as<std::string>();
                    bulletMeshCollection.entities.clear();
                    for (int meshDex = 0; meshDex < sourceMeshes.size(); ++meshDex)
                    {
                        auto meshEntity = registry.create();
                        // copy GPUInstance and GeometryData from model's mesh entities
                            // GPUInstance
                        if (registry.all_of<RENDERING::GPUInstance>(sourceMeshes[meshDex]))
                        {
                            RENDERING::GPUInstance gpuInstaCopy = registry.get<RENDERING::GPUInstance>(sourceMeshes[meshDex]);
                            // override transform with the bullet's transform so the mesh appears at spawn location of player
                            gpuInstaCopy.transform.row4 = bulletTransform.position;
                            registry.emplace<RENDERING::GPUInstance>(meshEntity, gpuInstaCopy);
                        }

                        // GeometryData
                        if (registry.all_of<RENDERING::GeometryData>(sourceMeshes[meshDex]))
                        {
                            RENDERING::GeometryData geoCopy = registry.get<RENDERING::GeometryData>(sourceMeshes[meshDex]);
                            registry.emplace<RENDERING::GeometryData>(meshEntity, geoCopy);
                        }

                        bulletMeshCollection.entities.push_back(meshEntity);
                    }
                    // add collision for the bullet
                    GAME::Collidable bulletCollidable;
                    bulletCollidable.center = { 0.0f, 0.0f, 0.0f, 1.0f };
                    bulletCollidable.extent = { 0.1f, 0.1f, 0.1f, 0.0f };
                    bulletCollidable.rotation = { 0.0f, 0.0f, 0.0f, 1.0f };

                    bulletCollidable.collider.center = bulletCollidable.center;
                    bulletCollidable.collider.extent = bulletCollidable.extent;
                    bulletCollidable.collider.rotation = bulletCollidable.rotation;

                    registry.emplace<GAME::Collidable>(bulletEntity, bulletCollidable);

                    // and add collider to mesh collection
                    bulletMeshCollection.collider = bulletCollidable.collider;
                }

                // Add the Firing component to the player
                registry.emplace<GAME::FiringState>(entity);
            }
        }

        // Use Gateware keys to move the player
        float keyState = 0.0f;
        if (+input.immediateInput.GetState(G_KEY_W, keyState) && keyState != 0.0f)
        {
            translation.z += speed * static_cast<float>(deltaTime.dtSec);
        }
        if (+input.immediateInput.GetState(G_KEY_S, keyState) && keyState != 0.0f)
        {
            translation.z -= speed * static_cast<float>(deltaTime.dtSec);
        }
        if (+input.immediateInput.GetState(G_KEY_A, keyState) && keyState != 0.0f)
        {
            translation.x -= speed * static_cast<float>(deltaTime.dtSec);
        }
        if (+input.immediateInput.GetState(G_KEY_D, keyState) && keyState != 0.0f)
        {
            translation.x += speed * static_cast<float>(deltaTime.dtSec);
        }
    } // end if gameAcceptsInput

    // if the Invulnerability component is present (timers should still tick even if input is disabled)
    if (auto* playerInvulnerability = registry.try_get<GAME::InvulnerabilityState>(entity))
    {
        // reduce the timer and if it’s 0, remove the component.
        playerInvulnerability->cooldown -= deltaTime.dtSec;
        if (playerInvulnerability->cooldown <= 0.0f)
        {
            registry.remove<GAME::InvulnerabilityState>(entity);
        }
    }

    // Use Gateware keys to move the player
    float keyState = 0.0f;
    if (+input.immediateInput.GetState(G_KEY_W, keyState) && keyState != 0.0f)
    {
        translation.z += speed * static_cast<float>(deltaTime.dtSec);
    }
    if (+input.immediateInput.GetState(G_KEY_S, keyState) && keyState != 0.0f)
    {
        translation.z -= speed * static_cast<float>(deltaTime.dtSec);
    }
    if (+input.immediateInput.GetState(G_KEY_A, keyState) && keyState != 0.0f)
    {
        translation.x -= speed * static_cast<float>(deltaTime.dtSec);
    }
    if (+input.immediateInput.GetState(G_KEY_D, keyState) && keyState != 0.0f)
    {
        translation.x += speed * static_cast<float>(deltaTime.dtSec);
    }

    // Apply translation to player transform
    playerTransform.position = GW::MATH::GVECTORF
    {
        {
            playerTransform.position.data[0] + translation.data[0],
            playerTransform.position.data[1] + translation.data[1],
            playerTransform.position.data[2] + translation.data[2],
            1.0f
        }
    };
}
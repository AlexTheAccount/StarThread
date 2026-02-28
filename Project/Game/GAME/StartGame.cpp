#include "GameComponents.h"
#include "../../Engine/RENDERING/RenderingComponents.h"
#include "../../Engine/UTILITIES/UtilityComponents.h"
#include <string>

using namespace GAME;
using namespace RENDERING;


void GAME::StartGame(entt::registry& registry)
{
    auto manager = registry.create();
    registry.emplace<GAME::GameManager>(manager);

    // spawn player
    auto player = registry.create();
    registry.emplace<GAME::Player>(player);
    registry.emplace<RENDERING::Transform>(player);

    // set player transform to origin
    auto& transform = registry.get<RENDERING::Transform>(player);
    transform.position = { { 0.0f, -10.0f, 20.0f, 1.0f } };
    transform.recomputeWorld = true;

    // populate PlayerShip stats from config if available
    if (registry.ctx().contains<UTILITIES::Config>())
    {
        auto config = registry.ctx().get<UTILITIES::Config>().gameConfig;
        if (config)
        {
            try
            {
                GAME::PlayerShip playerShip;
                playerShip.health.startHealth = (*config).at("Player").at("hitpoints").as<int>();
                playerShip.health.currentHealth = playerShip.health.startHealth;
                playerShip.speed.startSpeed = static_cast<int>((*config).at("Player").at("speed").as<float>());
                playerShip.speed.currentSpeed = playerShip.speed.startSpeed;
                registry.emplace<GAME::PlayerShip>(player, playerShip);

                // attach player model
                std::string modelName = (*config).at("Player").at("model").as<std::string>();
                AttachModelToEntity(registry, player, modelName);
            }
            catch (...)
            {
                printf("Failed to read player config; using defaults\n");
                registry.emplace<GAME::PlayerShip>(player);
                AttachModelToEntity(registry, player, "TestShip");
            }
        }
        else
        {
            // fallback model
            registry.emplace<GAME::PlayerShip>(player);
            AttachModelToEntity(registry, player, "TestShip");
        }
    }
    else
    {
        registry.emplace<GAME::PlayerShip>(player);
        AttachModelToEntity(registry, player, "TestShip");
    }
}
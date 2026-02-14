#pragma once
#include <entt/entt.hpp>
#include "../../gateware-26.33.16/Gateware.h"
#include "../../Engine/ENGINE/EngineComponents.h"
#include "../../Engine/RENDERING/RenderingComponents.h"

namespace GAME
{

    //*** TAGS ***//
    struct Player {};
    struct Enemy {};
    struct Bullet {};
    struct Obstacle {};
    struct ToDestroy {};
    struct GameOver {};


    //*** COMPONENTS ***//
    struct GameManager {};

    struct FiringState
    {
        float cooldown;
    };

    struct InvulnerabilityState
    {
        float cooldown;
    };

    struct Velocity
    {
        float startSpeed;
        GW::MATH::GVECTORF velocity;
    };

    struct Health
    {
        int startHealth = -1;
        int currentHealth = -1;
    };

    struct Speed
    {
        int startSpeed = -1;
        int currentSpeed = -1;
    };

    struct PlayerShip
    {
        Health health;
        Speed speed;
    };

    struct Collidable
    {
        GW::MATH::GOBBF collider;
        GW::MATH::GVECTORF extent;
        GW::MATH::GVECTORF center;
        GW::MATH::GQUATERNIONF rotation;
    };

    // *** FUNCTIONS *** //
    void AttachModelToEntity(entt::registry& registry, entt::entity entity, const std::string& modelName);

    // *** GLOBALS *** //
    inline entt::registry& GlobalRegistry()
    {
        return ENGINE::GlobalRegistry();
    }
}
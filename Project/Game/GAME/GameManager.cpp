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
    }
}
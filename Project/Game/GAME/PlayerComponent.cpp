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

}
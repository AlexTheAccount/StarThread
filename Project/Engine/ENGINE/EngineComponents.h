#pragma once
#include <entt/entt.hpp>

namespace ENGINE
{
    inline entt::registry& GlobalRegistry()
    {
        static entt::registry registry;
        return registry;
    }
}
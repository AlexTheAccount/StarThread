#pragma once
#include <entt/entt.hpp>

inline entt::registry& GlobalRegistry()
{
    static entt::registry registry;
    return registry;
}
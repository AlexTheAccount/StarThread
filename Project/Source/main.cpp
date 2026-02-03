#include <windows.h>
#include <cstdio>
#include <chrono>
#include <entt/entt.hpp>

int main()
{
    printf("Hello World!\n");

    // Seed the rand
    unsigned int time = std::chrono::steady_clock::now().time_since_epoch().count();
    srand(time);

    // store everything related to entities and components in a single registry
    entt::registry registry;
}
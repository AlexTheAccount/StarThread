#pragma once
#include <array>

namespace UTIL
{
    class XInputVibration
    {
    public:
        XInputVibration();
        void StartVibration(unsigned int controllerIndex, float leftMotor, float rightMotor, int durationMS);
        void Update(float deltaSeconds);

    private:
        struct State { float left; float right; float remaining; };
        std::array<State, 4> states;
    };
}
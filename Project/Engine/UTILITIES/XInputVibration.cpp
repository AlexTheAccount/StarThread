#include "UtilityComponents.h"
#include <algorithm>
#include <Xinput.h>
#include <cmath>
#pragma comment(lib, "Xinput.lib")

using namespace UTILITIES;

namespace
{
    inline WORD ToMotorSpeed(float speed)
    {
        float clamped = std::clamp(speed, 0.0f, 1.0f);
        return static_cast<WORD>(std::lroundf(clamped * 65535.0f));
    }
}

namespace UTILITIES
{
    XInputVibration::XInputVibration()
    {
        for (auto& state : states)
            state = { 0.f, 0.f, 0.f };
    }

    void XInputVibration::StartVibration(unsigned int controllerIndex, float leftMotor, float rightMotor, int durationMS)
    {
        if (controllerIndex >= states.size())
            return;

        if (durationMS <= 0)
        {
            states[controllerIndex] = { 0.f, 0.f, 0.f };
            XINPUT_VIBRATION vibration{};
            vibration.wLeftMotorSpeed = 0;
            vibration.wRightMotorSpeed = 0;
            XInputSetState(controllerIndex, &vibration);
            return;
        }

        float left = std::clamp(leftMotor, 0.0f, 1.0f);
        float right = std::clamp(rightMotor, 0.0f, 1.0f);

        states[controllerIndex].left = left;
        states[controllerIndex].right = right;
        states[controllerIndex].remaining = durationMS / 1000.0f;

        XINPUT_VIBRATION vibration{};
        vibration.wLeftMotorSpeed = ToMotorSpeed(left);
        vibration.wRightMotorSpeed = ToMotorSpeed(right);
        DWORD result = XInputSetState(controllerIndex, &vibration);
        (void)result;
    }

    void XInputVibration::Update(float deltaSeconds)
    {
        for (int index = 0; index < static_cast<int>(states.size()); ++index)
        {
            State& state = states[index];
            if (state.remaining > 0.0f)
            {
                state.remaining -= deltaSeconds;
                if (state.remaining <= 0.0f)
                {
                    // turn off motors
                    XINPUT_VIBRATION vibration{};
                    vibration.wLeftMotorSpeed = 0;
                    vibration.wRightMotorSpeed = 0;
                    XInputSetState(static_cast<DWORD>(index), &vibration);
                    state = { 0.f, 0.f, 0.f };
                }
            }
        }
    }
}
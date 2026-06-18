#pragma once
#include <cstdint>
#include <algorithm>

// Pure math helpers — no SDL, no Qt, fully unit-testable.
namespace GamepadMath {

// SDL Sint16 axis [-32768, 32767] → float [-1.0, 1.0]
inline float normalizeAxis(int16_t v)
{
    return v / 32767.f;
}

// Trigger or inverted-pedal value [0, 32767] → float [0.0, 1.0]
// Also used for raw joystick pedals after sign inversion.
inline float normalizeTrigger(int16_t v)
{
    return std::clamp(v / 32767.f, 0.f, 1.f);
}

} // namespace GamepadMath

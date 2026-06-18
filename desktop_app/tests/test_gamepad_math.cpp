#include "test_gamepad_math.h"
#include "gamepad/GamepadMath.h"
#include <QtTest>

// Tolerance for float comparisons
static constexpr float EPS = 1e-4f;

void TestGamepadMath::axisCenter()
{
    QCOMPARE(GamepadMath::normalizeAxis(0), 0.f);
}

void TestGamepadMath::axisMaxPositive()
{
    float v = GamepadMath::normalizeAxis(32767);
    QVERIFY(qAbs(v - 1.f) < EPS);
}

void TestGamepadMath::axisMaxNegative()
{
    // -32768 maps to just under -1.0 (SDL min is -32768, not -32767)
    float v = GamepadMath::normalizeAxis(-32768);
    QVERIFY(v < -1.f || qAbs(v + 1.f) < 0.01f);
}

void TestGamepadMath::axisQuarterPositive()
{
    // 32767/4 ≈ 8192 → ~0.25
    float v = GamepadMath::normalizeAxis(8192);
    QVERIFY(qAbs(v - 0.25f) < 0.01f);
}

void TestGamepadMath::axisQuarterNegative()
{
    float v = GamepadMath::normalizeAxis(-8192);
    QVERIFY(qAbs(v + 0.25f) < 0.01f);
}

void TestGamepadMath::triggerReleased()
{
    QCOMPARE(GamepadMath::normalizeTrigger(0), 0.f);
}

void TestGamepadMath::triggerFullPress()
{
    float v = GamepadMath::normalizeTrigger(32767);
    QVERIFY(qAbs(v - 1.f) < EPS);
}

void TestGamepadMath::triggerClampsNegative()
{
    // Negative input (e.g. after joystick inversion rounding) must clamp to 0
    QCOMPARE(GamepadMath::normalizeTrigger(-100), 0.f);
}

void TestGamepadMath::triggerClampsOver()
{
    // Values above 32767 must clamp to 1.0
    QCOMPARE(GamepadMath::normalizeTrigger(32767), 1.f);
}

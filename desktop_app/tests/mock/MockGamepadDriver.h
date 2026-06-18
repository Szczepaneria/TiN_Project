#pragma once
#include "gamepad/GamepadInputBase.h"

// Fake driver — no SDL, no hardware. Lets tests drive MainWindow's control
// loop by injecting arbitrary GamepadState values via setState().
class MockGamepadDriver : public GamepadInputBase {
    Q_OBJECT
public:
    explicit MockGamepadDriver(QObject* parent = nullptr)
        : GamepadInputBase(parent) {}

    bool initialize() override { return true; }
    void shutdown()   override {}

    bool isConnected() const override { return m_state.connected; }

    QList<GamepadDeviceInfo> availableDevices() const override
    {
        return {{ 0, "Mock Device", true }};
    }

    bool selectDevice(int) override
    {
        m_state.connected = true;
        emit deviceConnected("Mock Device");
        return true;
    }

    GamepadState currentState() const override { return m_state; }

    // Test helpers
    void setState(GamepadState s)
    {
        m_state = s;
        emit stateUpdated(s);
    }

    void setConnected(bool c)
    {
        m_state.connected = c;
        if (c) emit deviceConnected("Mock Device");
        else   emit deviceDisconnected();
    }

private:
    GamepadState m_state{};
};

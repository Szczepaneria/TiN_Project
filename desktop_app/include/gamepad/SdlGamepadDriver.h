#pragma once
#include "GamepadInputBase.h"
#include <QTimer>
#include <SDL3/SDL.h>

class SdlGamepadDriver : public GamepadInputBase {
    Q_OBJECT
public:
    explicit SdlGamepadDriver(QObject* parent = nullptr);
    ~SdlGamepadDriver() override;

    bool initialize() override;
    void shutdown() override;
    bool isConnected() const override;
    QList<GamepadDeviceInfo> availableDevices() const override;
    bool selectDevice(int index) override;
    GamepadState currentState() const override;

private slots:
    void poll();
    void refreshDeviceList();

private:
    void closeCurrentDevice();
    void readGamepadState();
    void readJoystickState();

    QTimer* m_pollTimer;    // 10 ms → 100 Hz state reads
    QTimer* m_refreshTimer; // 2 s   → hot-plug detection

    // At most one device open at a time; only one of these is non-null.
    SDL_Gamepad*  m_gamepad  = nullptr;  // standard mapped controller
    SDL_Joystick* m_joystick = nullptr;  // raw joystick (unmapped wheel)

    SDL_JoystickID m_currentId = 0;
    bool           m_isGamepad = false;

    GamepadState             m_state;
    QList<GamepadDeviceInfo> m_devices;
    bool                     m_initialized = false;
};

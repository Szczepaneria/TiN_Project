#include "gamepad/SdlGamepadDriver.h"
#include "gamepad/GamepadMath.h"
#include <QDebug>

static constexpr int POLL_INTERVAL_MS    = 10;   // 100 Hz
static constexpr int REFRESH_INTERVAL_MS = 2000; // hot-plug check

SdlGamepadDriver::SdlGamepadDriver(QObject* parent)
    : GamepadInputBase(parent)
    , m_pollTimer(new QTimer(this))
    , m_refreshTimer(new QTimer(this))
{
    connect(m_pollTimer,    &QTimer::timeout, this, &SdlGamepadDriver::poll);
    connect(m_refreshTimer, &QTimer::timeout, this, &SdlGamepadDriver::refreshDeviceList);
}

SdlGamepadDriver::~SdlGamepadDriver()
{
    shutdown();
}

bool SdlGamepadDriver::initialize()
{
    if (m_initialized)
        return true;

    if (!SDL_Init(SDL_INIT_GAMEPAD | SDL_INIT_JOYSTICK | SDL_INIT_EVENTS)) {
        emit errorOccurred(QString("SDL_Init failed: %1").arg(SDL_GetError()));
        return false;
    }

    m_initialized = true;
    refreshDeviceList();

    m_refreshTimer->start(REFRESH_INTERVAL_MS);
    m_pollTimer->start(POLL_INTERVAL_MS);
    return true;
}

void SdlGamepadDriver::shutdown()
{
    m_pollTimer->stop();
    m_refreshTimer->stop();
    closeCurrentDevice();

    if (m_initialized) {
        SDL_Quit();
        m_initialized = false;
    }
}

bool SdlGamepadDriver::isConnected() const
{
    return m_state.connected;
}

QList<GamepadDeviceInfo> SdlGamepadDriver::availableDevices() const
{
    return m_devices;
}

bool SdlGamepadDriver::selectDevice(int index)
{
    if (index < 0 || index >= m_devices.size()) {
        emit errorOccurred("Invalid device index");
        return false;
    }

    closeCurrentDevice();

    const GamepadDeviceInfo& info = m_devices[index];

    // Retrieve the SDL instance ID from a fresh joystick list so we always
    // have a current handle even after a hot-plug cycle.
    int count = 0;
    SDL_JoystickID* ids = SDL_GetJoysticks(&count);
    if (!ids || index >= count) {
        SDL_free(ids);
        emit errorOccurred("Device no longer available");
        return false;
    }

    SDL_JoystickID id = ids[index];
    SDL_free(ids);

    m_isGamepad = info.isGamepad;
    m_currentId = id;

    if (m_isGamepad) {
        m_gamepad = SDL_OpenGamepad(id);
        if (!m_gamepad) {
            emit errorOccurred(QString("Failed to open gamepad: %1").arg(SDL_GetError()));
            return false;
        }
        qDebug() << "[SDL] Opened gamepad:" << info.name;
    } else {
        m_joystick = SDL_OpenJoystick(id);
        if (!m_joystick) {
            emit errorOccurred(QString("Failed to open joystick: %1").arg(SDL_GetError()));
            return false;
        }
        qDebug() << "[SDL] Opened joystick (raw):" << info.name;
    }

    m_state.connected = true;
    emit deviceConnected(info.name);
    return true;
}

GamepadState SdlGamepadDriver::currentState() const
{
    return m_state;
}

// ── Private slots ────────────────────────────────────────────────────────────

void SdlGamepadDriver::poll()
{
    SDL_PumpEvents();

    if (!m_state.connected)
        return;

    GamepadState prev = m_state;

    if (m_isGamepad && m_gamepad)
        readGamepadState();
    else if (!m_isGamepad && m_joystick)
        readJoystickState();

    // Check for disconnection
    bool stillConnected = m_gamepad
        ? SDL_GamepadConnected(m_gamepad)
        : (m_joystick && SDL_JoystickConnected(m_joystick));

    if (!stillConnected) {
        closeCurrentDevice();
        m_state = {};
        emit deviceDisconnected();
        return;
    }

    // Only emit if something changed (avoids flooding the UI at 100 Hz)
    if (std::abs(m_state.steering  - prev.steering)  > 0.001f ||
        std::abs(m_state.throttle  - prev.throttle)  > 0.001f ||
        std::abs(m_state.brake     - prev.brake)     > 0.001f ||
        m_state.connected != prev.connected)
    {
        emit stateUpdated(m_state);
    }
}

void SdlGamepadDriver::refreshDeviceList()
{
    SDL_PumpEvents();

    int count = 0;
    SDL_JoystickID* ids = SDL_GetJoysticks(&count);

    QList<GamepadDeviceInfo> newList;
    for (int i = 0; i < count; ++i) {
        GamepadDeviceInfo info;
        info.index     = i;
        info.isGamepad = SDL_IsGamepad(ids[i]);

        const char* name = info.isGamepad
            ? SDL_GetGamepadNameForID(ids[i])
            : SDL_GetJoystickNameForID(ids[i]);

        info.name = name ? QString::fromUtf8(name) : QString("Device %1").arg(i);
        newList.append(info);
    }

    SDL_free(ids);

    if (newList != m_devices) {
        m_devices = newList;
        emit deviceListChanged(m_devices);
    }
}

// ── Private helpers ──────────────────────────────────────────────────────────

void SdlGamepadDriver::closeCurrentDevice()
{
    if (m_gamepad) {
        SDL_CloseGamepad(m_gamepad);
        m_gamepad = nullptr;
    }
    if (m_joystick) {
        SDL_CloseJoystick(m_joystick);
        m_joystick = nullptr;
    }
    m_state.connected = false;
    m_currentId = 0;
}

void SdlGamepadDriver::readGamepadState()
{
    // Standard mapping (works for most gamepads and SDL-mapped racing wheels):
    //   Left stick X  → steering
    //   Right trigger → throttle
    //   Left trigger  → brake
    m_state.steering = GamepadMath::normalizeAxis(SDL_GetGamepadAxis(m_gamepad, SDL_GAMEPAD_AXIS_LEFTX));
    m_state.throttle = GamepadMath::normalizeTrigger(SDL_GetGamepadAxis(m_gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER));
    m_state.brake    = GamepadMath::normalizeTrigger(SDL_GetGamepadAxis(m_gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER));
    m_state.connected = true;
}

void SdlGamepadDriver::readJoystickState()
{
    // Raw joystick fallback for wheels SDL doesn't have a mapping for.
    // Common layout (Logitech G29/G920 and similar):
    //   Axis 0 → steering   (signed, -32768..32767)
    //   Axis 1 → throttle   (inverted: -32767 = full, 32767 = released)
    //   Axis 2 → brake      (inverted: -32767 = full, 32767 = released)
    int numAxes = SDL_GetNumJoystickAxes(m_joystick);

    if (numAxes > 0)
        m_state.steering = GamepadMath::normalizeAxis(SDL_GetJoystickAxis(m_joystick, 0));

    if (numAxes > 1) {
        Sint16 raw = SDL_GetJoystickAxis(m_joystick, 1);
        m_state.throttle = GamepadMath::normalizeTrigger(static_cast<Sint16>(-raw));
    }

    if (numAxes > 2) {
        Sint16 raw = SDL_GetJoystickAxis(m_joystick, 2);
        m_state.brake = GamepadMath::normalizeTrigger(static_cast<Sint16>(-raw));
    }

    m_state.connected = true;
}


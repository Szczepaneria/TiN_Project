#pragma once
#include <QObject>
#include <QList>

struct GamepadDeviceInfo {
    int     index;
    QString name;
    bool    isGamepad;  // true = SDL recognized it as a standard gamepad (has mapping)
                        // false = raw joystick (racing wheel without SDL mapping)

    bool operator==(const GamepadDeviceInfo& o) const {
        return index == o.index && name == o.name && isGamepad == o.isGamepad;
    }
};

struct GamepadState {
    float steering  = 0.f;   // -1.0 (full left)  ..  1.0 (full right)
    float throttle  = 0.f;   //  0.0 (released)   ..  1.0 (full throttle)
    float brake     = 0.f;   //  0.0 (released)   ..  1.0 (full brake)
    bool  connected = false;
};

// Abstract base — the main app holds a pointer to this and never touches SDL,
// DirectInput, or any other backend directly.
class GamepadInputBase : public QObject {
    Q_OBJECT
public:
    explicit GamepadInputBase(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~GamepadInputBase() = default;

    // Call once after construction. Returns false if the backend failed to start.
    virtual bool initialize() = 0;

    // Release all backend resources. Safe to call multiple times.
    virtual void shutdown() = 0;

    // True while a device is open and producing data.
    virtual bool isConnected() const = 0;

    // Snapshot of current device list (updated by the driver internally).
    virtual QList<GamepadDeviceInfo> availableDevices() const = 0;

    // Open device at position |index| in availableDevices(). Returns false on error.
    virtual bool selectDevice(int index) = 0;

    // Last read state — call this in your 50 Hz control loop.
    virtual GamepadState currentState() const = 0;

signals:
    // Emitted at the driver's poll rate whenever at least one value changed.
    void stateUpdated(GamepadState state);

    // Emitted when the detected device list changes (hot-plug / unplug).
    void deviceListChanged(QList<GamepadDeviceInfo> devices);

    void deviceConnected(QString name);
    void deviceDisconnected();
    void errorOccurred(QString message);
};

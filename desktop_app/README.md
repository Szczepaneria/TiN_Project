# desktop_app

Main entry point and orchestrator for the RC FPV car system.

Reads steering input from a gamepad or racing wheel, forwards it to the ESP32 controller over UDP, and displays the FPV video stream from the AMB82 camera.

---

## System overview

```mermaid
graph LR
    HW([ Gamepad / Wheel\nUSB])
    APP[desktop_app\nQt6 + SDL3]
    ESP[(ESP32\nWiFi AP\n192.168.4.1)]
    AMB([AMB82 Mini\nRTSP camera])
    CAR([ RC Car])

    HW -->|SDL3 HID| APP
    APP -->|UDP :1234\n8-byte PWM packet\n50 Hz| ESP
    AMB -->|RTSP rtsp://192.168.4.1/live| APP
    ESP -->|PWM GPIO 18/19| CAR
    AMB -.->|mounted on| CAR
```

---

## Architecture

```mermaid
classDiagram
    direction TB

    class MainWindow {
        -GamepadInputBase* m_gamepad
        -VehicleConnection* m_vehicle
        -VideoStream* m_videoStream
        -QTimer* m_controlTimer 50Hz
        +onToggleStream()
        +onToggleControl()
        +sendControlPacket()
    }

    class GamepadInputBase {
        <<abstract>>
        +initialize() bool
        +shutdown()
        +selectDevice(index) bool
        +currentState() GamepadState
        +availableDevices() QList
        --signals--
        +stateUpdated(GamepadState)
        +deviceListChanged(devices)
        +deviceConnected(name)
        +deviceDisconnected()
    }

    class SdlGamepadDriver {
        -SDL_Gamepad* m_gamepad
        -SDL_Joystick* m_joystick
        -QTimer* m_pollTimer 100Hz
        -QTimer* m_refreshTimer 2s
        +initialize() bool
        +selectDevice(index) bool
        -readGamepadState()
        -readJoystickState()
    }

    class VehicleConnection {
        -QUdpSocket m_socket
        -QString m_host
        -quint16 m_port
        +send(steering, motor)
    }

    class VideoStream {
        -QMediaPlayer* m_player
        -QVideoWidget* m_video
        +play()
        +stop()
        --signals--
        +streamStarted()
        +streamError(msg)
    }

    class SettingsDialog {
        +loadFromQSettings()
        -onApply()
        -onRefreshDevices()
    }

    MainWindow --> GamepadInputBase : holds interface ptr
    MainWindow --> VehicleConnection
    MainWindow --> VideoStream
    MainWindow --> SettingsDialog
    SdlGamepadDriver --|> GamepadInputBase : implements
    SettingsDialog --> GamepadInputBase : device list only
```

---

## Data flow

```mermaid
sequenceDiagram
    participant HW as Gamepad HW
    participant SDL as SdlGamepadDriver<br/>(100 Hz poll)
    participant MW as MainWindow<br/>(50 Hz timer)
    participant VC as VehicleConnection
    participant ESP as ESP32

    HW->>SDL: axis values via HID
    SDL->>SDL: normalise → GamepadState
    SDL-->>MW: stateUpdated(state)
    MW->>MW: cache m_lastState

    loop every 20 ms (50 Hz)
        MW->>VC: send(steering, throttle−brake)
        VC->>VC: map [-1,1] → [1000,2000] µs
        VC->>ESP: UDP 8 bytes<br/>[motor_duty][servo_duty]
        ESP->>ESP: MCPWM PWM out<br/>GPIO 18 + 19
    end
```

---

## Driver interface

Adding a new input backend only touches two things: a new class and one line in `mainwindow.cpp`.

```mermaid
graph TD
    MW[MainWindow\nholds GamepadInputBase*]

    MW --> SDL[SdlGamepadDriver\ncross-platform\nSDL3]
    MW -.-> DI[DirectInputDriver\nWindows only\nnot yet implemented]
    MW -.-> MOCK[MockDriver\nsine-wave test signal\nnot yet implemented]

    style DI stroke-dasharray: 5 5
    style MOCK stroke-dasharray: 5 5
```

---

## File structure

```
desktop_app/
├── src/
│   ├── main.cpp                  Entry point
│   ├── mainwindow.h/cpp          Top-level window — toolbar, video, status bar
│   ├── settingsdialog.h/cpp      Settings: RTSP URL, ESP32 IP/port, device picker
│   └── gamepad/
│       └── SdlGamepadDriver.cpp  SDL3 concrete gamepad/wheel driver
├── include/
│   └── gamepad/
│       ├── GamepadInputBase.h    Abstract driver interface (no SDL dependency)
│       └── SdlGamepadDriver.h
└── CMakeLists.txt
```

---

## Dependencies

| Dependency | Purpose | How to get |
|------------|---------|------------|
| Qt 6 (Core, Widgets, Network, Multimedia, MultimediaWidgets) | UI + video + UDP | See below |
| SDL3 | Gamepad / wheel input | See below — auto-downloaded if missing |

### macOS

```bash
brew install qt6 sdl3
```

### Windows (MinGW)

- Qt6: download from [qt.io](https://www.qt.io/download-qt-installer) or `winget install qt`
- SDL3: extract `SDL3-devel-3.4.4-mingw.zip` (included in this folder) to a known path, then pass `-DSDL3_DIR=<path>/cmake` to CMake. Alternatively, CMake will auto-download SDL3 via FetchContent if SDL3 is not found.

---

## Build

```bash
cd desktop_app
mkdir build && cd build

# macOS
cmake .. -DCMAKE_PREFIX_PATH="$(brew --prefix qt6)"

# Windows (MinGW, Qt installed to C:\Qt\6.x.x\mingw_64)
# cmake .. -DCMAKE_PREFIX_PATH="C:/Qt/6.x.x/mingw_64"

make -j$(nproc)        # Linux / macOS
# mingw32-make         # Windows MinGW
```

The first build fetches and compiles SDL3 from GitHub if it was not found locally — this takes a few extra minutes.

---

## First run

```mermaid
flowchart TD
    A([Power on ESP32]) --> B[ESP32 creates WiFi AP\ncar_control / qwerty1234]
    B --> C([Power on AMB82])
    C --> D[AMB82 joins car_control AP]
    D --> E([Connect PC to car_control])
    E --> F([Launch desktop_app])
    F --> G[Open Settings\nverify IPs and select gamepad]
    G --> H[Click Stream]
    H --> I[Click Control]
    I --> J([Drive])
```

Default addresses (set in Settings):
- RTSP URL: `rtsp://192.168.4.1/live`
- ESP32 IP: `192.168.4.1`, Port: `1234`

---

## Gamepad / wheel support

SDL3 recognises most modern gamepads automatically (Xbox, PlayStation, Switch Pro, and many racing wheels). The driver tries the standard **gamepad** mapping first:

| Axis | Control |
|------|---------|
| Left stick X | Steering |
| Right trigger | Throttle |
| Left trigger | Brake |

If the device is not in SDL's database it falls back to **raw joystick** mode:

| SDL axis index | Control |
|----------------|---------|
| 0 | Steering |
| 1 | Throttle (auto-inverted) |
| 2 | Brake (auto-inverted) |

To add a new driver backend, implement `GamepadInputBase` and replace `new SdlGamepadDriver(this)` in `mainwindow.cpp`.

---

## Control packet format

```mermaid
packet-beta
0-31: "motor_duty (uint32 LE)"
32-63: "servo_duty (uint32 LE)"
```

8 bytes, little-endian, sent over UDP to the ESP32:

```
bytes 0–3 : uint32_t  motor_duty   (1000–2000 µs PWM)
bytes 4–7 : uint32_t  servo_duty   (1000–2000 µs PWM)
```

Neutral position is 1500 µs for both channels. Packets are sent at 50 Hz when **Control** is active. The ESP32 resets to neutral after 1 second without a packet.

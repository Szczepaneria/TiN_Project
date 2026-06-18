# gui_app_QT — Architecture & Current Operational Scope

> **Status:** Skeleton / partially implemented. Many methods are stubs ("omitted for brevity").  
> Windows-only (WinRT, Win32 shared memory, DirectInput).

---

## 1. Class Relationships

```mermaid
classDiagram
    direction TB

    class MainWindow {
        +setupUi() ❌ STUB
        +createConnections() ❌ STUB
        +loadSettings() ❌ STUB
        +onTransmissionStartStop()
        +onStreamStartStop() ❌ STUB
        +onWheelInputTimeout()
        +readWheelInput()
        -m_wheelInputTimer : QTimer [20ms / 50 Hz]
        -m_wheelManager : WheelBridgeManager*
        -m_videoStreamManager : VideoStreamManager*
        -m_dataTransmissionWorker : DataTransmissionWorker*
        -m_transmissionThread : QThread*
    }

    class SettingsDialog {
        +setupUi()
        +loadSettings() ⚠️ shows popup on load
        +saveSettings()
        +setWheelManager()
        +refreshWheelList()
        +onApplyAll()
        -tabs: Camera | Wheel | Transfer | Steering | Throttle | Appearance
    }

    class WheelBridgeManager {
        +initialize()
        +getAvailableWheels() ❌ STUB
        +selectWheel() ❌ STUB
        +selectWheelById() ❌ STUB
        +getCurrentState()
        +sendForceFeedback() ❌ STUB
        -createSharedMemory() ❌ STUB
        -startBridgeProcess() ❌ STUB
        -updateWheelList() ❌ STUB
        -pollWheelData() ❌ STUB
        -m_pollTimer : QTimer [10ms / 100 Hz]
        -m_hMapFile : HANDLE
        -m_sharedData : SharedMemoryLayout*
        -m_bridgeProcess : PROCESS_INFORMATION
    }

    class DataTransmissionWorker {
        +startTransmission()
        +stopTransmission()
        +updateTransmissionData()
        +setTransferType() ❌ STUB
        +setSerialSettings() ❌ STUB
        +setUdpSettings() ❌ STUB
        +setSteeringCalibration() ❌ STUB
        +setThrottleCalibration() ❌ STUB
        -initSerialPort() ❌ STUB
        -initUdpSocket() ❌ STUB
        -sendData()
        -formatDataForTransmission()
        -m_transmissionTimer : QTimer [20ms / 50 Hz]
    }

    class VideoStreamManager {
        +startStreaming() ❌ STUB
        +stopStreaming() ❌ STUB
        +changeResolution() ❌ STUB
        -onConnected() ❌ STUB
        -onDisconnected() ❌ STUB
        -onReadyRead() ❌ STUB
        -processVideoData() ❌ STUB
        -onErrorOccurred() ❌ STUB
        -m_socket : QTcpSocket*
        -m_networkThread : QThread*
    }

    class UwpWheelBridge_exe {
        <<external process>>
        Reads Windows.Gaming.Input
        Writes SharedMemoryLayout
        Started by WheelBridgeManager
    }

    class SharedMemoryLayout {
        <<struct>>
        totalConnectedWheels : int
        wheelList[8] : WheelInfo
        wheelStates[8] : WheelState
        activeWheelIndex : int
    }

    class ControlPacket {
        <<struct - WRONG FORMAT>>
        steering : int16_t
        throttle : uint16_t
        brake : uint16_t
        checksum : uint8_t
        -- total: 7 bytes --
    }

    MainWindow --> SettingsDialog
    MainWindow --> WheelBridgeManager
    MainWindow --> DataTransmissionWorker
    MainWindow --> VideoStreamManager
    SettingsDialog --> WheelBridgeManager
    WheelBridgeManager --> UwpWheelBridge_exe : spawns via CreateProcess()
    WheelBridgeManager --> SharedMemoryLayout : reads via MapViewOfFile()
    UwpWheelBridge_exe --> SharedMemoryLayout : writes
    DataTransmissionWorker --> ControlPacket : sends ❌ wrong format
```

---

## 2. Thread Architecture

```mermaid
graph TB
    subgraph Main Thread
        MW[MainWindow]
        SD[SettingsDialog]
        WBM[WheelBridgeManager]
        T1[m_wheelInputTimer\n50 Hz / 20 ms]
        T2[WheelBridgeManager::m_pollTimer\n100 Hz / 10 ms]
    end

    subgraph TransmissionThread
        DTW[DataTransmissionWorker]
        T3[m_transmissionTimer\n50 Hz / 20 ms]
    end

    subgraph VideoStreamThread
        VSM[VideoStreamManager]
        TCP[QTcpSocket ❌ wrong for RTSP]
    end

    T1 -->|onWheelInputTimeout\nreads WheelState| MW
    MW -->|invokeMethod Qt::QueuedConnection\nupdateTransmissionData| DTW
    T3 -->|sendData| DTW
    DTW -->|UDP / Serial| ESP32[(ESP32\n192.168.4.1:1234)]

    T2 -->|pollWheelData ❌ STUB| WBM
    WBM -.->|shared memory read ❌ STUB| SHM[(Win32\nShared Memory)]

    VSM --> TCP
    TCP -.->|connects to ❌| AMB82[(AMB82\nRTSP stream)]
```

---

## 3. Wheel Input Data Flow

```mermaid
flowchart LR
    HW([Racing Wheel\nHardware])
    UWP[UwpWheelBridge.exe\nWindows.Gaming.Input\n❌ separate VS project]
    SHM[(Win32\nNamed Shared Memory\nSharedMemoryLayout)]
    WBM[WheelBridgeManager\npollWheelData ❌ STUB\n100 Hz poll]
    MW[MainWindow\nreadWheelInput\n50 Hz timer]
    DTW[DataTransmissionWorker\nsendData\n50 Hz timer]
    ESP[(ESP32\nUDP :1234)]

    HW -->|HID / USB| UWP
    UWP -->|writes| SHM
    SHM -->|reads via MapViewOfFile ❌ STUB| WBM
    WBM -->|wheelDataUpdated signal\nWheelState| MW
    MW -->|updateTransmissionData\nTransmissionData| DTW
    DTW -->|ControlPacket 7B ❌\nshould be 2×uint32 8B| ESP

    style UWP fill:#f66,color:#fff
    style WBM fill:#f96,color:#fff
    style DTW fill:#f96,color:#fff
    style ESP fill:#fc6,color:#000
```

---

## 4. Video Stream Flow

```mermaid
flowchart LR
    AMB82([AMB82 Mini\nRTSP @ rtsp://IP/live])
    VSM[VideoStreamManager\nstartStreaming ❌ STUB]
    TCP[QTcpSocket\ncustom frame protocol\n❌ incompatible with RTSP]
    CORRECT[✅ Correct approach:\nQMediaPlayer + RTSP URL\nas in desktop_app/VideoStream.cpp]
    MW[MainWindow\nonVideoFrameReceived ❌ STUB]
    LBL[QLabel\nm_videoDisplayLabel]

    AMB82 -->|RTSP / RTP| TCP
    TCP --> VSM
    VSM -->|frameReceived signal\nQImage| MW
    MW --> LBL

    AMB82 -.->|should use| CORRECT

    style TCP fill:#f66,color:#fff
    style VSM fill:#f96,color:#fff
    style MW fill:#f96,color:#fff
    style CORRECT fill:#6a6,color:#fff
```

---

## 5. UDP Protocol Mismatch

```mermaid
flowchart TB
    subgraph App sends — ControlPacket 7 bytes ❌
        direction LR
        A1[int16_t steering\n2 B]
        A2[uint16_t throttle\n2 B]
        A3[uint16_t brake\n2 B]
        A4[uint8_t checksum\n1 B]
    end

    subgraph ESP32 expects — 8 bytes ✅
        direction LR
        B1[uint32_t motor_duty\n4 B\n1000–2000 µs]
        B2[uint32_t servo_duty\n4 B\n1000–2000 µs]
    end

    subgraph ESP32 validation
        V1{len == 8?}
        V2{servo 1000–2000?}
        V3[Accept & apply PWM]
        V4[Drop packet]
    end

    App -->|sends 7 B to 192.168.0.20:8888 ❌\nshould be 192.168.4.1:1234| ESP32_recv

    ESP32_recv --> V1
    V1 -->|No — 7 ≠ 8| V4
    V1 -->|Yes| V2
    V2 -->|out of range| V4
    V2 -->|ok| V3

    style V4 fill:#f66,color:#fff
    style V3 fill:#6a6,color:#fff
```

---

## 6. Settings Dialog — Tab Map

```mermaid
mindmap
  root((SettingsDialog))
    Camera
      IP address input
      Resolution combo 320x240 .. 1080p
      Test Connection button stub
    Wheel
      Wheel list combo
      Refresh button calls getAvailableWheels ❌ STUB
      Apply Selection button
    Transfer
      Type Serial or UDP
      Serial group port + baud rate
      UDP group host + port defaults wrong 192.168.0.20:8888
    Steering
      Middle point °
      Max left °
      Max right °
      Sync button info only no effect
    Throttle
      Middle point °
      Sync button info only no effect
    Appearance
      Dark mode checkbox works
```

---

## 7. Implementation Status Summary

```mermaid
%%{init: {"pie": {"textPosition": 0.5}, "themeVariables": {"pie1": "#6aaa6a", "pie2": "#f96020", "pie3": "#f63030"}} }%%
pie title gui_app_QT Implementation Status
    "Implemented & working" : 35
    "Partial / stubs with logic" : 30
    "Missing / empty stubs" : 35
```

| Component | Status | Blocker |
|-----------|--------|---------|
| `mainwindow.cpp` — layout & buttons | ⚠️ Partial | `setupUi()` stub |
| `mainwindow.cpp` — wheel timer loop | ✅ Done | — |
| `mainwindow.cpp` — stream/transmission UI | ❌ Missing | `onVideoFrameReceived`, `onStreamStatusChanged` etc. stubs |
| `wheelbridgemanager.cpp` — process spawn | ❌ Missing | `startBridgeProcess()` stub |
| `wheelbridgemanager.cpp` — shared memory | ❌ Missing | `createSharedMemory()` stub |
| `wheelbridgemanager.cpp` — poll loop | ❌ Missing | `pollWheelData()` stub |
| `datatransmissionworker.cpp` — UDP send | ⚠️ Partial | `initUdpSocket()` stub |
| `datatransmissionworker.cpp` — serial send | ⚠️ Partial | `initSerialPort()` stub |
| `datatransmissionworker.cpp` — packet format | ❌ Wrong | 7 B sent, 8 B expected |
| `videostreammanager.cpp` — all methods | ❌ Missing | Entire implementation missing |
| `videostreammanager` — protocol | ❌ Wrong | TCP custom frames ≠ RTSP |
| `settingsdialog.cpp` | ✅ Mostly done | Minor UX issues |
| UDP target address | ❌ Wrong | `192.168.0.20:8888` → should be `192.168.4.1:1234` |
| UWP bridge `.exe` | ❌ Missing binary | Must be built separately in VS |

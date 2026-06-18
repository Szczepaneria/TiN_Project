#pragma once
#include <QMainWindow>
#include <QTimer>
#include "gamepad/GamepadInputBase.h"

class QPushButton;
class QLabel;
class VideoStream;
class VehicleConnection;
class SettingsDialog;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onToggleStream();
    void onToggleControl();
    void onOpenSettings();
    void onSettingsAccepted();

    void onGamepadState(GamepadState state);
    void onGamepadConnected(QString name);
    void onGamepadDisconnected();
    void onGamepadError(QString msg);

    void onStreamStarted();
    void onStreamError(QString msg);

    void sendControlPacket();

private:
    void setupUi();
    void applyDarkMode(bool dark);
    void rebuildVehicleConnection();

    // Toolbar
    QPushButton* m_streamBtn   = nullptr;
    QPushButton* m_controlBtn  = nullptr;
    QPushButton* m_settingsBtn = nullptr;
    QLabel*      m_gamepadLabel = nullptr;

    // Central
    VideoStream* m_videoStream = nullptr;

    // Status bar labels
    QLabel* m_steerLabel = nullptr;
    QLabel* m_throttleLabel = nullptr;
    QLabel* m_statusLabel   = nullptr;

    // Logic
    GamepadInputBase*  m_gamepad = nullptr;
    VehicleConnection* m_vehicle = nullptr;
    SettingsDialog*    m_settings = nullptr;

    QTimer* m_controlTimer = nullptr;  // 50 Hz — sends UDP when control is active

    GamepadState m_lastState;
    bool m_streaming   = false;
    bool m_controlling = false;
};

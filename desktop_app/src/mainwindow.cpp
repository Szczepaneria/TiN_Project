#include "mainwindow.h"
#include "settingsdialog.h"
#include "gamepad/SdlGamepadDriver.h"
#include "VideoStream.hpp"
#include "VehicleConnection.hpp"

#include <QToolBar>
#include <QPushButton>
#include <QLabel>
#include <QStatusBar>
#include <QSettings>
#include <QMessageBox>
#include <QApplication>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setupUi();

    // Gamepad — concrete driver hidden behind the interface
    m_gamepad = new SdlGamepadDriver(this);
    connect(m_gamepad, &GamepadInputBase::stateUpdated,    this, &MainWindow::onGamepadState);
    connect(m_gamepad, &GamepadInputBase::deviceConnected, this, &MainWindow::onGamepadConnected);
    connect(m_gamepad, &GamepadInputBase::deviceDisconnected, this, &MainWindow::onGamepadDisconnected);
    connect(m_gamepad, &GamepadInputBase::errorOccurred,   this, &MainWindow::onGamepadError);
    m_gamepad->initialize();

    // Settings dialog
    m_settings = new SettingsDialog(m_gamepad, this);
    connect(m_settings, &SettingsDialog::accepted, this, &MainWindow::onSettingsAccepted);

    // Video stream (RTSP via QMediaPlayer)
    QSettings s("prj33", "desktop_app");
    const QString rtspUrl = s.value("rtsp_url", "rtsp://192.168.4.1/live").toString();
    m_videoStream = new VideoStream(rtspUrl, this);
    connect(m_videoStream, &VideoStream::streamStarted, this, &MainWindow::onStreamStarted);
    connect(m_videoStream, &VideoStream::streamError,   this, &MainWindow::onStreamError);
    setCentralWidget(m_videoStream);

    // Vehicle UDP connection
    rebuildVehicleConnection();

    // 50 Hz control send loop
    m_controlTimer = new QTimer(this);
    m_controlTimer->setInterval(20);
    connect(m_controlTimer, &QTimer::timeout, this, &MainWindow::sendControlPacket);

    // Restore dark mode preference
    applyDarkMode(s.value("dark_mode", false).toBool());
}

MainWindow::~MainWindow()
{
    m_controlTimer->stop();
    m_gamepad->shutdown();
    delete m_vehicle;
}

// ── UI setup ─────────────────────────────────────────────────────────────────

void MainWindow::setupUi()
{
    setWindowTitle("RC FPV Controller");
    resize(900, 600);

    auto* toolbar = addToolBar("Main");
    toolbar->setMovable(false);

    m_streamBtn = new QPushButton("▶  Stream", this);
    m_streamBtn->setCheckable(true);
    m_streamBtn->setFixedWidth(110);

    m_controlBtn = new QPushButton("⚡  Control", this);
    m_controlBtn->setCheckable(true);
    m_controlBtn->setFixedWidth(110);

    m_settingsBtn = new QPushButton("⚙  Settings", this);
    m_settingsBtn->setFixedWidth(110);

    m_gamepadLabel = new QLabel("🎮  No device", this);
    m_gamepadLabel->setMargin(8);

    toolbar->addWidget(m_streamBtn);
    toolbar->addWidget(m_controlBtn);
    toolbar->addSeparator();
    toolbar->addWidget(m_settingsBtn);
    toolbar->addSeparator();
    toolbar->addWidget(m_gamepadLabel);

    connect(m_streamBtn,  &QPushButton::clicked, this, &MainWindow::onToggleStream);
    connect(m_controlBtn, &QPushButton::clicked, this, &MainWindow::onToggleControl);
    connect(m_settingsBtn,&QPushButton::clicked, this, &MainWindow::onOpenSettings);

    // Status bar
    m_steerLabel    = new QLabel("Steer: —", this);
    m_throttleLabel = new QLabel("Throttle: — / Brake: —", this);
    m_statusLabel   = new QLabel("Ready", this);

    statusBar()->addWidget(m_steerLabel);
    statusBar()->addWidget(m_throttleLabel);
    statusBar()->addPermanentWidget(m_statusLabel);
}

// ── Slots — toolbar ───────────────────────────────────────────────────────────

void MainWindow::onToggleStream()
{
    if (!m_streaming) {
        QSettings s("prj33", "desktop_app");
        const QString url = s.value("rtsp_url", "rtsp://192.168.4.1/live").toString();

        // Rebuild widget only if URL changed since construction
        if (url != m_videoStream->property("_url").toString()) {
            delete m_videoStream;
            m_videoStream = new VideoStream(url, this);
            connect(m_videoStream, &VideoStream::streamStarted, this, &MainWindow::onStreamStarted);
            connect(m_videoStream, &VideoStream::streamError,   this, &MainWindow::onStreamError);
            m_videoStream->setProperty("_url", url);
            setCentralWidget(m_videoStream);
        }

        m_videoStream->play();
        m_streaming = true;
        m_streamBtn->setText("⏹  Stop Stream");
        statusBar()->showMessage("Stream starting…");
    } else {
        m_videoStream->stop();
        m_streaming = false;
        m_streamBtn->setText("▶  Stream");
        statusBar()->showMessage("Stream stopped");
    }
}

void MainWindow::onToggleControl()
{
    if (!m_controlling) {
        if (!m_vehicle) {
            QMessageBox::warning(this, "No connection",
                "Configure ESP32 IP/port in Settings first.");
            m_controlBtn->setChecked(false);
            return;
        }
        m_controlTimer->start();
        m_controlling = true;
        m_controlBtn->setText("⏹  Stop Control");
        statusBar()->showMessage("Sending control…");
    } else {
        m_controlTimer->stop();
        m_controlling = false;
        m_controlBtn->setText("⚡  Control");
        statusBar()->showMessage("Control stopped");
    }
}

void MainWindow::onOpenSettings()
{
    m_settings->loadFromQSettings();
    m_settings->exec();
}

void MainWindow::onSettingsAccepted()
{
    QSettings s("prj33", "desktop_app");
    applyDarkMode(s.value("dark_mode", false).toBool());
    rebuildVehicleConnection();
}

// ── Slots — gamepad ───────────────────────────────────────────────────────────

void MainWindow::onGamepadState(GamepadState state)
{
    m_lastState = state;
    m_steerLabel->setText(QString("Steer: %1").arg(state.steering,  5, 'f', 2));
    m_throttleLabel->setText(QString("Thr: %1  Brk: %2")
        .arg(state.throttle, 4, 'f', 2)
        .arg(state.brake,    4, 'f', 2));
}

void MainWindow::onGamepadConnected(QString name)
{
    m_gamepadLabel->setText(QString("🎮  %1").arg(name));
    statusBar()->showMessage(QString("Gamepad connected: %1").arg(name), 3000);
}

void MainWindow::onGamepadDisconnected()
{
    m_gamepadLabel->setText("🎮  No device");
    m_lastState = {};
    statusBar()->showMessage("Gamepad disconnected", 3000);
}

void MainWindow::onGamepadError(QString msg)
{
    statusBar()->showMessage(QString("Gamepad error: %1").arg(msg), 5000);
}

// ── Slots — video ─────────────────────────────────────────────────────────────

void MainWindow::onStreamStarted()
{
    m_statusLabel->setText("Stream ●");
    statusBar()->showMessage("Stream active", 2000);
}

void MainWindow::onStreamError(QString msg)
{
    m_statusLabel->setText("Stream ✕");
    m_streamBtn->setChecked(false);
    m_streaming = false;
    m_streamBtn->setText("▶  Stream");
    statusBar()->showMessage(QString("Stream error: %1").arg(msg), 5000);
}

// ── Control send ──────────────────────────────────────────────────────────────

void MainWindow::sendControlPacket()
{
    if (!m_vehicle || !m_lastState.connected)
        return;

    try {
        double motor = static_cast<double>(m_lastState.throttle - m_lastState.brake);
        m_vehicle->send(static_cast<double>(m_lastState.steering), motor);
    } catch (const std::exception& e) {
        statusBar()->showMessage(QString("Send error: %1").arg(e.what()), 3000);
    }
}

// ── Helpers ───────────────────────────────────────────────────────────────────

void MainWindow::rebuildVehicleConnection()
{
    delete m_vehicle;
    QSettings s("prj33", "desktop_app");
    QString host = s.value("esp32_host", "192.168.4.1").toString();
    int     port = s.value("esp32_port", 1234).toInt();
    m_vehicle = new VehicleConnection(host, static_cast<quint16>(port));
}

void MainWindow::applyDarkMode(bool dark)
{
    if (dark) {
        qApp->setStyleSheet(
            "QMainWindow,QDialog{background:#2b2b2b;color:#fff}"
            "QPushButton{background:#404040;color:#fff;border:1px solid #555;padding:5px}"
            "QPushButton:hover{background:#505050}"
            "QPushButton:checked{background:#336699;border:1px solid #4488bb}"
            "QLineEdit,QComboBox,QSpinBox{background:#404040;color:#fff;border:1px solid #555;padding:3px}"
            "QLabel{color:#fff}"
            "QToolBar{background:#333;border:none}"
            "QStatusBar{background:#222;color:#aaa}"
            "QGroupBox{color:#fff;border:1px solid #555;margin-top:8px}"
            "QGroupBox::title{color:#fff}"
            "QCheckBox{color:#fff}"
        );
    } else {
        qApp->setStyleSheet("");
    }
}

#include "settingsdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QSettings>
#include <QDialogButtonBox>

SettingsDialog::SettingsDialog(GamepadInputBase* gamepad, QWidget* parent)
    : QDialog(parent), m_gamepad(gamepad)
{
    setupUi();

    if (m_gamepad) {
        connect(m_gamepad, &GamepadInputBase::deviceListChanged,
                this, &SettingsDialog::onDeviceListChanged);
    }
}

void SettingsDialog::setupUi()
{
    setWindowTitle("Settings");
    setMinimumWidth(400);

    auto* root = new QVBoxLayout(this);

    // ── Camera ────────────────────────────────────────────────────────────────
    auto* camGroup = new QGroupBox("Camera (AMB82)", this);
    auto* camForm  = new QFormLayout(camGroup);

    m_rtspEdit = new QLineEdit(this);
    m_rtspEdit->setPlaceholderText("rtsp://192.168.4.1/live");
    camForm->addRow("RTSP URL:", m_rtspEdit);

    root->addWidget(camGroup);

    // ── ESP32 ─────────────────────────────────────────────────────────────────
    auto* espGroup = new QGroupBox("ESP32 Controller", this);
    auto* espForm  = new QFormLayout(espGroup);

    m_esp32HostEdit = new QLineEdit(this);
    m_esp32HostEdit->setPlaceholderText("192.168.4.1");

    m_esp32PortSpin = new QSpinBox(this);
    m_esp32PortSpin->setRange(1, 65535);
    m_esp32PortSpin->setValue(1234);

    espForm->addRow("IP address:", m_esp32HostEdit);
    espForm->addRow("UDP port:",   m_esp32PortSpin);

    root->addWidget(espGroup);

    // ── Gamepad ───────────────────────────────────────────────────────────────
    auto* padGroup  = new QGroupBox("Gamepad / Wheel", this);
    auto* padLayout = new QVBoxLayout(padGroup);
    auto* padRow    = new QHBoxLayout;

    m_deviceCombo = new QComboBox(this);
    m_deviceCombo->addItem("<no devices>");
    m_deviceCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_refreshBtn = new QPushButton("Refresh", this);
    m_refreshBtn->setFixedWidth(80);

    padRow->addWidget(m_deviceCombo);
    padRow->addWidget(m_refreshBtn);
    padLayout->addLayout(padRow);
    padLayout->addWidget(new QLabel(
        "Tip: racing wheels not in SDL's database appear as raw joysticks.\n"
        "Axis 0 = steering, axis 1 = throttle, axis 2 = brake.", this));

    root->addWidget(padGroup);

    // ── Appearance ────────────────────────────────────────────────────────────
    auto* appGroup  = new QGroupBox("Appearance", this);
    auto* appLayout = new QVBoxLayout(appGroup);

    m_darkModeCheck = new QCheckBox("Dark mode", this);
    appLayout->addWidget(m_darkModeCheck);

    root->addWidget(appGroup);

    // ── Buttons ───────────────────────────────────────────────────────────────
    m_applyBtn = new QPushButton("Apply & Close", this);
    m_closeBtn = new QPushButton("Cancel", this);

    auto* btnRow = new QHBoxLayout;
    btnRow->addStretch();
    btnRow->addWidget(m_applyBtn);
    btnRow->addWidget(m_closeBtn);
    root->addLayout(btnRow);

    connect(m_applyBtn,   &QPushButton::clicked, this, &SettingsDialog::onApply);
    connect(m_closeBtn,   &QPushButton::clicked, this, &QDialog::reject);
    connect(m_refreshBtn, &QPushButton::clicked, this, &SettingsDialog::onRefreshDevices);
}

void SettingsDialog::loadFromQSettings()
{
    QSettings s("prj33", "desktop_app");
    m_rtspEdit->setText(s.value("rtsp_url", "rtsp://192.168.4.1/live").toString());
    m_esp32HostEdit->setText(s.value("esp32_host", "192.168.4.1").toString());
    m_esp32PortSpin->setValue(s.value("esp32_port", 1234).toInt());
    m_darkModeCheck->setChecked(s.value("dark_mode", false).toBool());

    // Populate device list with whatever the driver already knows
    if (m_gamepad)
        onDeviceListChanged(m_gamepad->availableDevices());

    // Restore previously selected device index
    int savedIdx = s.value("gamepad_index", 0).toInt();
    if (savedIdx < m_deviceCombo->count())
        m_deviceCombo->setCurrentIndex(savedIdx);
}

void SettingsDialog::saveToQSettings()
{
    QSettings s("prj33", "desktop_app");
    s.setValue("rtsp_url",      m_rtspEdit->text().trimmed());
    s.setValue("esp32_host",    m_esp32HostEdit->text().trimmed());
    s.setValue("esp32_port",    m_esp32PortSpin->value());
    s.setValue("dark_mode",     m_darkModeCheck->isChecked());
    s.setValue("gamepad_index", m_deviceCombo->currentIndex());
}

void SettingsDialog::onApply()
{
    saveToQSettings();

    // Apply gamepad selection immediately
    if (m_gamepad && m_deviceCombo->count() > 0) {
        int idx = m_deviceCombo->currentData().toInt();
        m_gamepad->selectDevice(idx);
    }

    accept();  // emits accepted() → MainWindow::onSettingsAccepted
}

void SettingsDialog::onRefreshDevices()
{
    if (!m_gamepad)
        return;
    // Trigger a device list refresh via the driver's own refresh slot
    // (the driver emits deviceListChanged which we're connected to)
    QMetaObject::invokeMethod(m_gamepad, "refreshDeviceList", Qt::DirectConnection);
}

void SettingsDialog::onDeviceListChanged(QList<GamepadDeviceInfo> devices)
{
    int prevIdx = m_deviceCombo->currentData().toInt();
    m_deviceCombo->clear();

    if (devices.isEmpty()) {
        m_deviceCombo->addItem("<no devices>");
        return;
    }

    for (const GamepadDeviceInfo& d : devices) {
        QString label = d.isGamepad
            ? QString("%1 [gamepad]").arg(d.name)
            : QString("%1 [joystick]").arg(d.name);
        m_deviceCombo->addItem(label, d.index);
    }

    // Restore selection if still present
    for (int i = 0; i < m_deviceCombo->count(); ++i) {
        if (m_deviceCombo->itemData(i).toInt() == prevIdx) {
            m_deviceCombo->setCurrentIndex(i);
            break;
        }
    }
}

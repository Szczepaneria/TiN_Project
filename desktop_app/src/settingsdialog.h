#pragma once
#include <QDialog>
#include "gamepad/GamepadInputBase.h"

class QLineEdit;
class QSpinBox;
class QComboBox;
class QCheckBox;
class QPushButton;

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    // |gamepad| pointer is used only to populate the device list; ownership stays with MainWindow.
    explicit SettingsDialog(GamepadInputBase* gamepad, QWidget* parent = nullptr);

    // Populate the form from QSettings before showing the dialog.
    void loadFromQSettings();

private slots:
    void onApply();
    void onRefreshDevices();
    void onDeviceListChanged(QList<GamepadDeviceInfo> devices);

private:
    void setupUi();
    void saveToQSettings();

    GamepadInputBase* m_gamepad;

    // Camera / stream
    QLineEdit* m_rtspEdit   = nullptr;

    // ESP32
    QLineEdit* m_esp32HostEdit = nullptr;
    QSpinBox*  m_esp32PortSpin = nullptr;

    // Gamepad
    QComboBox*   m_deviceCombo   = nullptr;
    QPushButton* m_refreshBtn    = nullptr;

    // Appearance
    QCheckBox* m_darkModeCheck = nullptr;

    QPushButton* m_applyBtn = nullptr;
    QPushButton* m_closeBtn = nullptr;
};

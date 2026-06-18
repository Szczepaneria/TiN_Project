#pragma once
#include <QString>
#include <QUdpSocket>
#include <stdexcept>

class VehicleConnection {
public:
    // Defaults match the ESP32 firmware AP address and port.
    explicit VehicleConnection(const QString& host = "192.168.4.1", quint16 port = 1234);

    // Send control values to the ESP32.
    //   steering : -1.0 (full left) .. 1.0 (full right)
    //   motor    : -1.0 (full brake/reverse) .. 1.0 (full throttle)
    void send(double steering, double motor);

    // Encode to 8-byte packet without a live socket — exposed for unit tests.
    static QByteArray encodePacket(double steering, double motor);

    bool isReady() const;

private:
    QUdpSocket m_socket;
    QString    m_host;
    quint16    m_port;
};

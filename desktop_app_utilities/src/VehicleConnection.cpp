#include "VehicleConnection.hpp"
#include <QtEndian>
#include <QHostAddress>
#include <algorithm>
#include <cstring>

VehicleConnection::VehicleConnection(const QString& host, quint16 port)
    : m_host(host), m_port(port)
{}

QByteArray VehicleConnection::encodePacket(double steering, double motor)
{
    steering = std::clamp(steering, -1.0, 1.0);
    motor    = std::clamp(motor,    -1.0, 1.0);

    // Map [-1, 1] → [1000, 2000] µs PWM pulse width
    auto toUs = [](double v) -> uint32_t {
        return static_cast<uint32_t>(1500.0 + v * 500.0);
    };

    // ESP32 packet layout (8 bytes, little-endian):
    //   bytes 0-3 : uint32_t motor_duty
    //   bytes 4-7 : uint32_t servo_duty
    uint32_t motor_le = qToLittleEndian(toUs(motor));
    uint32_t servo_le = qToLittleEndian(toUs(steering));

    uint8_t buf[8];
    std::memcpy(buf,     &motor_le, 4);
    std::memcpy(buf + 4, &servo_le, 4);

    return QByteArray(reinterpret_cast<const char*>(buf), 8);
}

void VehicleConnection::send(double steering, double motor)
{
    QByteArray datagram = encodePacket(steering, motor);
    qint64 written = m_socket.writeDatagram(datagram, QHostAddress(m_host), m_port);

    if (written < 0) {
        throw std::runtime_error("UDP send failed: " + m_socket.errorString().toStdString());
    }
}

bool VehicleConnection::isReady() const
{
    return true;  // UDP is connectionless — always ready after construction
}

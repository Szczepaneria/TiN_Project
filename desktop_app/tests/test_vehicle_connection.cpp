#include "test_vehicle_connection.h"
#include "VehicleConnection.hpp"
#include <QtTest>
#include <cstring>

// Helper: decode bytes [offset, offset+4) as a little-endian uint32
static uint32_t readU32LE(const QByteArray& b, int offset)
{
    uint32_t v = 0;
    std::memcpy(&v, b.constData() + offset, 4);
    // On little-endian hosts (x86, ARM Apple Silicon) this is already correct.
    // qFromLittleEndian handles big-endian hosts if they ever appear.
    return qFromLittleEndian(v);
}

void TestVehicleConnection::packetSize()
{
    QByteArray p = VehicleConnection::encodePacket(0.0, 0.0);
    QCOMPARE(p.size(), 8);
}

void TestVehicleConnection::neutralPosition()
{
    QByteArray p = VehicleConnection::encodePacket(0.0, 0.0);
    QCOMPARE(readU32LE(p, 0), 1500u);  // motor neutral
    QCOMPARE(readU32LE(p, 4), 1500u);  // servo neutral
}

void TestVehicleConnection::fullRight()
{
    // steering = 1.0 → servo = 2000 µs
    QByteArray p = VehicleConnection::encodePacket(1.0, 0.0);
    QCOMPARE(readU32LE(p, 4), 2000u);
}

void TestVehicleConnection::fullLeft()
{
    // steering = -1.0 → servo = 1000 µs
    QByteArray p = VehicleConnection::encodePacket(-1.0, 0.0);
    QCOMPARE(readU32LE(p, 4), 1000u);
}

void TestVehicleConnection::fullThrottle()
{
    // motor = 1.0 → motor_duty = 2000 µs
    QByteArray p = VehicleConnection::encodePacket(0.0, 1.0);
    QCOMPARE(readU32LE(p, 0), 2000u);
}

void TestVehicleConnection::fullBrake()
{
    // motor = -1.0 → motor_duty = 1000 µs
    QByteArray p = VehicleConnection::encodePacket(0.0, -1.0);
    QCOMPARE(readU32LE(p, 0), 1000u);
}

void TestVehicleConnection::clampHigh()
{
    // Values above 1.0 must be clamped to 2000 µs
    QByteArray p = VehicleConnection::encodePacket(2.0, 5.0);
    QCOMPARE(readU32LE(p, 0), 2000u);
    QCOMPARE(readU32LE(p, 4), 2000u);
}

void TestVehicleConnection::clampLow()
{
    // Values below -1.0 must be clamped to 1000 µs
    QByteArray p = VehicleConnection::encodePacket(-3.0, -9.0);
    QCOMPARE(readU32LE(p, 0), 1000u);
    QCOMPARE(readU32LE(p, 4), 1000u);
}

void TestVehicleConnection::byteOrderMotorFirst()
{
    // Verify raw byte layout: motor bytes come first (bytes 0-3), servo second (bytes 4-7).
    // Neutral (1500 = 0x000005DC) in little-endian: DC 05 00 00
    QByteArray p = VehicleConnection::encodePacket(0.0, 0.0);
    QCOMPARE((uint8_t)p[0], 0xDCu);
    QCOMPARE((uint8_t)p[1], 0x05u);
    QCOMPARE((uint8_t)p[2], 0x00u);
    QCOMPARE((uint8_t)p[3], 0x00u);
    QCOMPARE((uint8_t)p[4], 0xDCu);
    QCOMPARE((uint8_t)p[5], 0x05u);
}

void TestVehicleConnection::steeringIndependentOfMotor()
{
    // Changing steering must not affect motor bytes, and vice versa.
    QByteArray a = VehicleConnection::encodePacket(1.0,  0.0);
    QByteArray b = VehicleConnection::encodePacket(0.0,  0.0);

    // Motor bytes identical (both neutral)
    QCOMPARE(readU32LE(a, 0), readU32LE(b, 0));

    // Servo bytes differ
    QVERIFY(readU32LE(a, 4) != readU32LE(b, 4));
}

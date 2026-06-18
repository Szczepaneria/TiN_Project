#pragma once
#include <QObject>

class TestVehicleConnection : public QObject {
    Q_OBJECT
private slots:
    void packetSize();
    void neutralPosition();
    void fullRight();
    void fullLeft();
    void fullThrottle();
    void fullBrake();
    void clampHigh();
    void clampLow();
    void byteOrderMotorFirst();
    void steeringIndependentOfMotor();
};

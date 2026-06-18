#pragma once
#include <QObject>

class TestGamepadMath : public QObject {
    Q_OBJECT
private slots:
    void axisCenter();
    void axisMaxPositive();
    void axisMaxNegative();
    void axisQuarterPositive();
    void axisQuarterNegative();
    void triggerReleased();
    void triggerFullPress();
    void triggerClampsNegative();
    void triggerClampsOver();
};

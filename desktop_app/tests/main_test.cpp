#include <QtTest>

#include "test_vehicle_connection.h"
#include "test_gamepad_math.h"

int main(int argc, char* argv[])
{
    int status = 0;

    {
        TestVehicleConnection t;
        status |= QTest::qExec(&t, argc, argv);
    }
    {
        TestGamepadMath t;
        status |= QTest::qExec(&t, argc, argv);
    }

    return status;
}

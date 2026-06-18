//main.cpp
#include <QCoreApplication>
#include <QDebug>
#include "VehicleConnection.hpp"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    try {
        VehicleConnection vehicle("192.168.4.1", 4210);

        qDebug() << "Sending test packet...";

        vehicle.send({0.5, -0.3});

        qDebug() << "Packet sent successfully!";
    }
    catch (const std::exception& e) {
        qDebug() << "Error:" << e.what();
    }

    return 0;
}
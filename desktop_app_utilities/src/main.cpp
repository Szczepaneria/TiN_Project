// main.cpp
#include <QApplication>
#include <QDebug>

#include "VehicleConnection.hpp"
#include "VideoStream.hpp"

int main(int argc, char* argv[])
{
    //Create App - this is a showcase
    QApplication app(argc, argv);

    // Video Stream
    // The AMB82 serves RTSP at rtsp://<IP>/live.
    // IP 192.168.4.1 is the default
    VideoStream videoWindow("rtsp://192.168.4.1/live");
    videoWindow.show();
    videoWindow.play();

    QObject::connect(&videoWindow, &VideoStream::streamStarted, []() {
        qDebug() << "[Main] Stream active, first frame received.";
    });

    QObject::connect(&videoWindow, &VideoStream::streamError,
                     [](const QString& msg) {
        qWarning() << "[Main] Stream error:" << msg;
    });

    return app.exec();
}
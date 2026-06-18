#include "../include/fpvsystem.hpp"

#include <WiFi.h>
#include <StreamIO.h>
#include <VideoStream.h>
#include <RTSP.h>

#define CHANNEL 0
#define FPV_LOG(x) Serial.println(String("[FPV] ") + x);
#define FPV_LOG_N(x) Serial.print(String("[FPV] ") + x);

VideoSetting config(CHANNEL);
RTSP rtsp;
StreamIO videoStreamer(1, 1);

const char* ssid = "car_control";
const char* pass = "qwerty1234";

void FpvSystem::begin() {
    Serial.begin(115200);
    delay(500);

    FPV_LOG("Booting FPV");

    state = SystemState::STARTUP;
    last_stats_time = 0;
}

void FpvSystem::update() {
    switch (state) {

        case SystemState::STARTUP:
            initCamera();
            state = SystemState::WIFI_CONNECTING;
            break;

        case SystemState::WIFI_CONNECTING:
            connectWiFi();
            break;

        case SystemState::STREAMING:
            logStats();
            break;

        case SystemState::ERROR:
            delay(1000);
            break;
    }
}

void FpvSystem::initCamera() {
    config._resolution = VIDEO_VGA;
    config._fps = 30;
    config._gop = 1;
    config.setBitrate(1000000);

    FPV_LOG("Init camera");

    Camera.configVideoChannel(CHANNEL, config);
    Camera.videoInit();

    rtsp.configVideo(config);
    rtsp.begin();

    videoStreamer.registerInput(Camera.getStream(CHANNEL));
    videoStreamer.registerOutput(rtsp);
    videoStreamer.begin();

    Camera.channelBegin(CHANNEL);

    FPV_LOG("Camera ready");
}

void FpvSystem::connectWiFi() {
    static bool started = false;

    if (!started) {
        FPV_LOG("WiFi connect");
        WiFi.begin((char*)ssid, pass);
        started = true;
    }

    if (WiFi.status() != WL_CONNECTED) {
        FPV_LOG_N(".");
        delay(500);
        return;
    }

    FPV_LOG("");
    FPV_LOG("WiFi OK");

    IPAddress ip = WiFi.localIP();

    FPV_LOG(String("IP: ") +
        String(ip[0]) + "." +
        String(ip[1]) + "." +
        String(ip[2]) + "." +
        String(ip[3])
    );

    FPV_LOG("Command for connecting: ")
    FPV_LOG(String("  ffplay --fflags nobuffer -flags low_delay -framedrop -probesize 32 -analyzeduration 0 rtsp://") +
        String(ip[0]) + "." +
        String(ip[1]) + "." +
        String(ip[2]) + "." +
        String(ip[3]) + "/live")

    state = SystemState::STREAMING;
}

void FpvSystem::logStats() {
    static uint32_t last = 0;

    if (millis() - last > 5000) {
        last = millis();
        FPV_LOG(String("RSSI: ") + WiFi.RSSI() + " dBm");
    }
}
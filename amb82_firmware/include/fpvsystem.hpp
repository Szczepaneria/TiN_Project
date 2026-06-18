#ifndef FPVSYSTEM_HPP
#define FPVSYSTEM_HPP

#include <Arduino.h>

enum class SystemState {
    STARTUP,
    WIFI_CONNECTING,
    STREAMING,
    ERROR
};

class FpvSystem {
public:
    void begin();
    void update();

private:
    SystemState state;
    uint32_t last_stats_time;

    void initCamera();
    void connectWiFi();
    void logStats();
};

#endif
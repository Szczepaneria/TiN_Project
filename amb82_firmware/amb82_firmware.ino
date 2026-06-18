#include "include/fpvsystem.hpp"

FpvSystem fpv;

void setup() {
    fpv.begin();
}

void loop() {
    fpv.update();
}
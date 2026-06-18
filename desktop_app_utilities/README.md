# Desktop App

## Responsible for:
Proof of concept - contains logic classes that are meant to be used in desktop application. Handles sending steering output to ESP32 controller (esp32_firmware/) and Receiving Video input from amb_82.

## main_steering.cpp
This is a Qt application - built for a full operating system platform. Main task is to:
1. Send serialized steering (control) data to ESP32 control platform

## main.cpp
This is a Qt application - built for a test of RTSP stream receive. Main task is to:

1. Acquire the FPV camera livestream and show it in real time to user


> Currently the FPV controller data send part is implemented, although not tested, as the project has not reached the state of testing yet.

### Usage:
This application uses CMake. To change or investiagte the build process, please refer to the [CMakeLists](CMakeLists.txt) file.

To build the application, use the following:
1.  ```bash
     mkdir build
    ```
2. ```bash
    cmake ..
    ```
2. ```bash
    make
    ```
2. ```bash
    ./PRJ33
    ```
    

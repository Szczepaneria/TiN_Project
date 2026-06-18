# This line tells CMake that the configuration should be compatible with CMake 3.25 or later.
cmake_minimum_required(VERSION 3.25)

# This line sets the name of the project
project(PRJ33)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)   # runs MOC on headers containing Q_OBJECT

# -- Qt packages --
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Network
    Widgets
    Multimedia
    MultimediaWidgets
)

# -- Sources --
add_executable(PRJ33
    src/main.cpp
    src/VehicleConnection.cpp
    src/VideoStream.cpp
)

target_link_libraries(PRJ33
    Qt6::Core
    Qt6::Network
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::MultimediaWidgets
)

target_include_directories(PRJ33 PRIVATE include)
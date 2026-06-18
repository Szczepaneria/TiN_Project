find_package(Qt6 REQUIRED COMPONENTS Test Core Network)
set(CMAKE_AUTOMOC ON)

set(UTILITIES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../desktop_app_utilities")

add_executable(desktop_app_tests
    main_test.cpp
    test_vehicle_connection.h
    test_vehicle_connection.cpp
    test_gamepad_math.h
    test_gamepad_math.cpp
    mock/MockGamepadDriver.h                        # AUTOMOC: Q_OBJECT
    "${CMAKE_CURRENT_SOURCE_DIR}/../include/gamepad/GamepadInputBase.h"  # AUTOMOC: abstract Q_OBJECT

    "${UTILITIES_DIR}/src/VehicleConnection.cpp"
)

target_include_directories(desktop_app_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"           # test headers
    "${CMAKE_CURRENT_SOURCE_DIR}/../include" # gamepad/GamepadInputBase.h, GamepadMath.h
    "${UTILITIES_DIR}/include"              # VehicleConnection.hpp
)

target_link_libraries(desktop_app_tests PRIVATE
    Qt6::Test
    Qt6::Core
    Qt6::Network
)

add_test(NAME desktop_app_tests COMMAND desktop_app_tests)

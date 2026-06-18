cmake_minimum_required(VERSION 3.25)
project(desktop_app VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# Set OFF in CI test jobs to skip SDL3 entirely and speed up the build.
option(BUILD_APP "Build the desktop application (requires SDL3 + Qt Multimedia)" ON)

set(UTILITIES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../desktop_app_utilities")

# ── Qt ────────────────────────────────────────────────────────────────────────
if(BUILD_APP)
    find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Multimedia MultimediaWidgets)
else()
    find_package(Qt6 REQUIRED COMPONENTS Core Network)
endif()

# ── SDL3 (only needed for the app, not the tests) ────────────────────────────
if(BUILD_APP)
    find_package(SDL3 QUIET CONFIG)
    if(NOT SDL3_FOUND)
        message(STATUS "SDL3 not found — fetching from GitHub (one-time build)...")
        include(FetchContent)
        FetchContent_Declare(
            SDL3
            GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
            GIT_TAG        release-3.4.4
            GIT_SHALLOW    TRUE
        )
        set(SDL_SHARED OFF CACHE BOOL "" FORCE)
        set(SDL_STATIC ON  CACHE BOOL "" FORCE)
        set(SDL_TEST   OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(SDL3)
    endif()

    add_executable(desktop_app
        src/main.cpp
        src/mainwindow.cpp
        src/mainwindow.h
        src/settingsdialog.cpp
        src/settingsdialog.h
        src/gamepad/SdlGamepadDriver.cpp
        include/gamepad/GamepadInputBase.h
        include/gamepad/SdlGamepadDriver.h
        "${UTILITIES_DIR}/src/VehicleConnection.cpp"
        "${UTILITIES_DIR}/src/VideoStream.cpp"
        "${UTILITIES_DIR}/include/VideoStream.hpp"
    )

    target_include_directories(desktop_app PRIVATE
        include
        src
        "${UTILITIES_DIR}/include"
    )

    target_link_libraries(desktop_app PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Network
        Qt6::Multimedia
        Qt6::MultimediaWidgets
        SDL3::SDL3
    )

    if(WIN32)
        set_target_properties(desktop_app PROPERTIES WIN32_EXECUTABLE TRUE)
    endif()
    if(APPLE)
        set_target_properties(desktop_app PROPERTIES MACOSX_BUNDLE TRUE)
    endif()
endif()

# ── Tests (always built) ──────────────────────────────────────────────────────
enable_testing()
add_subdirectory(tests)

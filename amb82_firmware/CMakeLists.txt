cmake_minimum_required(VERSION 3.22)
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(amb82_firmware C CXX ASM)

set(SDK_PATH $ENV{AMEBA_SDK_PATH})

set(COMMON_FLAGS 
    -mthumb 
    -mcpu=cortex-m33 
    -mfloat-abi=hard 
    -mfpu=fpv5-sp-d16 
    -mcmse
)

add_definitions(
    -D__DOMAIN_NS=1
    -D__ARM_ARCH_8M_MAIN__=1
    -DCONFIG_PLATFORM_8735B=1
)

add_executable(fpv_app
    src/main.cpp
    src/fpvsystem.cpp
)

target_compile_options(fpv_app PRIVATE
    ${COMMON_FLAGS}
    -std=gnu++17
    -fpermissive
    -Wno-register
)

target_link_options(fpv_app PRIVATE ${COMMON_FLAGS})

target_include_directories(fpv_app PRIVATE 
    include
    "${SDK_PATH}/cores/ambpro2"
    "${SDK_PATH}/variants/ameba_amb82-mini"
    "${SDK_PATH}/libraries/Multimedia/src"
    "${SDK_PATH}/libraries/WiFi/src"
    "${SDK_PATH}/libraries/RTSP/src"
    "${SDK_PATH}/libraries/Stream/src"
    "${SDK_PATH}/system/component/mbed/targets/hal/rtl8735b"
    "${SDK_PATH}/system/component/soc/8735b/cmsis/rtl8735b/include"
    "${SDK_PATH}/system/component/soc/8735b/cmsis/cmsis-core/include"
    "${SDK_PATH}/system/component/soc/8735b/fwlib/rtl8735b/include"
    "${SDK_PATH}/system/component/soc/8735b/fwlib/rtl8735b/lib/include"
    "${SDK_PATH}/system/component/soc/8735b/app/rtl_printf/include"
    "${SDK_PATH}/system/component/soc/8735b/misc/utilities/include"
    "${SDK_PATH}/system/component/soc/8735b/app/stdio_port"
    "${SDK_PATH}/system/component/soc/8735b/cmsis/rtl8735b/lib/include"
    "${SDK_PATH}/system/component/os/freertos"
    "${SDK_PATH}/system/component/os/freertos/freertos_v202012.00/Source/include"
    "${SDK_PATH}/system/component/os/freertos/freertos_v202012.00/Source/portable/GCC/ARM_CM33_NTZ/non_secure"
    "${SDK_PATH}/system/project/realtek_amebapro2_v0_example/inc"
    "${SDK_PATH}/system/component/soc/8735b/lib/network/ssl/mbedtls-2.4.0/include"
    "${SDK_PATH}/system/component/ssl/ssl_ram_map/rom"
    "${SDK_PATH}/system/component/ssl/mbedtls_ram_map/rom"
    "${SDK_PATH}/system/component/stdlib"
    "${SDK_PATH}/system/component/media/mmfv2"
    "${SDK_PATH}/system/component/media/framework"
    "${SDK_PATH}/system/component/media/codecs"
    "${SDK_PATH}/system/component/video/driver/RTL8735B/fcs_hal"
    "${SDK_PATH}/system/component/video/driver/RTL8735B/common"
    "${SDK_PATH}/system/component/video/driver/common"
    "${SDK_PATH}/system/component/media/mmfv2/inc"
    "${SDK_PATH}/system/component/mbed/hal_ext"
    "${SDK_PATH}/system/component/audio/3rdparty/AEC"
    "${SDK_PATH}/system/component/media/rtp_codec"
)
cmake_minimum_required(VERSION 3.28)
project(FlightMonitor LANGUAGES C CXX ASM VERSION 0.0.1)

add_subdirectory(deps)
configure_file(src/config.h.in src/config.h @ONLY)

if(TARGET_PLATFORM STREQUAL "stm32")
    message(STATUS "[${PROJECT_NAME}]Building firware for STM32")
    add_subdirectory(src/firmware)

elseif(TARGET_PLATFORM STREQUAL "x64")
    message(STATUS "[${PROJECT_NAME}]Building desktop application for x64")
    add_subdirectory(src/desktop)

else()
    message(FATAL_ERROR "[${PROJECT_NAME}]Unknown TARGET_PLATFORM: ${TARGET_PLATFORM}")

endif()

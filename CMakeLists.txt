cmake_minimum_required(VERSION 3.28)
project(FlightMonitor LANGUAGES C CXX ASM VERSION 0.0.1)

set(BLE_PASSWORD_TIMEOUT_MS 10000)
set(BLE_EXPECTED_PASSWORD "DEFAULT_PASS")
string(LENGTH ${BLE_EXPECTED_PASSWORD} BLE_PASS_LEN)
if(${BLE_PASS_LEN} GREATER 25)
  message(FATAL_ERROR "Password length must be <= 25 symbols")
endif()

configure_file(${PROJECT_SOURCE_DIR}/src/config.h.in ${PROJECT_SOURCE_DIR}/src/config.h @ONLY)
include(FetchContent)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

FetchContent_Declare(
    mavlink
    GIT_REPOSITORY https://github.com/mavlink/mavlink
    GIT_TAG        1b1e896aa3be0d591166e086e6b9d8613bd03716
    GIT_SUBMODULES_RECURSE TRUE
)

FetchContent_MakeAvailable(mavlink)

set(MAVLINK_VENV ${CMAKE_BINARY_DIR}/mavlink-venv)
set(MAVLINK_PYTHON ${MAVLINK_VENV}/bin/python)
set(MAVLINK_DIR ${CMAKE_BINARY_DIR}/generated/include)

set(MAVLINK_STAMP ${CMAKE_BINARY_DIR}/mavlink_generated.stamp)
add_custom_command(
    OUTPUT ${MAVLINK_PYTHON}
    COMMAND ${Python3_EXECUTABLE} -m venv ${MAVLINK_VENV}
    COMMAND ${MAVLINK_PYTHON} -m pip install --upgrade pip
    COMMAND ${MAVLINK_PYTHON} -m pip install -r ${mavlink_SOURCE_DIR}/pymavlink/requirements.txt
    COMMENT "[${PROJECT_NAME}] Creating MAVLink Python environment"
    VERBATIM)

add_custom_command(
    OUTPUT ${MAVLINK_STAMP}
    COMMAND ${CMAKE_COMMAND} -E env "PYTHONPATH=${mavlink_SOURCE_DIR}"
            ${MAVLINK_PYTHON} -m pymavlink.tools.mavgen
            --lang=C --wire-protocol=2.0
            --output=${MAVLINK_DIR}/mavlink
            ${mavlink_SOURCE_DIR}/message_definitions/v1.0/common.xml
    COMMAND ${CMAKE_COMMAND} -E touch ${MAVLINK_STAMP}
    DEPENDS ${MAVLINK_PYTHON} ${mavlink_SOURCE_DIR}/message_definitions/v1.0/common.xml
    COMMENT "[${PROJECT_NAME}] Generating MAVLink libraries for C."
    VERBATIM
)

add_custom_target(MAVLINK_LIBC DEPENDS ${MAVLINK_STAMP})


if(TARGET_PLATFORM STREQUAL "stm32")
    message(STATUS "[${PROJECT_NAME}]Building firware for STM32")
    add_subdirectory(src/firmware)

elseif(TARGET_PLATFORM STREQUAL "x64")
    message(STATUS "[${PROJECT_NAME}]Building desktop application for x64")
    add_subdirectory(src/desktop)

else()
    message(FATAL_ERROR "[${PROJECT_NAME}]Unknown TARGET_PLATFORM: ${TARGET_PLATFORM}")

endif()

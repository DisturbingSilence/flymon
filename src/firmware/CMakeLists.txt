

include(FetchContent)

set(CMSIS_Dcore "Cortex-M4")
set(CMSIS_Dtz "TZ-disabled")
set(CMSIS_Dsecure "Non-secure")
#set(CMSIS_ENTIRE_STMicroelectronics_CMSIS_6_3_0 ON)

FetchContent_Declare(
  cmsis
  GIT_REPOSITORY https://github.com/STMicroelectronics/cmsis-device-f4
  GIT_TAG 0fa0e489e053fa1ca7790bb40b4d76458f64c55d # release v2.6.11
  )
FetchContent_Declare(
  cmsis_core
  GIT_REPOSITORY https://github.com/STMicroelectronics/cmsis-core
  GIT_TAG 2327f7224ff212b2436e5a4cadda3288143fd041 # release 6.3.0
  )
FetchContent_Declare(
  hal
  GIT_REPOSITORY https://github.com/STMicroelectronics/stm32f4xx-hal-driver
  GIT_TAG b6f0ed3829f3829eb358a2e7417d80bba1a42db7 # release v1.8.5
  )
FetchContent_MakeAvailable(cmsis cmsis_core hal)

set(CMSIS_SOURCES
    ${cmsis_SOURCE_DIR}/Source/Templates/system_stm32f4xx.c
    ${cmsis_SOURCE_DIR}/Source/Templates/gcc/startup_stm32f411xe.s
)
set(LL_SOURCES
    ${hal_SOURCE_DIR}/Src/stm32f4xx_ll_gpio.c
    ${hal_SOURCE_DIR}/Src/stm32f4xx_ll_rcc.c
    #${hal_SOURCE_DIR}/Src/stm32f4xx_ll_utils.c
    ${hal_SOURCE_DIR}/Src/stm32f4xx_ll_usart.c
    ${hal_SOURCE_DIR}/Src/stm32f4xx_ll_i2c.c
    ${hal_SOURCE_DIR}/Src/stm32f4xx_ll_dma.c
    ${hal_SOURCE_DIR}/Src/stm32f4xx_ll_spi.c
    #${hal_SOURCE_DIR}/Src/stm32f4xx_ll_pwr.c
    ${hal_SOURCE_DIR}/Src/stm32f4xx_ll_tim.c
    #${hal_SOURCE_DIR}/Src/stm32f4xx_ll_exti.c
    ${hal_SOURCE_DIR}/Src/stm32f4xx_ll_adc.c
)
add_library(stm32f4_drivers STATIC ${LL_SOURCES} ${CMSIS_SOURCES})
target_include_directories(stm32f4_drivers PUBLIC
    ${cmsis_SOURCE_DIR}/Include
    ${cmsis_core_SOURCE_DIR}/CMSIS/Core/Include
    ${hal_SOURCE_DIR}/Inc
)
target_link_libraries(stm32f4_drivers PUBLIC STMicroelectronics_CMSIS_6_3_0)
target_compile_options(stm32f4_drivers PUBLIC -mcpu=cortex-m4 -mthumb)

set(BSP_P ${CMAKE_CURRENT_SOURCE_DIR}/bsp)
set(DRV_P ${CMAKE_CURRENT_SOURCE_DIR}/drivers)
set(SRV_P ${CMAKE_CURRENT_SOURCE_DIR}/services)
set(APP_P ${CMAKE_CURRENT_SOURCE_DIR}/app)

set(BSP_SOURCES
  ${BSP_P}/STM32F411CEU6.c)

set(DRIVER_SOURCES
  ${DRV_P}/peripherals/dma.c
  ${DRV_P}/peripherals/usart.c
  ${DRV_P}/peripherals/i2c.c
  ${DRV_P}/peripherals/spi.c

  ${DRV_P}/devices/w25qxx.c
  ${DRV_P}/devices/mpu60x0.c
  ${DRV_P}/devices/ble.c
  ${DRV_P}/devices/ssd1306.c

  ${DRV_P}/err.c
  ${DRV_P}/syscalls.c
  ${DRV_P}/systime.c
  ${DRV_P}/ringbuffer.c)

set(SERVICE_SOURCES
  ${SRV_P}/font.c
  ${SRV_P}/scheduler.c
  ${SRV_P}/buttons.c
  ${SRV_P}/ui.c)

add_executable(firmware
  ${CMAKE_CURRENT_SOURCE_DIR}/firmware_main.c
  ${BSP_SOURCES}
  ${DRIVER_SOURCES}
  ${SERVICE_SOURCES})

add_dependencies(firmware MAVLINK_LIBC)
target_include_directories(firmware PRIVATE ${PROJECT_SOURCE_DIR}/src ${MAVLINK_DIR})
set_target_properties(firmware PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set_target_properties(firmware PROPERTIES SUFFIX ".elf")
target_link_libraries(firmware PRIVATE stm32f4_drivers)
target_compile_features(firmware PRIVATE c_std_23)

set(COMMON_COMPILE_OPTIONS -Wall -mcpu=cortex-m4 -mthumb -Wall -pedantic -Wno-address-of-packed-member)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_options(firmware PRIVATE ${COMMON_COMPILE_OPTIONS} -O0 -g3 -ggdb)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
  target_compile_options(firmware PRIVATE ${COMMON_COMPILE_OPTIONS} -O3 -march=cortex-m4)
else()
  message(FATAL_ERROR "[${PROJECT_NAME}]Unknown CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE} for target STM32")
endif()

target_include_directories(firmware PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_options(firmware PRIVATE -T ${CMAKE_CURRENT_SOURCE_DIR}/linkerscript.ld -mcpu=cortex-m4 -mthumb -nostartfiles --specs=nano.specs --specs=nosys.specs)
target_compile_definitions(stm32f4_drivers PUBLIC USE_FULL_LL_DRIVER STM32F411xE)

add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/firmware.bin
  DEPENDS firmware
  COMMAND ${CMAKE_OBJCOPY} -O binary ${CMAKE_BINARY_DIR}/firmware.elf ${CMAKE_BINARY_DIR}/firmware.bin
  COMMENT "[${PROJECT_NAME}]Converting ELF ---> BINARY"
)

add_custom_target(
  flash
  DEPENDS ${CMAKE_BINARY_DIR}/firmware.bin
  COMMAND st-flash --reset write ${CMAKE_BINARY_DIR}/firmware.bin 0x8000000
  COMMENT "[${PROJECT_NAME}]Flashing firmware"
)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
add_custom_target(
  gdb
  DEPENDS gen_firmware
  COMMAND st-util
  COMMENT "[${PROJECT_NAME}]Starting gdb session"
)
endif()
add_custom_target(
  gen_firmware ALL
  DEPENDS ${CMAKE_BINARY_DIR}/firmware.bin
)

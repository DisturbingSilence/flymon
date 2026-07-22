cmake_minimum_required(VERSION 3.28)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_CROSSCOMPILING TRUE)

set(CMAKE_C_COMPILER "arm-none-eabi-gcc")
set(CMAKE_CXX_COMPILER "arm-none-eabi-g++")
set(CMAKE_ASM_COMPILER "arm-none-eabi-gcc")
set(CMAKE_OBJCOPY "arm-none-eabi-objcopy")

set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)
set(CMAKE_ASM_COMPILER_WORKS 1)
set(CMAKE_C_STANDARD 23)

project(AttitudeIndicator LANGUAGES C ASM)

add_subdirectory(deps)

set(BSP_P src/bsp)
set(DRV_P src/drivers)
set(SRV_P src/services)

set(BSP_SOURCES
  ${BSP_P}/STM32F411CEU6.c)
set(DRIVER_SOURCES
  ${DRV_P}/dma.c
  ${DRV_P}/err.c
  ${DRV_P}/i2c.c
  ${DRV_P}/ssd1306.c
  ${DRV_P}/syscalls.c
  ${DRV_P}/systime.c
  ${DRV_P}/spi.c
  ${DRV_P}/w25qxx.c)
set(SERVICE_SOURCES
  ${SRV_P}/font.c
  ${SRV_P}/scheduler.c)
add_executable(main
  src/app/main.c
  ${BSP_SOURCES}
  ${DRIVER_SOURCES}
  ${SERVICE_SOURCES})
set_target_properties(main PROPERTIES SUFFIX ".elf")
target_link_libraries(main PRIVATE stm32f4_drivers)
target_compile_options(main PRIVATE -O0 -g3 -Wall -mcpu=cortex-m4 -mthumb -ggdb)
target_include_directories(main PRIVATE src)
target_link_options(main PRIVATE -T ${CMAKE_SOURCE_DIR}/linkerscript.ld -mcpu=cortex-m4 -mthumb -nostartfiles --specs=nano.specs --specs=nosys.specs)
target_compile_definitions(stm32f4_drivers PUBLIC USE_FULL_LL_DRIVER STM32F411xE)
add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/firmware.bin
  DEPENDS main
  COMMAND ${CMAKE_OBJCOPY} -O binary ${CMAKE_BINARY_DIR}/main.elf ${CMAKE_BINARY_DIR}/firmware.bin
  COMMENT "Converting ELF ---> BINARY"
)
add_custom_target(
  flash
  DEPENDS gen_firmware
  COMMAND st-flash --reset write ${CMAKE_BINARY_DIR}/firmware.bin 0x8000000
  COMMENT "Flashing firmware"
)
add_custom_target(
  gdb
  DEPENDS gen_firmware
  COMMAND st-util
  COMMENT "Starting"
)
add_custom_target(
  gen_firmware ALL
  DEPENDS ${CMAKE_BINARY_DIR}/firmware.bin
)

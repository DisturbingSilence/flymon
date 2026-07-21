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
    #${hal_SOURCE_DIR}/Src/stm32f4xx_ll_spi.c
    #${hal_SOURCE_DIR}/Src/stm32f4xx_ll_pwr.c
    ${hal_SOURCE_DIR}/Src/stm32f4xx_ll_tim.c
    #${hal_SOURCE_DIR}/Src/stm32f4xx_ll_exti.c
)
add_library(stm32f4_drivers STATIC ${LL_SOURCES} ${CMSIS_SOURCES})
target_include_directories(stm32f4_drivers PUBLIC
    ${cmsis_SOURCE_DIR}/Include
    ${cmsis_core_SOURCE_DIR}/CMSIS/Core/Include
    ${hal_SOURCE_DIR}/Inc
)
target_link_libraries(stm32f4_drivers PUBLIC STMicroelectronics_CMSIS_6_3_0)
target_compile_options(stm32f4_drivers PUBLIC
    -mcpu=cortex-m4
    -mthumb -O0 -ggdb -g)

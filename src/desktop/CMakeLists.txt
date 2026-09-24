

# TODO: disable superfluous things in simpleBLE
set(FLTK_BUILD_FLTK_OPTIONS OFF)
set(FLTK_BUILD_FLUID OFF)
set(FLTK_BUILD_TEST OFF)
set(FLTK_OPTION_PRINT_SUPPORT OFF)
set(FLTK_OPTION_PEN_SUPPORT OFF)
set(FLTK_OPTION_SVG OFF)
set(FLTK_BUILD_HTML_DOCS OFF)
set(FLTK_BUILD_PDF_DOCS OFF)

FetchContent_Declare(
    simpleble
    GIT_REPOSITORY https://github.com/simpleble/simpleble
    GIT_TAG        3f73eff1b45d824fa4e5f78104310a4aad8bec4d
)
FetchContent_Declare(
    fltk
    GIT_REPOSITORY https://github.com/fltk/fltk
    GIT_TAG        95009ccd4291a4e1d536b3181529552a86d86428
)
FetchContent_GetProperties(simpleble)
FetchContent_MakeAvailable(fltk)

if(NOT simpleble_POPULATED)
    FetchContent_Populate(simpleble)
    list(APPEND CMAKE_MODULE_PATH "${simpleble_SOURCE_DIR}/cmake/find")
    add_subdirectory("${simpleble_SOURCE_DIR}/simpleble" "${simpleble_BINARY_DIR}")
endif()

add_executable(desktop_app
  desktop_main.cpp
  main_window.cpp
  widgets/ble_discovery_menu.cpp
  widgets/attitude_widget.cpp)

set_property(TARGET desktop_app PROPERTY CXX_EXTENSIONS OFF)
add_dependencies(desktop_app MAVLINK_LIBC)

target_include_directories(desktop_app PRIVATE ${PROJECT_SOURCE_DIR}/src ${MAVLINK_DIR})
target_compile_features(desktop_app PRIVATE cxx_std_20)
target_link_libraries(desktop_app PRIVATE simpleble::simpleble fltk fltk_gl)
target_compile_options(desktop_app PRIVATE -O3 -Wall -Wno-address-of-packed-member -Wno-unused-function)
set_target_properties(desktop_app PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

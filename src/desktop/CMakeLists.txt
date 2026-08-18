
add_executable(desktop_app desktop_main.cpp)
target_compile_features(desktop_app PRIVATE cxx_std_17)
set_target_properties(desktop_app PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

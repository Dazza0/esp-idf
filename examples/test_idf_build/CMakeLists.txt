# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

project(test_idf_build C)

# Include CMake file that provides ESP-IDF CMake build system APIs.
include($ENV{IDF_PATH}/tools/cmake/idf.cmake)

#set(EXTRA_COMPONENT_DIRS "./some_comp")
idf_build_component("./some_comp")

# Include ESP-IDF components in the build, may be thought as an equivalent of
# add_subdirectory() but with some additional processing and magic for ESP-IDF build
# specific build processes.
idf_build_process(esp32
                COMPONENTS freertos esptool_py some_comp
                SDKCONFIG ${CMAKE_CURRENT_LIST_DIR}/sdkconfig
                BUILD_DIR ${CMAKE_BINARY_DIR})

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(feat_a)
add_subdirectory(feat_b)
#add_subdirectory(exec)
add_executable(exec.elf "./exec/main.c")
target_link_libraries(exec.elf PUBLIC feat_a idf::newlib some_comp)
idf_build_executable(exec.elf)




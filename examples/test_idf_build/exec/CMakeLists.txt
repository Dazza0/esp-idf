# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

add_executable(exec.elf main.c)

target_link_libraries(exec.elf PUBLIC feat_a idf::newlib)

# Let the build system know what the project executable is to attach more targets, dependencies, etc.
idf_build_executable(exec.elf)


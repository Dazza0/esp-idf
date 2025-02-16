# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

add_library(feat_a STATIC "feat_a.c" "feat_a_helper.c")

target_include_directories(feat_a
                            PUBLIC "include"
                            PRIVATE "priv_include")

target_link_libraries(feat_a PRIVATE feat_b)

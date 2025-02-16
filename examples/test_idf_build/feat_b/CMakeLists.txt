# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

add_library(feat_b STATIC "feat_b.c" "feat_b_helper.c")

target_include_directories(feat_b
                            PUBLIC "include"
                            PRIVATE "priv_include")

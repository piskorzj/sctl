cmake_minimum_required(VERSION 3.19)
project(sctl)

set(CMAKE_CXX_STANDARD 20)

add_library(sctl INTERFACE)
target_include_directories(sctl INTERFACE inc)

enable_testing()
set(INSTALL_GTEST OFF)
add_subdirectory(externals/googletest)
include(GoogleTest)

add_executable(sctl_test test/simple_fsm.cpp test/complex_state_chart.cpp)
target_link_libraries(sctl_test sctl gmock gtest_main)
target_compile_options(sctl_test PRIVATE -Wall -Wextra -pedantic -Werror)

gtest_discover_tests(sctl_test)

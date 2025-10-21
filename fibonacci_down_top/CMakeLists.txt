cmake_minimum_required(VERSION 3.10)
project(FibonacciBottomUp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add GoogleTest	
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})
enable_testing()

# Include header files
include_directories(include)

# Main library
add_library(fibonacci_bottom_up src/fibonacci_bottom_up.cpp)

# Tests
add_executable(run_tests tests/test_fibonacci_bottom_up.cpp)
add_executable(fibonacci_main src/main.cpp)
target_link_libraries(fibonacci_main fibonacci_bottom_up)
target_link_libraries(run_tests fibonacci_bottom_up GTest::gtest_main)


add_test(NAME FibonacciBottomUpTests COMMAND run_tests)


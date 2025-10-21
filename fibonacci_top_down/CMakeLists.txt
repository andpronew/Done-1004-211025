cmake_minimum_required(VERSION 3.10)
project(fibonacci_test)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add source
add_subdirectory(src)
add_subdirectory(tests)

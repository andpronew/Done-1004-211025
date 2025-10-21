cmake_minimum_required(VERSION 3.16)
project(BudgetPlanner LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Включение флагов покрытия, если включено
option(COVERAGE "Enable coverage reporting" OFF)
if(COVERAGE)
    message(STATUS "Building with coverage flags")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g --coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()

# Пути
include_directories(include)
add_subdirectory(src)


enable_testing()
add_subdirectory(tests)

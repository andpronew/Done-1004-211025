cmake_minimum_required(VERSION 3.10)

project(my_zstd_project)

set(CMAKE_CXX_STANDARD 17)

add_executable(my_zstd my_zstd.cpp)

# Указываем путь к уже собранной статической библиотеке
target_link_libraries(my_zstd
    PRIVATE
    ${CMAKE_SOURCE_DIR}/zstd/build_zstd/lib/libzstd.a
)

target_include_directories(my_zstd
    PRIVATE
    ${CMAKE_SOURCE_DIR}/zstd/lib
)


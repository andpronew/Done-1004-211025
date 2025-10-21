cmake_minimum_required(VERSION 3.10)
project(binance_collector LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


include_directories(${CMAKE_SOURCE_DIR}/external)


add_executable(uploader src/uploader.cpp)
add_executable(downloader src/downloader.cpp)


# no extra libraries required (we use system calls for aws/curl)

cmake_minimum_required(VERSION 3.10)
project(binance_collector VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Find dependencies
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/external)

# Uploader (uses libcurl + OpenSSL)
add_executable(uploader src/uploader_real_binance.cpp)
target_link_libraries(uploader PRIVATE CURL::libcurl OpenSSL::Crypto)

# Downloader (uses libcurl or system curl; still link libcurl)
add_executable(downloader src/downloader.cpp)
target_link_libraries(downloader PRIVATE CURL::libcurl OpenSSL::Crypto)
target_link_libraries(uploader PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(downloader PRIVATE nlohmann_json::nlohmann_json)

# Install targets (optional)
install(TARGETS uploader downloader
    RUNTIME DESTINATION bin)


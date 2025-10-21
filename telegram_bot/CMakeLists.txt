cmake_minimum_required(VERSION 3.5)
project(telegram_bot)

set(CMAKE_CXX_STANDARD 17)

# Подключаем Boost вручную
find_package(Boost REQUIRED COMPONENTS system)

# Затем подключаем TgBot
find_package(TgBot REQUIRED)
find_package(CURL REQUIRED)

add_executable(telegram_bot main.cpp)
target_link_libraries(telegram_bot
    TgBot::TgBot
    Boost::system
)

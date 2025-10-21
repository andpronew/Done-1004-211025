# Собираем библиотеку
add_library(budget_lib STATIC budget_planner.cpp)

# Приложение с main.cpp
add_executable(budget_app main.cpp)
target_link_libraries(budget_app PRIVATE budget_lib)


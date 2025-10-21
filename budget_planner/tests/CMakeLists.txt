	# tests/CMakeLists.txt

cmake_minimum_required(VERSION 3.15)

# Используем системный GTest, установленный через apt
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

add_executable(run_tests test_budget_planner.cpp)
target_link_libraries(run_tests PRIVATE budget_lib GTest::gtest_main)
add_test(NAME BudgetPlannerTests COMMAND run_tests)


# Линкуем с библиотекой проекта и GTest
target_link_libraries(run_tests PRIVATE budget_lib GTest::gtest_main)

# Включаем поддержку тестов CTest
enable_testing()
add_test(NAME run_tests COMMAND run_tests)


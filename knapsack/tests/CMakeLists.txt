# Create the test executable
add_executable(run_knapsack_tests test_knapsack.cpp)

# Link with knapsack library and GoogleTest
target_link_libraries(run_knapsack_tests
    PRIVATE
        knapsack_lib
        gtest_main
)



# Enable CTest integration
include(GoogleTest)
gtest_discover_tests(run_knapsack_tests)



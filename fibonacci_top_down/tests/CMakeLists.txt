# Enable testing
enable_testing()

# Use local copy of GoogleTest
add_subdirectory(${CMAKE_SOURCE_DIR}/external/googletest ${CMAKE_BINARY_DIR}/external/googletest)

add_executable(test_fibonacci test_fibonacci.cpp)
target_link_libraries(test_fibonacci PRIVATE fibonacci_lib gtest_main)

include(GoogleTest)
gtest_discover_tests(test_fibonacci)

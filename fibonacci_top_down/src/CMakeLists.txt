add_library(fibonacci_lib fibonacci.cpp)
add_executable(fibonacci_main main.cpp)
target_link_libraries(fibonacci_main PRIVATE fibonacci_lib)

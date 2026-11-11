cmake_minimum_required(VERSION 3.16)
project(kitezh LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(kitezh
    src/vessel.cpp
    src/lock.cpp
    src/shift.cpp
)
target_include_directories(kitezh PUBLIC include)
target_compile_options(kitezh PRIVATE -Wall -Wextra -Wpedantic)

add_executable(kitezh_tests
    tests/main.cpp
    tests/test_vessel.cpp
    tests/test_lock.cpp
    tests/test_shift.cpp
)
target_include_directories(kitezh_tests PRIVATE tests)
target_link_libraries(kitezh_tests PRIVATE kitezh)
target_compile_options(kitezh_tests PRIVATE -Wall -Wextra -Wpedantic)

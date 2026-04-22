cmake_minimum_required(VERSION 3.16)
project(orion VERSION 0.1.0)



#Enforce modern C++ to make code cleaner and clearer
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


#-Wall -Wextra -Wpedantic are compiler flags that enable additional warnings to help catch potential issues in the code.
add_compile_options(-Wall -Wextra -Wpedantic)

#Which directories to look for the files
include_directories(include)

#We add source files here as we build them
# "Create a program called orion_tests and build it from the file main.cpp
#A CI system will run this executable to make sure our code is working as expected.
add_executable(orion_tests
    tests/main.cpp
    src/simulator.cpp
    src/test_engine.cpp
    src/anomaly_detector.cpp
    src/logger.cpp
)
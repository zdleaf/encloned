cmake_minimum_required(VERSION 3.0.0)
project(enclone VERSION 0.1.0)

#include(CTest)
#enable_testing()

# flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -lstdc++fs")

# include files
include_directories( ./include ./src )

# target
add_executable(enclone ./src/main.cpp ./src/Watch.cpp)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

include(CPack)

cmake_minimum_required(VERSION 3.0.0)
project(enclone VERSION 0.1.0)

set(CMAKE_VERBOSE_MAKEFILE OFF)

include(CTest)
enable_testing()

set(CMAKE_CXX_STANDARD, 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -lstdc++fs -lsqlite3")

# include files
include_directories( include src )

# target
add_executable(${PROJECT_NAME} ./src/main.cpp ./src/Watch.cpp ./src/FileIndex.cpp)

# link std filesystem library
target_link_libraries(enclone stdc++fs sqlite3)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

include(CPack)

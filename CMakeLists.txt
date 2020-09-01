cmake_minimum_required(VERSION 3.0.0)
project(encloned VERSION 0.1.0)

set(CMAKE_VERBOSE_MAKEFILE OFF)

include(CTest)
enable_testing()

set(CMAKE_CXX_STANDARD, 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# use shared libraries (default ON)
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_LIST_DIR}/cmake")

# find required components
find_package(AWSSDK REQUIRED COMPONENTS s3 transfer)
find_package(Boost REQUIRED COMPONENTS program_options)
find_package(sodium REQUIRED) # uses cmake/Findsodium.cmake
find_package(SQLite3 REQUIRED) # uses cmake/FindSQLite3.cmake

# flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lboost_system -lboost_thread -std=c++17 -lstdc++fs -lsqlite3 -pthread -lboost_program_options -lsodium")
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -latomic") # required for ARM processors e.g. RaspberryPi

# include files
include_directories(include src ${Boost_INCLUDE_DIR})

# targets
add_executable(encloned ./src/encloned.cpp ./src/Watch.cpp ./src/DB.cpp ./src/Socket.cpp ./src/remote/S3.cpp ./src/remote/Queue.cpp ./src/remote/Remote.cpp ./src/Encryption.cpp)
add_executable(enclone ./src/enclone.cpp)

# link required libraries
target_link_libraries(encloned stdc++fs sqlite3 ${AWSSDK_LINK_LIBRARIES})
target_link_libraries(enclone stdc++fs ${Boost_LIBRARIES})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

include(CPack)

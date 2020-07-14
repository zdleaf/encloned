cmake_minimum_required(VERSION 3.0.0)
project(enclone VERSION 0.1.0)

set(CMAKE_VERBOSE_MAKEFILE OFF)

include(CTest)
enable_testing()

set(CMAKE_CXX_STANDARD, 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# use shared libraries (default ON)
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

# find required components
find_package(AWSSDK REQUIRED COMPONENTS s3 transfer)
find_package(OpenSSL REQUIRED)
find_package(Boost COMPONENTS program_options REQUIRED)

# flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lboost_system -lboost_thread -std=c++17 -lstdc++fs -lsqlite3 -pthread -lcrypto -lboost_program_options")

# include files
include_directories( include src ${Boost_INCLUDE_DIR})

# targets
add_executable(encloned ./src/encloned.cpp ./src/Watch.cpp ./src/DB.cpp ./src/Socket.cpp ./src/remote/S3.cpp ./src/remote/Queue.cpp ./src/remote/Remote.cpp ./src/Encryption.cpp)
add_executable(enclone ./src/enclone.cpp)

# link std filesystem library
target_link_libraries(encloned stdc++fs sqlite3 ${AWSSDK_LINK_LIBRARIES} OpenSSL::SSL)
target_link_libraries(enclone stdc++fs ${Boost_LIBRARIES})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

include(CPack)

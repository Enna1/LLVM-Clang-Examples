cmake_minimum_required(VERSION 2.8.8)

set(PROJECT_NAME bf-jit)

project(${PROJECT_NAME})

find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_executable(${PROJECT_NAME} BFJit.h main.cpp)

#LLVM_AVAILABLE_LIBS is set in LLVMConfig.cmake

target_link_libraries(${PROJECT_NAME} ${LLVM_AVAILABLE_LIBS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -g -O0 -fno-strict-aliasing -fno-exceptions -fno-rtti")
message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

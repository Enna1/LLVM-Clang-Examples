cmake_minimum_required(VERSION 2.8.8)

set(PROJECT_NAME test-cvp)

project(${PROJECT_NAME})

find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "Using ClangConifg.cmake in ${CLANG_CMAKE_DIR}")
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${CLANG_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_executable(${PROJECT_NAME} FooPass.h main.cpp)

#LLVM_AVAILABLE_LIBS is set in LLVMConfig.cmake
target_link_libraries(${PROJECT_NAME} ${LLVM_AVAILABLE_LIBS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -g -O0 -Wno-unused-parameter -fno-strict-aliasing -fno-exceptions -fno-rtti")
message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

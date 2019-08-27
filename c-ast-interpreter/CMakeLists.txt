cmake_minimum_required(VERSION 2.8.8)

set(PROJECT_NAME ast-interpreter)

project(${PROJECT_NAME})

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR
    "In-source builds are not allowed. Please clean your source tree and try again.")
endif()

find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "Using ClangConifg.cmake in ${CLANG_CMAKE_DIR}")
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${CLANG_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

set(DEP_CLANG_LIBS
    clangAST
    clangBasic
    clangFrontend
    clangTooling
)

add_executable(${PROJECT_NAME} src/ASTInterpreter.cpp)

target_link_libraries(${PROJECT_NAME} ${DEP_CLANG_LIBS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -g -O0 -Wno-unused-parameter -fno-strict-aliasing -fno-exceptions -fno-rtti")

message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
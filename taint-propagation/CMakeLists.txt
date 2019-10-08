cmake_minimum_required(VERSION 2.8.8)

set(PROJECT_NAME test-tp)

project(${PROJECT_NAME})

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR
    "In-source builds are not allowed. Please clean your source tree and try again.")
endif()

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_executable(${PROJECT_NAME} src/TaintPropagation.cpp src/main.cpp)

llvm_map_components_to_libnames(DEP_LLVM_LIBS
  aggressiveinstcombine
  analysis
  asmparser
  bitreader
  bitwriter
  codegen
  core
  coroutines
  instcombine
  instrumentation
  ipo
  irreader
  mc
  objcarcopts
  scalaropts
  support
  target
  transformutils
  vectorize
)

target_link_libraries(${PROJECT_NAME} ${DEP_LLVM_LIBS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -g -O0 -fno-strict-aliasing -fno-exceptions -fno-rtti")
message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

cmake_minimum_required(VERSION 2.8.8)

set(PROJECT_NAME ast-interpreter)

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

add_definitions(
-D__STDC_LIMIT_MACROS
-D__STDC_CONSTANT_MACROS
)

#exec_program(find ARGS "${LLVM_LIBRARY_DIRS} -name 'libclang*.a'" OUTPUT_VARIABLE CLANG_LIBS_TMP)
#string(REPLACE "\n" "\\;" CLANG_LIBS_TMP "${CLANG_LIBS_TMP}")
#string(REGEX REPLACE "${LLVM_LIBRARY_DIRS}/lib|[.]a" "" CLANG_AVAILABLE_LIBS ${CLANG_LIBS_TMP})
#message(STATUS "${CLANG_AVAILABLE_LIBS}")
set(CLANG_AVAILABLE_LIBS
	clangAST
	clangBasic
	clangFrontend
	clangTooling
)

add_executable(${PROJECT_NAME} ASTInterpreter.cpp)
#LLVM_AVAILABLE_LIBS is set above
target_link_libraries(${PROJECT_NAME} ${CLANG_AVAILABLE_LIBS})
#LLVM_AVAILABLE_LIBS is set in LLVMConfig.cmake
target_link_libraries(${PROJECT_NAME} ${LLVM_AVAILABLE_LIBS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -g -O0 -Wno-unused-parameter -fno-strict-aliasing -fno-exceptions -fno-rtti")
cmake_minimum_required(VERSION 3.20.0)

project(topt C CXX ASM)

option(BUILD_SHARED_LIBS
  "Build all libraries as shared libraries instead of static" ON)
option(LLVM_BUILD_TOOLS
  "Build the LLVM tools. If OFF, just generate build targets." ON)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Using LLVMConfig.cmake in: ${LLVM_CMAKE_DIR}")

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")

include(HandleLLVMOptions)
include(AddLLVM)
include(TableGen)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/bin)

include_directories(${LLVM_INCLUDE_DIRS})

add_definitions(${LLVM_DEFINITIONS})

add_compile_options(-O0 -g)

set(TOPT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(TOPT_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(TOPT_LIB_DIRS ${TOPT_SOURCE_DIR}/lib)
set(TOPT_INCLUDE_DIRS ${TOPT_SOURCE_DIR}/include)

include_directories(AFTER ${TOPT_INCLUDE_DIRS})

add_subdirectory(lib)
add_subdirectory(tools)
add_subdirectory(test)

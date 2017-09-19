cmake_minimum_required(VERSION 2.6)
project(proto4z)

set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "")

set(LIB_SUFFIX "")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-Wall -g -ggdb -O0 )
    set(LIB_SUFFIX "_d")
else()
    add_definitions(-Wall -O2 -DNDEBUG )
endif()
set(CMAKE_CXX_FLAGS -std=c++11)



SET(PROTO4Z_GEN_CODE_PATH ${proto4z_SOURCE_DIR}/test/genCode)
SET(PROTO4Z_BIN_OUT_PATH ${proto4z_SOURCE_DIR}/test/bin)

add_subdirectory(genProto.tools)
add_subdirectory(test)

install(FILES 
${proto4z_SOURCE_DIR}/proto4z.h 
${proto4z_SOURCE_DIR}/dbHelper.h 
${proto4z_SOURCE_DIR}/Proto4z.cs 
${proto4z_SOURCE_DIR}/proto4z.lua 
${proto4z_SOURCE_DIR}/README.md 
${proto4z_SOURCE_DIR}/COPYRIGHT 
DESTINATION include/proto4z)


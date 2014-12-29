cmake_minimum_required(VERSION 2.6)
project(zsummerX)


set(DEP_PROTO4Z_INC_DIR  ${zsummerX_SOURCE_DIR}/depends CACHE PATH "")

set(DEP_LOG4Z_INC_DIR  ${zsummerX_SOURCE_DIR}/depends CACHE PATH "")

set(DEP_LOG4Z_LIB_DIR  ${zsummerX_SOURCE_DIR}/depends/log4z/lib CACHE PATH "")

set(DEP_LUA_INC_DIR  ${zsummerX_SOURCE_DIR}/depends/lua CACHE PATH "")

set(DEP_LUA_LIB_DIR  ${zsummerX_SOURCE_DIR}/depends/lua/lib CACHE PATH "")

set(ZSUMMERX_LIB_DIR ${zsummerX_SOURCE_DIR}/lib CACHE PATH "")

set(ZSUMMERX_INC_DIR ${zsummerX_SOURCE_DIR}/include CACHE PATH "")

set(ZSUMMERX_SRC_DIR ${zsummerX_SOURCE_DIR}/src CACHE PATH "")

if (APPLE)
  set(ZSUMMERX_USE_SELECT 1)
endif()


SET(LIBRARY_OUTPUT_PATH ${ZSUMMERX_LIB_DIR})

include_directories(${DEP_PROTO4Z_INC_DIR})
include_directories(${DEP_LOG4Z_INC_DIR})
include_directories(${DEP_LUA_INC_DIR})
include_directories(${ZSUMMERX_INC_DIR})

link_directories(${DEP_LOG4Z_LIB_DIR})
link_directories(${DEP_LUA_LIB_DIR})
link_directories(${ZSUMMERX_LIB_DIR})



set(LIB_SUFFIX "")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
set(LIB_SUFFIX "_d")
endif()



message(STATUS  DEP_PROTO4Z_INC_DIR=${DEP_PROTO4Z_INC_DIR})
message(STATUS  DEP_LOG4Z_INC_DIR=${DEP_LOG4Z_INC_DIR})
message(STATUS  DEP_LOG4Z_LIB_DIR=${DEP_LOG4Z_LIB_DIR})
message(STATUS  ZSUMMERX_INC_DIR=${ZSUMMERX_INC_DIR})
message(STATUS  ZSUMMERX_SRC_DIR=${ZSUMMERX_SRC_DIR})
message(STATUS  ZSUMMERX_LIB_NAME=${ZSUMMERX_LIB_NAME})
message(STATUS  ZSUMMERX_LIB_DIR=${ZSUMMERX_LIB_DIR})

add_subdirectory(depends)
add_subdirectory(g++)
add_subdirectory(example)



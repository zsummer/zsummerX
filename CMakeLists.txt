cmake_minimum_required(VERSION 2.6)
project(zsummerX)


set(DEP_PROTO4Z_INC_DIR  ${zsummerX_SOURCE_DIR}/depends CACHE PATH "")

set(DEP_LOG4Z_INC_DIR  ${zsummerX_SOURCE_DIR}/depends CACHE PATH "")

set(DEP_LOG4Z_LIB_DIR  ${zsummerX_SOURCE_DIR}/depends/log4z/lib CACHE PATH "")

set(ZSUMMERX_LIB_DIR ${zsummerX_SOURCE_DIR}/lib CACHE PATH "")

set(ZSUMMERX_INC_DIR ${zsummerX_SOURCE_DIR}/include CACHE PATH "")

set(ZSUMMERX_SRC_DIR ${zsummerX_SOURCE_DIR}/src CACHE PATH "")

if (ZSUMMERX_USE_SELECT)
  set(ZSUMMERX_LIB_NAME zsummerX_select)
else()
  set(ZSUMMERX_LIB_NAME zsummerX)
endif()

set(LIB_SUFFIX "")

SET(LIBRARY_OUTPUT_PATH ${ZSUMMERX_LIBS_DIR})

include_directories(${DEP_PROTO4Z_INC_DIR})
include_directories(${DEP_LOG4Z_INC_DIR})
include_directories(${ZSUMMERX_INC_DIR})

link_directories(${DEP_LOG4Z_LIB_DIR})
link_directories(${ZSUMMERX_LIB_DIR})



set(LIB_SUFFIX "")
if (WIN32)
  if(CMAKE_GENERATOR MATCHES "Win64")
    if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
      set(LIB_SUFFIX "_win64")
    else()
      set(LIB_SUFFIX "_win64d")
    endif()
  else()
    if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
      set(LIB_SUFFIX "_win32")
    else()
      set(LIB_SUFFIX "_win32d")
    endif()
  endif()
else()
  if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(ZSUMMERX_USE_SELECT)
      add_definitions(-Wall -g -ggdb -O0 -std=c++11 -D_GLIBCXX_USE_NANOSLEEP -DZSUMMERX_USE_SELECT)
    else()
      add_definitions(-Wall -g -ggdb -O0 -std=c++11 -D_GLIBCXX_USE_NANOSLEEP )
    endif()
    set(LIB_SUFFIX "_d")
  else()
    if(ZSUMMERX_USE_SELECT)
      add_definitions(-Wall  -O2 -DNDEBUG -std=c++11 -D_GLIBCXX_USE_NANOSLEEP -DZSUMMERX_USE_SELECT)
    else()
      add_definitions(-Wall  -O2 -DNDEBUG -std=c++11 -D_GLIBCXX_USE_NANOSLEEP )
    endif()
    set(LIB_SUFFIX "")
  endif()
endif()


message(STATUS  DEP_PROTO4Z_INC_DIR=${DEP_PROTO4Z_INC_DIR})
message(STATUS  DEP_LOG4Z_INC_DIR=${DEP_LOG4Z_INC_DIR})
message(STATUS  DEP_LOG4Z_LIB_DIR=${DEP_LOG4Z_LIB_DIR})
message(STATUS  ZSUMMERX_INC_DIR=${ZSUMMERX_INC_DIR})
message(STATUS  ZSUMMERX_SRC_DIR=${ZSUMMERX_SRC_DIR})
message(STATUS  ZSUMMERX_LIB_NAME=${ZSUMMERX_LIB_NAME})

add_subdirectory(depends)
if (ZSUMMERX_USE_SELECT)
  message(STATUS  "defined ZSUMMERX_USE_SELECT. make lib  ${ZSUMMERX_LIB_NAME} in  g++_select") 
  add_subdirectory(g++_select)
else()
  message(STATUS  "no define ZSUMMERX_USE_SELECT. make lib  ${ZSUMMERX_LIB_NAME} in  g++") 
  add_subdirectory(g++)
endif()
add_subdirectory(example)



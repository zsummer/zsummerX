cmake_minimum_required(VERSION 2.6)
project(zsummerX)

set(INC_DIR  ${zsummerX_SOURCE_DIR}/include)
set(SRC_DIR  ${zsummerX_SOURCE_DIR}/src)


set(DEP_PROTOCOL4Z_INC  ${zsummerX_SOURCE_DIR}/depends CACHE PATH "")

set(DEP_LOG4Z_INC  ${zsummerX_SOURCE_DIR}/depends CACHE PATH "")

set(DEP_LOG4Z_LIBS_DIR  ${zsummerX_SOURCE_DIR}/depends/log4z/libs CACHE PATH "")

set(ZSUMMERX_LIBS_DIR ${zsummerX_SOURCE_DIR}/libs CACHE PATH "")

set(ZSUMMERX_LIB_NAME zsummerX)
set(LIB_SUFFIX "")

SET(LIBRARY_OUTPUT_PATH ${ZSUMMERX_LIBS_DIR})

include_directories(${DEP_PROTOCOL4Z_INC})
include_directories(${DEP_LOG4Z_INC})
include_directories(${INC_DIR})

link_directories(${DEP_LOG4Z_LIBS_DIR})
link_directories(${ZSUMMERX_LIBS_DIR})



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
else(WIN32)
  if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-Wall -g -ggdb -O0 -std=c++11)
    set(LIB_SUFFIX "_d")
  else()
    add_definitions(-Wall -O2 -DNDEBUG -std=c++11)
    set(LIB_SUFFIX "")
  endif()
endif(WIN32)




add_subdirectory(g++)
add_subdirectory(example)
add_subdirectory(depends)


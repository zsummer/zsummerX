cmake_minimum_required(VERSION 2.6)
project(zsummer_11x)
set(LIB_DIR ../libs)
set(DEP_DIR ../depends)
set(INC_DIR ../include)
set(SRC_DIR ../src)
set(LIB_NAME ${PROJECT_NAME})

include_directories(${DEP_DIR})
include_directories(${INC_DIR})

aux_source_directory(${SRC_DIR}/common COMMON)
aux_source_directory(${SRC_DIR}/epoll NETWORK)
aux_source_directory(${SRC_DIR}/timer TIMER)
set(source  ${COMMON} ${NETWORK} ${TIMER} )

if (CMAKE_BUILD_TYPE STREQUAL "DEBUG")
  add_definitions(-Wall -g -ggdb -O0 -std=c++11)
  set(LIB_NAME ${PROJECT_NAME}_d)
else()
  add_definitions(-Wall -O2 -std=c++11 -DNODEBUG)
endif()

message("LIB_NAME=" ${LIB_NAME} ", LINKER FLAGS=" ${CMAKE_EXE_LINKER_FLAGS})

SET(LIBRARY_OUTPUT_PATH ${LIB_DIR})
add_library(${LIB_NAME} STATIC ${source})



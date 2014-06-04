cmake_minimum_required(VERSION 2.6)
project(log4z)

set(LIB_DIR ../libs)
#set(LIB_DIR ../../../libs)
set(LIB_NAME ${PROJECT_NAME})

set(source  ../log4z.cpp )

if (WIN32)
 string(REGEX MATCH "Win64" OUTR ${CMAKE_GENERATOR})
 if(${OUTR} STREQUAL "Win64")
  set(LIB_NAME ${LIB_NAME}_win64)
 else()
  set(LIB_NAME ${LIB_NAME}_win32)
 endif()
 if (CMAKE_BUILD_TYPE STREQUAL "DEBUG")
  set(LIB_NAME ${LIB_NAME}d)
 endif()
else()
 if (CMAKE_BUILD_TYPE STREQUAL "DEBUG")
   add_definitions(-Wall -g -ggdb -O0 )
   set(LIB_NAME ${LIB_NAME}_d)
 else()
   add_definitions(-Wall -O2 -DNODEBUG)
 endif()
endif()

SET(LIBRARY_OUTPUT_PATH ${LIB_DIR})
if(MSVC)
    set_target_properties( ${targetname} PROPERTIES LIBRARY_OUTPUT_PATH ${LIB_DIR} )
    set_target_properties( ${targetname} PROPERTIES LIBRARY_OUTPUT_PATH_Debug ${LIB_DIR} )
    set_target_properties( ${targetname} PROPERTIES LIBRARY_OUTPUT_PATH_Release ${LIB_DIR} )
endif()
message("LIB_NAME=" ${LIB_NAME} ", LINKER FLAGS=" ${CMAKE_EXE_LINKER_FLAGS})

add_library(${LIB_NAME} STATIC ${source})



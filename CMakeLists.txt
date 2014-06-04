cmake_minimum_required(VERSION 2.6)
project(zsummerX)

set(INC_DIR  ${zsummerX_SOURCE_DIR}/include)
set(SRC_DIR  ${zsummerX_SOURCE_DIR}/src)

if(NOT DEP_PROTOCOL4Z_DIR)
set(DEP_PROTOCOL4Z_DIR  ${zsummerX_SOURCE_DIR}/depends)
message(WARNING "DEP_PROTOCOL4Z_DIR NOT FOUND")
endif()
if(NOT DEP_LOG4Z_DIR)
set(DEP_LOG4Z_DIR  ${zsummerX_SOURCE_DIR}/depends)
message(WARNING "DEP_LOG4Z_DIR NOT FOUND")
endif()

if(NOT DEP_LIBS_DIR)
set(DEP_LIBS_DIR ${zsummerX_SOURCE_DIR}/depends/log4z/libs})
message(WARNING "DEP_LIBS_DIR NOT FOUND")
endif()
if(NOT OUT_LIB_DIR)
set(OUT_LIB_DIR ${zsummerX_SOURCE_DIR}/libs)
message(WARNING "OUT_LIB_DIR NOT FOUND")
endif()

SET(LIBRARY_OUTPUT_PATH ${OUT_LIB_DIR})

include_directories(${DEP_PROTOCOL4Z_DIR})
include_directories(${DEP_LOG4Z_DIR})
include_directories(${INC_DIR})

link_directories(${DEP_LIBS_DIR})
link_directories(${OUT_LIB_DIR})

add_subdirectory(g++)
add_subdirectory(example)
add_subdirectory(depends)


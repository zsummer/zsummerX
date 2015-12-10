cmake_minimum_required(VERSION 2.6)
project(cpptest)

include_directories(../../)
include_directories(../genCode)

set(EXECUTABLE_OUTPUT_PATH ${PROTO4Z_BIN_OUT_PATH})

add_executable(cpptest${LIB_SUFFIX} test.cpp)
if(APPLE)
	target_link_libraries(cpptest${LIB_SUFFIX}  pthread  m)
else()
	target_link_libraries(cpptest${LIB_SUFFIX}  pthread  rt m)
endif()



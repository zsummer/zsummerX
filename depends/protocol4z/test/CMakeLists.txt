cmake_minimum_required(VERSION 2.6)
project(test)


set(EXECUTABLE_OUTPUT_PATH ${PROTOCOL4Z_BIN_OUT_PATH})

add_executable(test${LIB_SUFFIX} test.cpp)




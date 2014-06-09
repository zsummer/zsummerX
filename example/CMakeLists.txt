cmake_minimum_required(VERSION 2.6)
project(example)
set(BIN_DIR ${example_SOURCE_DIR}/bin)
set(EXP_DIR ${example_SOURCE_DIR})
add_subdirectory(tcpTest)
add_subdirectory(udpTest)
add_subdirectory(frameTest)


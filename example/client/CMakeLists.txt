cmake_minimum_required(VERSION 2.6)
project(client)


aux_source_directory(../../depends/log4z LG_S)
aux_source_directory(../../network/epoll EP_S)
aux_source_directory(./ LL_S)

set(source ${LG_S} ${EP_S} ${LL_S})
message(${source})
link_libraries( rt pthread)
add_definitions( -g -ggdb -Wall -O0 -std=c++11)

set(EXECUTABLE_OUTPUT_PATH "../bin/")
add_executable(client ${source})



cmake_minimum_required(VERSION 2.6)
project(zsummer_server)

aux_source_directory(../../utility UT_S)
aux_source_directory(../../depends/thread4z TH_S)
aux_source_directory(../../depends/log4z LG_S)
aux_source_directory(../../network/epoll EP_S)
aux_source_directory(./ LL_S)

set(source ${UT_S} ${TH_S} ${LG_S} ${EP_S} ${LL_S})
message(${source})
link_libraries( rt pthread)
add_definitions( -g -ggdb -Wall -O0 -std=c++11)

set(EXECUTABLE_OUTPUT_PATH "../bin/")
add_executable(zsummer_server ${source})



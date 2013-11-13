cmake_minimum_required(VERSION 2.6)
project(client)


aux_source_directory(../../depends/log4z LOG_S)
aux_source_directory(../../network/common COM_S)
aux_source_directory(../../network/epoll EPL_S)
aux_source_directory(./ LOCAL_S)

set(source ${LOG_S} ${COM_S} ${EPL_S} ${LOCAL_S})
message(${source})
link_libraries( rt pthread)
add_definitions( -g -ggdb -Wall -O0 -std=c++11)

set(EXECUTABLE_OUTPUT_PATH "../bin/")
add_executable(client ${source})



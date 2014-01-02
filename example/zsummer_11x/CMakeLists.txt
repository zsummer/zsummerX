cmake_minimum_required(VERSION 2.6)
project(zsummer_11x)
aux_source_directory(${NET_DIR}/common COMMON)
aux_source_directory(${NET_DIR}/epoll NETWORK)
aux_source_directory(${NET_DIR}/timer TIMER)
set(source  ${COMMON} ${NETWORK} ${TIMER} )

include(${EXP_DIR}/library.txt)



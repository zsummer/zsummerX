cmake_minimum_required(VERSION 2.6)
project(libzsummerX)


aux_source_directory(${SRC_DIR}/common COMMON)
if(WIN32)
aux_source_directory(${SRC_DIR}/iocp NETWORK)
else()
aux_source_directory(${SRC_DIR}/epoll NETWORK)
endif(WIN32)
aux_source_directory(${SRC_DIR}/timer TIMER)
aux_source_directory(${SRC_DIR}/frame FRAME)

set(source  ${COMMON} ${NETWORK} ${TIMER} ${FRAME})

set(LIBRARY_OUTPUT_PATH, ${ZSUMMERX_LIBS_DIR})

add_library(${ZSUMMERX_LIB_NAME}${LIB_SUFFIX} STATIC ${source})

install(TARGETS ${ZSUMMERX_LIB_NAME}${LIB_SUFFIX} 
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib)
install(DIRECTORY ${zsummerX_SOURCE_DIR}/include/zsummerX  DESTINATION include)

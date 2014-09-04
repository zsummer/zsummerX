cmake_minimum_required(VERSION 2.6)
project(libzsummerX_select)


aux_source_directory(${SRC_DIR}/common COMMON)
aux_source_directory(${SRC_DIR}/select NETWORK_SELECT)
if(WIN32)
aux_source_directory(${SRC_DIR}/iocp NETWORK)
else()
aux_source_directory(${SRC_DIR}/epoll NETWORK)
endif(WIN32)
aux_source_directory(${SRC_DIR}/timer TIMER)
aux_source_directory(${SRC_DIR}/frame FRAME)

set(source  ${COMMON} ${TIMER} ${FRAME})

set(LIBRARY_OUTPUT_PATH ${ZSUMMERX_LIBS_DIR})

add_definitions(-DZSUMMERX_USE_SELECT)
add_library(${ZSUMMERX_LIB_NAME}_select${LIB_SUFFIX} STATIC ${source} ${NETWORK_SELECT})

install(TARGETS ${ZSUMMERX_LIB_NAME}_select${LIB_SUFFIX} 
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib)
install(DIRECTORY ${zsummerX_SOURCE_DIR}/include/zsummerX  DESTINATION include)

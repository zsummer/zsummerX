cmake_minimum_required(VERSION 2.6)
project(makelib_select)


aux_source_directory(${ZSUMMERX_SRC_DIR}/common COMMON)
aux_source_directory(${ZSUMMERX_SRC_DIR}/select NETWORK)
aux_source_directory(${ZSUMMERX_SRC_DIR}/timer TIMER)
aux_source_directory(${ZSUMMERX_SRC_DIR}/frame FRAME)

set(source  ${NETWORK} ${COMMON} ${TIMER} ${FRAME})

set(LIBRARY_OUTPUT_PATH ${ZSUMMERX_LIBS_DIR})

add_library(${ZSUMMERX_LIB_NAME}${LIB_SUFFIX} STATIC ${source} )

message(STATUS "add_library " ${ZSUMMERX_LIB_NAME}${LIB_SUFFIX} )

install(TARGETS ${ZSUMMERX_LIB_NAME}${LIB_SUFFIX} 
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib)
install(DIRECTORY ${zsummerX_SOURCE_DIR}/include/zsummerX  DESTINATION include)

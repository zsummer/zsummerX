cmake_minimum_required(VERSION 2.6)
project(makelib)


aux_source_directory(${ZSUMMERX_SRC_DIR}/common COMMON_SRC)

aux_source_directory(${ZSUMMERX_SRC_DIR}/select SELECT_SRC)

aux_source_directory(${ZSUMMERX_SRC_DIR}/epoll EPOLL_SRC)

aux_source_directory(${ZSUMMERX_SRC_DIR}/timer TIMER_SRC)

aux_source_directory(${ZSUMMERX_SRC_DIR}/frame FRAME_SRC)

if(APPLE)
else()
add_library(zsummerX${LIB_SUFFIX} STATIC ${COMMON_SRC} ${EPOLL_SRC} ${TIMER_SRC} ${FRAME_SRC})
endif()

add_library(zsummerX_select${LIB_SUFFIX} STATIC ${COMMON_SRC} ${SELECT_SRC} ${TIMER_SRC} ${FRAME_SRC})
set_target_properties(zsummerX_select${LIB_SUFFIX} PROPERTIES COMPILE_DEFINITIONS "__SELECT__=1")

 if (CMAKE_BUILD_TYPE STREQUAL "Debug")
 	add_definitions(-Wall -g -ggdb -O0 -std=c++11 -D_GLIBCXX_USE_NANOSLEEP)
 else()
 	add_definitions(-Wall  -O2 -DNDEBUG -std=c++11 -D_GLIBCXX_USE_NANOSLEEP)
 endif()





set(LIBRARY_OUTPUT_PATH ${ZSUMMERX_LIB_DIR})


if(APPLE)
else()
install(TARGETS zsummerX${LIB_SUFFIX}
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib)
endif()

install(TARGETS zsummerX_select${LIB_SUFFIX}
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib)

install(DIRECTORY ${zsummerX_SOURCE_DIR}/include/zsummerX  DESTINATION include)

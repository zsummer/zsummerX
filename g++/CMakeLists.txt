cmake_minimum_required(VERSION 2.6)
project(makelib)

aux_source_directory(${ZSUMMERX_SRC_DIR}/common COMMON_SRC)

aux_source_directory(${ZSUMMERX_SRC_DIR}/select SELECT_SRC)

aux_source_directory(${ZSUMMERX_SRC_DIR}/epoll EPOLL_SRC)

aux_source_directory(${ZSUMMERX_SRC_DIR}/timer TIMER_SRC)

aux_source_directory(${ZSUMMERX_SRC_DIR}/frame FRAME_SRC)

add_library(log4z${LIB_SUFFIX} STATIC ${ZSUMMERX_DEPENDS_DIR}/log4z/log4z.cpp)

if(APPLE)
else()
    add_library(zsummerX${LIB_SUFFIX} STATIC ${COMMON_SRC} ${EPOLL_SRC} ${TIMER_SRC} ${FRAME_SRC})
endif()

add_library(zsummerX_select${LIB_SUFFIX} STATIC ${COMMON_SRC} ${SELECT_SRC} ${TIMER_SRC} ${FRAME_SRC})
set_target_properties(zsummerX_select${LIB_SUFFIX} PROPERTIES COMPILE_DEFINITIONS "__SELECT__=1")


set(LIBRARY_OUTPUT_PATH ${ZSUMMERX_LIB_DIR})





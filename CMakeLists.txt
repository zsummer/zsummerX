cmake_minimum_required(VERSION 2.6)
project(zsummerX)


set(DEP_PROTO4Z_INC_DIR  ${zsummerX_SOURCE_DIR}/depends CACHE PATH "")

set(DEP_LOG4Z_INC_DIR  ${zsummerX_SOURCE_DIR}/depends CACHE PATH "")

set(DEP_LOG4Z_LIB_DIR  ${zsummerX_SOURCE_DIR}/depends/log4z/lib CACHE PATH "")

set(DEP_LUA_INC_DIR  ${zsummerX_SOURCE_DIR}/depends/lua CACHE PATH "")

set(DEP_LUA_LIB_DIR  ${zsummerX_SOURCE_DIR}/depends/lua/lib CACHE PATH "")

set(ZSUMMERX_LIB_DIR ${zsummerX_SOURCE_DIR}/lib CACHE PATH "")

set(ZSUMMERX_INC_DIR ${zsummerX_SOURCE_DIR}/include CACHE PATH "")

set(ZSUMMERX_SRC_DIR ${zsummerX_SOURCE_DIR}/src CACHE PATH "")

if (APPLE)
  set(__SELECT__ 1)
endif()


SET(LIBRARY_OUTPUT_PATH ${ZSUMMERX_LIB_DIR})

include_directories(${DEP_PROTO4Z_INC_DIR})
include_directories(${DEP_LOG4Z_INC_DIR})
include_directories(${DEP_LUA_INC_DIR})
include_directories(${ZSUMMERX_INC_DIR})

link_directories(${DEP_LOG4Z_LIB_DIR})
link_directories(${DEP_LUA_LIB_DIR})
link_directories(${ZSUMMERX_LIB_DIR})



set(LIB_SUFFIX "")
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	set(LIB_SUFFIX "_d")
endif()



message(STATUS  DEP_PROTO4Z_INC_DIR=${DEP_PROTO4Z_INC_DIR})
message(STATUS  DEP_LOG4Z_INC_DIR=${DEP_LOG4Z_INC_DIR})
message(STATUS  DEP_LOG4Z_LIB_DIR=${DEP_LOG4Z_LIB_DIR})
message(STATUS  ZSUMMERX_INC_DIR=${ZSUMMERX_INC_DIR})
message(STATUS  ZSUMMERX_SRC_DIR=${ZSUMMERX_SRC_DIR})
message(STATUS  ZSUMMERX_LIB_NAME=${ZSUMMERX_LIB_NAME})
message(STATUS  ZSUMMERX_LIB_DIR=${ZSUMMERX_LIB_DIR})

add_subdirectory(depends)
add_subdirectory(g++)
add_subdirectory(example)

install_files(lib ${DEP_LOG4Z_LIB_DIR}/*.a)
install_files(lib ${ZSUMMERX_LIB_DIR}/*.a)
install(FILES ${DEP_LOG4Z_INC_DIR}/log4z/log4z.h ${DEP_LOG4Z_INC_DIR}/log4z/COPYRIGHT DESTINATION include/log4z)
install(FILES ${DEP_PROTO4Z_INC_DIR}/proto4z/proto4z.h ${DEP_PROTO4Z_INC_DIR}/proto4z/COPYRIGHT ${DEP_PROTO4Z_INC_DIR}/proto4z/proto4z.lua DESTINATION include/proto4z)
install(FILES ${DEP_PROTO4Z_INC_DIR}/rc4/rc4_encryption.h DESTINATION include/rc4)
install(DIRECTORY ${ZSUMMERX_INC_DIR}/zsummerX DESTINATION include)
install(FILES ${zsummerX_SOURCE_DIR}/COPYRIGHT DESTINATION include/zsummerX)


#cmake . -DCMAKE_INSTALL_PREFIX=/Users/zhangyawei/Documents/github/breeze
#make
#make install


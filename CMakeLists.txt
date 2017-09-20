cmake_minimum_required(VERSION 2.6)
project(zsummerX)

#set(GLIBC_FLAG  "-v -static-libgcc -static-libstdc++ -Wl,--no-export-dynamic")
#set(GLIBC_INCLUDE "/home/summer/dependlibs/glibc_static/include")
#set(GLIBC_LIB "/home/summer/dependlibs/glibc_static/lib")
#add_definitions(-DGLIBC_INCLUDEDIR=${GLIBC_INCLUDE} -DGLIBC_LIBRARYDIR=${GLIBC_LIB})
#include_directories(${GLIBC_INCLUDE} )
#link_directories(${GLIBC_LIB})
#set_target_properties (${BINNAME} PROPERTIES LINK_FLAGS "${GLIBC_FLAG}")

set(ZSUMMERX_DEPENDS_DIR  ${zsummerX_SOURCE_DIR}/depends CACHE PATH "")

set(ZSUMMERX_LIB_DIR ${zsummerX_SOURCE_DIR}/lib CACHE PATH "")

set(ZSUMMERX_INC_DIR ${zsummerX_SOURCE_DIR}/include CACHE PATH "")

set(ZSUMMERX_SRC_DIR ${zsummerX_SOURCE_DIR}/src CACHE PATH "")

if (APPLE)
  set(__SELECT__ 1)
endif()


SET(LIBRARY_OUTPUT_PATH ${ZSUMMERX_LIB_DIR})

include_directories(${ZSUMMERX_DEPENDS_DIR})
include_directories(${ZSUMMERX_INC_DIR})

link_directories(${ZSUMMERX_LIB_DIR})



set(LIB_SUFFIX "")
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(LIB_SUFFIX "_d")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-Wall -g -ggdb -O0  -D_GLIBCXX_USE_NANOSLEEP)
else()
    add_definitions(-Wall  -O2 -DNDEBUG  -D_GLIBCXX_USE_NANOSLEEP)
endif()

set(CMAKE_CXX_FLAGS -std=c++11)

message(STATUS  ZSUMMERX_DEPENDS_DIR=${ZSUMMERX_DEPENDS_DIR})
message(STATUS  ZSUMMERX_INC_DIR=${ZSUMMERX_INC_DIR})
message(STATUS  ZSUMMERX_SRC_DIR=${ZSUMMERX_SRC_DIR})
message(STATUS  ZSUMMERX_LIB_DIR=${ZSUMMERX_LIB_DIR})


add_subdirectory(g++)
add_subdirectory(example)


install(FILES ${ZSUMMERX_DEPENDS_DIR}/log4z/log4z.h 
            ${ZSUMMERX_DEPENDS_DIR}/log4z/COPYRIGHT 
    DESTINATION include/log4z)

install(FILES ${ZSUMMERX_DEPENDS_DIR}/proto4z/proto4z.h 
        ${ZSUMMERX_DEPENDS_DIR}/proto4z/COPYRIGHT 
        ${ZSUMMERX_DEPENDS_DIR}/proto4z/proto4z.lua 
    DESTINATION include/proto4z)

install(FILES ${ZSUMMERX_DEPENDS_DIR}/rc4/rc4_encryption.h 
    DESTINATION include/rc4)

install(DIRECTORY ${ZSUMMERX_LIB_DIR}/ DESTINATION lib
	FILES_MATCHING PATTERN "*.a")

install(DIRECTORY ${ZSUMMERX_INC_DIR}/zsummerX 
    DESTINATION include)

install(FILES ${zsummerX_SOURCE_DIR}/COPYRIGHT 
    DESTINATION include/zsummerX)


#cmake . -DCMAKE_INSTALL_PREFIX=/Users/zhangyawei/Documents/github/breeze
#make
#make install


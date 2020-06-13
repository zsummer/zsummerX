cmake_minimum_required(VERSION 3.5)
project(zsummerX)
#enable_language(ASM)

#from github.com/zsummer

# plat : WIN32 APPLE UNIX    (UNIX contain UNIX like)

# CMAKE_CXX_COMPILER_ID:  GNU Intel Clang AppleClang MSVC 
# example IF (CMAKE_CXX_COMPILER_ID MATCHES "Clang") ENDIF()

# jump compiler works check
# if(WIN32)
#    set(CMAKE_C_COMPILER_WORKS TRUE)
#    set(CMAKE_CXX_COMPILER_WORKS TRUE)
# endif(WIN32)

# jump this project build when msvc 
# set_target_properties(${PROJECT_NAME} PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)

# set this project setup build when msvc 
# set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

# show msvc folder
#  SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON) 

# use C++ 14
# set(CMAKE_CXX_FLAGS -std=c++14) 

# 
# CMAKE_SOURCE_DIR   cmake root dir 
# CMAKE_CURRENT_SOURCE_DIR current cmakelist.txt dir  
# EXECUTABLE_OUTPUT_PATH can set it change bin out dir
# CMAKE_MODULE_PATH can set it change module dir 
# PROJECT_NAME cur project name 

# include 
# include_directories  
# link_directories 
# link_libraries 

# 
# execute_process
if( NOT CMAKE_CONFIGURATION_TYPES )
    set( CMAKE_CONFIGURATION_TYPES Debug Release )
endif( NOT CMAKE_CONFIGURATION_TYPES )

set(CMAKE_BUILD_TYPE Debug CACHE STRING "cache debug release "  )
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${CMAKE_CONFIGURATION_TYPES} )

#分组添加到IDE
MACRO(GROUP_SRC_BY_DIR FILES)
    FOREACH(PATH_NAME ${${FILES}})
        STRING(REGEX REPLACE ${CMAKE_CURRENT_SOURCE_DIR}/\(.*\) \\1 RELATIVE_PATH ${PATH_NAME})
        STRING(REGEX MATCH "(.+)/.*" HAS_GROUP ${RELATIVE_PATH})
        IF(HAS_GROUP)
                STRING(REGEX REPLACE "(.+)/.*" \\1 GROUP_NAME ${RELATIVE_PATH})
                STRING(REPLACE "/" "\\" GROUP_NAME ${GROUP_NAME})
                MESSAGE(${PATH_NAME} " into " ${GROUP_NAME})
                SOURCE_GROUP(${GROUP_NAME} FILES ${PATH_NAME})
        ELSE()
                SOURCE_GROUP("" FILES ${PATH_NAME})
        ENDIF()
    ENDFOREACH(PATH_NAME)
ENDMACRO(GROUP_SRC_BY_DIR)

MACRO(AUTO_INCLUDE HEADS)
    UNSET(DIR_LIST)
    FOREACH(HEADFILE ${HEADS})
        STRING( REGEX REPLACE "[^/\\\\]+$" " " DIR_NAME ${HEADFILE} )
        LIST(APPEND DIR_LIST ${DIR_NAME})
    ENDFOREACH()

    IF(DIR_LIST)
        LIST(REMOVE_DUPLICATES DIR_LIST)
    ENDIF()

    FOREACH(DIR_NAME ${DIR_LIST})
            MESSAGE("auto include " ${DIR_NAME} )
            INCLUDE_DIRECTORIES(${DIR_NAME})
    ENDFOREACH()
ENDMACRO(AUTO_INCLUDE)


macro(SUB_DIR_LIST result curdir)
    file(GLOB children RELATIVE ${curdir} ${curdir}/*)
    set(dirlist "")
    foreach(child ${children})
        if(IS_DIRECTORY ${curdir}/${child})
            LIST(APPEND dirlist ${child})
        endif()
    endforeach()
    set(${result} ${dirlist})
endmacro()

#通用设置部分 包括启用分组 设置 启动项目  
IF(WIN32)
    ADD_DEFINITIONS(-DWIN32 -W3)
    SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON) 
    #SET_PROPERTY(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
ELSE()

    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_definitions(-Wall -g -ggdb -O0  -D_GLIBCXX_USE_NANOSLEEP)
    else()
        add_definitions(-Wall  -O2 -DNDEBUG  -D_GLIBCXX_USE_NANOSLEEP)
    endif()
    ADD_DEFINITIONS(-Wall -O2 )
    SET(CMAKE_CXX_FLAGS -std=c++11)
    SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
    SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")
ENDIF()
add_definitions( -DLUA_COMPAT_FLOATSTRING)

#输出
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/example/bin)
LINK_DIRECTORIES(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include/)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/depends/)



SET(LIB_SUFFIX "")
IF (CMAKE_BUILD_TYPE STREQUAL "Debug")
    SET(LIB_SUFFIX "_d")
ENDIF()



#TEST
#自定义部分 
SUB_DIR_LIST(SUB_DIRS ${CMAKE_SOURCE_DIR}/example/)
FOREACH(SUB_DIR ${SUB_DIRS}) 
    MESSAGE(${SUB_DIR})
ENDFOREACH()

string(REPLACE "\\" "/" SOURCE_DIR ${CMAKE_SOURCE_DIR}/)
string(REPLACE "\\" "/" TARGET_DIR ${CMAKE_SOURCE_DIR}\\b/a.h)
string(REPLACE ${SOURCE_DIR} "" TARGET_DIR ${TARGET_DIR})
STRING(REGEX REPLACE "(.*)/.*\\..*" "\\1" TARGET_DIR ${TARGET_DIR})

#MESSAGE(${TARGET_DIR})
#STRING(REGEX REPLACE ".+/(.+)\\..*" "\\1" TEST_NAME ${CMAKE_SOURCE_DIR}/b/a.h)
#STRING(REGEX REPLACE ".+/(.+)\\..*" "\\1" TEST_NAME ${CMAKE_SOURCE_DIR}/b/a.h)
#MESSAGE(${TEST_NAME})

# third libs  
FILE(GLOB_RECURSE OTHER_H ${CMAKE_SOURCE_DIR}/depends/*.h)
AUTO_INCLUDE(OTHER_H)



# libzsummerx
FILE(GLOB_RECURSE LIB_H ${CMAKE_SOURCE_DIR}/include/*.h)
FILE(GLOB_RECURSE LIB_CROSS_C 
    ${CMAKE_SOURCE_DIR}/src/common/*.cpp 
    ${CMAKE_SOURCE_DIR}/src/timer/*.cpp 
    ${CMAKE_SOURCE_DIR}/src/frame/*.cpp)
FILE(GLOB_RECURSE LIB_EPOLL_C ${CMAKE_SOURCE_DIR}/src/epoll/*.cpp)
FILE(GLOB_RECURSE LIB_IOCP_C ${CMAKE_SOURCE_DIR}/src/iocp/*.cpp)
FILE(GLOB_RECURSE LIB_SELECT_C ${CMAKE_SOURCE_DIR}/src/select/*.cpp)

#SET(LIB_FILES ${LIB_H} ${LIB_C})

#GROUP_SRC_BY_DIR(LIB_FILES)


IF(APPLE)
    ADD_LIBRARY(zsummerX${LIB_SUFFIX} STATIC ${LIB_CROSS_C} ${LIB_SELECT_C})
    SET_TARGET_PROPERTIES(zsummerX${LIB_SUFFIX} PROPERTIES COMPILE_DEFINITIONS "__SELECT__=1")
ELSEIF(UNIX)
    ADD_LIBRARY(zsummerX${LIB_SUFFIX} STATIC ${LIB_CROSS_C} ${LIB_EPOLL_C})
ENDIF()

IF(UNIX)
    ADD_LIBRARY(zsummerX_select${LIB_SUFFIX} STATIC ${LIB_CROSS_C} ${LIB_SELECT_C})
    SET_TARGET_PROPERTIES(zsummerX_select${LIB_SUFFIX} PROPERTIES COMPILE_DEFINITIONS "__SELECT__=1")
ENDIF()

IF(WIN32)
    ADD_LIBRARY(zsummerX${LIB_SUFFIX} STATIC ${LIB_CROSS_C} ${LIB_IOCP_C})
    ADD_LIBRARY(zsummerX_epoll${LIB_SUFFIX} STATIC ${LIB_CROSS_C} ${LIB_EPOLL_C})
    SET_TARGET_PROPERTIES(zsummerX_epoll${LIB_SUFFIX} PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD TRUE)
    ADD_LIBRARY(zsummerX_select${LIB_SUFFIX} STATIC ${LIB_CROSS_C} ${LIB_SELECT_C})
    SET_TARGET_PROPERTIES(zsummerX_select${LIB_SUFFIX} PROPERTIES COMPILE_DEFINITIONS "__SELECT__=1")
    SET_TARGET_PROPERTIES(zsummerX_select${LIB_SUFFIX} PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD TRUE)
ENDIF()





#测试生成
SUB_DIR_LIST(SUB_DIRS ${CMAKE_SOURCE_DIR}/example/)
FOREACH(SUB_DIR ${SUB_DIRS}) 
    if("${SUB_DIR}" STREQUAL "bin")
        continue()
    elseif("${SUB_DIR}" STREQUAL "linux")
        continue()
    elseif("${SUB_DIR}" STREQUAL "boost_ping_pong")
        continue()
    endif()

    FILE(GLOB_RECURSE TEST_CPP  
        ${CMAKE_SOURCE_DIR}/example/${SUB_DIR}/*.cpp 
        ${CMAKE_SOURCE_DIR}/example/${SUB_DIR}/*.c)

    if("${SUB_DIR}" STREQUAL "luaTest")
        LIST(APPEND TEST_CPP ${CMAKE_SOURCE_DIR}/depends/proto4z/luasrc/lproto4z.c)
    endif()


    ADD_EXECUTABLE(${SUB_DIR}${LIB_SUFFIX} ${TEST_CPP})
    add_dependencies(${SUB_DIR}${LIB_SUFFIX} zsummerX${LIB_SUFFIX}) 
    if("${SUB_DIR}" STREQUAL "luaTest")
        target_include_directories(${SUB_DIR}${LIB_SUFFIX} PUBLIC ${CMAKE_SOURCE_DIR}/example/luaTest/lua)
    endif()

    IF(APPLE)
        target_link_libraries(${SUB_DIR}${LIB_SUFFIX}  zsummerX${LIB_SUFFIX} log4z${LIB_SUFFIX} pthread  m c)
        SET_TARGET_PROPERTIES(${SUB_DIR}${LIB_SUFFIX} PROPERTIES COMPILE_DEFINITIONS "__SELECT__=1")
    ELSEIF(UNIX)
        target_link_libraries(${SUB_DIR}${LIB_SUFFIX}  zsummerX${LIB_SUFFIX} log4z${LIB_SUFFIX} pthread  rt m c)
    ELSE()
        target_link_libraries(${SUB_DIR}${LIB_SUFFIX}  zsummerX${LIB_SUFFIX} log4z${LIB_SUFFIX})
    ENDIF()
    add_dependencies(${SUB_DIR}${LIB_SUFFIX} zsummerX${LIB_SUFFIX})



    ADD_EXECUTABLE(${SUB_DIR}_select${LIB_SUFFIX} ${TEST_CPP})
    add_dependencies(${SUB_DIR}_select${LIB_SUFFIX} zsummerX_select${LIB_SUFFIX})
    SET_TARGET_PROPERTIES(${SUB_DIR}_select${LIB_SUFFIX} PROPERTIES COMPILE_DEFINITIONS "__SELECT__=1")

    if("${SUB_DIR}" STREQUAL "luaTest")
        target_include_directories(${SUB_DIR}_select${LIB_SUFFIX} PUBLIC ${CMAKE_SOURCE_DIR}/example/luaTest/lua)
    endif()

    IF(APPLE)
        target_link_libraries(${SUB_DIR}_select${LIB_SUFFIX}  zsummerX${LIB_SUFFIX} log4z${LIB_SUFFIX} pthread m c)
    ELSEIF(UNIX)
        target_link_libraries(${SUB_DIR}_select${LIB_SUFFIX}  zsummerX${LIB_SUFFIX} log4z${LIB_SUFFIX} pthread  rt m c)
    ELSE()
        target_link_libraries(${SUB_DIR}_select${LIB_SUFFIX}  zsummerX${LIB_SUFFIX} log4z${LIB_SUFFIX})
        SET_TARGET_PROPERTIES(${SUB_DIR}_select${LIB_SUFFIX} PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD TRUE)
    ENDIF()

ENDFOREACH()








install(FILES ${ZSUMMERX_DEPENDS_DIR}/log4z/log4z.h 
            ${ZSUMMERX_DEPENDS_DIR}/log4z/COPYRIGHT 
    DESTINATION include/log4z)

install(FILES ${ZSUMMERX_DEPENDS_DIR}/proto4z/proto4z.h 
        ${ZSUMMERX_DEPENDS_DIR}/proto4z/COPYRIGHT 
        ${ZSUMMERX_DEPENDS_DIR}/proto4z/proto4z.lua 
        ${ZSUMMERX_DEPENDS_DIR}/proto4z/Proto4z.cs
        ${ZSUMMERX_DEPENDS_DIR}/proto4z/dbHelper.h
    DESTINATION include/proto4z)

install(FILES ${ZSUMMERX_DEPENDS_DIR}/proto4z/luasrc/lproto4z.c
    DESTINATION include/proto4z/luasrc)

install(FILES ${ZSUMMERX_DEPENDS_DIR}/rc4/rc4_encryption.h 
    DESTINATION include/rc4)

install(DIRECTORY ${ZSUMMERX_LIB_DIR}/ DESTINATION lib
	FILES_MATCHING PATTERN "*.a")

install(DIRECTORY ${ZSUMMERX_INC_DIR}/zsummerX 
    DESTINATION include)

install(FILES ${zsummerX_SOURCE_DIR}/COPYRIGHT 
    DESTINATION include/zsummerX)

install(FILES ${ZSUMMERX_DEPENDS_DIR}/proto4z/luasrc/lproto4z.c
	${zsummerX_SOURCE_DIR}/example/luaTest/summer.h
	${zsummerX_SOURCE_DIR}/example/luaTest/summer.cpp
    DESTINATION include/zsummerX/luasrc)

#cmake . -DCMAKE_INSTALL_PREFIX=/Users/zhangyawei/Documents/github/breeze
#make
#make install


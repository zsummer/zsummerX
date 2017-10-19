cmake_minimum_required(VERSION 2.6)
project(luaTest)
include_directories(${example_SOURCE_DIR}/luaTest/lua/)
aux_source_directory(./lua/ lua_src)  
set(lua_src ${lua_src} ${ZSUMMERX_DEPENDS_DIR}/proto4z/luasrc/lproto4z.c)

include(${EXP_DIR}/common.txt)





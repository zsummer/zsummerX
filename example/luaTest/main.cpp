/*
 * zsummerX License
 * -----------
 * 
 * zsummerX is licensed under the terms of the MIT license reproduced below.
 * This means that zsummerX is free software and can be used for both academic
 * and commercial purposes at absolutely no cost.
 * 
 * 
 * ===============================================================================
 * 
 * Copyright (C) 2010-2014 YaweiZhang <yawei_zhang@foxmail.com>.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 * ===============================================================================
 * 
 * (end of COPYRIGHT)
 */


#include "summer.h"
extern "C"
{
#include "lua/lpack.h"
int luaopen_protoz_bit(lua_State *L);
}




#include <zsummerX/frameX.h>
using namespace zsummer::log4z;

int main(int argc, char* argv[])
{
	ILog4zManager::getRef().start();
	ILog4zManager::getRef().setLoggerFileLine(LOG4Z_MAIN_LOGGER_ID, false);
	int status;
	lua_State *L = luaL_newstate();  /* create state */
	if (L == NULL) 
	{
		return EXIT_FAILURE;
	}

	lua_gc(L, LUA_GCSTOP, 0);  /* stop collector during initialization */
	luaL_openlibs(L);  /* open libraries */
	luaopen_summer(L);
	luaopen_protoz_bit(L);
	luaopen_pack(L);

	lua_gc(L, LUA_GCRESTART, 0);
	status = luaL_dofile(L, "./main.lua");

	if (status && !lua_isnil(L, -1)) 
	{
		const char *msg = lua_tostring(L, -1);
		if (msg == NULL) msg = "(error object is not a string)";
		LOGE( msg);
		lua_pop(L, 1);
	}
	lua_close(L);
	return (status) ? EXIT_FAILURE : EXIT_SUCCESS;
}


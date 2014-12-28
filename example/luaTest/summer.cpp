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

#include <zsummerX/frameX.h>


const static char * _staticConnect = "connect";


static int logt(lua_State * L)
{
	const char * log = luaL_checkstring(L, 1);
	LOGT("from lua: " << log);
	return 0;
}
static int logd(lua_State * L)
{
	const char * log = luaL_checkstring(L, 1);
	LOGD("from lua: " << log);
	return 0;
}
static int logi(lua_State * L)
{
	const char * log = luaL_checkstring(L, 1);
	LOGI("from lua: " << log);
	return 0;
}
static int logw(lua_State * L)
{
	const char * log = luaL_checkstring(L, 1);
	LOGW("from lua: " << log);
	return 0;
}
static int loge(lua_State * L)
{
	const char * log = luaL_checkstring(L, 1);
	LOGE("from lua: " << log);
	return 0;
}
static int logf(lua_State * L)
{
	const char * log = luaL_checkstring(L, 1);
	LOGF("from lua: " << log);
	return 0;
}
static int loga(lua_State * L)
{
	const char * log = luaL_checkstring(L, 1);
	LOGA("from lua: " << log);
	return 0;
}

//////////////////////////////////////////////////////////////////////////


static int start(lua_State *L)
{
	TcpSessionManager::getRef().start();
	return 0;
}

static int stop(lua_State *L)
{
	TcpSessionManager::getRef().stop();
	return 0;
}

static int runOnce(lua_State * L)
{
	bool isImmediately = false;
	if (lua_gettop(L) > 0 && !lua_isnil(L, -1))
	{
		isImmediately = true;
	}
	TcpSessionManager::getRef().runOnce(isImmediately);
	return 0;
}



static int addConnect(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 1)
	{
		LOGE("addConnect error. argc is not 1.  the argc=" << argc);
		lua_settop(L, 0);
		lua_pushnil(L);
		return 1;
	}

	tagConnctorConfigTraits config;


	lua_getfield(L, 1, "ip");
	const char * ip = luaL_checkstring(L, -1);
	if (ip)
	{
		config.remoteIP = ip;
	}
	
	lua_getfield(L, 1, "port");
	unsigned short port = luaL_checknumber(L, -1);
	config.remotePort = port;

	

	lua_pop(L, 0);

	LOGD("addConnect:" << config);
	SessionID id = TcpSessionManager::getRef().addConnector(config);
	if (id == InvalidSeesionID)
	{
		lua_pushnil(L);
	}
	else
	{
		lua_pushnumber(L, id);
	}
	return 1;
}


void _checkCallbackTable(lua_State *L, const char * tname)
{
	int index = lua_gettop(L);
	lua_getfield(L, LUA_GLOBALSINDEX, "summer");
	luaL_checktype(L, -1, LUA_TTABLE);

	lua_getfield(L, -1, "__summerCallback");
	if (lua_isnil(L, -1))
	{
		lua_pop(L, 1);
		lua_newtable(L);
		lua_setfield(L, -2, "__summerCallback");
		lua_getfield(L, -1, "__summerCallback");
	}
	lua_getfield(L, -1, tname);
	index = lua_gettop(L);
}
static int registerMessage(lua_State * L)
{
// 	int index = lua_gettop(L);
// 	if (index != 1)
// 	{
// 		LOGE("addConnect error. argc is not 1.  the argc=" << index);
// 		lua_settop(L, 0);
// 		lua_pushnil(L);
// 		return 1;
// 	}
// 	if (!_checkCallbackTable(L, "message"))
// 	{
// 		lua_settop(L, 0);
// 		lua_pushnil(L);
// 		return 1;
// 	}

	
	return 1;
}



void _onConnectCallback(lua_State * L, SessionID sID)
{
	if (!isConnectID(sID))
	{
		return;
	}
	
	// if register, don't free L before stop zsummerX,  another it's will have wrong in here.
	int index = lua_gettop(L);
	_checkCallbackTable(L, _staticConnect);
	if (lua_isnil(L, -1))
	{
		lua_settop(L, index);
		return;
	}
	index = lua_gettop(L);
	lua_pushnumber(L, sID);
	index = lua_gettop(L);
	lua_pcall(L, 1, 0, 0);
}

static int registConnect(lua_State * L)
{
	int index = lua_gettop(L);
	if (index != 1)
	{
		LOGE("addConnect error. argc is not 1.  the argc=" << index);
		lua_settop(L, 0);
		return 0;
	}
	_checkCallbackTable(L, _staticConnect);
	if (!lua_isnil(L, -1))
	{
		lua_settop(L, 0);
		return 0;
	}
	lua_pop(L, 1);
	lua_pushvalue(L, index);
	lua_setfield(L, -2, _staticConnect);
	lua_settop(L, 0);

	lua_pushfstring(L, "mycallback");
	_checkCallbackTable(L, "zsummer");
	lua_pop(L, 1);
	lua_pushvalue(L, 1);
	lua_setfield(L, -2, "zsummer");

	_checkCallbackTable(L, _staticConnect);
	if (lua_isnil(L, -1))
	{
		lua_settop(L, 0);
		return 0;
	}

	MessageDispatcher::getRef().registerOnSessionEstablished(std::bind(_onConnectCallback, L, std::placeholders::_1));
	return 0;
}


luaL_Reg summer[] = {
	{ "logd", logd },
	{ "logi", logi },
	{ "logt", logt },
	{ "logw", logw },
	{ "loge", loge },
	{ "logf", logf },
	{ "loga", loga },

	{ "start", start },
	{ "stop", stop },
	{ "runOnce", runOnce },
	{ "addConnect", addConnect },
	{ "registConnect", registConnect },

	{ NULL, NULL }
};



void registerSummer(lua_State* L)
{
	luaL_register(L, "summer", summer);
 	lua_pushcfunction(L, logi);
 	lua_setglobal(L, "print");
}





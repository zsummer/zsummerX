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

const static char * _staticConnect = "connect";
const static char * _staticMessage = "message";
const static char * _staticDisconnect = "disconnect";

static std::vector<lua_State*> _staticValidVM;

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
	unsigned short port = (unsigned short)luaL_checknumber(L, -1);
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





void _onConnecterCallback(lua_State * L, SessionID sID)
{
	if (!isConnectID(sID))
	{
		return;
	}
	auto founder = std::find_if(_staticValidVM.begin(), _staticValidVM.end(), [L](lua_State* l){return l == L; });
	if (founder == _staticValidVM.end())
	{
		LOGW("_onConnecterCallback warning: L is not found . L=" << L);
		return;
	}

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

static int registerConnect(lua_State * L)
{
	int index = lua_gettop(L);
	if (index != 1)
	{
		LOGE("registerConnect error. argc is not 1.  the argc=" << index);
		lua_settop(L, 0);
		return 0;
	}
	_checkCallbackTable(L, _staticConnect);
	if (!lua_isnil(L, -1))
	{
		LOGE("registerMessage error. already registered.");
		lua_settop(L, 0);
		return 0;
	}
	lua_pop(L, 1);
	lua_pushvalue(L, index);
	lua_setfield(L, -2, _staticConnect);
	lua_settop(L, 0);

	MessageDispatcher::getRef().registerOnSessionEstablished(std::bind(_onConnecterCallback, L, std::placeholders::_1));
	return 0;
}


void _onMessageCallback(lua_State * L, SessionID sID, ProtoID pID, ReadStreamPack & rs)
{
	if (!isConnectID(sID))
	{
		return;
	}
	auto founder = std::find_if(_staticValidVM.begin(), _staticValidVM.end(), [L](lua_State* l){return l == L; });
	if (founder == _staticValidVM.end())
	{
		LOGW("_onConnecterCallback warning: L is not found . L=" << L);
		return;
	}

	int index = lua_gettop(L);
	_checkCallbackTable(L, _staticMessage);
	if (lua_isnil(L, -1))
	{
		lua_settop(L, index);
		return;
	}
	index = lua_gettop(L);
	lua_pushinteger(L, sID);
	lua_pushinteger(L, pID);
	lua_pushlstring(L, rs.getStreamUnread(), rs.getStreamUnreadLen());
	index = lua_gettop(L);
	lua_pcall(L, 3, 0, 0);
}

static int registerMessage(lua_State * L)
{
	int index = lua_gettop(L);
	if (index != 1)
	{
		LOGE("registerMessage error. argc is not 1.  the argc=" << index);
		lua_settop(L, 0);
		return 0;
	}
	_checkCallbackTable(L, _staticMessage);
	if (!lua_isnil(L, -1))
	{
		LOGE("registerMessage error. already registered.");
		lua_settop(L, 0);
		return 0;
	}
	lua_pop(L, 1);
	lua_pushvalue(L, index);
	lua_setfield(L, -2, _staticMessage);
	lua_settop(L, 0);

	MessageDispatcher::getRef().registerSessionDefaultMessage(std::bind(_onMessageCallback, L, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	return 0;
}



void _onDisconnectCallback(lua_State * L, SessionID sID)
{
	if (!isConnectID(sID))
	{
		return;
	}
	auto founder = std::find_if(_staticValidVM.begin(), _staticValidVM.end(), [L](lua_State* l){return l == L; });
	if (founder == _staticValidVM.end())
	{
		LOGW("_onDisconnectCallback warning: L is not found . L=" << L);
		return;
	}

	int index = lua_gettop(L);
	_checkCallbackTable(L, _staticDisconnect);
	if (lua_isnil(L, -1))
	{
		lua_settop(L, index);
		return;
	}
	index = lua_gettop(L);
	lua_pushinteger(L, sID);
	index = lua_gettop(L);
	lua_pcall(L, 1, 0, 0);
}

static int registerDisconnect(lua_State * L)
{
	int index = lua_gettop(L);
	if (index != 1)
	{
		LOGE("registerDisconnect error. argc is not 1.  the argc=" << index);
		lua_settop(L, 0);
		return 0;
	}
	_checkCallbackTable(L, _staticDisconnect);
	if (!lua_isnil(L, -1))
	{
		LOGE("registerDisconnect error. already registered.");
		lua_settop(L, 0);
		return 0;
	}
	lua_pop(L, 1);
	lua_pushvalue(L, index);
	lua_setfield(L, -2, _staticDisconnect);
	lua_settop(L, 0);

	MessageDispatcher::getRef().registerOnSessionDisconnect(std::bind(_onDisconnectCallback, L, std::placeholders::_1));
	return 0;
}


static int sendContent(lua_State * L)
{
	int index = lua_gettop(L);
	if (index != 3)
	{
		LOGE("sendContent error. argc is not 3.  the argc=" << index);
		lua_settop(L, 0);
		return 0;
	}
	SessionID sID = luaL_checkint(L, 1);
	unsigned short pID = luaL_checkint(L, 2);

	size_t len = 0;
	const char * content = luaL_checklstring(L, 3, &len);
	WriteStreamPack ws;
	ws << pID;
	ws.appendOriginalData(content, len);
	TcpSessionManager::getRef().sendOrgSessionData(sID, ws.getStream(), ws.getStreamLen());

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
	{ "registerConnect", registerConnect },
	{ "registerMessage", registerMessage },
	{ "registerDisconnect", registerDisconnect },
	{ "sendContent", sendContent },

	{ NULL, NULL }
};



void registerSummer(lua_State* L)
{
	auto founder = std::find_if(_staticValidVM.begin(), _staticValidVM.end(), [L](lua_State* l){return l == L; });
	if (founder == _staticValidVM.end())
	{
		LOGI("registerSummer: L=" << L);
		_staticValidVM.push_back(L);
	}
	else
	{
		LOGW("registerSummer warning: L is registered . L=" << L);
	}
	luaL_register(L, "summer", summer);
 	lua_pushcfunction(L, logi);
 	lua_setglobal(L, "print");
}

void unregisterSummer(lua_State *L)
{
	auto founder = std::find_if(_staticValidVM.begin(), _staticValidVM.end(), [L](lua_State* l){return l == L; });
	if (founder != _staticValidVM.end())
	{
		LOGI("unregisterSummer: L=" << L);
		_staticValidVM.erase(founder);
	}
	else
	{
		LOGW("unregisterSummer warning: L is not found. L=" << L);
	}
}





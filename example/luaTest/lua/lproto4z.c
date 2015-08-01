/*
 * proto4z License
 * -----------
 * 
 * proto4z is licensed under the terms of the MIT license reproduced below.
 * This means that proto4z is free software and can be used for both academic
 * and commercial purposes at absolutely no cost.
 * 
 * 
 * ===============================================================================
 * 
 * Copyright (C) 2013-2015 YaweiZhang <yawei.zhang@foxmail.com>.
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




#include "lua.h"
#include "lauxlib.h"
#include <string.h>
#include <stdbool.h>


int luaopen_proto4z_tag(lua_State *L);


static int newTag(lua_State * L)
{
    unsigned long long tag = 0;
    lua_pushlstring(L, (const char*)&tag, sizeof(tag));
    return 1;
}

static int setTag(lua_State * L)
{
    unsigned long long tag = 0;
    const char * luaTag = luaL_checkstring(L, 1);
    int n = (int)luaL_checkinteger(L, 2);
    memcpy(&tag, luaTag, sizeof(tag));
    tag |= (unsigned long long)1 << (n - 1);
    lua_pushlstring(L, (const char*)&tag, sizeof(tag));
    return 1;
}

static int unsetTag(lua_State * L)
{
    unsigned long long tag = 0;
    const char * luaTag = luaL_checkstring(L, 1);
    int n = (int)luaL_checkinteger(L, 2);
    memcpy(&tag, luaTag, sizeof(tag));
    tag &= ~((unsigned long long)1 << (n - 1));
    lua_pushlstring(L, (const char*)&tag, sizeof(tag));
    return 1;
}

static int testTag(lua_State * L)
{
    unsigned long long tag = 0;
    const char * luaTag = luaL_checkstring(L, 1);
    int n = (int)luaL_checkinteger(L, 2);
    memcpy(&tag, luaTag, sizeof(tag));
    int ret = tag & ((unsigned long long)1 << (n - 1));
    lua_pushboolean(L, ret);
    return 1;
}


static luaL_Reg tagReg[] = {
    { "newTag", newTag },
    { "setTag", setTag },
    { "unsetTag", unsetTag },
    { "testTag", testTag },
    { NULL, NULL }
};


int luaopen_proto4z_tag(lua_State *L)
{
    lua_newtable(L);
    luaL_Reg *l;
    for (l = tagReg; l->name != NULL; l++) 
    {
        lua_pushcclosure(L, l->func, 0);
        lua_setfield(L, -2, l->name);
    }
    lua_setglobal(L, "Proto4zTag");
    return 0;
}


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
 * Copyright (C) 2010-2017 YaweiZhang <yawei.zhang@foxmail.com>.
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


#include "performance.h"

static void hook_run_fn(lua_State *L, lua_Debug *ar);
using namespace zsummer::luaperf;
static int newindex(lua_State * L)
{
    lua_pushvalue(L, 1);
    lua_pushvalue(L, 2);
    lua_pushvalue(L, 3);
    lua_rawset(L, 4);
    lua_pop(L, 1);
    const char * key = luaL_checkstring(L, 2);
    const char * v = luaL_typename(L, 3);
    std::stringstream ss;
    ss << "catch one operation that it's set a global value. key=" << key << ", type(v)=" << v << ", is it correct ?";
    if (lua_getglobal(L, "summer") == LUA_TTABLE && lua_getfield(L, -1, "logw") == LUA_TFUNCTION)
    {
        lua_pushstring(L, ss.str().c_str());
        lua_pcall(L, 1, 0, 0);
    }
    else if (lua_getglobal(L, "print") == LUA_TFUNCTION)
    {
        lua_pushstring(L, ss.str().c_str());
        lua_pcall(L, 1, 0, 0);
    }
    lua_pop(L, lua_gettop(L) - 3);
    
    return 0;
}


zsummer::luaperf::Performence __perf;
void luaopen_performence(lua_State * L)
{
    //lua_Hook oldhook = lua_gethook(L);
    //int oldmask = lua_gethookmask(L);
    lua_sethook(L, &hook_run_fn, LUA_MASKCALL | LUA_MASKRET, 0);
    lua_getglobal(L, "_G");
    lua_newtable(L);
    lua_pushcclosure(L, newindex, 0);
    lua_setfield(L, -2, "__newindex");
    lua_setmetatable(L, -2);
}

void hook_run_fn(lua_State *L, lua_Debug *ar)
{
    auto & stack = __perf._stack.push();
    stack.len = 0;
    // 获取Lua调用信息
    lua_getinfo(L, "Snl", ar);
    std::string key;
    if (!ar->name)
    {
        __perf._stack.pop();
        return;
    }
    if (ar->name)
    {
        size_t len = strlen(ar->name);
        if (len == 0)
        {
            __perf._stack.pop();
            return;
        }
        if (len > 0 && len + stack.len < 300 + 1)
        {
            memcpy(stack.buf + stack.len, ar->name, len);
            stack.len += len + 1;
            stack.buf[stack.len - 1] = '_';
        }
    }
    /*
    if (ar->namewhat)
    {
        size_t len = strlen(ar->namewhat);
        if (len > 0 && len + stack.len < 300 + 1)
        {
            memcpy(stack.buf + stack.len, ar->namewhat, len);
            stack.len += len + 1;
            stack.buf[stack.len - 1] = '_';
        }
    }

    if (ar->what)
    {
        size_t len = strlen(ar->what);
        if (len > 0 && len + stack.len < 300 + 1)
        {
            memcpy(stack.buf + stack.len, ar->what, len);
            stack.len += len + 1;
            stack.buf[stack.len - 1] = '_';
        }
    }
    */
    if (ar->source)
    {
        size_t len = strlen(ar->source);
        if (len == 4 && memcmp(ar->source, "=[C]", 4) == 0)
        {
            __perf._stack.pop();
            return;
        }
        if (len > 0 && len + stack.len < 300+1)
        {
            memcpy(stack.buf + stack.len, ar->source, len);
            stack.len += len + 1;
            stack.buf[stack.len - 1] = '_';
        }
    }





    if (stack.len == 0)
    {
        __perf._stack.pop();
        return;
    }
    if (PERF_MEM)
    {
        stack.mem = lua_gc(L, LUA_GCCOUNT, 0) /1024.0 + lua_gc(L, LUA_GCCOUNTB, 0)/1024.0/1024.0;
    }
    stack.hash = MurmurHash64A(stack.buf, stack.len, 0);
    stack.sec = getSteadyNow();
    if (ar->event == LUA_HOOKCALL)
    {
        
    }
    else if (ar->event == LUA_HOOKRET)
    {
        //lua_gc(L, LUA_GCCOLLECT, 0);
        for (size_t i = __perf._stack._offset - 1; i > 0; i--) //back is current.  
        {
            auto & param = __perf._stack._stack[i - 1];
            if (stack.hash != param.hash)
            {
                continue;
            }
            if (stack.len != param.len)
            {
                continue;
            }
            if (memcmp(stack.buf, param.buf, param.len) != 0)
            {
                continue;
            }
            stack.mem -= param.mem;
            stack.sec -= param.sec;
            __perf.call(stack);
            __perf._stack._offset = i; //not i-1 that next code will pop(1)
            break;
        }
        __perf._stack.pop(); //pop stack pop(1)

        if (__perf.expire(5))
        {
            __perf.dump(100);
        }
    }
}

uint64_t MurmurHash64A(const void * key, int len, unsigned int seed)
{
    const uint64_t m = 0xc6a4a7935bd1e995;
    const int r = 47;

    uint64_t h = seed ^ (len * m);

    const uint64_t * data = (const uint64_t *)key;
    const uint64_t * end = data + (len / 8);

    while (data != end)
    {
        uint64_t k = *data++;

        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;
    }

    const unsigned char * data2 = (const unsigned char*)data;

    switch (len & 7)
    {
    case 7: h ^= uint64_t(data2[6]) << 48;
    case 6: h ^= uint64_t(data2[5]) << 40;
    case 5: h ^= uint64_t(data2[4]) << 32;
    case 4: h ^= uint64_t(data2[3]) << 24;
    case 3: h ^= uint64_t(data2[2]) << 16;
    case 2: h ^= uint64_t(data2[1]) << 8;
    case 1: h ^= uint64_t(data2[0]);
        h *= m;
    };

    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    return h;
}
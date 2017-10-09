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


#include "summer.h"
extern "C"
{
int luaopen_proto4z_util(lua_State *L);
}




#include <zsummerX/zsummerX.h>
using namespace zsummer::log4z;
using namespace zsummer::network;
#include <fstream>
void sigFun(int sig)
{
    SessionManager::getRef().stop();
    SessionManager::getRef().post(std::bind([](){SessionManager::getRef().kickClientSession(); SessionManager::getRef().kickConnect(); }));
}

static int pcall_error(lua_State *L)
{
    const char *msg = lua_tostring(L, 1);
    if (msg)
    {
        luaL_traceback(L, L, msg, 1);
    }
    else if (!lua_isnoneornil(L, 1)) 
    {  /* is there an error object? */
        if (!luaL_callmeta(L, 1, "__tostring"))  /* try its 'tostring' metamethod */
        {
            lua_pushliteral(L, "(no error message)");
        }
    }
    return 1;
}

int safedofile(lua_State * L, const char * file)
{
    int index = lua_gettop(L);
    lua_pushcfunction(L, pcall_error);
    int status = luaL_loadfile(L, file) || lua_pcall(L, 0, 0, index + 1);
    lua_remove(L, index + 1);
    return status;

}





int main(int argc, char* argv[])
{
#ifndef _WIN32
    //! ignore some signal
    signal(SIGHUP, SIG_IGN);
    signal(SIGALRM, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGXCPU, SIG_IGN);
    signal(SIGXFSZ, SIG_IGN);
    signal(SIGPROF, SIG_IGN);
    signal(SIGVTALRM, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
#endif
    signal(SIGINT, sigFun);
    if(argc < 3)
    {
        printf("please input param like 'tcp server', 'tcp client', 'web server', 'web client'\n");
        return 1;
    }

//    ILog4zManager::getRef().setLoggerDisplay(LOG4Z_MAIN_LOGGER_ID, false);
//    ILog4zManager::getRef().setLoggerLevel(LOG4Z_MAIN_LOGGER_ID, LOG_LEVEL_TRACE);
    ILog4zManager::getRef().start();
    int status;
    lua_State *L = luaL_newstate();  /* create state */
    if (L == NULL) 
    {
        return EXIT_FAILURE;
    }

    lua_gc(L, LUA_GCSTOP, 0);  /* stop collector during initialization */
    luaL_openlibs(L);  /* open libraries */
    lua_newtable(L);
    lua_setglobal(L, "Proto4z");
    luaopen_summer(L);
    luaopen_proto4z_util(L);
    lua_gc(L, LUA_GCRESTART, 0);

    //luaopen_performence(L);
    

    if (strcmp(argv[1], "tcp") == 0)
    {
        if(strcmp(argv[2], "server") == 0)
        {
            status = safedofile(L, "./tcpserver.lua");
        }
        else
        {
            status = safedofile(L, "./tcpclient.lua");
        }
    }
    else
    {
        if(strcmp(argv[2], "server") == 0)
        {
            status = safedofile(L, "./webserver.lua");
        }
        else
        {
            status = safedofile(L, "./webclient.lua");
        }
    }
    
    

    if (status) 
    {
        const char *msg = lua_tostring(L, -1);
        if (msg == NULL) msg = "(error object is not a string)";
        LOGE( msg);
        lua_pop(L, 1);
    }
    lua_close(L);
    return (status) ? EXIT_FAILURE : EXIT_SUCCESS;
}


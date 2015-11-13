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
 * Copyright (C) 2010-2015 YaweiZhang <yawei.zhang@foxmail.com>.
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
    SessionManager::getRef().stopAccept();
    SessionManager::getRef().stopClients();
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




class Timestamp
{
public:
    inline Timestamp()
    {
        _timpstamp = now();
    }
public:
    //获取当前时间和上次的流逝时间
    inline double lapse()
    {
        return std::chrono::duration_cast<std::chrono::duration<double>>(now() - _timpstamp).count()*1000.0;
    }
    inline std::chrono::steady_clock::time_point now()
    {
        return std::chrono::steady_clock::now();
    }
    inline void flush()
    {
        _timpstamp = now();
    }
private:
    std::chrono::steady_clock::time_point _timpstamp;
};

class LuaStack
{
public:
    using StackParam = std::tuple<std::string, Timestamp, double>;
    using Stack = std::vector<StackParam>;
public:
    void push(const std::string & func, double mem)
    {
        _stack.push_back(std::make_tuple(func, Timestamp(), mem));
    }
    std::tuple<bool, double, double> autoback(const std::string & func, double mem)
    {
        auto founder = std::find_if(_stack.begin(), _stack.end(), [&func](const StackParam & p){return std::get<0>(p) == func; });
        if (founder != _stack.end())
        {
            while (!_stack.empty())
            {
                if (std::get<0>(_stack.back()) == func)
                {
                    auto ret = std::make_tuple(true, std::get<1>(_stack.back()).lapse(), mem - std::get<2>(_stack.back()));
                    _stack.pop_back();
                    return std::move(ret);
                }
                _stack.pop_back();
            }
        }
        return std::make_tuple(false, 0.0, 0.0);
    }
private:
    Stack _stack;
};

class Performence
{
public:
    using OrderTuple = std::tuple<std::string, double, double, double, double>;
    inline void callOnce(const std::string & func, double usetime, double mem)
    {
        auto founder = _perf.find(func);
        if (founder == _perf.end())
        {
            _perf[func] = std::make_tuple(func, usetime, 1.0, usetime, mem);
        }
        else
        {
            std::get<1>(founder->second) += usetime;
            std::get<2>(founder->second) += 1.0;
            if (std::get<3>(founder->second) < usetime)
            {
                std::get<3>(founder->second) = usetime;
            }
            std::get<4>(founder->second) += mem;
        }
    }
    template<class T>
    inline std::string toString(const T & t)
    {
        std::stringstream ss;
        ss << t;
        std::string temp = std::move(ss.str());
        while (temp.length() < 15)
        {
            temp.append(" ");
        }
        return std::move(temp);
    }

    inline std::string getProcessName()
    {
        std::string name = "MainLog";
        char buf[260] = { 0 };
#ifdef WIN32
        if (GetModuleFileNameA(NULL, buf, 259) > 0)
        {
            name = buf;
        }
        std::string::size_type pos = name.rfind("\\");
        if (pos != std::string::npos)
        {
            name = name.substr(pos + 1, std::string::npos);
        }
        pos = name.rfind(".");
        if (pos != std::string::npos)
        {
            name = name.substr(0, pos - 0);
        }

#elif defined(__APPLE__)

        proc_name(getpid(), buf, 260);
        name = buf;
        return name;;
#else
        sprintf(buf, "/proc/%d/cmdline", (int)getpid());
        std::fstream f(buf, std::ios::in);
        if (f.is_open())
        {
            std::getline(f, name);
            f.close();
        }
        std::string::size_type pos = name.rfind("/");
        if (pos != std::string::npos)
        {
            name = name.substr(pos + 1, std::string::npos);
        }
        name.pop_back();
        
#endif


        return name;
    }

    inline void serialize(const std::list<OrderTuple> & l, const std::string & prefix, const std::string & head, int maxCount)
    {
        std::string filename;
        if (true)
        {
            char buf[100];
            time_t cur = time(NULL);
            struct tm *  t = localtime(&cur);
            sprintf(buf, "%04d%02d%02d_%02d%02d%02d_", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
            filename += buf;
        }
        filename += getProcessName();
        filename += "_";
        filename += prefix;
        filename += ".log";

        std::fstream f(filename, std::ios::out);
        if (!f.is_open())
        {
            return;
        }
        f.write(head.c_str(), head.length());
        std::string temp;
        for (const auto & o : l)
        {
            if (maxCount <= 0)
            {
                break;
            }
            maxCount--;

            temp = std::get<0>(o);
            while (temp.length() < 70)
            {
                temp.append(" ");
            }
            temp += "\t\t" + toString(std::get<1>(o)) + "\t\t" + toString(std::get<2>(o)) + "\t\t" + toString(std::get<3>(o)) + "\t\t" + toString(std::get<4>(o)) + "\n";
            f.write(temp.c_str(), temp.length());
        }
        f.close();
    }
    inline void dumpTopPerformence(int maxCount)
    {
        std::string head = "total perf function size is[" + toString(_perf.size()) + "] \n";
        std::list<OrderTuple> orderList;
        for (auto & kv : _perf)
        {
            orderList.push_back(kv.second);
        }
        orderList.sort([](const OrderTuple & first, const OrderTuple & second){return std::get<1>(first) > std::get<1>(second); });
        serialize(orderList, "alluse", head, maxCount);
        orderList.sort([](const OrderTuple & first, const OrderTuple & second){return std::get<2>(first) > std::get<2>(second); });
        serialize(orderList, "allcount", head, maxCount);
        orderList.sort([](const OrderTuple & first, const OrderTuple & second){return std::get<1>(first) / std::get<2>(first) > std::get<1>(second) / std::get<2>(second); });
        serialize(orderList, "aveuse", head, maxCount);
        orderList.sort([](const OrderTuple & first, const OrderTuple & second){return std::get<3>(first) > std::get<3>(second); });
        serialize(orderList, "maxuse", head, maxCount);
        orderList.sort([](const OrderTuple & first, const OrderTuple & second){return std::get<4>(first) > std::get<4>(second); });
        serialize(orderList, "maxmem", head, maxCount);
    }
    inline bool needPrint(double interval)
    {
        if (_lastPerf.lapse() > interval)
        {
            _lastPerf.flush();
            return true;
        }
        return false;
    }
public:
    LuaStack _stack;
private:
    std::map<std::string, OrderTuple > _perf;
    std::map<std::string, OrderTuple > _once;
    Timestamp _lastPerf;
};

Performence __perf;

inline void hook_run_fn(lua_State *L, lua_Debug *ar) 
{
    // 获取Lua调用信息
    lua_getinfo(L, "Snl", ar);
    std::string key;
    if (ar->source)
    {
        key += ar->source;
    }
    key += "_";
    if (ar->what)
    {
        key += ar->what;
    }
    key += "_";
    if (ar->namewhat)
    {
        key += ar->namewhat;
    }
    key += "_";
    if (ar->name)
    {
        key += ar->name;
    }
    if (ar->event == 0)
    {
        __perf._stack.push(key, lua_gc(L, LUA_GCCOUNT, 0) * 1024 + lua_gc(L, LUA_GCCOUNTB, 0));
    }
    else
    {
        //lua_gc(L, LUA_GCCOLLECT, 0);
        auto t = __perf._stack.autoback(key, lua_gc(L, LUA_GCCOUNT, 0) * 1024 + lua_gc(L, LUA_GCCOUNTB, 0));
        if (std::get<0>(t))
        {
            __perf.callOnce(key, std::get<1>(t), std::get<2>(t));
        }
        if (__perf.needPrint(30000.0))
        {
            __perf.dumpTopPerformence(100);
        }
    }
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
    luaopen_proto4z_util(L);
    lua_gc(L, LUA_GCRESTART, 0);

    {
        //lua_Hook oldhook = lua_gethook(L);
        //int oldmask = lua_gethookmask(L);
        //lua_sethook(L, &hook_run_fn, LUA_MASKCALL | LUA_MASKRET, 0);
    }
    

    if (argc > 1)
    {
        status = safedofile(L, "./server.lua");
    }
    else
    {
        status = safedofile(L, "./client.lua");
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


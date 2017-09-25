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


#ifndef _SUMMER_PERFORMANCE_H_
#define _SUMMER_PERFORMANCE_H_

extern "C"
{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}
#include <string.h>
#include <algorithm>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <thread>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <deque>
#include <array>
#include <unordered_map>
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <io.h>
#include <shlwapi.h>
#include <process.h>
#pragma comment(lib, "shlwapi")
#pragma warning(disable:4996)

#else
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include<pthread.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <semaphore.h>
#endif


#ifdef __APPLE__
#include <dispatch/dispatch.h>
#include <libproc.h>
#endif


uint64_t MurmurHash64A(const void * key, int len, unsigned int seed);
void luaopen_performence(lua_State * L);

namespace zsummer
{
    namespace luaperf
    {
        inline double getSteadyNow()
        {
            return std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now().time_since_epoch()).count();
        }

        class Timestamp
        {
        public:
            inline Timestamp() { _last = getSteadyNow(); }
        public:
            //get lapse time. millisecond.
            inline double lapse() { return getSteadyNow() - _last; }
            inline void flush() { _last = getSteadyNow(); }
        private:
            double _last;
        };

        const size_t LUA_STACK_DEPTH = 10000;
        const bool PERF_MEM = false;
        class LuaStack
        {
        public:
            
            struct Param
            {
                char buf[300];
                size_t len = 0;
                uint64_t hash = 0;
                double mem = 0;
                double sec = 0;
            };
            std::array<Param, LUA_STACK_DEPTH> _stack;
            size_t _offset = 0;

        public:
            inline bool empty() { return _offset == 0; }
            inline Param & back() 
            {
                if (_offset == 0)
                {
                    throw std::runtime_error("LuaStack stack no any data");
                }
                return _stack[_offset - 1]; 
            }
            inline Param & push() 
            {
                if (_offset >= LUA_STACK_DEPTH)
                {
                    throw std::runtime_error("LuaStack stack overflow");
                }
                return _stack[_offset++];
            }
            inline void pop()
            {
                if (_offset <= 0)
                {
                    throw std::runtime_error("LuaStack stack empty can't pop");
                }
                _offset--;
            }
        };



        class Performence
        {
        public:
            struct PerfInfo
            {
                char buf[300];
                size_t len = 0;
                uint64_t hash = 0;
                double mem = 0;
                double sec = 0;
                double count = 0;
                double maxsec = 0;
            };
            using Param = std::tuple<std::string, double, double, double, double>;
            inline void call(const LuaStack::Param & param)
            {
                auto founder = _perf.find(param.hash);
                if (founder == _perf.end())
                {
                    PerfInfo pi;
                    memcpy(pi.buf, param.buf, param.len);
                    pi.len = param.len;
                    pi.hash = param.hash;
                    pi.maxsec = param.sec;
                    pi.count = 1;
                    pi.mem = param.mem;
                    _perf[param.hash] = pi;
                }
                else
                {
                    PerfInfo & pi = founder->second;
                    if (pi.len != param.len || memcmp(pi.buf, param.buf, pi.len) != 0)
                    {
                        return;
                    }
                    pi.count++;
                    pi.maxsec = pi.maxsec > param.sec ? pi.maxsec : param.sec;
                    pi.mem += param.mem;
                    pi.sec += param.sec;
                }
            }
            template<class T>
            inline std::string toString(const T & t)
            {
                std::stringstream ss;
                ss << t;
                std::string temp = ss.str();
                if (temp.length() < 15)
                {
                    temp.append(15 - temp.length(), ' ');
                }
                return temp;
            }
            inline std::string getProcessID()
            {
                std::string pid = "0";
                char buf[260] = { 0 };
#ifdef WIN32
                DWORD winPID = GetCurrentProcessId();
                sprintf(buf, "%06u", winPID);
                pid = buf;
#else
                sprintf(buf, "%06d", getpid());
                pid = buf;
#endif
                return pid;
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

            inline void serialize(const std::vector<std::unordered_map<uint64_t, PerfInfo>::iterator> & l, const std::string & prefix, const std::string & head, int maxCount)
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
                if (_processID.empty() && _processName.empty())
                {
                    _processID = getProcessID();
                    _processID = _processID.c_str();
                    _processName = getProcessName();
                    _processName = _processName.c_str();
                }
                filename += _processName;
                filename += "_";
                filename += _processID;
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
                    temp.assign(o->second.buf, o->second.len);
                    if (temp.length() < 70)
                    {
                        temp.append(70 - temp.length(), ' ');
                    }
                    temp += "\t\t" + toString(o->second.count) + "\t\t" + toString(o->second.maxsec) + "\t\t" + toString(o->second.sec) + "\t\t" + toString(o->second.mem) + "\n";
                    f.write(temp.c_str(), temp.length());
                }
                f.close();
                _historyFile.push_back(filename);
                while (_historyFile.size() > 10)
                {
                    ::remove(_historyFile.front().c_str());
                    _historyFile.pop_front();
                }
            }
            inline void dump(int maxCount)
            {
                std::string head = "function name,  function call count,  function max second, function total second, function memory \n";
                using PerfIter = std::unordered_map<uint64_t, PerfInfo>::iterator;
                std::vector<PerfIter> orderList;
                for (auto iter = _perf.begin(); iter != _perf.end(); iter++ )
                {
                    orderList.push_back(iter);
                }

                std::sort(orderList.begin(), orderList.end(), [](const PerfIter & first, const PerfIter & second) {return first->second.sec > second->second.sec; });
                serialize(orderList, "top_total_sec", head, maxCount);

  
                std::sort(orderList.begin(), orderList.end(), [](const PerfIter & first, const PerfIter & second) {return first->second.maxsec > second->second.maxsec; });
                serialize(orderList, "top_max_sec", head, maxCount);

                std::sort(orderList.begin(), orderList.end(), [](const PerfIter & first, const PerfIter & second)
                {return first->second.sec / first->second.count > second->second.sec / second->second.count; });
                serialize(orderList, "top_avg_sec", head, maxCount);


                std::sort(orderList.begin(), orderList.end(), [](const PerfIter & first, const PerfIter & second) {return first->second.count > second->second.count; });
                serialize(orderList, "top_total_ccount", head, maxCount);

                if (PERF_MEM)
                {
                    std::sort(orderList.begin(), orderList.end(), [](const PerfIter & first, const PerfIter & second) {return first->second.mem > second->second.mem; });
                    serialize(orderList, "top_total_mem", head, maxCount);
                }


            }
            inline bool expire(double interval)
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
            std::unordered_map<uint64_t, PerfInfo> _perf;
            std::map<std::string, Param > _once;
            std::string _processID;
            std::string _processName;
            std::deque<std::string> _historyFile;
            Timestamp _lastPerf;
        };
    };
    

};











#endif



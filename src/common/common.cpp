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
#include <atomic>
#include <zsummerX/common/common.h>
using namespace zsummer::network;


#ifdef WIN32
#pragma comment(lib, "ws2_32")
#pragma comment(lib, "shlwapi")
#pragma comment(lib, "psapi")
#pragma comment(lib, "Mswsock")
#endif

ZSummerEnvironment::ZSummerEnvironment()
{
#ifdef WIN32
    WORD version = MAKEWORD(2,2);
    WSADATA d;
    if (WSAStartup(version, &d) != 0)
    {
        assert(0);
    }
#endif
    _netLoggerID = zsummer::log4z::ILog4zManager::getRef().createLogger("NetWork");
    zsummer::log4z::ILog4zManager::getRef().setLoggerLevel(_netLoggerID, LOG_LEVEL_DEBUG);
}
ZSummerEnvironment::~ZSummerEnvironment()
{
#ifdef WIN32
    WSACleanup();
#endif
}

//server environment.
namespace zsummer
{
    namespace network
    {
        ZSummerEnvironment g_appEnvironment;
    }
}


std::string zsummer::network::getHostByName(const std::string & name)
{
    if (std::find_if(name.begin(), name.end(), [](char ch) {return !isdigit(ch) && ch != '.'; }) == name.end())
    {
        return name; //ipv4 
    }
    if (std::find_if(name.begin(), name.end(), [](char ch) {return !isxdigit(ch) && ch != ':'; }) == name.end())
    {
        return name; //ipv6 
    }
    struct addrinfo *res = nullptr;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if (getaddrinfo(name.c_str(), "3306", &hints, &res) == 0)
    {
        char buf[100] = { 0 };
        if (res->ai_family == AF_INET)
        {
            inet_ntop(res->ai_family, &(((sockaddr_in*)res->ai_addr)->sin_addr), buf, 100);
        }
        else if (res->ai_family == AF_INET6)
        {
            inet_ntop(res->ai_family, &(((sockaddr_in6*)res->ai_addr)->sin6_addr), buf, 100);
        }
        return buf;
    }

    return "";
}

std::string zsummer::network::getPureHostName(const std::string & host)
{
    static std::string ipv6pre = "::ffff:";
    if (host.length() > ipv6pre.length() && host.compare(0, ipv6pre.length(), ipv6pre) == 0)
    {
        return host.substr(ipv6pre.length());
    }
    return host;
}

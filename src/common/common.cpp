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
 * Copyright (C) 2013 YaweiZhang <yawei_zhang@foxmail.com>.
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


#ifdef WIN32
#pragma comment(lib, "ws2_32")
#pragma comment(lib, "shlwapi")
#pragma comment(lib, "psapi")
#pragma comment(lib, "Mswsock")
#endif

zsummer::network::ZSummerEnvironment::ZSummerEnvironment()
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
	zsummer::log4z::ILog4zManager::getRef().setLoggerLevel(_netLoggerID, LOG_LEVEL_WARN);
}
zsummer::network::ZSummerEnvironment::~ZSummerEnvironment()
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


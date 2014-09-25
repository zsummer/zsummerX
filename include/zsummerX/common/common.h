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
#pragma once

#ifndef _ZSUMMERX_COMMON_H_
#define _ZSUMMERX_COMMON_H_

#ifdef WIN32

#include <WinSock2.h>
#include <Windows.h>
#include <MSWSock.h>
#else
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <string.h>

#endif

#include <assert.h>
#include <string>
#include <iostream>
#include <map>
#include <set>
#include <list>
#include <queue>
#include <utility>

#include <memory>
#include <functional>
#include <chrono>
#include <thread>
#include <mutex>
#include <algorithm>
#include <log4z/log4z.h>
#include <rc4/rc4_encryption.h>

namespace zsummer
{
	namespace network
	{
		//! 错误码
		enum ErrorCode
		{
			EC_SUCCESS = 0,
			EC_ERROR,
			EC_REMOTE_CLOSED,
			EC_REMOTE_HANGUP,
		};

		
		//! 处理器
		typedef std::function<void()> _OnPostHandler;
		//unsigned long long : timer ID
		typedef std::function<void()> _OnTimerHandler;

		class CTcpSocket;
		typedef std::shared_ptr<CTcpSocket> CTcpSocketPtr;
		typedef std::function<void(ErrorCode, CTcpSocketPtr)> _OnAcceptHandler;

		typedef std::function<void(ErrorCode)> _OnConnectHandler;
		//! int : translate bytes
		typedef std::function<void(ErrorCode, int)> _OnSendHandler;
		typedef _OnSendHandler _OnRecvHandler;

		//
		class CUdpSocket;
		typedef std::shared_ptr<CUdpSocket>CUdpSocketPtr;
		//! const char *: remote ip
		//! unsigned short: remote port
		//! int : translate bytes
		typedef std::function<void (ErrorCode, const char*, unsigned short, int)> _OnRecvFromHandler;
		typedef std::function<void(ErrorCode)> _OnSendToHandler;

		enum LINK_STATUS
		{
			LS_UNINITIALIZE, //socket构造后的状态
			LS_WAITLINK, // socket init之后的状态
			LS_ESTABLISHED, //正常通信状态
			LS_CLOSED,
		};

		class CInitEnvironment
		{
		public:
			CInitEnvironment();
			~CInitEnvironment();
		};

#ifndef WIN32
		inline bool SetNonBlock(int fd) 
		{
			return fcntl((fd), F_SETFL, fcntl(fd, F_GETFL)|O_NONBLOCK) == 0;
		}
		inline bool SetNoDelay(int fd)
		{
			int bTrue = true?1:0;
			return setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*)&bTrue, sizeof(bTrue)) == 0;
		}
#else
		inline bool SetNonBlock(SOCKET s) 
		{		
			unsigned long val = 1;
			int nb = ioctlsocket(s, FIONBIO, &val);
			if (nb != NO_ERROR)
			{
				return false;
			}
			return true;
		}
		inline bool SetNoDelay(SOCKET s)
		{
			BOOL bTrue = TRUE;
			if (setsockopt(s,IPPROTO_TCP, TCP_NODELAY, (char*)&bTrue, sizeof(bTrue)) != 0)
			{
				return false;
			}
			return true;
		}
#endif
	}
}



extern LoggerId g_coreID;


#define LCD( log ) LOG_DEBUG( g_coreID, __FUNCTION__ <<": "<< log )
#define LCI( log ) LOG_INFO( g_coreID,__FUNCTION__ <<": "<<  log )
#define LCW( log ) LOG_WARN( g_coreID,__FUNCTION__ << ": "<< log )
#define LCE( log ) LOG_ERROR( g_coreID,__FUNCTION__ << ": "<< log )
#define LCA( log ) LOG_ALARM( g_coreID,__FUNCTION__ << ": "<< log )
#define LCF( log ) LOG_FATAL( g_coreID,__FUNCTION__ << ": "<< log )




















#endif












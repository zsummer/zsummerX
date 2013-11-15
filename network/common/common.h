/*
* ZSUMMER_11X License
* -----------
* 
* ZSUMMER_11X is licensed under the terms of the MIT license reproduced below.
* This means that ZSUMMER_11X is free software and can be used for both academic
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

#ifndef _ZSUMMER_11X_COMMON_H_
#define _ZSUMMER_11X_COMMON_H_

#ifdef WIN32
#define _WIN32_WINNT 0x501
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
#include <sys/epoll.h>
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

#include "../../depends/log4z/log4z.h"

namespace zsummer
{
	namespace network
	{
		enum ErrorCode
		{
			EC_SUCCESS = 0,
			EC_ERROR,
			EC_REMOTE_CLOSED,
			EC_REMOTE_HANGUP,
		};
		enum POST_COM_KEY
		{
			PCK_USER_DATA,
		};
					

		typedef std::function<void()> _OnPostHandler;
		typedef std::function<void(unsigned long long)> _OnTimerHandler;

		class CTcpSocket;
		typedef std::shared_ptr<CTcpSocket> CTcpSocketPtr;
		typedef std::function<void(ErrorCode, CTcpSocketPtr)> _OnAcceptHandler;

		typedef std::function<void(ErrorCode)> _OnConnectHandler;
		typedef std::function<void(ErrorCode, int)> _OnSendHandler;
		typedef _OnSendHandler _OnRecvHandler;
		// const char * remoteIP, unsigned short remotePort, nTranslate
		typedef std::function<void (ErrorCode, const char*, unsigned short, int)> _OnRecvFromHandler;
		typedef std::function<void(ErrorCode)> _OnSendToHandler;

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


namespace zsummer
{
	namespace network
	{
#ifdef WIN32
		struct tagReqHandle 
		{
			OVERLAPPED	 _overlapped;
			unsigned char _type;
			enum
			{
				HANDLE_ACCEPT, 
				HANDLE_RECV, 
				HANDLE_SEND,
				HANDLE_CONNECT, 
				HANDLE_RECVFROM,
				HANDLE_SENDTO,
			};
		};
#define HandlerFromOverlaped(ptr)  ((tagReqHandle*)((char*)ptr - (char*)&((tagReqHandle*)NULL)->_overlapped))

		enum LINK_STATUS
		{
			LS_UNINITIALIZE,
			LS_ESTABLISHED,
			LS_CLOSED,
		};


#else
		/* 
			typedef union epoll_data {
               void    *ptr;
               int      fd;
               uint32_t u32;
               uint64_t u64;
           } epoll_data_t;

           struct epoll_event {
               uint32_t     events;    Epoll events 
               epoll_data_t data;      User data variable 
           };
		*/
		struct tagRegister
		{
			enum REGISTER_TYPE
			{
				REG_INVALIDE,
				REG_ACCEPT, // listen
				REG_ESTABLISHED_TCP, //socket write & read
				REG_ESTABLISHED_UDP, //socket write & read
				REG_CONNECT, // connect
				REG_THREAD, // user message router
			};
			epoll_event   _event; //event, auto set
			unsigned char _type; //register type
			int			  _fd;   //file descriptor
			void *		  _ptr;  //user pointer
			inline void reset()
			{
				_event.data.ptr = this;
				_event.events = 0;
				_type = REG_INVALIDE;
				_fd = -1;
				_ptr = NULL;
			}
		};


#endif

		class CInitEnvironment
		{
		public:
			CInitEnvironment();
			~CInitEnvironment();
		};
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












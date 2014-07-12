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
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x501
#endif

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

#include <log4z/log4z.h>

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
#define InvalideFD -1
#ifdef WIN32
		struct tagReqHandle 
		{
			OVERLAPPED	 _overlapped;
			unsigned char _type;
			enum HANDLE_TYPE
			{
				HANDLE_ACCEPT, 
				HANDLE_RECV, 
				HANDLE_SEND,
				HANDLE_CONNECT, 
				HANDLE_RECVFROM,
				HANDLE_SENDTO,
			};
		};
		template <class T>
		T & operator <<(T &t, const tagReqHandle & h)
		{
			t << h._type;
			return t;
		}
		//! 完成键
		enum POST_COM_KEY
		{
			PCK_USER_DATA,
		};
#define HandlerFromOverlaped(ptr)  ((tagReqHandle*)((char*)ptr - (char*)&((tagReqHandle*)NULL)->_overlapped))

#else
		
#ifdef WIN32

		enum EPOLL_CTL_ENUM
		{
			EPOLL_CTL_ADD,
			EPOLL_CTL_MOD,
			EPOLL_CTL_DEL,
		};

		typedef union epoll_data {
			void    *ptr;
			int      fd;
			uint32_t u32;
			uint64_t u64;
		} epoll_data_t;

		struct epoll_event {
			uint32_t     events;//    Epoll events 
			epoll_data_t data;//      User data variable 
		};
#endif

		
		struct tagRegister
		{
			enum REG_TYPE
			{
				REG_INVALID,
				REG_ZSUMMER,
				REG_TCP_SOCKET,
				REG_TCP_ACCEPT,
				REG_UDP_SOCKET
			};

			epoll_event   _event; //event, auto set
			unsigned char _type = REG_INVALID; //register type
			unsigned char _linkstat = LS_UNINITIALIZE;
			int			  _fd = InvalideFD;   //file descriptor
			void *		  _ptr = nullptr;  //user pointer
		};

		template <class T>
		T& operator <<(T &t, const tagRegister & reg)
		{
			   t<< "RegisterEvent Info: epoll_event.events[" << reg._event.events
				   << "] _type[" << (int)reg._type << "] _linkstat[" << (int)reg._linkstat
				<< "] _fd[" << reg._fd << "] _ptr[" << (void*)reg._ptr
				<< "] Notes: REG_INVALID[" << tagRegister::REG_INVALID << "] REG_ZSUMMER[" << tagRegister::REG_ZSUMMER
				<< "] REG_TCP_SOCKET[" << tagRegister::REG_TCP_SOCKET << "] REG_TCP_ACCEPT[" << tagRegister::REG_TCP_ACCEPT 
				<< "] REG_UDP_SOCKET[" << tagRegister::REG_UDP_SOCKET
				<< "] EPOLL_CTL_ADD[" << EPOLL_CTL_ADD << "] EPOLL_CTL_MOD[" << EPOLL_CTL_MOD << "] EPOLL_CTL_DEL[" << EPOLL_CTL_DEL
				<< "];   EPOLLIN[" << EPOLLIN << "] EPOLLOUT[" << EPOLLOUT << "] EPOLLERR[" << EPOLLERR << "] EPOLLHUP[" << EPOLLHUP;
			   return t;
		}

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












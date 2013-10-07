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

#ifndef _ZSUMMER_11X_PUBLIC_H_
#define _ZSUMMER_11X_PUBLIC_H_

#define _WIN32_WINNT 0x501
#include <WinSock2.h>
#include <Windows.h>
#include <MSWSock.h>

#include "../../depends/log4z/log4z.h"
#include <assert.h>
#include <string>
#include <iostream>


#include <map>
#include <list>
#include <queue>

#include <memory>
#include <functional>
#include <chrono>
#include <thread>
#include <mutex>

namespace zsummer
{
	namespace network
	{
		enum ErrorCode
		{
			EC_SUCCESS = 0,
			EC_ERROR,
		};
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

		enum POST_COM_KEY
		{
			PCK_USER_DATA,
		};

		class CInitWSASocketEnv
		{
		public:
			CInitWSASocketEnv();
			~CInitWSASocketEnv();
		};
	}
}
extern LoggerId g_coreID;



#define LCD( log ) LOG_DEBUG( g_coreID, log )
#define LCI( log ) LOG_INFO( g_coreID, log )
#define LCW( log ) LOG_WARN( g_coreID, log )
#define LCE( log ) LOG_ERROR( g_coreID, log )
#define LCA( log ) LOG_ALARM( g_coreID, log )
#define LCF( log ) LOG_FATAL( g_coreID, log )




















#endif












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




#ifndef _ZSUMMERX_UDPSOCKET_H_
#define _ZSUMMERX_UDPSOCKET_H_
#ifdef WIN32
#include "iocp/udpsocket_impl.h"
#else
#include "epoll/udpsocket_impl.h"
#endif
namespace zsummer
{
	namespace network
	{
		class CUdpSocket
		{
		public:
			CUdpSocket(){}
			~CUdpSocket(){}
			bool Initialize(CZSummerPtr summer, const char *localIP, unsigned short localPort = 0)
			{
				return m_impl.Initialize(summer, localIP, localPort);
			}

			//handle: std::function<void (ErrorCode, const char*, unsigned short, int)>
			template<typename H>
			bool DoRecvFrom(char * buf, unsigned int len, const H &h)
			{
				return m_impl.DoRecv(buf, len, h);
			}

			//handle: std::function<void(ErrorCode)>
			bool DoSendTo(char * buf, unsigned int len, const char *dstip, unsigned short dstport)
			{
				return m_impl.DoSend(buf, len, dstip, dstport);
			}
		private:
			CUdpSocketImpl m_impl;
		};
	};
	
};























#endif












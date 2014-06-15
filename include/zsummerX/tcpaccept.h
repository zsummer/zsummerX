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



#ifndef _ZSUMMERX_TCPACCEPT_H_
#define _ZSUMMERX_TCPACCEPT_H_
#ifdef WIN32
#include "iocp/tcpaccept_impl.h"
#else
#include "epoll/tcpaccept_impl.h"
#endif
namespace zsummer
{
	namespace network
	{
		class CTcpAccept
		{
		public:
			CTcpAccept(CZSummerPtr summer){ m_impl.Initialize(summer); }
			~CTcpAccept(){}
			bool OpenAccept(const char * ip, unsigned short port)
			{
				return m_impl.OpenAccept(ip, port);
			}

			//handle: std::function<void(ErrorCode, CTcpSocketPtr)>
			template<typename H>
			bool DoAccept(CTcpSocketPtr &s, const H &h)
			{
				return m_impl.DoAccept(s, h);
			}
		private:
			CTcpAcceptImpl m_impl;
		};
		typedef std::shared_ptr<CTcpAccept> CTcpAcceptPtr;
	};
	
};























#endif












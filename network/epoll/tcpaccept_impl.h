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

#ifndef _ZSUMMER_TCPACCEPT_IMPL_H_
#define _ZSUMMER_TCPACCEPT_IMPL_H_

#include "public.h"
#include "../zsummer.h"
#include "../tcpsocket.h"
/*
*	CTcpAccept: EPOLL LT EPOLLONESHOT模式 每次MOD accept一次 以尽量保持和IOCP的PROACTOR设计的一致性
*/

namespace zsummer
{
	namespace network
	{
		class CTcpAcceptImpl
		{
		public:
			typedef std::shared_ptr<CTcpSocket> CTcpSocketPtr;
			typedef std::function<void(zsummer::network::ErrorCode, CTcpSocketPtr)> _OnAcceptHandler;

			CTcpAcceptImpl(CZSummer &summer);
			~CTcpAcceptImpl();
			bool Initialize();
			bool OpenAccept(const char * ip, unsigned short port);
			bool DoAccept(const _OnAcceptHandler &handle);
			bool OnEPOLLMessage(bool bSuccess);
			bool Close();

			void OnPostClose();

			CZSummer 		*m_summer;
			std::string		m_ip;
			short			m_port;

			sockaddr_in		m_addr;

			tagRegister m_handle;
			_OnAcceptHandler m_onAcceptHandler;
			bool	m_isAcceptLock;

		};
	}
}






















#endif












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
#ifndef _ZSUMMER_11X_UDPSOCKET_IMPL_H_
#define _ZSUMMER_11X_UDPSOCKET_IMPL_H_

#include "public.h"
#include "../zsummer.h"

namespace zsummer
{
	namespace network
	{
		class CUdpSocketImpl
		{
		public:
			typedef std::function<void (zsummer::network::ErrorCode, const char*, unsigned short, char*, int)> _OnRecvHandler;
			typedef std::function<void(zsummer::network::ErrorCode)> _OnSendHandler;
			CUdpSocketImpl();
			~CUdpSocketImpl();
			bool Initialize(CZSummer & summer, const char *ip, unsigned short port);
			bool DoRecv(char * buf, unsigned int len, const _OnRecvHandler& handler);
			bool DoSend(char * buf, unsigned int len, const char *dstip, unsigned short dstport, const _OnSendHandler& handler);
			bool OnIOCPMessage(BOOL bSuccess, DWORD dwTranceCount, unsigned char cType);
		public:
			//private
			CZSummer * m_summer;
			
			SOCKET		m_socket;
			SOCKADDR_IN	m_addr;

			//recv
			tagReqHandle m_recvHandle;
			WSABUF		 m_recvWSABuf;
			sockaddr_in  m_recvFrom;
			int			 m_recvFromLen;
			_OnRecvHandler m_onRecvHander;

			//send
			tagReqHandle m_sendHandle;
			WSABUF		 m_sendWSABuf;
			_OnSendHandler m_onSendHandler;

			LINK_STATUS m_nLinkStatus;
		};
	}
}
























#endif












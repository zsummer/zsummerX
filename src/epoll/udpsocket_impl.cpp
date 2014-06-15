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

#include <zsummerX/epoll/epoll_impl.h>
#include <zsummerX/epoll/udpsocket_impl.h>

using namespace zsummer::network;



CUdpSocketImpl::CUdpSocketImpl()
{
	m_register._event.data.ptr = &m_register;
	m_register._event.events = 0;
	m_register._fd = -1;
	m_register._linkstat = LS_UNINITIALIZE;
	m_register._ptr = this;
	m_register._type = tagRegister::REG_UDP_SOCKET;
	m_iRecvLen  = 0;
	m_pRecvBuf = nullptr;
}


CUdpSocketImpl::~CUdpSocketImpl()
{
	if (m_register._fd != -1)
	{
		if (m_onRecvFromHandler )
		{
			LCE("CUdpSocketImpl::~CUdpSocketImpl[this0x" << this << "] Destruct CUdpSocketImpl Error. socket handle not invalid and some request was not completed. fd="
				<< m_register._fd );
		}
		close(m_register._fd);
		m_register._fd = -1;
	}
}
bool  CUdpSocketImpl::Initialize(CZSummerPtr summer, const char *localIP, unsigned short localPort)
{
	if (!m_summer)
	{
		LCE("CUdpSocketImpl::Initialize[this0x" << this << "] socket is aready used, m_ios not is nullptr. this=" << this);
		return false;
	}
	if (m_register._fd != -1)
	{
		LCE("CUdpSocketImpl::Initialize[this0x" << this << "] socket is aready used, _fd not is -1. this=" << this << ", fd=" << m_register._fd);
		return false;
	}
	m_summer = summer;
	m_register._ptr = this;
	m_register._fd = socket(AF_INET, SOCK_DGRAM, 0);
	m_register._linkstat = LS_WAITLINK;
	if (m_register._fd == -1)
	{
		LCE("CUdpSocketImpl::Initialize[this0x" << this << "] create socket fail. this=" << this << ", errno=" << strerror(errno));
		return false;
	}
	SetNonBlock(m_register._fd);
	sockaddr_in	localAddr;
	localAddr.sin_family = AF_INET;
	localAddr.sin_addr.s_addr = inet_addr(localIP);
	localAddr.sin_port = htons(localPort);
	if (bind(m_register._fd, (sockaddr *) &localAddr, sizeof(localAddr)) != 0)
	{
		LCE("CUdpSocketImpl::Initialize[this0x" << this << "]: socket bind err, errno=" << strerror(errno));
		close(m_register._fd);
		m_register._fd = -1;
		return false;
	}
	if (!m_summer->m_impl.RegisterEvent(EPOLL_CTL_ADD, m_register))
	{
		LCF("CUdpSocketImpl::Initialize[this0x" << this << "] EPOLL_CTL_ADD error. _register =" << m_register << ", errno=" << strerror(errno));
		return false;
	}
	m_register._linkstat = LS_ESTABLISHED;
	return true;
}

bool CUdpSocketImpl::DoSend(char * buf, unsigned int len, const char *dstip, unsigned short dstport)
{
	if (!m_summer)
	{
		LCE("CUdpSocketImpl::DoSend[this0x" << this << "] IIOServer not bind!");
		return false;
	}
	if (len == 0 || len >1500)
	{
		LCE("CUdpSocketImpl::DoSend[this0x" << this << "] argument err! len=" << len);
		return false;
	}
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(dstip);
	addr.sin_port = htons(dstport);
	sendto(m_register._fd, buf, len, 0, (sockaddr*)&addr, sizeof(addr));
	return true;
}


bool CUdpSocketImpl::DoRecv(char * buf, unsigned int len, const _OnRecvFromHandler& handler)
{
	if (!m_summer)
	{
		LCE("CUdpSocketImpl::DoRecv[this0x" << this << "] m_summer not bind!");
		return false;
	}
	if (len == 0 || len >1500)
	{
		LCE("CUdpSocketImpl::DoRecv[this0x" << this << "] argument err! len=" << len);
		return false;
	}
	if (len == 0 )
	{
		LCE("CUdpSocketImpl::DoRecv[this0x" << this << "] argument err !!!  len==0");
		return false;
	}
	
	if (m_pRecvBuf || m_iRecvLen != 0 || m_onRecvFromHandler)
	{
		LCE("CUdpSocketImpl::DoRecv[this0x" << this << "] (m_pRecvBuf != nullptr || m_iRecvLen != 0) == TRUE");
		return false;
	}

	
	m_pRecvBuf = buf;
	m_iRecvLen = len;
	m_register._event.events = m_register._event.events|EPOLLIN;
	if (!m_summer->m_impl.RegisterEvent(EPOLL_CTL_MOD, m_register))
	{
		LCF("CUdpSocketImpl::DoRecv[this0x" << this << "] EPOLLMod error. m_register=" << m_register << ", errno=" << strerror(errno));
		m_onRecvFromHandler = nullptr;
		m_pRecvBuf = nullptr;
		m_iRecvLen = 0;
		return false;
	}
	m_onRecvFromHandler = handler;
	return true;
}


bool CUdpSocketImpl::OnEPOLLMessage(int type, int flag)
{
	if (!m_onRecvFromHandler)
	{
		LCE("CUdpSocketImpl::OnEPOLLMessage[this0x" << this << "] unknown error");
		return false;
	}
	if (flag & EPOLLHUP || flag & EPOLLERR)
	{
		if (flag & EPOLLHUP)
		{
			LCE("CUdpSocketImpl::OnEPOLLMessage[this0x" << this << "] EPOLLHUP  error. m_register fd=" << m_register << ", events=" << flag);
		}
		if (flag & EPOLLERR)
		{
			LCE("CUdpSocketImpl::OnEPOLLMessage[this0x" << this << "]  EPOLLERR error. m_register fd=" << m_register << ", events=" << flag);
		}
		if (m_onRecvFromHandler)
		{
			_OnRecvFromHandler onRecv;
			onRecv.swap(m_onRecvFromHandler);
			m_pRecvBuf = nullptr;
			m_iRecvLen = 0;
			onRecv(EC_ERROR, "", 0, 0);
		}
		return false;
	}

	if (flag & EPOLLIN && m_onRecvFromHandler)
	{
		_OnRecvFromHandler onRecv;
		onRecv.swap(m_onRecvFromHandler);
		m_register._event.events = m_register._event.events&~EPOLLIN;

		if (!m_summer->m_impl.RegisterEvent(EPOLL_CTL_MOD, m_register))
		{
			LCF("CUdpSocketImpl::OnEPOLLMessage[this0x" << this << "] EPOLLMod error. m_register=" << m_register << ", errno=" << strerror(errno));
			return false;
		}

		sockaddr_in raddr;
		memset(&raddr, 0, sizeof(raddr));
		socklen_t len = sizeof(raddr);
		int ret = recvfrom(m_register._fd, m_pRecvBuf, m_iRecvLen, 0, (sockaddr*)&raddr, &len);

		m_pRecvBuf = nullptr;
		m_iRecvLen = 0;
		if (ret == 0 || (ret ==-1 && (errno !=EAGAIN && errno != EWOULDBLOCK)) )
		{
			LCE("CUdpSocketImpl::OnEPOLLMessage[this0x" << this << "] recv error.  m_register=" << m_register << ", ret=" << ret << ", errno=" << strerror(errno));
			onRecv(EC_ERROR, "", 0, 0);
			return false;
		}
		if (ret == -1)
		{
			LCE("CUdpSocketImpl::OnEPOLLMessage[this0x" << this << "] recv error.  m_register=" << m_register << ", ret=" << ret << ", errno=" << strerror(errno));
			onRecv(EC_ERROR, "", 0, 0);
			return false;
		}

		onRecv(EC_SUCCESS, inet_ntoa(raddr.sin_addr), ntohs(raddr.sin_port), ret);
	}
	
	

	return true;
}


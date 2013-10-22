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

#include "udpsocket_impl.h"
#include "../zsummer.h"
using namespace zsummer::network;



CUdpSocketImpl::CUdpSocketImpl()
{
	m_summer = NULL;
	m_register.reset();
	m_iRecvLen  = 0;
	m_pRecvBuf = NULL;
	m_iSendLen = 0;
	m_pSendBuf = NULL;
	m_dstip.clear();
	m_dstport = 0;
	m_isRecvFromLock = false;
	m_isSendToLock = false;
}


CUdpSocketImpl::~CUdpSocketImpl()
{
	if (m_register._fd != -1)
	{
		close(m_register._fd);
		m_register._fd = -1;
	}
}
bool  CUdpSocketImpl::Initialize(CZSummer & summer, const char *localIP, unsigned short localPort)
{
	if (m_summer != NULL)
	{
		LCE("CUdpSocketImpl socket is aready used, m_ios not is NULL. this=" << this);
		return false;
	}
	if (m_register._fd != -1)
	{
		LCE("CUdpSocketImpl socket is aready used, _fd not is -1. this=" << this << ", fd=" << m_register._fd);
		return false;
	}
	m_summer = &summer;
	m_register.reset();
	m_register._ptr = this;
	m_register._fd = socket(AF_INET, SOCK_DGRAM, 0);
	m_register._type = tagRegister::REG_ESTABLISHED_UDP;
	if (m_register._fd == -1)
	{
		LCE("CUdpSocketImpl create socket fail. this=" << this << ", errno=" << strerror(errno));
		return false;
	}
	SetNonBlock(m_register._fd);
	sockaddr_in	localAddr;
	localAddr.sin_family = AF_INET;
	localAddr.sin_addr.s_addr = inet_addr(localIP);
	localAddr.sin_port = htons(localPort);
	if (bind(m_register._fd, (sockaddr *) &localAddr, sizeof(localAddr)) != 0)
	{
		LCE("CUdpSocketImpl: socket bind err, errno=" << strerror(errno));
		close(m_register._fd);
		m_register._fd = -1;
		return false;
	}
	if (epoll_ctl(m_summer->m_impl.m_epoll, EPOLL_CTL_ADD, m_register._fd, &m_register._event) != 0)
	{
		LCE("CUdpSocketImpl::Initialize()" << this << " EPOLL_CTL_ADD error. epfd="<<m_summer->m_impl.m_epoll << ", m_register fd=" << m_register._fd << ", errno=" << strerror(errno));
		return false;
	}
	return true;
}

bool CUdpSocketImpl::DoSend(char * buf, unsigned int len, const char *dstip, unsigned short dstport, const _OnSendToHandler& handler)
{
	if (m_summer == NULL)
	{
		LCE("CUdpSocketImpl::DoSend()" << this << " IIOServer not bind!");
		return false;
	}
	if (len == 0 || len >1500)
	{
		LCE("CUdpSocketImpl::DoSend()" << this << " argument err! len=" << len);
		return false;
	}
	if (m_pSendBuf != NULL || m_iSendLen != 0 || m_isSendToLock)
	{
		LCE("CUdpSocketImpl::DoRecv()" << this << "    (m_pRecvBuf != NULL || m_iRecvLen != 0) == TRUE");
		return false;
	}
	m_onSendToHandler = handler;
	m_pSendBuf = buf;
	m_iSendLen = len;
	m_dstip = dstip;
	m_dstport = dstport;
	m_register._event.events = m_register._event.events|EPOLLOUT;
	if (epoll_ctl(m_summer->m_impl.m_epoll, EPOLL_CTL_MOD, m_register._fd, &m_register._event) != 0)
	{
		LCE("CUdpSocketImpl::DoSend()" << this << " EPOLLMod error. epfd="<<m_summer->m_impl.m_epoll << ", m_register fd=" << m_register._fd << ", errno=" << strerror(errno));
		return false;
	}
	m_isSendToLock = true;
  return true;
}


bool CUdpSocketImpl::DoRecv(char * buf, unsigned int len, const _OnRecvFromHandler& handler)
{
	if (m_summer == NULL)
	{
		LCE("CUdpSocketImpl::DoRecv()" << this << " IIOServer not bind!");
		return false;
	}
	if (len == 0 || len >1500)
	{
		LCE("CUdpSocketImpl::DoRecv()" << this << " argument err! len=" << len);
		return false;
	}
	if (len == 0 )
	{
		LCE("CUdpSocketImpl::DoRecv()" << this << "  argument err !!!  len==0");
		return false;
	}
	
	if (m_pRecvBuf != NULL || m_iRecvLen != 0 || m_onRecvFromHandler != nullptr)
	{
		LCE("CUdpSocketImpl::DoRecv()" << this << "    (m_pRecvBuf != NULL || m_iRecvLen != 0) == TRUE");
		return false;
	}

	m_onRecvFromHandler = handler;
	m_pRecvBuf = buf;
	m_iRecvLen = len;
	m_register._event.events = m_register._event.events|EPOLLIN;
	if (epoll_ctl(m_summer->m_impl.m_epoll, EPOLL_CTL_MOD, m_register._fd, &m_register._event) != 0)
	{
		LCE("CUdpSocketImpl::DoRecv()" << this << " EPOLLMod error. epfd="<<m_summer->m_impl.m_epoll << ", m_register fd=" << m_register._fd << ", errno=" << strerror(errno));
		return false;
	}
	m_isRecvFromLock = true;
	return true;
}


bool CUdpSocketImpl::OnEPOLLMessage(int type, int flag)
{
	if (m_isRecvFromLock && m_isSendToLock)
	{
		LCE("unknown error" );
		return false;
	}

	if (flag & EPOLLHUP || flag & EPOLLERR)
	{
		if (flag & EPOLLHUP)
		{
			LCE("CUdpSocketImpl::OnEPOLLMessage()" << this << " EPOLLHUP  error. epfd="<<m_summer->m_impl.m_epoll << ", m_register fd=" << m_register._fd << ", events=" << flag);
		}
		if (flag & EPOLLERR)
		{
			LCE("CUdpSocketImpl::OnEPOLLMessage()" << this << "  EPOLLERR error. epfd="<<m_summer->m_impl.m_epoll << ", m_register fd=" << m_register._fd << ", events=" << flag);
		}
		if (m_isRecvFromLock)
		{
			m_isRecvFromLock = false;
			m_onRecvFromHandler(EC_ERROR, "", 0, 0);
		}
		if (m_isSendToLock)
		{
			m_isSendToLock = false;
			m_onSendToHandler(EC_ERROR);
		}
		return false;
	}

	if (flag & EPOLLIN && m_isRecvFromLock)
	{
		m_isRecvFromLock = false;
		m_register._event.events = m_register._event.events&~EPOLLIN;
		if (epoll_ctl(m_summer->m_impl.m_epoll, EPOLL_CTL_MOD, m_register._fd, &m_register._event) != 0)
		{
			LCE("CUdpSocketImpl::DoRecv()" << this << " EPOLLMod error. epfd="<<m_summer->m_impl.m_epoll << ", m_register fd=" << m_register._fd << ", errno=" << strerror(errno));
			return false;
		}

		sockaddr_in raddr;
		memset(&raddr, 0, sizeof(raddr));
		socklen_t len = sizeof(raddr);
		int ret = recvfrom(m_register._fd, m_pRecvBuf, m_iRecvLen, 0, (sockaddr*)&raddr, &len);
		if (ret == 0 || (ret ==-1 && (errno !=EAGAIN && errno != EWOULDBLOCK)) )
		{
			LCE("CUdpSocketImpl::OnEPOLLMessage()" << this << " recv error. epfd="<<m_summer->m_impl.m_epoll << ", m_register fd=" << m_register._fd << ", ret=" << ret << ", errno=" << strerror(errno));
			return false;
		}
		if (ret == -1)
		{
			LCE("CUdpSocketImpl::OnEPOLLMessage()" << this << " recv error. epfd="<<m_summer->m_impl.m_epoll << ", m_register fd=" << m_register._fd << ", ret=" << ret << ", errno=" << strerror(errno));
			return false;
		}

		m_pRecvBuf = NULL;
		m_iRecvLen = 0;
		m_onRecvFromHandler(EC_SUCCESS, inet_ntoa(raddr.sin_addr), ntohs(raddr.sin_port), ret);
	}
	if (flag & EPOLLOUT && m_isSendToLock)
	{
		m_isSendToLock = false;
		m_register._event.events = m_register._event.events&~EPOLLOUT;
		if (epoll_ctl(m_summer->m_impl.m_epoll, EPOLL_CTL_MOD, m_register._fd, &m_register._event) != 0)
		{
			LCE("CUdpSocketImpl::DoRecv()" << this << " EPOLLMod error. epfd="<<m_summer->m_impl.m_epoll << ", m_register fd=" << m_register._fd << ", errno=" << strerror(errno));
		}

		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = inet_addr(m_dstip.c_str());
		addr.sin_port = htons(m_dstport);
		int sl = sendto(m_register._fd, m_pSendBuf, m_iSendLen, 0, (sockaddr*)&addr, sizeof(addr));
		if (sl<= 0)
		{
			LCE("CUdpSocket sendto error, sentlen=" << sl);
			m_onSendToHandler(EC_ERROR);
			return false;
		}
		else
		{
			m_onSendToHandler(EC_SUCCESS);
		}
	}
	

	return true;
}


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
	m_handle.reset();
	m_iRecvLen  = 0;
	m_pRecvBuf = NULL;
	m_iSendLen = 0;
	m_pSendBuf = NULL;
	m_dstip;
	m_dstport = 0;
}


CUdpSocketImpl::~CUdpSocketImpl()
{
	if (m_handle._fd != -1)
	{
		close(m_handle._fd);
		m_handle._fd = -1;
	}
}
bool  CUdpSocketImpl::Initialize(CZSummer & summer, const char *ip, unsigned short port)
{
	if (m_summer != NULL)
	{
		LCE("CUdpSocket socket is aready used, m_ios not is NULL. this=" << this);
		return false;
	}
	if (m_handle._fd != -1)
	{
		LCE("CUdpSocket socket is aready used, _fd not is -1. this=" << this << ", fd=" << m_handle._fd);
		return false;
	}
	m_summer = &summer;
	m_handle.reset();
	m_handle._ptr = this;
	m_handle._fd = socket(AF_INET, SOCK_DGRAM, 0);
	m_handle._type = tagRegister::REG_ESTABLISHED_UDP;
	if (m_handle._fd == -1)
	{
		LCE("CUdpSocket create socket fail. this=" << this << ", errno=" << strerror(errno));
		return false;
	}
	SetNonBlock(m_handle._fd);
	m_addr.sin_family = AF_INET;
	m_addr.sin_addr.s_addr = inet_addr(ip);
	m_addr.sin_port = htons(port);
	if (bind(m_handle._fd, (sockaddr *) &m_addr, sizeof(m_addr)) != 0)
	{
		LCE("CUdpSocket: socket bind err, errno=" << strerror(errno));
		close(m_handle._fd);
		m_handle._fd = 0;
		return false;
	}
	if (epoll_ctl(m_summer->m_impl.m_epoll, EPOLL_CTL_ADD, m_handle._fd, &m_handle._event) != 0)
	{
		LCE("CUdpSocketImpl::Initialize()" << this << " EPOLL_CTL_ADD error. epfd="<<m_summer->m_impl.m_epoll << ", handle fd=" << m_handle._fd << ", errno=" << strerror(errno));
		return false;
	}
	return true;
}

bool CUdpSocketImpl::DoSend(char * buf, unsigned int len, const char *dstip, unsigned short dstport, const _OnSendHandler& handler)
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
	m_onSendHandler = handler;
	m_pSendBuf = buf;
	m_iSendLen = len;
	m_dstip = dstip;
	m_dstport = dstport;
	m_handle.addEvent(EPOLLOUT);
	if (!EPOLLMod(m_summer->m_impl.m_epoll, m_handle._fd, &m_handle._event) != 0)
	{
		LCE("CUdpSocketImpl::DoSend()" << this << " EPOLLMod error. epfd="<<m_summer->m_impl.m_epoll << ", handle fd=" << m_handle._fd << ", errno=" << strerror(errno));
		return false;
	}
  return true;
}


bool CUdpSocketImpl::DoRecv(char * buf, unsigned int len, const _OnRecvHandler& handler)
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
	m_onRecvHandler = handler;
	if (m_pRecvBuf != NULL || m_iRecvLen != 0)
	{
		LCE("CUdpSocketImpl::DoRecv()" << this << "    (m_pRecvBuf != NULL || m_iRecvLen != 0) == TRUE");
		return false;
	}

	m_pRecvBuf = buf;
	m_iRecvLen = len;
	m_handle.addEvent(EPOLLIN);
	if (!EPOLLMod(m_summer->m_impl.m_epoll, m_handle._fd, &m_handle._event) != 0)
	{
		LCE("CUdpSocketImpl::DoRecv()" << this << " EPOLLMod error. epfd="<<m_summer->m_impl.m_epoll << ", handle fd=" << m_handle._fd << ", errno=" << strerror(errno));
		return false;
	}
	return true;
}


bool CUdpSocketImpl::OnEPOLLMessage(int type, int flag)
{
	if (flag & EPOLLHUP)
	{
		LCE("CUdpSocketImpl::OnEPOLLMessage()" << this << " EPOLLHUP  error. epfd="<<m_summer->m_impl.m_epoll << ", handle fd=" << m_handle._fd << ", events=" << flag);
		return false;
	}
	if (flag & EPOLLERR)
	{
		LCE("CUdpSocketImpl::OnEPOLLMessage()" << this << "  EPOLLERR error. epfd="<<m_summer->m_impl.m_epoll << ", handle fd=" << m_handle._fd << ", events=" << flag);
		return false;
	}
	if (flag & EPOLLIN)
	{
		m_handle.removeEvent(EPOLLIN);
		if (!EPOLLMod(m_summer->m_impl.m_epoll, m_handle._fd, &m_handle._event))
		{
			LCE("CUdpSocketImpl::OnEPOLLMessage()" << this << "recv EPOLLMod error. epfd="<<m_summer->m_impl.m_epoll << ", handle fd=" << m_handle._fd << ", errno=" << strerror(errno));
			return false;
		}
		if (m_pRecvBuf == NULL || m_iRecvLen == 0)
		{
			LCE("CUdpSocketImpl::OnEPOLLMessage()" << this << " recv error. epfd="<<m_summer->m_impl.m_epoll << ", handle fd=" << m_handle._fd << ", m_pRecvBuf=" <<(void*)m_pRecvBuf << ", m_iRecvLen=" << m_iRecvLen);
		}
		else
		{
			sockaddr_in raddr;
			memset(&raddr, 0, sizeof(raddr));
			socklen_t len = sizeof(raddr);
			int ret = recvfrom(m_handle._fd, m_pRecvBuf, m_iRecvLen, 0, (sockaddr*)&raddr, &len);
			if (ret == 0 || (ret ==-1 && (errno !=EAGAIN && errno != EWOULDBLOCK)) )
			{
				LCE("CUdpSocketImpl::OnEPOLLMessage()" << this << " recv error. epfd="<<m_summer->m_impl.m_epoll << ", handle fd=" << m_handle._fd << ", ret=" << ret << ", errno=" << strerror(errno));
				return false;
			}

			if (ret != -1)
			{
				m_pRecvBuf = NULL;
				m_iRecvLen = 0;
				m_onRecvHandler(EC_SUCCESS, inet_ntoa(raddr.sin_addr), ntohs(raddr.sin_port), ret);
			}
		}
	}
	if (flag & EPOLLOUT)
	{
		if (m_pSendBuf == NULL || m_iRecvLen == 0)
		{
			LCE("CUdpSocketImpl::OnEPOLLMessage()" << this << " recv error. epfd="<<m_summer->m_impl.m_epoll  << ", handle fd=" << m_handle._fd << ", m_pRecvBuf=" <<(void*)m_pRecvBuf << ", m_iRecvLen=" << m_iRecvLen);
			return false;
		}
		m_handle.removeEvent(EPOLLOUT);
		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = inet_addr(m_dstip.c_str());
		addr.sin_port = htons(m_dstport);
		int sl = sendto(m_handle._fd, m_pSendBuf, m_iSendLen, 0, (sockaddr*)&addr, sizeof(addr));
		if (sl<= 0)
		{
			LCE("CUdpSocket sendto error, sentlen=" << sl);
			m_onSendHandler(EC_ERROR);
			return false;
		}
		else
		{
			m_onSendHandler(EC_SUCCESS);
		}
	}
	

	return true;
}


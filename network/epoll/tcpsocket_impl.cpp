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

#include "tcpsocket_impl.h"
using namespace zsummer::network;



CTcpSocketImpl::CTcpSocketImpl(int fd, std::string remoteIP, unsigned short remotePort)
{
	m_summer = NULL;
	m_handle.reset();
	m_handle._fd = fd;
	if (m_handle._fd != -1)
	{
		m_handle._type = tagRegister::REG_ESTABLISHED_TCP;
		m_handle._ptr = this;
	}
	m_remoteIP = remoteIP;
	m_remotePort = remotePort;
	m_iRecvLen  = 0;
	m_pRecvBuf = NULL;
	m_iSendLen = 0;
	m_pSendBuf = NULL;

}


CTcpSocketImpl::~CTcpSocketImpl()
{

}

 bool CTcpSocketImpl::Initialize(CZSummer & summer)
{
	m_summer = &summer;
	if (m_handle._fd != -1)
	{
		m_handle._event.events = 0;
		if (epoll_ctl(m_summer->m_impl.m_epoll, EPOLL_CTL_ADD, m_handle._fd, &m_handle._event) != 0)
		{
			LCE("CTcpSocket::Initialize()" << this << " EPOLL_CTL_ADD error. epfd="<<m_summer->m_impl.m_epoll << ", handle fd=" << m_handle._fd << ", errno=" << strerror(errno));
			return false;
		}
	}
	return true;
}




bool CTcpSocketImpl::DoConnect(const _OnConnectHandler & handler)
{
	if (m_summer == NULL)
	{
		LCE("CTcpSocket::DoConnect()" << this << " IIOServer not bind!");
		return false;
	}
	if (m_handle._fd != -1)
	{
		LCE("CTcpSocket::DoConnect()" << this << " DoConnect ERR:  fd aready used!");
		return false;
	}
	m_handle._fd = socket(AF_INET, SOCK_STREAM, 0);
	if (m_handle._fd == -1)
	{
		LCE("CTcpSocket::DoConnect()" << this << " socket create err errno =" << strerror(errno));
		return false;
	}

	SetNonBlock(m_handle._fd);
	SetNoDelay(m_handle._fd);
	m_handle._event.events = EPOLLOUT;
	m_handle._event.data.ptr = &m_handle;
	m_handle._ptr = this;
	m_handle._type = tagRegister::REG_CONNECT;
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(m_remoteIP.c_str());
	addr.sin_port = htons(m_remotePort);
	m_onConnectHandler = handler;
	
	int ret = connect(m_handle._fd, (sockaddr *) &addr, sizeof(addr));
	if (ret!=0 && errno != EINPROGRESS)
	{
		LCE("CTcpSocket::DoConnect()" << this << " ::connect error. epfd="<<m_summer->m_impl.m_epoll << ", handle fd=" << m_handle._fd << ", errno=" << strerror(errno));
		return false;
	}
	
	if (epoll_ctl(m_summer->m_impl.m_epoll, EPOLL_CTL_ADD, m_handle._fd, &m_handle._event) != 0)
	{
		LCE("CTcpSocket::DoConnect()" << this << " EPOLL_CTL_ADD error. epfd="<<m_summer->m_impl.m_epoll << ", handle fd=" << m_handle._fd << ", errno=" << strerror(errno));
		return false;
	}

	return true;
}



bool CTcpSocketImpl::DoSend(char * buf, unsigned int len, const _OnSendHandler &handler)
{
	if (m_summer == NULL)
	{
		LCE("CTcpSocket::DoSend()" << this << " IIOServer not bind!");
		return false;
	}
	if (len == 0)
	{
		LCE("CTcpSocket::DoSend()" << this << " argument err! len ==0");
		return false;
	}
	if (m_pSendBuf != NULL || m_iSendLen != 0)
	{
		LCE("CTcpSocket::DoSend()" << this << " (m_pSendBuf != NULL || m_iSendLen != 0) == TRUE");
		return false;
	}
	if (m_handle._event.events & EPOLLOUT)
	{
		LCE("CTcpSocket::DoSend()" << this << " (m_handle._event.events & EPOLLOUT) == TRUE");
		return false;
	}
	
	
	m_pSendBuf = buf;
	m_iSendLen = len;
	m_onSendHandler = handler;

	m_handle._event.events = m_handle._event.events|EPOLLOUT;
	if (!EPOLLMod(m_summer->m_impl.m_epoll, m_handle._fd, &m_handle._event) != 0)
	{
		LCE("CTcpSocket::DoSend()" << this << " EPOLLMod error. epfd="<<m_summer->m_impl.m_epoll << ", handle fd=" << m_handle._fd << ", errno=" << strerror(errno));
		return false;
	}
	//LOGI("do send");
	return true;
}


bool CTcpSocketImpl::DoRecv(char * buf, unsigned int len, const _OnRecvHandler & handler)
{
	if (m_summer == NULL)
	{
		LCE("CTcpSocket::DoRecv()" << this << "  IIOServer not bind!");
		return false;
	}
	if (len == 0 )
	{
		LCE("CTcpSocket::DoRecv()" << this << "  argument err !!!  len==0");
		return false;
	}
	if (m_pRecvBuf != NULL || m_iRecvLen != 0)
	{
		LCE("CTcpSocket::DoRecv()" << this << "    (m_pRecvBuf != NULL || m_iRecvLen != 0) == TRUE");
		return false;
	}
	if (m_handle._event.events & EPOLLIN)
	{
		LCE("CTcpSocket::DoRecv()" << this << "  (m_handle._event.events & EPOLLIN) == TRUE");
		return false;
	}
	
	m_pRecvBuf = buf;
	m_iRecvLen = len;
	m_onRecvHandler = handler;

	m_handle._event.events = m_handle._event.events|EPOLLIN;
	if (!EPOLLMod(m_summer->m_impl.m_epoll, m_handle._fd, &m_handle._event) != 0)
	{
		LCE("CTcpSocket::DoRecv()" << this << " EPOLLMod error. epfd="<<m_summer->m_impl.m_epoll << ", handle fd=" << m_handle._fd << ", errno=" << strerror(errno));
		return false;
	}
	return true;
}


bool CTcpSocketImpl::OnEPOLLMessage(int type, int flag)
{
	if (m_handle._type == tagRegister::REG_CONNECT)
	{
		if (flag & EPOLLOUT)
		{
			m_handle._event.events = /*EPOLLONESHOT*/ 0;
			if (!EPOLLMod(m_summer->m_impl.m_epoll, m_handle._fd, &m_handle._event))
			{
				LCE("CTcpSocket::OnEPOLLMessage()" << this << " connect true & EPOLLMod error. epfd="<<m_summer->m_impl.m_epoll << ", handle fd=" << m_handle._fd << ", errno=" << strerror(errno));
				m_onConnectHandler(EC_ERROR);
				return false;
			}
			m_handle._type = tagRegister::REG_ESTABLISHED_TCP;
			m_onConnectHandler(EC_SUCCESS);

		}
		else 
		{
			if (epoll_ctl(m_summer->m_impl.m_epoll, EPOLL_CTL_DEL, m_handle._fd, &m_handle._event) != 0)
			{
				LCE("CTcpSocket::OnEPOLLMessage()" << this << " connect false & EPOLLMod error. epfd="<<m_summer->m_impl.m_epoll << ", handle fd=" << m_handle._fd << ", errno=" << strerror(errno));
			}
			close(m_handle._fd);
			m_handle._fd = 0;
			m_onConnectHandler(EC_ERROR);
		}
		return true;
	}

	if (flag & EPOLLHUP || flag & EPOLLERR)
	{
		if (flag & EPOLLHUP)
		{
			//LCI("CTcpSocket::OnEPOLLMessage()" << this << " EPOLLHUP  error. epfd="<<((CIOServer *)m_ios)->m_epoll << ", handle fd=" << m_handle._fd << ", events=" << flag);
		}

		if (flag & EPOLLERR)
		{
			//LCI("CTcpSocket::OnEPOLLMessage()" << this << "  EPOLLERR error. epfd="<<m_summer->m_impl.m_epoll << ", handle fd=" << m_handle._fd << ", events=" << flag);
		}
		if (m_handle._event.events  & EPOLLIN)
		{
			bool ret = true;
			ZSUMMER_EPOLL_MOD_DEL(ret, EPOLLIN);
			m_onRecvHandler(EC_ERROR,0);
		}
		if (m_handle._event.events  & EPOLLOUT)
		{
			bool ret = true;
			ZSUMMER_EPOLL_MOD_DEL(ret, EPOLLOUT);
			m_onSendHandler(EC_ERROR,0);
		}
		return false;
	}


	if (flag & EPOLLIN)
	{
		int ret = recv(m_handle._fd, m_pRecvBuf, m_iRecvLen, 0);
		if (ret == 0)
		{
			LCE("CTcpSocket::OnEPOLLMessage()" << this << " recv error. epfd="<<m_summer->m_impl.m_epoll << ", handle fd=" << m_handle._fd << ", ret=" << ret << ", errno=" << strerror(errno));
			m_onRecvHandler(EC_REMOTE_CLOSED,ret);
			return false;
		}
		if (ret ==-1 && (errno !=EAGAIN && errno != EWOULDBLOCK) )
		{
			LCE("CTcpSocket::OnEPOLLMessage()" << this << " recv error. epfd="<<m_summer->m_impl.m_epoll << ", handle fd=" << m_handle._fd << ", ret=" << ret << ", errno=" << strerror(errno));
			m_onRecvHandler(EC_ERROR,ret);
			return false;
		}

		if (ret != -1)
		{
			m_pRecvBuf = NULL;
			m_iRecvLen = 0;
			bool mod = true;
			ZSUMMER_EPOLL_MOD_DEL(mod, EPOLLIN);
			if (!mod)
			{
				m_onRecvHandler(EC_ERROR,0);
				return false;
			}
			m_onRecvHandler(EC_SUCCESS,ret);
		}
	}

	if (flag & EPOLLOUT)
	{
		if (m_pSendBuf == NULL || m_iSendLen == 0)
		{
			LCE("CTcpSocket::OnEPOLLMessage()" << this << " send error. epfd="<<m_summer->m_impl.m_epoll << ", handle fd=" << m_handle._fd << ", m_pSendBuf=" <<(void*)m_pSendBuf << ", m_iSendLen=" << m_iSendLen);
		}
		else
		{
			int ret = send(m_handle._fd, m_pSendBuf, m_iSendLen, 0);
			if (ret == -1 && (errno != EAGAIN && errno != EWOULDBLOCK))
			{
				LCE("CTcpSocket::OnEPOLLMessage()" << this << " send error. epfd="<<m_summer->m_impl.m_epoll << ", handle fd=" << m_handle._fd << ", ret=" << ret << ", errno=" << strerror(errno));
				return false;
			}

			if (ret != -1)
			{
				m_pSendBuf = NULL;
				m_iSendLen = 0;
				bool mod = true;
				ZSUMMER_EPOLL_MOD_DEL(mod, EPOLLOUT);
				if (!mod)
				{
					m_onSendHandler(EC_ERROR,0);
					return false;
				}
				m_onSendHandler(EC_SUCCESS, ret);
			}
		}
	}

	return true;
}


bool CTcpSocketImpl::DoClose()
{
	if (m_handle._type == tagRegister::REG_ESTABLISHED_TCP)
	{
		m_handle._type = tagRegister::REG_INVALIDE;
		if (epoll_ctl(m_summer->m_impl.m_epoll, EPOLL_CTL_DEL, m_handle._fd, &m_handle._event) != 0)
		{
			LCE("CTcpSocket::Close()" << this << " EPOLL_CTL_DEL error. epfd="<<m_summer->m_impl.m_epoll << ", handle fd=" << m_handle._fd << ", errno=" << strerror(errno));
		}
		shutdown(m_handle._fd, SHUT_RDWR);
		close(m_handle._fd);
	}
	return true;
}


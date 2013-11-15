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
	m_register.reset();
	m_register._fd = fd;
	if (m_register._fd != -1)
	{
		m_register._type = tagRegister::REG_ESTABLISHED_TCP;
		m_register._ptr = this;
	}
	m_remoteIP = remoteIP;
	m_remotePort = remotePort;

	m_iRecvLen  = 0;
	m_pRecvBuf = NULL;
	m_isRecvLock = false;

	m_iSendLen = 0;
	m_pSendBuf = NULL;
	m_isSendLock = false;
}


CTcpSocketImpl::~CTcpSocketImpl()
{
	if (m_isRecvLock || m_isSendLock)
	{
		LCF("CTcpSocketImpl::~CTcpSocketImpl() m_isRecvLock || m_isSendLock");
	}
	if (m_register._fd != -1)
	{
		close(m_register._fd);
		m_register._fd = -1;
	}
}

 bool CTcpSocketImpl::Initialize(CZSummer & summer)
{
	m_summer = &summer;
	if (m_register._fd != -1)
	{
		m_register._event.events = 0;
		if (epoll_ctl(m_summer->m_impl.m_epoll, EPOLL_CTL_ADD, m_register._fd, &m_register._event) != 0)
		{
			LCF("CTcpSocketImpl EPOLL_CTL_ADD error. epfd="<<m_summer->m_impl.m_epoll << ", m_register fd=" << m_register._fd << ", errno=" << strerror(errno));
		}
	}
	return true;
}




bool CTcpSocketImpl::DoConnect(const _OnConnectHandler & handler)
{
	if (m_summer == NULL)
	{
		LCE("CTcpSocketImpl::DoConnect()" << this << " summer not bind!");
		return false;
	}
	if (m_register._type != tagRegister::REG_INVALIDE)
	{
		LCE("CTcpSocketImpl::DoConnect()" << this << "  type not REG_INVALIDE!");
		return false;
	}
	if (m_register._fd != -1)
	{
		LCE("CTcpSocketImpl::DoConnect()" << this << " fd aready used!");
		return false;
	}
	m_register._fd = socket(AF_INET, SOCK_STREAM, 0);
	if (m_register._fd == -1)
	{
		LCE("CTcpSocketImpl::DoConnect()" << this << " socket create err errno =" << strerror(errno));
		return false;
	}

	SetNonBlock(m_register._fd);
	SetNoDelay(m_register._fd);
	m_register._event.events = EPOLLOUT;
	m_register._ptr = this;
	m_register._type = tagRegister::REG_CONNECT;
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(m_remoteIP.c_str());
	addr.sin_port = htons(m_remotePort);
	m_onConnectHandler = handler;
	
	int ret = connect(m_register._fd, (sockaddr *) &addr, sizeof(addr));
	if (ret!=0 && errno != EINPROGRESS)
	{
		LCE("CTcpSocket::DoConnect()" << this << " ::connect error. epfd="<<m_summer->m_impl.m_epoll << ", m_register fd=" << m_register._fd << ", errno=" << strerror(errno));
		m_onConnectHandler = nullptr;
		return false;
	}
	
	if (epoll_ctl(m_summer->m_impl.m_epoll, EPOLL_CTL_ADD, m_register._fd, &m_register._event) != 0)
	{
		LCF("CTcpSocket::DoConnect()" << this << " EPOLL_CTL_ADD error. epfd="<<m_summer->m_impl.m_epoll << ", m_register fd=" << m_register._fd << ", errno=" << strerror(errno));
		m_onConnectHandler = nullptr;
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
	if (m_register._type != tagRegister::REG_ESTABLISHED_TCP)
	{
		LCE("CTcpSocketImpl::DoRecv()" << this << "  type not REG_ESTABLISHED_TCP!");
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
	if (m_isSendLock)
	{
		LCE("CTcpSocket::DoSend()" << this << " m_isSendLock == TRUE");
		return false;
	}

	
	
	m_pSendBuf = buf;
	m_iSendLen = len;
	m_onSendHandler = handler;

	m_register._event.events = m_register._event.events|EPOLLOUT;
	if (epoll_ctl(m_summer->m_impl.m_epoll,EPOLL_CTL_MOD, m_register._fd, &m_register._event) != 0)
	{
		LCD("CTcpSocket::DoSend()" << this << " EPOLLMod error. epfd="<<m_summer->m_impl.m_epoll << ", m_register fd=" << m_register._fd << ", errno=" << strerror(errno));
		m_pSendBuf = nullptr;
		m_iSendLen = 0;
		m_onSendHandler = nullptr;
		return false;
	}
	m_isSendLock = true;
	//LOGI("do send");
	return true;
}


bool CTcpSocketImpl::DoRecv(char * buf, unsigned int len, const _OnRecvHandler & handler)
{
	if (m_summer == NULL)
	{
		LCE("CTcpSocketImpl::DoRecv()" << this << "  IIOServer not bind!");
		return false;
	}
	if (m_register._type != tagRegister::REG_ESTABLISHED_TCP)
	{
		LCE("CTcpSocketImpl::DoRecv()" << this << "  type not REG_ESTABLISHED_TCP!");
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
	if (m_isRecvLock)
	{
		LCE("CTcpSocket::DoRecv()" << this << "  (m_isRecvLock) == TRUE");
		return false;
	}
	
	m_pRecvBuf = buf;
	m_iRecvLen = len;
	m_onRecvHandler = handler;

	m_register._event.events = m_register._event.events|EPOLLIN;
	if (epoll_ctl(m_summer->m_impl.m_epoll, EPOLL_CTL_MOD, m_register._fd, &m_register._event) != 0)
	{
		LCD("CTcpSocket::DoRecv()" << this << " EPOLLMod error. epfd="<<m_summer->m_impl.m_epoll << ", m_register fd=" << m_register._fd << ", errno=" << strerror(errno));
		m_pRecvBuf = nullptr;
		m_iRecvLen = 0;
		m_onRecvHandler = nullptr;
		return false;
	}
	m_isRecvLock = true;
	return true;
}


bool CTcpSocketImpl::OnEPOLLMessage(int type, int flag)
{
	bool isNeedUnlockRecv = m_isRecvLock;
	bool isNeedUnlockSend = m_isSendLock;
	unsigned char linkType = m_register._type;
	ErrorCode ec = EC_ERROR;

	if (!isNeedUnlockRecv && !isNeedUnlockSend && linkType != tagRegister::REG_CONNECT)
	{
		LCE("unknown error.  epfd="<<m_summer->m_impl.m_epoll << ", m_register fd=" << m_register._fd << ", errno=" << strerror(errno));
		return false;
	}

	if (linkType == tagRegister::REG_CONNECT)
	{
		_OnConnectHandler onConnect;
		onConnect.swap(m_onConnectHandler);
		if (flag & EPOLLOUT && !(flag &EPOLLHUP) && ! (flag & EPOLLERR))
		{
			m_register._event.events = /*EPOLLONESHOT*/ 0;
			if (epoll_ctl(m_summer->m_impl.m_epoll, EPOLL_CTL_MOD, m_register._fd, &m_register._event) != 0)
			{
				LCD("CTcpSocketImpl::OnEPOLLMessage()" << this << " connect true & EPOLLMod error. epfd="<<m_summer->m_impl.m_epoll << ", m_register fd=" << m_register._fd << ", errno=" << strerror(errno));
			}
			m_register._type = tagRegister::REG_ESTABLISHED_TCP;
			onConnect(EC_SUCCESS);
		}
		else 
		{
			m_register._type = tagRegister::REG_INVALIDE;
			if (epoll_ctl(m_summer->m_impl.m_epoll, EPOLL_CTL_DEL, m_register._fd, &m_register._event) != 0)
			{
				LCD("CTcpSocketImpl::OnEPOLLMessage()" << this << " connect true & EPOLL DEL error. epfd="<<m_summer->m_impl.m_epoll << ", m_register fd=" << m_register._fd << ", errno=" << strerror(errno));
			}
			onConnect(EC_ERROR);
		}
		return true;
	}

	if (linkType != tagRegister::REG_ESTABLISHED_TCP)
	{
		LCE("CTcpSocketImpl::OnEPOLLMessage()" << this << " unknow type !=REG_ESTABLISHED_TCP. epfd="<<m_summer->m_impl.m_epoll << ", m_register fd=" << m_register._fd << ", errno=" << strerror(errno));
		return true;
	}

	if (flag & EPOLLHUP || flag & EPOLLERR)
	{
		ec = EC_REMOTE_HANGUP;
		goto clean;
		return false;
	}


	if (flag & EPOLLIN && isNeedUnlockRecv)
	{
		int ret = recv(m_register._fd, m_pRecvBuf, m_iRecvLen, 0);
		m_register._event.events = m_register._event.events &~EPOLLIN;
		if (epoll_ctl(m_summer->m_impl.m_epoll, EPOLL_CTL_MOD, m_register._fd, &m_register._event) != 0)
		{
			LCF("CTcpSocketImpl::OnEPOLLMessage()" << this << " connect true & EPOLLMod error. epfd="<<m_summer->m_impl.m_epoll << ", m_register fd=" << m_register._fd << ", errno=" << strerror(errno));
		}
		if (ret == 0)
		{
			ec = EC_REMOTE_CLOSED;
			goto clean;
			return false;
		}
		if (ret ==-1 && (errno !=EAGAIN && errno != EWOULDBLOCK) )
		{
			ec = EC_ERROR;
			goto clean;
			return false;
		}
		if (ret != -1)
		{
			_OnRecvHandler onRecv;
			onRecv.swap(m_onRecvHandler);
			m_isRecvLock = false;
			m_pRecvBuf = NULL;
			m_iRecvLen = 0;
			onRecv(EC_SUCCESS,ret);
		}
	}

	if (flag & EPOLLOUT && isNeedUnlockSend)
	{
		int ret = send(m_register._fd, m_pSendBuf, m_iSendLen, 0);
		m_register._event.events = m_register._event.events &~EPOLLOUT;
		if (epoll_ctl(m_summer->m_impl.m_epoll, EPOLL_CTL_MOD, m_register._fd, &m_register._event) != 0)
		{
			LCF("CTcpSocketImpl::OnEPOLLMessage()" << this << " connect true & EPOLLMod error. epfd="<<m_summer->m_impl.m_epoll << ", m_register fd=" << m_register._fd << ", errno=" << strerror(errno));
		}
		if (ret == -1 && (errno != EAGAIN && errno != EWOULDBLOCK))
		{
			ec = EC_ERROR;
			goto clean;
			return false;
		}
		if (ret != -1)
		{
			m_isSendLock = false;
			_OnSendHandler onSend;
			onSend.swap(m_onSendHandler);
			m_pSendBuf = NULL;
			m_iSendLen = 0;
			onSend(EC_SUCCESS, ret);
		}

	}
	return true;

clean:

	if (m_register._type == tagRegister::REG_ESTABLISHED_TCP)
	{
		m_register._type = tagRegister::REG_INVALIDE;
	}
	if (epoll_ctl(m_summer->m_impl.m_epoll, EPOLL_CTL_DEL, m_register._fd, &m_register._event) != 0)
	{
		LCD("CTcpSocketImpl::OnEPOLLMessage()" << this << " connect true & EPOLL DEL error. epfd="<<m_summer->m_impl.m_epoll << ", m_register fd=" << m_register._fd << ", errno=" << strerror(errno));
	}

	if (isNeedUnlockRecv)
	{
		m_isRecvLock = false;
		_OnRecvHandler onRecv;
		onRecv.swap(m_onRecvHandler);
		onRecv(ec,0);
	}
	if (isNeedUnlockSend)
	{
		m_isSendLock = false;
		_OnSendHandler onSend;
		onSend.swap(m_onSendHandler);
		onSend(ec,0);
	}
	return true;
}


bool CTcpSocketImpl::DoClose()
{
	if (m_register._type == tagRegister::REG_ESTABLISHED_TCP)
	{
		m_register._type = tagRegister::REG_INVALIDE;
		if (epoll_ctl(m_summer->m_impl.m_epoll, EPOLL_CTL_DEL, m_register._fd, &m_register._event) != 0)
		{
			LCD("CTcpSocket::Close()" << this << " EPOLL_CTL_DEL error. epfd="<<m_summer->m_impl.m_epoll << ", m_register fd=" << m_register._fd << ", errno=" << strerror(errno));
		}
	}
	if (m_register._fd != -1)
	{
		shutdown(m_register._fd, SHUT_RDWR);
	}
	return true;
}


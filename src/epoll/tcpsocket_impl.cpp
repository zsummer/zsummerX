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
 * Copyright (C) 2013-2014 YaweiZhang <yawei_zhang@foxmail.com>.
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


#include <zsummerX/epoll/tcpsocket_impl.h>

using namespace zsummer::network;



CTcpSocket::CTcpSocket()
{
	m_register._event.data.ptr = &m_register;
	m_register._event.events = 0;
	m_register._type = tagRegister::REG_TCP_SOCKET;
	g_appEnvironment.AddCreatedSocketCount();
}


CTcpSocket::~CTcpSocket()
{
	g_appEnvironment.AddClosedSocketCount();
	if (m_onRecvHandler || m_onSendHandler || m_onConnectHandler)
	{
		LCT("CTcpSocket::~CTcpSocket[this0x" << this << "] Handler status error. " << SocketSection());
	}
	if (m_register._fd != InvalideFD)
	{
		close(m_register._fd);
		m_register._fd = InvalideFD;
	}
}

std::string CTcpSocket::SocketSection()
{
	std::stringstream os;
	os << ";; Status: summer.user_count()=" << m_summer.use_count() << ", remoteIP=" << m_remoteIP << ", remotePort=" << m_remotePort
		<< ", m_onConnectHandler = " << (bool)m_onConnectHandler 
		<< ", m_onRecvHandler = " << (bool)m_onRecvHandler << ", m_pRecvBuf=" << (void*)m_pRecvBuf << ", m_iRecvLen=" << m_iRecvLen 
		<< ", m_onSendHandler = " << (bool)m_onSendHandler << ", m_pSendBuf=" << (void*)m_pSendBuf << ", m_iSendLen=" << m_iSendLen
		<< "; m_register=" << m_register; 
	return os.str();
}

bool CTcpSocket::Initialize(const ZSummerPtr& summer)
{
	 m_summer = summer;
	if (m_register._linkstat != LS_UNINITIALIZE)
	{
		if (!m_summer->RegisterEvent(EPOLL_CTL_ADD, m_register))
		{
			LCE("CTcpSocket::Initialize[this0x" << this << "] socket already used or not initilize." << SocketSection());
			return false;
		}
		m_register._linkstat = LS_ESTABLISHED;
	}
	else
	{
		if (m_register._fd != -1)
		{
			LCE("CTcpSocket::Initialize[this0x" << this << "] fd aready used!" << SocketSection());
			return false;
		}
		m_register._fd = socket(AF_INET, SOCK_STREAM, 0);
		if (m_register._fd == -1)
		{
			LCE("CTcpSocket::Initialize[this0x" << this << "] fd create failed!" << SocketSection());
			return false;
		}
		m_register._linkstat = LS_WAITLINK;
	}
	SetNonBlock(m_register._fd);
	SetNoDelay(m_register._fd);

	return true;
}
bool CTcpSocket::AttachSocket(int s, const std::string & remoteIP, unsigned short remotePort)
{
	m_register._fd = s;
	m_remoteIP = remoteIP;
	m_remotePort = remotePort;
	m_register._linkstat = LS_WAITLINK;
	return true;
}




bool CTcpSocket::DoConnect(const std::string& remoteIP, unsigned short remotePort, _OnConnectHandler && handler)
{
	if (!m_summer)
	{
		LCE("CTcpSocket::DoConnect[this0x" << this << "] summer not bind!" << SocketSection());
		return false;
	}
	if (m_register._linkstat != LS_WAITLINK)
	{
		LCE("CTcpSocket::DoConnect[this0x" << this << "] _linkstat not LS_WAITLINK!" << SocketSection());
		return false;
	}

	m_register._event.events = EPOLLOUT;

	m_remoteIP = remoteIP;
	m_remotePort = remotePort;
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(m_remoteIP.c_str());
	addr.sin_port = htons(m_remotePort);
	
	
	int ret = connect(m_register._fd, (sockaddr *) &addr, sizeof(addr));
	if (ret!=0 && errno != EINPROGRESS)
	{
		LCT("CTcpSocket::DoConnect[this0x" << this << "] ::connect error. errno=" << strerror(errno) << SocketSection());
		close(m_register._fd);
		m_register._fd = InvalideFD;
		return false;
	}
	if (!m_summer->RegisterEvent(EPOLL_CTL_ADD, m_register))
	{
		LCT("CTcpSocket::Initialize[this0x" << this << "] socket already used or not initilize." << SocketSection());
		return false;
	}
	m_onConnectHandler = std::move(handler);
	m_register._tcpSocketConnectPtr = shared_from_this();
	return true;
}


bool CTcpSocket::DoSend(char * buf, unsigned int len, _OnSendHandler && handler)
{
	if (m_register._linkstat != LS_ESTABLISHED)
	{
		LCT("CTcpSocket::DoSend[this0x" << this << "] _linkstat not REG_ESTABLISHED_TCP!" << SocketSection());
		return false;
	}

	if (!m_summer)
	{
		LCE("CTcpSocket::DoSend[this0x" << this << "] m_summer not bind!" << SocketSection());
		return false;
	}

	if (len == 0)
	{
		LCE("CTcpSocket::DoSend[this0x" << this << "] argument err! len ==0" << SocketSection());
		return false;
	}
	if (m_pSendBuf != NULL || m_iSendLen != 0)
	{
		LCE("CTcpSocket::DoSend[this0x" << this << "] (m_pSendBuf != NULL || m_iSendLen != 0) == TRUE" << SocketSection());
		return false;
	}
	if (m_onSendHandler)
	{
		LCE("CTcpSocket::DoSend[this0x" << this << "] m_onSendHandler == TRUE" << SocketSection());
		return false;
	}

	
	
	m_pSendBuf = buf;
	m_iSendLen = len;
	

	m_register._event.events = m_register._event.events|EPOLLOUT;
	if (!m_summer->RegisterEvent(EPOLL_CTL_MOD, m_register))
	{
		LCT("CTcpSocket::DoSend[this0x" << this << "] RegisterEvent Error" << SocketSection());
		m_pSendBuf = nullptr;
		m_iSendLen = 0;
		return false;
	}
	m_onSendHandler = std::move(handler);
	m_register._tcpSocketSendPtr = shared_from_this();
	return true;
}


bool CTcpSocket::DoRecv(char * buf, unsigned int len, _OnRecvHandler && handler)
{
	if (m_register._linkstat != LS_ESTABLISHED)
	{
		LCT("CTcpSocket::DoRecv[this0x" << this << "] type not REG_ESTABLISHED_TCP!" << SocketSection());
		return false;
	}

	if (!m_summer)
	{
		LCE("CTcpSocket::DoRecv[this0x" << this << "] m_summer not bind!" << SocketSection());
		return false;
	}

	if (len == 0 )
	{
		LCE("CTcpSocket::DoRecv[this0x" << this << "] argument err !!!  len==0" << SocketSection());
		return false;
	}
	if (m_pRecvBuf != NULL || m_iRecvLen != 0)
	{
		LCE("CTcpSocket::DoRecv[this0x" << this << "]  (m_pRecvBuf != NULL || m_iRecvLen != 0) == TRUE" << SocketSection());
		return false;
	}
	if (m_onRecvHandler)
	{
		LCE("CTcpSocket::DoRecv[this0x" << this << "] (m_onRecvHandler) == TRUE" << SocketSection());
		return false;
	}
	
	m_pRecvBuf = buf;
	m_iRecvLen = len;
	

	m_register._event.events = m_register._event.events|EPOLLIN;
	if (!m_summer->RegisterEvent(EPOLL_CTL_MOD, m_register))
	{
		LCT("CTcpSocket::DoRecv[this0x" << this << "] RegisterEvent Error" << SocketSection());
		m_pRecvBuf = nullptr;
		m_iRecvLen = 0;
		DoClose();
		return false;
	}
	m_register._tcpSocketRecvPtr = shared_from_this();
	m_onRecvHandler = std::move(handler);
	return true;
}


void CTcpSocket::OnEPOLLMessage(int flag, bool err)
{
	unsigned char linkstat = m_register._linkstat;
	ErrorCode ec = EC_ERROR;

	if (!m_onRecvHandler && !m_onSendHandler && linkstat != LS_WAITLINK)
	{
		LCE("CTcpSocket::OnEPOLLMessage[this0x" << this << "] unknown error. errno=" << strerror(errno) << SocketSection());
		return ;
	}

	if (linkstat == LS_WAITLINK)
	{
		std::shared_ptr<CTcpSocket> guard(std::move(m_register._tcpSocketConnectPtr));
		_OnConnectHandler onConnect(std::move(m_onConnectHandler));

		if (flag & EPOLLOUT && !err)
		{
			m_register._event.events = /*EPOLLONESHOT*/ 0;
			m_summer->RegisterEvent(EPOLL_CTL_MOD, m_register);
			m_register._linkstat = LS_ESTABLISHED;
			onConnect(EC_SUCCESS);
			return;
		}
		else 
		{
			m_register._linkstat = LS_WAITLINK;
			m_summer->RegisterEvent(EPOLL_CTL_DEL, m_register);
			onConnect(EC_ERROR);
			return;
		}
		return ;
	}

	if (flag & EPOLLIN && m_onRecvHandler)
	{
		std::shared_ptr<CTcpSocket> guard(std::move(m_register._tcpSocketRecvPtr));
		int ret = recv(m_register._fd, m_pRecvBuf, m_iRecvLen, 0);
		m_register._event.events = m_register._event.events &~EPOLLIN;

		if (!m_summer->RegisterEvent(EPOLL_CTL_MOD, m_register))
		{
			LCF("CTcpSocket::OnEPOLLMessage[this0x" << this << "] connect true & EPOLLMod error.  errno=" << strerror(errno) << SocketSection());
		}
		if (ret == 0 || 
			(ret == -1 && (errno != EAGAIN && errno != EWOULDBLOCK)) || err)
		{
			ec = EC_REMOTE_CLOSED;
			m_register._linkstat = LS_CLOSED;
			if (m_onRecvHandler)
			{
				_OnRecvHandler onRecv(std::move(m_onRecvHandler));
				onRecv(ec, 0);
			}
			if (!m_onSendHandler && !m_onRecvHandler)
			{
				if (!m_summer->RegisterEvent(EPOLL_CTL_DEL, m_register))
				{
					LCW("CTcpSocket::OnEPOLLMessage[this0x" << this << "] connect true & EPOLL DEL error.  errno=" << strerror(errno) << SocketSection());
				}
			}
			return ;
		}
		else if (ret != -1)
		{
			_OnRecvHandler onRecv(std::move(m_onRecvHandler));
			m_pRecvBuf = NULL;
			m_iRecvLen = 0;
			onRecv(EC_SUCCESS,ret);
		}
	}
	else if (flag & EPOLLOUT && m_onSendHandler)
	{
		std::shared_ptr<CTcpSocket> guard(std::move(m_register._tcpSocketSendPtr));

		int ret = send(m_register._fd, m_pSendBuf, m_iSendLen, 0);
		m_register._event.events = m_register._event.events &~EPOLLOUT;
		if (!m_summer->RegisterEvent(EPOLL_CTL_MOD, m_register))
		{
			LCF("CTcpSocket::OnEPOLLMessage[this0x" << this << "] connect true & EPOLLMod error. errno=" << strerror(errno) << SocketSection());
		}
		
		if ((ret == -1 && (errno != EAGAIN && errno != EWOULDBLOCK)) || m_register._linkstat == LS_CLOSED || err)
		{
			ec = EC_ERROR;
			m_register._linkstat = LS_CLOSED;
			m_onSendHandler = nullptr;
			if (!m_onSendHandler && !m_onRecvHandler)
			{
				if (!m_summer->RegisterEvent(EPOLL_CTL_DEL, m_register))
				{
					LCW("CTcpSocket::OnEPOLLMessage[this0x" << this << "] connect true & EPOLL DEL error.  errno=" << strerror(errno) << SocketSection());
				}
			}
			return ;
		}
		else if (ret != -1)
		{
			_OnSendHandler onSend(std::move(m_onSendHandler));
			m_pSendBuf = NULL;
			m_iSendLen = 0;
			onSend(EC_SUCCESS, ret);
		}
	}
	return ;
}


bool CTcpSocket::DoClose()
{
	if (m_register._fd != InvalideFD)
	{
		shutdown(m_register._fd, SHUT_RDWR);
	}
	return true;
}


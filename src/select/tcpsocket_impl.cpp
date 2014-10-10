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


#include <zsummerX/select/tcpsocket_impl.h>

using namespace zsummer::network;




CTcpSocket::CTcpSocket()
{
	g_appEnvironment.AddCreatedSocketCount();
	m_register._type = tagRegister::REG_TCP_SOCKET;
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
		closesocket(m_register._fd);
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

bool CTcpSocket::Initialize(ZSummerPtr summer)
{
	 m_summer = summer;
	if (m_register._linkstat != LS_UNINITIALIZE)
	{
		if (!m_summer->RegisterEvent(0, m_register))
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
			LCE("CTcpSocket::DoConnect[this0x" << this << "] fd aready used!" << SocketSection());
			return false;
		}
		m_register._fd = socket(AF_INET, SOCK_STREAM, 0);
		if (m_register._fd == -1)
		{
			LCE("CTcpSocket::DoConnect[this0x" << this << "] fd create failed!" << SocketSection());
			return false;
		}
		SetNonBlock(m_register._fd);
		SetNoDelay(m_register._fd);
		m_register._linkstat = LS_WAITLINK;
	}



	return true;
}
bool CTcpSocket::AttachSocket(SOCKET s, std::string remoteIP, unsigned short remotePort)
{
	 m_register._fd = s;
	 m_remoteIP = remoteIP;
	 m_remotePort = remotePort;
	 m_register._linkstat = LS_WAITLINK;
	return true;
}




bool CTcpSocket::DoConnect(std::string remoteIP, unsigned short remotePort, const _OnConnectHandler & handler)
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

	m_register._wt = true;

	m_remoteIP = remoteIP;
	m_remotePort = remotePort;
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(m_remoteIP.c_str());
	addr.sin_port = htons(m_remotePort);
	
	
	int ret = connect(m_register._fd, (sockaddr *) &addr, sizeof(addr));
#ifndef WIN32
	if (ret != 0 && errno != EINPROGRESS)
#else
	if (ret != 0 && WSAGetLastError() != WSAEWOULDBLOCK)
#endif
	{
		LCT("CTcpSocket::DoConnect[this0x" << this << "] ::connect error. " << OSTREAM_GET_LASTERROR << SocketSection());
		closesocket(m_register._fd);
		m_register._fd = InvalideFD;
		return false;
	}
	
	m_register._tcpSocketConnectPtr = shared_from_this();
	if (!m_summer->RegisterEvent(0, m_register))
	{
		LCE("CTcpSocket::DoConnect[this0x" << this << "] RegisterEvent Error" << SocketSection());
		closesocket(m_register._fd);
		m_register._fd = InvalideFD;
		m_register._tcpSocketConnectPtr.reset();
		return false;
	}
	m_onConnectHandler = handler;
	return true;
}


bool CTcpSocket::DoSend(char * buf, unsigned int len, const _OnSendHandler &handler)
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
		LCE("CTcpSocket::DoSend[this0x" << this << "] m_pSendBuf =" << (void *) m_pSendBuf << " m_iSendLen =" << m_iSendLen<< SocketSection());
		return false;
	}
	if (m_onSendHandler)
	{
		LCE("CTcpSocket::DoSend[this0x" << this << "] m_onSendHandler == TRUE" << SocketSection());
		return false;
	}

	
	
	m_pSendBuf = buf;
	m_iSendLen = len;
	

	m_register._wt = true;
	m_register._tcpSocketSendPtr = shared_from_this();
	if (!m_summer->RegisterEvent(1, m_register))
	{
		LCT("CTcpSocket::DoSend[this0x" << this << "] RegisterEvent Error" << SocketSection());
		m_pSendBuf = nullptr;
		m_iSendLen = 0;
		m_register._tcpSocketSendPtr.reset();
		DoClose();
		return false;
	}
	m_onSendHandler = handler;
	
	return true;
}


bool CTcpSocket::DoRecv(char * buf, unsigned int len, const _OnRecvHandler & handler)
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
	

	m_register._rd = true;
	m_register._tcpSocketRecvPtr = shared_from_this();
	if (!m_summer->RegisterEvent(1, m_register))
	{
		LCT("CTcpSocket::DoRecv[this0x" << this << "] RegisterEvent Error" << SocketSection());
		m_pRecvBuf = nullptr;
		m_iRecvLen = 0;
		m_register._tcpSocketRecvPtr.reset();
		return false;
	}
	m_onRecvHandler = handler;
	
	return true;
}


void CTcpSocket::OnSelectMessage(bool rd, bool wt, bool err)
{
	unsigned char linkstat = m_register._linkstat;
	ErrorCode ec = EC_ERROR;

	if (!m_onRecvHandler && !m_onSendHandler && !m_onConnectHandler)
	{
		LCE("CTcpSocket::OnSelectMessage[this0x" << this << "] unknown error. " << OSTREAM_GET_LASTERROR << SocketSection());
		return ;
	}

	if (linkstat == LS_WAITLINK)
	{
		std::shared_ptr<CTcpSocket> guad(m_register._tcpSocketConnectPtr);
		m_register._tcpSocketConnectPtr.reset();
		_OnConnectHandler onConnect;
		onConnect.swap(m_onConnectHandler);
		int errCode = 0;
		socklen_t len = sizeof(int);
		if (err || getsockopt(m_register._fd, SOL_SOCKET, SO_ERROR, (char*)&errCode, &len) != 0 || errCode != 0)
		{
			LOGT("OnConnect False. " << OSTREAM_GET_LASTERROR);
			m_register._linkstat = LS_WAITLINK;
			m_summer->RegisterEvent(2, m_register);
			onConnect(EC_ERROR);
			return;
		}
		else
		{
			m_register._wt =  0;
			m_summer->RegisterEvent(1, m_register);
			m_register._linkstat = LS_ESTABLISHED;
			onConnect(EC_SUCCESS);
			return;
		}
		return ;
	}


	if (rd && m_onRecvHandler)
	{
		std::shared_ptr<CTcpSocket> guad(m_register._tcpSocketRecvPtr);
		m_register._tcpSocketRecvPtr.reset();

		int ret = recv(m_register._fd, m_pRecvBuf, m_iRecvLen, 0);
		m_register._rd = false;
		if (!m_summer->RegisterEvent(1, m_register))
		{
			LCF("CTcpSocket::OnSelectMessage[this0x" << this << "] connect true & EPOLLMod error.  " << OSTREAM_GET_LASTERROR << SocketSection());
		}
		if (ret == 0 || (ret == -1 && !IS_WOULDBLOCK))
		{
			ec = EC_ERROR;
			m_register._linkstat = LS_CLOSED;
			if (rd && m_onRecvHandler)
			{
				_OnRecvHandler onRecv;
				onRecv.swap(m_onRecvHandler);
				onRecv(ec, 0);
			}
			if (!m_onSendHandler && !m_onRecvHandler)
			{
				if (!m_summer->RegisterEvent(2, m_register))
				{
					LCW("CTcpSocket::OnSelectMessage[this0x" << this << "] connect true & EPOLL DEL error.  " << OSTREAM_GET_LASTERROR << SocketSection());
				}
			}
			return ;
		}
		else if (ret != -1)
		{
			_OnRecvHandler onRecv;
			onRecv.swap(m_onRecvHandler);
			m_pRecvBuf = NULL;
			m_iRecvLen = 0;
			onRecv(EC_SUCCESS,ret);
		}
		return;
	}
	else if (wt && m_onSendHandler)
	{
		std::shared_ptr<CTcpSocket> guad(m_register._tcpSocketSendPtr);
		m_register._tcpSocketSendPtr.reset();

		int ret = send(m_register._fd, m_pSendBuf, m_iSendLen, 0);
		m_register._wt = false;
		if (!m_summer->RegisterEvent(1, m_register))
		{
			LCF("CTcpSocket::OnSelectMessage[this0x" << this << "] connect true & EPOLLMod error. " << OSTREAM_GET_LASTERROR << SocketSection());
		}
		
		if ((ret == -1 && !IS_WOULDBLOCK) || m_register._linkstat == LS_CLOSED)
		{
			ec = EC_ERROR;
			m_register._linkstat = LS_CLOSED;
			m_onSendHandler = nullptr;
			if (!m_onSendHandler && !m_onRecvHandler)
			{
				if (!m_summer->RegisterEvent(2, m_register))
				{
					LCW("CTcpSocket::OnSelectMessage[this0x" << this << "] connect true & EPOLL DEL error.  " << OSTREAM_GET_LASTERROR << SocketSection());
				}
			}
			return ;
		}
		else if (ret != -1)
		{
			_OnSendHandler onSend;
			onSend.swap(m_onSendHandler);
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


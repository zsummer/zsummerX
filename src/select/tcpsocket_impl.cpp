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



CTcpSocketImpl::CTcpSocketImpl()
{
	m_register._ptr = this;
	m_register._type = tagRegister::REG_TCP_SOCKET;
}


CTcpSocketImpl::~CTcpSocketImpl()
{
	if (m_onRecvHandler || m_onSendHandler || m_onConnectHandler)
	{
		LCF("CTcpSocketImpl::~CTcpSocketImpl[this0x" << this << "] Handler status error. " << GetSocketStatus());
	}
	if (m_register._fd != InvalideFD)
	{
		closesocket(m_register._fd);
		m_register._fd = InvalideFD;
	}
}

std::string CTcpSocketImpl::GetSocketStatus()
{
	std::stringstream os;
	os << ";; Status: summer.user_count()=" << m_summer.use_count() << ", remoteIP=" << m_remoteIP << ", remotePort=" << m_remotePort
		<< ", m_onConnectHandler = " << (bool)m_onConnectHandler 
		<< ", m_onRecvHandler = " << (bool)m_onRecvHandler << ", m_pRecvBuf=" << (void*)m_pRecvBuf << ", m_iRecvLen=" << m_iRecvLen 
		<< ", m_onSendHandler = " << (bool)m_onSendHandler << ", m_pSendBuf=" << (void*)m_pSendBuf << ", m_iSendLen=" << m_iSendLen
		<< "; m_register=" << m_register;
	return os.str();
}

 bool CTcpSocketImpl::Initialize(CZSummerPtr summer)
{
	if (m_register._linkstat != LS_UNINITIALIZE)
	{
		LCF("CTcpSocketImpl::Initialize[this0x" << this << "] linkstat != LS_UNINITIALIZE" << GetSocketStatus());
		return false;
	}
	m_summer = summer;
	m_register._linkstat = LS_WAITLINK;
	return true;
}
 bool CTcpSocketImpl::AttachEstablishedSocket(SOCKET s, std::string remoteIP, unsigned short remotePort)
{
	if (!m_summer)
	{
		LCF("CTcpSocketImpl::AttachEstablishedSocket[this0x" << this << "] m_summer uninitialize" << GetSocketStatus());
		return false;
	}
	if (m_register._linkstat != LS_WAITLINK)
	{
		LCF("CTcpSocketImpl::AttachEstablishedSocket[this0x" << this << "] socket already used or not initilize." << GetSocketStatus());
		return false;
	}

	m_register._fd = s;
	if (!m_summer->m_impl.RegisterEvent(0, m_register))
	{
		LCE("CTcpSocketImpl::AttachEstablishedSocket[this0x" << this << "] socket already used or not initilize." << GetSocketStatus());
		return false;
	}
	m_remoteIP = remoteIP;
	m_remotePort = remotePort;
	m_register._linkstat = LS_ESTABLISHED;
	return true;
}




bool CTcpSocketImpl::DoConnect(std::string remoteIP, unsigned short remotePort, const _OnConnectHandler & handler)
{
	if (!m_summer)
	{
		LCE("CTcpSocketImpl::DoConnect[this0x" << this << "] summer not bind!" << GetSocketStatus());
		return false;
	}
	if (m_register._linkstat != LS_WAITLINK)
	{
		LCE("CTcpSocketImpl::DoConnect[this0x" << this << "] _linkstat not LS_WAITLINK!" << GetSocketStatus());
		return false;
	}
	if (m_register._fd != -1)
	{
		LCE("CTcpSocketImpl::DoConnect[this0x" << this << "] fd aready used!" << GetSocketStatus());
		return false;
	}
	m_register._fd = socket(AF_INET, SOCK_STREAM, 0);
	if (m_register._fd == -1)
	{
		LCE("CTcpSocketImpl::DoConnect[this0x" << this << "] fd create failed!" << GetSocketStatus());
		return false;
	}
	
	SetNonBlock(m_register._fd);
	SetNoDelay(m_register._fd);
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
		LCE("CTcpSocketImpl::DoConnect[this0x" << this << "] ::connect error. " << OSTREAM_GET_LASTERROR << GetSocketStatus());
		closesocket(m_register._fd);
		m_register._fd = InvalideFD;
		return false;
	}
	
	if (!m_summer->m_impl.RegisterEvent(0, m_register))
	{
		LCE("CTcpSocketImpl::DoConnect[this0x" << this << "] RegisterEvent Error" << GetSocketStatus());
		closesocket(m_register._fd);
		m_register._fd = InvalideFD;
		return false;
	}
	m_onConnectHandler = handler;
	return true;
}


bool CTcpSocketImpl::DoSend(char * buf, unsigned int len, const _OnSendHandler &handler)
{
	if (!m_summer)
	{
		LCE("CTcpSocketImpl::DoSend[this0x" << this << "] m_summer not bind!" << GetSocketStatus());
		return false;
	}
	if (m_register._linkstat != LS_ESTABLISHED)
	{
		LCW("CTcpSocketImpl::DoSend[this0x" << this << "] _linkstat not REG_ESTABLISHED_TCP!" << GetSocketStatus());
		return false;
	}
	if (len == 0)
	{
		LCE("CTcpSocketImpl::DoSend[this0x" << this << "] argument err! len ==0" << GetSocketStatus());
		return false;
	}
	if (m_pSendBuf != NULL || m_iSendLen != 0)
	{
		LCE("CTcpSocketImpl::DoSend[this0x" << this << "] (m_pSendBuf != NULL || m_iSendLen != 0) == TRUE" << GetSocketStatus());
		return false;
	}
	if (m_onSendHandler)
	{
		LCE("CTcpSocketImpl::DoSend[this0x" << this << "] m_onSendHandler == TRUE" << GetSocketStatus());
		return false;
	}

	
	
	m_pSendBuf = buf;
	m_iSendLen = len;
	

	m_register._wt = true;
	if (!m_summer->m_impl.RegisterEvent(1, m_register))
	{
		LCE("CTcpSocketImpl::DoSend[this0x" << this << "] RegisterEvent Error" << GetSocketStatus());
		m_pSendBuf = nullptr;
		m_iSendLen = 0;
		return false;
	}
	m_onSendHandler = handler;
	return true;
}


bool CTcpSocketImpl::DoRecv(char * buf, unsigned int len, const _OnRecvHandler & handler)
{
	if (!m_summer)
	{
		LCE("CTcpSocketImpl::DoRecv[this0x" << this << "] m_summer not bind!" << GetSocketStatus());
		return false;
	}
	if (m_register._linkstat != LS_ESTABLISHED)
	{
		LCW("CTcpSocketImpl::DoRecv[this0x" << this << "] type not REG_ESTABLISHED_TCP!" << GetSocketStatus());
		return false;
	}
	if (len == 0 )
	{
		LCE("CTcpSocketImpl::DoRecv[this0x" << this << "] argument err !!!  len==0" << GetSocketStatus());
		return false;
	}
	if (m_pRecvBuf != NULL || m_iRecvLen != 0)
	{
		LCE("CTcpSocketImpl::DoRecv[this0x" << this << "]  (m_pRecvBuf != NULL || m_iRecvLen != 0) == TRUE" << GetSocketStatus());
		return false;
	}
	if (m_onRecvHandler)
	{
		LCE("CTcpSocketImpl::DoRecv[this0x" << this << "] (m_onRecvHandler) == TRUE" << GetSocketStatus());
		return false;
	}
	
	m_pRecvBuf = buf;
	m_iRecvLen = len;
	

	m_register._rd = true;
	if (!m_summer->m_impl.RegisterEvent(1, m_register))
	{
		LCE("CTcpSocketImpl::DoRecv[this0x" << this << "] RegisterEvent Error" << GetSocketStatus());
		m_pRecvBuf = nullptr;
		m_iRecvLen = 0;
		return false;
	}
	m_onRecvHandler = handler;
	return true;
}


void CTcpSocketImpl::OnSelectMessage(int type, bool rd, bool wt)
{
	unsigned char linkstat = m_register._linkstat;
	ErrorCode ec = EC_ERROR;

	if (!m_onRecvHandler && !m_onSendHandler && linkstat != LS_WAITLINK)
	{
		LCE("CTcpSocketImpl::OnSelectMessage[this0x" << this << "] unknown error. " << OSTREAM_GET_LASTERROR << GetSocketStatus());
		return ;
	}

	if (linkstat == LS_WAITLINK)
	{
		_OnConnectHandler onConnect;
		onConnect.swap(m_onConnectHandler);
		int err = 0;
		socklen_t len = sizeof(int);
		if (wt && getsockopt(m_register._fd, SOL_SOCKET, SO_ERROR, (char*)&err, &len) == 0 && err == 0)
		{
			m_register._wt =  0;
			m_summer->m_impl.RegisterEvent(1, m_register);
			m_register._linkstat = LS_ESTABLISHED;
			onConnect(EC_SUCCESS);
			return;
		}
		else 
		{
			LOGW("OnConnect False. " << OSTREAM_GET_LASTERROR);
			m_register._linkstat = LS_WAITLINK;
			m_summer->m_impl.RegisterEvent(2, m_register);
			onConnect(EC_ERROR);
			return;
		}
		return ;
	}


	if (linkstat != LS_ESTABLISHED)
	{
		LCE("CTcpSocketImpl::OnSelectMessage[this0x" << this << "] unknow type !=REG_ESTABLISHED_TCP or LS_WAITLINK. "
			<< OSTREAM_GET_LASTERROR << GetSocketStatus());
		return ;
	}





	if (rd && m_onRecvHandler)
	{
		int ret = recv(m_register._fd, m_pRecvBuf, m_iRecvLen, 0);
		m_register._rd = false;
		if (!m_summer->m_impl.RegisterEvent(1, m_register))
		{
			LCF("CTcpSocketImpl::OnSelectMessage[this0x" << this << "] connect true & EPOLLMod error.  " << OSTREAM_GET_LASTERROR << GetSocketStatus());
		}
		if (ret == 0)
		{
			ec = EC_REMOTE_CLOSED;
			goto clean;
			return ;
		}
		if (ret ==-1 &&  !IS_WOULDBLOCK )
		{
			ec = EC_ERROR;
			goto clean;
			return ;
		}
		if (ret != -1)
		{
			_OnRecvHandler onRecv;
			onRecv.swap(m_onRecvHandler);
			m_pRecvBuf = NULL;
			m_iRecvLen = 0;
			onRecv(EC_SUCCESS,ret);
		}
	}

	if (wt && m_onSendHandler)
	{
		int ret = send(m_register._fd, m_pSendBuf, m_iSendLen, 0);
		m_register._wt = false;
		if (!m_summer->m_impl.RegisterEvent(1, m_register))
		{
			LCF("CTcpSocketImpl::OnSelectMessage[this0x" << this << "] connect true & EPOLLMod error. " << OSTREAM_GET_LASTERROR << GetSocketStatus());
		}
		
		if (ret == -1 && !IS_WOULDBLOCK)
		{
			ec = EC_ERROR;
			goto clean;
			return ;
		}
		if (ret != -1)
		{
			_OnSendHandler onSend;
			onSend.swap(m_onSendHandler);
			m_pSendBuf = NULL;
			m_iSendLen = 0;
			onSend(EC_SUCCESS, ret);
		}
	}
	return ;

clean:

	if (m_register._linkstat == LS_ESTABLISHED)
	{
		m_register._linkstat = LS_CLOSED;
	}
	if (!m_summer->m_impl.RegisterEvent(2, m_register))
	{
		LCD("CTcpSocketImpl::OnSelectMessage[this0x" << this << "] connect true & EPOLL DEL error.  " << OSTREAM_GET_LASTERROR << GetSocketStatus());
	}

	if (m_onRecvHandler)
	{
		m_onSendHandler = nullptr;
		_OnRecvHandler onRecv;
		onRecv.swap(m_onRecvHandler);
		onRecv(ec,0);
	}
	else if (m_onSendHandler)
	{
		_OnSendHandler onSend;
		onSend.swap(m_onSendHandler);
		onSend(ec,0);
	}
	return ;
}


bool CTcpSocketImpl::DoClose()
{
	if (m_register._fd != InvalideFD)
	{
		shutdown(m_register._fd, SHUT_RDWR);
	}
	return true;
}


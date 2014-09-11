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


#include <zsummerX/iocp/tcpsocket_impl.h>


using namespace zsummer::network;
CTcpSocketImpl::CTcpSocketImpl()
{
	//recv
	memset(&m_recvHandle, 0, sizeof(m_recvHandle));
	m_recvHandle._type = tagReqHandle::HANDLE_RECV;
	m_recvWSABuf.buf = NULL;
	m_recvWSABuf.len = 0;

	//send
	memset(&m_sendHandle, 0, sizeof(m_sendHandle));
	m_sendHandle._type = tagReqHandle::HANDLE_SEND;
	m_sendWsaBuf.buf = NULL;
	m_sendWsaBuf.len = 0;

	//connect
	memset(&m_connectHandle, 0, sizeof(m_connectHandle));
	m_connectHandle._type = tagReqHandle::HANDLE_CONNECT;
}


CTcpSocketImpl::~CTcpSocketImpl()
{
	if (m_onConnectHandler || m_onRecvHandler || m_onSendHandler)
	{
		LCE("CTcpSocketImpl::~CTcpSocketImpl[" << this << "] Destruct CTcpSocketImpl Error. socket handle not invalid and some request was not completed. " << GetTcpSocketImplStatus());
	}	
	if (m_socket != INVALID_SOCKET)
	{

		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
	}
}

std::string CTcpSocketImpl::GetTcpSocketImplStatus()
{
	std::stringstream os;
	os << " CTcpSocketImpl Status: m_summer.use_count()=" << m_summer.use_count() << ", m_socket=" << m_socket
		<< ", m_remoteIP=" << m_remoteIP << ", m_remotePort=" << m_remotePort
		<< ", m_recvHandle=" << m_recvHandle << ", m_recvWSABuf[0x" << (void*)m_recvWSABuf.buf << "," << m_recvWSABuf.len
		<< "], m_onRecvHandler=" << (bool)m_onRecvHandler << ", m_sendHandle=" << m_sendHandle
		<< ", m_sendWsaBuf[0x" << (void*)m_sendWsaBuf.buf << "," << m_sendWsaBuf.len << "], m_onSendHandler=" << (bool)m_onSendHandler
		<< ", m_connectHandle=" << m_connectHandle << ", m_onConnectHandler=" << (bool)m_onConnectHandler
		<< ", m_nLinkStatus=" << m_nLinkStatus << ", Notes: HANDLE_ACCEPT=0, HANDLE_RECV,HANDLE_SEND,HANDLE_CONNECT,HANDLE_RECVFROM,HANDLE_SENDTO=5"
		<< " LS_UNINITIALIZE=0,	LS_WAITLINK,LS_ESTABLISHED,LS_CLOSED";
	return os.str();
}

bool CTcpSocketImpl::Initialize(CZSummerPtr summer)
{
	if (m_nLinkStatus != LS_UNINITIALIZE)
	{
		LCE("CTcpSocketImpl::Initialize[" << this << "] LinkStatus != LS_UNINITIALIZE" << GetTcpSocketImplStatus());
		return false;
	}
	m_summer = summer;
	m_nLinkStatus = LS_WAITLINK;
	return true;
}


bool CTcpSocketImpl::AttachEstablishedSocket(SOCKET s, std::string remoteIP, unsigned short remotePort)
{
	if (!m_summer)
	{
		LCF("CTcpSocketImpl::AttachEstablishedSocket[" << this << "] m_summer pointer uninitialize." << GetTcpSocketImplStatus());
		return false;
	}
	if (m_nLinkStatus != LS_WAITLINK)
	{
		LCF("CTcpSocketImpl::AttachEstablishedSocket[" << this << "] socket already used or not initilize. " << GetTcpSocketImplStatus());
		return false;
	}
	m_socket = s;
	if (CreateIoCompletionPort((HANDLE)m_socket, m_summer->m_impl.m_io, (ULONG_PTR)this, 1) == NULL)
	{
		LCE("CTcpSocketImpl::AttachEstablishedSocket[" << this << "] bind socket to IOCP error.  ERRCODE=" << GetLastError() << GetTcpSocketImplStatus());
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
		return false;
	}
	m_remoteIP = remoteIP;
	m_remotePort = remotePort;
	m_nLinkStatus = LS_ESTABLISHED;
	return true;
}


typedef  BOOL (PASCAL  *ConnectEx)(  SOCKET s,  const struct sockaddr* name,  int namelen,
			 PVOID lpSendBuffer,  DWORD dwSendDataLength,  LPDWORD lpdwBytesSent,
			 LPOVERLAPPED lpOverlapped);

bool CTcpSocketImpl::DoConnect(std::string remoteIP, unsigned short remotePort, const _OnConnectHandler & handler)
{
	if (!m_summer)
	{
		LCF("CTcpSocketImpl::DoConnect[" << this << "] m_summer pointer uninitialize." << GetTcpSocketImplStatus());
		return false;
	}
	if (m_nLinkStatus != LS_WAITLINK)
	{
		LCW("CTcpSocketImpl::DoConnect[" << this << "] socket already used or not initilize. " << GetTcpSocketImplStatus());
		return false;
	}
	if (m_onConnectHandler)
	{
		LCF("CTcpSocketImpl::DoConnect[" << this << "] socket is connecting." << GetTcpSocketImplStatus());
		return false;
	}
	
	m_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (m_socket == INVALID_SOCKET)
	{
		LCE("CTcpSocketImpl::DoConnect[" << this << "] socket create error! ERRCODE=" << WSAGetLastError() << GetTcpSocketImplStatus());
		return false;
	}

	SOCKADDR_IN localAddr;
	memset(&localAddr, 0, sizeof(SOCKADDR_IN));
	localAddr.sin_family = AF_INET;
	if (bind(m_socket, (sockaddr *) &localAddr, sizeof(SOCKADDR_IN)) != 0)
	{
		LCE("CTcpSocketImpl::DoConnect[" << this << "] bind local addr error! ERRCODE=" << WSAGetLastError() << GetTcpSocketImplStatus());
		return false;
	}

	if (CreateIoCompletionPort((HANDLE)m_socket, m_summer->m_impl.m_io, (ULONG_PTR)this, 1) == NULL)
	{
		LCE("CTcpSocketImpl::DoConnect[" << this << "] bind socket to IOCP error. ERRCODE=" << GetLastError() << GetTcpSocketImplStatus());
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
		return false;
	}


	GUID gid = WSAID_CONNECTEX;
	ConnectEx lpConnectEx = NULL;
	DWORD dwSize = 0;
	if (WSAIoctl(m_socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &gid, sizeof(gid), &lpConnectEx, sizeof(lpConnectEx), &dwSize, NULL, NULL) != 0)
	{
		LCE("CTcpSocketImpl::DoConnect[" << this << "] Get ConnectEx pointer err!  ERRCODE= " << WSAGetLastError() << GetTcpSocketImplStatus());
		return false;
	}
	m_remoteIP = remoteIP;
	m_remotePort = remotePort;
	SOCKADDR_IN remoteAddr;
	remoteAddr.sin_addr.s_addr = inet_addr(m_remoteIP.c_str());
	remoteAddr.sin_family = AF_INET;
	remoteAddr.sin_port = htons(m_remotePort);
	

	char buf[1];
	DWORD dwLenth = 0;
	if (!lpConnectEx(m_socket, (sockaddr *)&remoteAddr, sizeof(remoteAddr), buf, 0, &dwLenth, &m_connectHandle._overlapped))
	{
		if (WSAGetLastError() != ERROR_IO_PENDING)
		{
			LCE("CTcpSocketImpl::DoConnect[" << this << "] DoConnect failed and ERRCODE!=ERROR_IO_PENDING, ERRCODE=" << WSAGetLastError() << GetTcpSocketImplStatus());
			return false;
		}

	}
	m_onConnectHandler = handler;
	return true;
}




bool CTcpSocketImpl::DoSend(char * buf, unsigned int len, const _OnSendHandler &handler)
{
	if (!m_summer)
	{
		LCF("CTcpSocketImpl::DoSend[" << this << "] m_summer pointer uninitialize." << GetTcpSocketImplStatus());
		return false;
	}
	if (m_nLinkStatus != LS_ESTABLISHED)
	{
		LCW("CTcpSocketImpl::DoSend[" << this << "] socket status != LS_ESTABLISHED." << GetTcpSocketImplStatus());
		return false;
	}
	if (m_onSendHandler)
	{
		LCF("CTcpSocketImpl::DoSend[" << this << "] socket is sending." << GetTcpSocketImplStatus());
		return false;
	}
	if (len == 0)
	{
		LCF("CTcpSocketImpl::DoSend[" << this << "] length is 0." << GetTcpSocketImplStatus());
		return false;
	}

	m_sendWsaBuf.buf = buf;
	m_sendWsaBuf.len = len;
	DWORD dwTemp1=0;
	if (WSASend(m_socket, &m_sendWsaBuf, 1, &dwTemp1, 0, &m_sendHandle._overlapped, NULL) != 0)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			LCW("CTcpSocketImpl::DoSend[" << this << "] DoSend failed and ERRCODE!=ERROR_IO_PENDING ERRCODE=" << WSAGetLastError() << GetTcpSocketImplStatus());
			m_sendWsaBuf.buf = nullptr;
			m_sendWsaBuf.len = 0;
			return false;
		}
	}
	m_onSendHandler = handler;
	return true;
}


bool CTcpSocketImpl::DoRecv(char * buf, unsigned int len, const _OnRecvHandler & handler)
{
	if (!m_summer)
	{
		LCF("CTcpSocketImpl::DoRecv[" << this << "] m_summer pointer uninitialize." << GetTcpSocketImplStatus());
		return false;
	}
	if (m_nLinkStatus != LS_ESTABLISHED)
	{
		LCW("CTcpSocketImpl::DoRecv[" << this << "] socket status != LS_ESTABLISHED. " << GetTcpSocketImplStatus());
		return false;
	}
	if (m_onRecvHandler)
	{
		LCF("CTcpSocketImpl::DoRecv[" << this << "] socket is recving. " << GetTcpSocketImplStatus());
		return false;
	}
	if (len == 0)
	{
		LCF("CTcpSocketImpl::DoRecv[" << this << "] length is 0." << GetTcpSocketImplStatus());
		return false;
	}


	m_recvWSABuf.buf = buf;
	m_recvWSABuf.len = len;
	
	DWORD dwRecv = 0;
	DWORD dwFlag = 0;
	if (WSARecv(m_socket, &m_recvWSABuf, 1, &dwRecv, &dwFlag, &m_recvHandle._overlapped, NULL) != 0)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			LCW("CTcpSocketImpl::DoRecv[" << this << "] DoRecv failed and ERRCODE!=ERROR_IO_PENDING, ERRCODE=" << WSAGetLastError() << GetTcpSocketImplStatus());
			m_recvWSABuf.buf = nullptr;
			m_recvWSABuf.len = 0;
			return false;
		}
	}
	m_onRecvHandler = handler;
	return true;
}

void CTcpSocketImpl::OnIOCPMessage(BOOL bSuccess, DWORD dwTranceCount, unsigned char cType)
{
	if (cType == tagReqHandle::HANDLE_CONNECT)
	{
		_OnConnectHandler onConnect;
		onConnect.swap(m_onConnectHandler);
		if (bSuccess)
		{
			m_nLinkStatus = LS_ESTABLISHED;
			{
				BOOL bTrue = TRUE;
				if (setsockopt(m_socket,IPPROTO_TCP, TCP_NODELAY, (char*)&bTrue, sizeof(bTrue)) != 0)
				{
					LCW("CTcpSocketImpl::OnIOCPMessage[" << this << "] setsockopt TCP_NODELAY fail!  last err=" << WSAGetLastError() << GetTcpSocketImplStatus());
				}
			}
			onConnect(ErrorCode::EC_SUCCESS);
		}
		else
		{
			closesocket(m_socket);
			m_socket = INVALID_SOCKET;
			onConnect(ErrorCode::EC_ERROR);
		}
		return ;
	}



	if (cType == tagReqHandle::HANDLE_RECV)
	{
		//server side active close.
		_OnRecvHandler onRecv;
		onRecv.swap(m_onRecvHandler);
		if (!bSuccess)
		{
			if (onRecv)
			{
				if (m_onSendHandler)
				{
					m_onSendHandler = nullptr;
				}
				onRecv(ErrorCode::EC_REMOTE_HANGUP, dwTranceCount);
				return;
			}
			return;
		}
		// client side active closed
		if (dwTranceCount == 0)
		{
			if (onRecv)
			{
				if (m_onSendHandler)
				{
					m_onSendHandler = nullptr;
				}
				onRecv(ErrorCode::EC_REMOTE_CLOSED, dwTranceCount);
				return;
			}
			return;
		}
		if (onRecv)
		{
			onRecv(ErrorCode::EC_SUCCESS, dwTranceCount);
			return;
		}
		return;
	}
	else if (cType == tagReqHandle::HANDLE_SEND)
	{
		//server side active close.
		_OnSendHandler onSend;
		onSend.swap(m_onSendHandler);
		if (!bSuccess)
		{
			if (onSend)
			{
				if (m_onRecvHandler)
				{
					m_onRecvHandler = nullptr;
				}
				onSend(ErrorCode::EC_REMOTE_HANGUP, dwTranceCount);
			}
			return;
		}
		if (onSend)
		{
			onSend(ErrorCode::EC_SUCCESS, dwTranceCount);
		}
		return ;
	}
}


bool CTcpSocketImpl::DoClose()
{
	if (m_nLinkStatus == LS_ESTABLISHED)
	{
		m_nLinkStatus = LS_CLOSED;
		closesocket(m_socket);
		return true;
	}
	else if (m_nLinkStatus == LS_UNINITIALIZE)
	{
		LCW("CTcpSocketImpl::DoClose[" << this << "] socket status = LS_UNINITIALIZE " << GetTcpSocketImplStatus());
	}
	else if (m_nLinkStatus == LS_CLOSED)
	{
		LCW("CTcpSocketImpl::DoClose[" << this << "] socket status = LS_CLOSED" << GetTcpSocketImplStatus());
	}
	return true;
}

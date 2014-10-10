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
CTcpSocket::CTcpSocket()
{
	g_appEnvironment.AddCreatedSocketCount();
	//recv
	memset(&m_recvHandle._overlapped, 0, sizeof(m_recvHandle._overlapped));
	m_recvHandle._type = tagReqHandle::HANDLE_RECV;
	m_recvWSABuf.buf = NULL;
	m_recvWSABuf.len = 0;

	//send
	memset(&m_sendHandle._overlapped, 0, sizeof(m_sendHandle._overlapped));
	m_sendHandle._type = tagReqHandle::HANDLE_SEND;
	m_sendWsaBuf.buf = NULL;
	m_sendWsaBuf.len = 0;

	//connect
	memset(&m_connectHandle._overlapped, 0, sizeof(m_connectHandle._overlapped));
	m_connectHandle._type = tagReqHandle::HANDLE_CONNECT;
}


CTcpSocket::~CTcpSocket()
{
	g_appEnvironment.AddClosedSocketCount();
	if (m_onConnectHandler || m_onRecvHandler || m_onSendHandler)
	{
		LCT("CTcpSocket::~CTcpSocket[" << this << "] Destruct CTcpSocket Error. socket handle not invalid and some request was not completed. " << GetTcpSocketImplStatus());
	}	
	if (m_socket != INVALID_SOCKET)
	{
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
	}
}

std::string CTcpSocket::GetTcpSocketImplStatus()
{
	std::stringstream os;
	os << " CTcpSocket Status: m_summer.use_count()=" << m_summer.use_count() << ", m_socket=" << m_socket
		<< ", m_remoteIP=" << m_remoteIP << ", m_remotePort=" << m_remotePort
		<< ", m_recvHandle=" << m_recvHandle << ", m_recvWSABuf[0x" << (void*)m_recvWSABuf.buf << "," << m_recvWSABuf.len
		<< "], m_onRecvHandler=" << (bool)m_onRecvHandler << ", m_sendHandle=" << m_sendHandle
		<< ", m_sendWsaBuf[0x" << (void*)m_sendWsaBuf.buf << "," << m_sendWsaBuf.len << "], m_onSendHandler=" << (bool)m_onSendHandler
		<< ", m_connectHandle=" << m_connectHandle << ", m_onConnectHandler=" << (bool)m_onConnectHandler
		<< ", m_nLinkStatus=" << m_nLinkStatus << ", Notes: HANDLE_ACCEPT=0, HANDLE_RECV,HANDLE_SEND,HANDLE_CONNECT,HANDLE_RECVFROM,HANDLE_SENDTO=5"
		<< " LS_UNINITIALIZE=0,	LS_WAITLINK,LS_ESTABLISHED,LS_CLOSED"; 
	return os.str();
}

//new socket to connect, or accept established socket
bool CTcpSocket::Initialize(ZSummerPtr summer)
{
	m_summer = summer;
	if (m_nLinkStatus != LS_UNINITIALIZE)
	{
		m_nLinkStatus = LS_ESTABLISHED;
	}
	else
	{
		m_nLinkStatus = LS_WAITLINK;
		m_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (m_socket == INVALID_SOCKET)
		{
			LCE("CTcpSocket::DoConnect[" << this << "] socket create error! ERRCODE=" << WSAGetLastError() << GetTcpSocketImplStatus());
			return false;
		}

		SOCKADDR_IN localAddr;
		memset(&localAddr, 0, sizeof(SOCKADDR_IN));
		localAddr.sin_family = AF_INET;
		if (bind(m_socket, (sockaddr *)&localAddr, sizeof(SOCKADDR_IN)) != 0)
		{
			LCE("CTcpSocket::DoConnect[" << this << "] bind local addr error! ERRCODE=" << WSAGetLastError() << GetTcpSocketImplStatus());
			closesocket(m_socket);
			m_socket = INVALID_SOCKET;
			return false;
		}
	}

	if (CreateIoCompletionPort((HANDLE)m_socket, m_summer->m_io, (ULONG_PTR)this, 1) == NULL)
	{
		LCE("CTcpSocket::Initialize[" << this << "] bind socket to IOCP error.  ERRCODE=" << GetLastError() << GetTcpSocketImplStatus());
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
		return false;
	}
	return true;
}


bool CTcpSocket::AttachSocket(SOCKET s, std::string remoteIP, unsigned short remotePort)
{

	m_socket = s;
	m_remoteIP = remoteIP;
	m_remotePort = remotePort;
	m_nLinkStatus = LS_WAITLINK;
	return true;
}


typedef  BOOL (PASCAL  *ConnectEx)(  SOCKET s,  const struct sockaddr* name,  int namelen,
			 PVOID lpSendBuffer,  DWORD dwSendDataLength,  LPDWORD lpdwBytesSent,
			 LPOVERLAPPED lpOverlapped);

bool CTcpSocket::DoConnect(std::string remoteIP, unsigned short remotePort, const _OnConnectHandler & handler)
{
	if (!m_summer)
	{
		LCF("CTcpSocket::DoConnect[" << this << "] m_summer pointer uninitialize." << GetTcpSocketImplStatus());
		return false;
	}
	if (m_nLinkStatus != LS_WAITLINK)
	{
		LCW("CTcpSocket::DoConnect[" << this << "] socket already used or not initilize. " << GetTcpSocketImplStatus());
		return false;
	}
	if (m_onConnectHandler)
	{
		LCF("CTcpSocket::DoConnect[" << this << "] socket is connecting." << GetTcpSocketImplStatus());
		return false;
	}
	

	GUID gid = WSAID_CONNECTEX;
	ConnectEx lpConnectEx = NULL;
	DWORD dwSize = 0;
	if (WSAIoctl(m_socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &gid, sizeof(gid), &lpConnectEx, sizeof(lpConnectEx), &dwSize, NULL, NULL) != 0)
	{
		LCT("CTcpSocket::DoConnect[" << this << "] Get ConnectEx pointer err!  ERRCODE= " << WSAGetLastError() << GetTcpSocketImplStatus());
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
			LCT("CTcpSocket::DoConnect[" << this << "] DoConnect failed and ERRCODE!=ERROR_IO_PENDING, ERRCODE=" << WSAGetLastError() << GetTcpSocketImplStatus());
			return false;
		}

	}
	m_onConnectHandler = handler;
	m_connectHandle._tcpSocket = shared_from_this();
	return true;
}




bool CTcpSocket::DoSend(char * buf, unsigned int len, const _OnSendHandler &handler)
{
	if (m_nLinkStatus != LS_ESTABLISHED)
	{
		LCT("CTcpSocket::DoSend[" << this << "] socket status != LS_ESTABLISHED." << GetTcpSocketImplStatus());
		return false;
	}
	if (!m_summer)
	{
		LCF("CTcpSocket::DoSend[" << this << "] m_summer pointer uninitialize." << GetTcpSocketImplStatus());
		return false;
	}
	if (m_onSendHandler)
	{
		LCF("CTcpSocket::DoSend[" << this << "] socket is sending." << GetTcpSocketImplStatus());
		return false;
	}
	if (len == 0)
	{
		LCF("CTcpSocket::DoSend[" << this << "] length is 0." << GetTcpSocketImplStatus());
		return false;
	}

	m_sendWsaBuf.buf = buf;
	m_sendWsaBuf.len = len;
	DWORD dwTemp1=0;
	if (WSASend(m_socket, &m_sendWsaBuf, 1, &dwTemp1, 0, &m_sendHandle._overlapped, NULL) != 0)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			LCT("CTcpSocket::DoSend[" << this << "] DoSend failed and ERRCODE!=ERROR_IO_PENDING ERRCODE=" << WSAGetLastError() << GetTcpSocketImplStatus());
			m_sendWsaBuf.buf = nullptr;
			m_sendWsaBuf.len = 0;
			DoClose();
			return false;
		}
	}
	m_onSendHandler = handler;
	m_sendHandle._tcpSocket = shared_from_this();
	return true;
}


bool CTcpSocket::DoRecv(char * buf, unsigned int len, const _OnRecvHandler & handler)
{
	if (m_nLinkStatus != LS_ESTABLISHED)
	{
		LCT("CTcpSocket::DoRecv[" << this << "] socket status != LS_ESTABLISHED. " << GetTcpSocketImplStatus());
		return false;
	}
	if (!m_summer)
	{
		LCF("CTcpSocket::DoRecv[" << this << "] m_summer pointer uninitialize." << GetTcpSocketImplStatus());
		return false;
	}
	if (m_onRecvHandler)
	{
		LCF("CTcpSocket::DoRecv[" << this << "] socket is recving. " << GetTcpSocketImplStatus());
		return false;
	}
	if (len == 0)
	{
		LCF("CTcpSocket::DoRecv[" << this << "] length is 0." << GetTcpSocketImplStatus());
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
			LCT("CTcpSocket::DoRecv[" << this << "] DoRecv failed and ERRCODE!=ERROR_IO_PENDING, ERRCODE=" << WSAGetLastError() << GetTcpSocketImplStatus());
			m_recvWSABuf.buf = nullptr;
			m_recvWSABuf.len = 0;
			DoClose();
			return false;
		}
	}
	m_onRecvHandler = handler;
	m_recvHandle._tcpSocket = shared_from_this();
	return true;
}

void CTcpSocket::OnIOCPMessage(BOOL bSuccess, DWORD dwTranceCount, unsigned char cType)
{
	if (cType == tagReqHandle::HANDLE_CONNECT)
	{
		_OnConnectHandler onConnect;
		onConnect.swap(m_onConnectHandler);
		std::shared_ptr<CTcpSocket> guad(m_connectHandle._tcpSocket);
		m_connectHandle._tcpSocket.reset();
		if (!onConnect)
		{
			return;
		}
		if (bSuccess)
		{
			m_nLinkStatus = LS_ESTABLISHED;
			{
				BOOL bTrue = TRUE;
				if (setsockopt(m_socket,IPPROTO_TCP, TCP_NODELAY, (char*)&bTrue, sizeof(bTrue)) != 0)
				{
					LCW("CTcpSocket::OnIOCPMessage[" << this << "] setsockopt TCP_NODELAY fail!  last err=" << WSAGetLastError() << GetTcpSocketImplStatus());
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


	if (cType == tagReqHandle::HANDLE_SEND)
	{
		_OnSendHandler onSend;
		onSend.swap(m_onSendHandler);
		std::shared_ptr<CTcpSocket> guad(m_sendHandle._tcpSocket);
		m_sendHandle._tcpSocket.reset();
		if (!onSend)
		{
			return;
		}
		if (!bSuccess || m_nLinkStatus != LS_ESTABLISHED)
		{
			return;
		}
		onSend(ErrorCode::EC_SUCCESS, dwTranceCount);
		return;
	}
	else if (cType == tagReqHandle::HANDLE_RECV)
	{
		std::shared_ptr<CTcpSocket> guad(m_recvHandle._tcpSocket);
		m_recvHandle._tcpSocket.reset();
		_OnRecvHandler onRecv;
		onRecv.swap(m_onRecvHandler);
		if (!onRecv)
		{
			return;
		}
		
		if (!bSuccess)
		{
			m_nLinkStatus = LS_CLOSED;
			onRecv(ErrorCode::EC_REMOTE_HANGUP, dwTranceCount);
			return;
		}
		else if (dwTranceCount == 0)
		{
			m_nLinkStatus = LS_CLOSED;
			onRecv(ErrorCode::EC_REMOTE_CLOSED, dwTranceCount);
			return;
		}
		onRecv(ErrorCode::EC_SUCCESS, dwTranceCount);
		return;
	}

}


bool CTcpSocket::DoClose()
{
	if (m_nLinkStatus == LS_ESTABLISHED || m_nLinkStatus == LS_WAITLINK)
	{
		m_nLinkStatus = LS_CLOSED;
		closesocket(m_socket);
		return true;
	}
	return true;
}

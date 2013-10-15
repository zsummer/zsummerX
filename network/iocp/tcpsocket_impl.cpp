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

#include "public.h"
#include "tcpsocket_impl.h"

using namespace zsummer::network;
CTcpSocketImpl::CTcpSocketImpl(SOCKET s, std::string remoteIP, unsigned short remotePort)
{
	m_summer = NULL;
	m_socket = s;
	m_remoteIP = remoteIP;
	m_remotePort = remotePort;
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

	//status
	m_isRecving = false;
	m_isSending = false;
	m_isConnecting = false;
	m_nLinkStatus = LS_UNINITIALIZE;
}


CTcpSocketImpl::~CTcpSocketImpl()
{
	if (m_socket != INVALID_SOCKET)
	{
		LCF("Destruct CTcpSocket Error. socket handle not invalid, socket=" << (unsigned int)m_socket);
		m_socket = INVALID_SOCKET;
	}
}

bool CTcpSocketImpl::Initialize(CZSummer & summer)
{
	m_summer = &summer;
	//! connect set
	if (m_socket == INVALID_SOCKET)
	{
		m_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (m_socket == INVALID_SOCKET)
		{
			LCE("connecter create socket  error! ERRCODE=" << WSAGetLastError());
			return false;
		}
		SOCKADDR_IN addr;
		memset(&addr, 0, sizeof(SOCKADDR_IN));
		addr.sin_family = AF_INET;
		if (bind(m_socket, (sockaddr *) &addr, sizeof(SOCKADDR_IN)) != 0)
		{
			LCE("connecter bind local addr error!  socket=" << (unsigned int)m_socket << ", ERRCODE=" << WSAGetLastError());
			return false;
		}
	}
	// accepted socket
	else
	{
		m_nLinkStatus = LS_ESTABLISHED;
	}

	if (CreateIoCompletionPort((HANDLE)m_socket, m_summer->m_impl.m_io, (ULONG_PTR)this, 1) == NULL)
	{
		LCE("bind socket to IOCP error. socket="<< (unsigned int) m_socket << ", ERRCODE=" << GetLastError());
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
		return false;
	}
	return true;
}





typedef  BOOL (PASCAL  *ConnectEx)(  SOCKET s,  const struct sockaddr* name,  int namelen,
			 PVOID lpSendBuffer,  DWORD dwSendDataLength,  LPDWORD lpdwBytesSent,
			 LPOVERLAPPED lpOverlapped);

bool CTcpSocketImpl::DoConnect(const _OnConnectHandler & handler)
{
	if (m_summer == NULL)
	{
		LCF("CTcpSocket::DoConnect m_summer pointer uninitialize.socket=" << (unsigned int) m_socket);
		return false;
	}
	if (m_nLinkStatus != LS_UNINITIALIZE)
	{
		LCF("CTcpSocket::DoConnect socket already used. socket="<<(unsigned int) m_socket);
		return false;
	}
	if (m_isConnecting)
	{
		LCF("CTcpSocket::DoConnect socket is connecting. socket="<<(unsigned int) m_socket);
		return false;
	}

	GUID gid = WSAID_CONNECTEX;
	ConnectEx lpConnectEx = NULL;
	DWORD dwSize = 0;
	if (WSAIoctl(m_socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &gid, sizeof(gid), &lpConnectEx, sizeof(lpConnectEx), &dwSize, NULL, NULL) != 0)
	{
		LCE("Get ConnectEx pointer err!  socket="<< (unsigned int) m_socket <<", ERRCODE= "<< WSAGetLastError());
		return false;
	}
	SOCKADDR_IN addr;
	addr.sin_addr.s_addr = inet_addr(m_remoteIP.c_str());
	addr.sin_family = AF_INET;
	addr.sin_port = htons(m_remotePort);
	m_onConnectHandler = handler;

	char buf[1];
	DWORD dwLenth = 0;
	if (!lpConnectEx(m_socket, (sockaddr *)&addr, sizeof(addr), buf, 0, &dwLenth, &m_connectHandle._overlapped))
	{
		if (WSAGetLastError() != ERROR_IO_PENDING)
		{
			LCE("CTcpSocket::DoConnect DoConnect failed and ERRCODE!=ERROR_IO_PENDING, socket="<< (unsigned int) m_socket << ", ERRCODE=" << WSAGetLastError());
			return false;
		}

	}
	m_isConnecting = true;
	return true;
}




bool CTcpSocketImpl::DoSend(char * buf, unsigned int len, const _OnSendHandler &handler)
{
	if (m_summer == NULL)
	{
		LCF("CTcpSocket::DoSend m_summer pointer uninitialize.socket=" << (unsigned int) m_socket);
		return false;
	}
	if (m_nLinkStatus != LS_ESTABLISHED)
	{
		LCF("CTcpSocket::DoSend socket status != LS_ESTABLISHED. socket="<<(unsigned int) m_socket);
		return false;
	}
	if (m_isSending)
	{
		LCF("CTcpSocket::DoSend socket is sending. socket="<<(unsigned int) m_socket);
		return false;
	}
	if (len == 0)
	{
		LCF("CTcpSocket::DoSend length is 0. socket="<<(unsigned int) m_socket);
		return false;
	}

	m_sendWsaBuf.buf = buf;
	m_sendWsaBuf.len = len;
	m_onSendHandler = handler;
	DWORD dwTemp1=0;
	if (WSASend(m_socket, &m_sendWsaBuf, 1, &dwTemp1, 0, &m_sendHandle._overlapped, NULL) != 0)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			LCE("CTcpSocket::DoSend DoSend failed and ERRCODE!=ERROR_IO_PENDING, socket="<< (unsigned int) m_socket << ", ERRCODE=" << WSAGetLastError());
			return false;
		}
	}
	m_isSending = true;
	return true;
}


bool CTcpSocketImpl::DoRecv(char * buf, unsigned int len, const _OnRecvHandler & handler)
{
	if (m_summer == NULL)
	{
		LCF("CTcpSocket::DoRecv m_summer pointer uninitialize.socket=" << (unsigned int) m_socket);
		return false;
	}
	if (m_nLinkStatus != LS_ESTABLISHED)
	{
		LCF("CTcpSocket::DoRecv socket status != LS_ESTABLISHED. socket="<<(unsigned int) m_socket);
		return false;
	}
	if (m_isRecving)
	{
		LCF("CTcpSocket::DoRecv socket is recving. socket="<<(unsigned int) m_socket);
		return false;
	}
	if (len == 0)
	{
		LCF("CTcpSocket::DoRecv length is 0. socket="<<(unsigned int) m_socket);
		return false;
	}


	m_recvWSABuf.buf = buf;
	m_recvWSABuf.len = len;
	m_onRecvHandler = handler;
	DWORD dwRecv = 0;
	DWORD dwFlag = 0;
	if (WSARecv(m_socket, &m_recvWSABuf, 1, &dwRecv, &dwFlag, &m_recvHandle._overlapped, NULL) != 0)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			LCE("CTcpSocket::DoRecv DoRecv failed and ERRCODE!=ERROR_IO_PENDING, socket="<< (unsigned int) m_socket << ", ERRCODE=" << WSAGetLastError());
			return false;
		}
	}
	m_isRecving = true;
	return true;
}

bool CTcpSocketImpl::OnIOCPMessage(BOOL bSuccess, DWORD dwTranceCount, unsigned char cType)
{
	if (cType == tagReqHandle::HANDLE_CONNECT)
	{
		m_isConnecting = false;
		if (bSuccess)
		{
			m_nLinkStatus = LS_ESTABLISHED;
			{
				BOOL bTrue = TRUE;
				if (setsockopt(m_socket,IPPROTO_TCP, TCP_NODELAY, (char*)&bTrue, sizeof(bTrue)) != 0)
				{
					LCW("setsockopt TCP_NODELAY fail!  last err=" << WSAGetLastError() );
				}
			}
			m_onConnectHandler(ErrorCode::EC_SUCCESS);
		}
		else
		{
			closesocket(m_socket);
			m_socket = INVALID_SOCKET;
			m_onConnectHandler(ErrorCode::EC_ERROR);
		}
		return true;
	}



	if (cType == tagReqHandle::HANDLE_RECV)
	{
		//server side active close.
		m_isRecving = false;
		if (!bSuccess)
		{
			m_onRecvHandler(ErrorCode::EC_ERROR, dwTranceCount);
			return true;
		}
		// client side active closed
		if (dwTranceCount == 0)
		{
			m_onRecvHandler(ErrorCode::EC_ERROR, dwTranceCount);
			return true;
		}
		m_onRecvHandler(ErrorCode::EC_SUCCESS, dwTranceCount);
		return true;
	}
	if (cType == tagReqHandle::HANDLE_SEND)
	{
		//server side active close.
		m_isSending = false;
		if (!bSuccess)
		{
			m_onSendHandler(ErrorCode::EC_ERROR,dwTranceCount);
			return true;
		}
		m_onSendHandler(ErrorCode::EC_SUCCESS, dwTranceCount);
		return true;
	}
	return true;
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
		LCW("CTcpSocket::Close(),  socket status = LS_UNINITIALIZE, socket=" << (unsigned int) m_socket);
	}
	else if (m_nLinkStatus == LS_CLOSED)
	{
		LCW("CTcpSocket::Close(),  socket status = LS_CLOSED, socket=" << (unsigned int) m_socket);
	}
	return true;
}

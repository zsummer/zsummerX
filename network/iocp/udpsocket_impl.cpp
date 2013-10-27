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
	m_socket=INVALID_SOCKET;
	memset(&m_addr, 0, sizeof(m_addr));

	//recv
	memset(&m_recvHandle, 0, sizeof(m_recvHandle));
	m_recvHandle._type = tagReqHandle::HANDLE_RECVFROM;
	m_recvWSABuf.buf = NULL;
	m_recvWSABuf.len = 0;
	m_recvLock = false;

	memset(&m_sendHandle, 0, sizeof(m_sendHandle));
	m_sendHandle._type = tagReqHandle::HANDLE_SENDTO;
	m_sendWSABuf.buf = NULL;
	m_sendWSABuf.len = 0;
	m_sendLock = false;


	m_nLinkStatus = LS_UNINITIALIZE;

}


CUdpSocketImpl::~CUdpSocketImpl()
{
	if (m_socket != INVALID_SOCKET)
	{
		LCF("Destruct CUdpSocket Error. socket handle not invalid, socket=" << (unsigned int)m_socket);
		m_socket = INVALID_SOCKET;
	}
}

bool CUdpSocketImpl::Initialize(CZSummer & summer, const char *localIP, unsigned short localPort)
{
	if (m_socket != INVALID_SOCKET)
	{
		LCF("CUdpSocket: socket is aread used , socket=" << (unsigned int)m_socket);
		return false;
	}	
	if (m_summer != NULL)
	{
		LCF("CUdpSocket: socket is aread used, m_summer =" << m_summer << ", socket=" << (unsigned int)m_socket);
		return false;
	}
	m_summer = &summer;
	m_socket = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (m_socket == INVALID_SOCKET)
	{
		LCE("CUdpSocket: create socket  error! ERRCODE=" << WSAGetLastError());
		return false;
	}

	sockaddr_in addr;
	memset(&addr, 0, sizeof(SOCKADDR_IN));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(localIP);
	addr.sin_port = ntohs(localPort);
	if (bind(m_socket, (sockaddr *) &addr, sizeof(sockaddr_in)) != 0)
	{
		LCE("CUdpSocket: bind local addr error!  socket=" << (unsigned int)m_socket << ", ERRCODE=" << WSAGetLastError());
		return false;
	}
	
	if (CreateIoCompletionPort((HANDLE)m_socket, m_summer->m_impl.m_io, (ULONG_PTR)this, 1) == NULL)
	{
		LCE("CUdpSocket::bind socket to IOCP error. socket="<< (unsigned int) m_socket << ", ERRCODE=" << GetLastError());
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
		return false;
	}
	m_nLinkStatus = LS_ESTABLISHED;
	return true;
}





bool CUdpSocketImpl::DoSend(char * buf, unsigned int len, const char *dstip, unsigned short dstport,const _OnSendToHandler& handler )
{
	if (m_summer == NULL)
	{
		LCF("CUdpSocketImpl::DoSend IIOServer pointer uninitialize.socket=" << (unsigned int) m_socket);
		return false;
	}
	if (m_nLinkStatus != LS_ESTABLISHED)
	{
		LCF("CUdpSocketImpl::DoSendto socket status != LS_ESTABLISHED. socket="<<(unsigned int) m_socket);
		return false;
	}

	if (len == 0 || len >1200)
	{
		LCF("CUdpSocketImpl::DoSend length is error. socket="<<(unsigned int) m_socket);
		return false;
	}

	if (m_sendLock)
	{
		LCF("CUdpSocketImpl::DoSend  is locking. socket="<<(unsigned int) m_socket);
		return false;
	}
	

	m_sendWSABuf.buf = buf;
	m_sendWSABuf.len = len;
	m_onSendHandler = handler;

	DWORD dwTemp1=0;
	sockaddr_in dst;
	dst.sin_addr.s_addr = inet_addr(dstip);
	dst.sin_port = htons(dstport);
	dst.sin_family = AF_INET;

 	if (WSASendTo(m_socket, &m_sendWSABuf, 1, &dwTemp1, 0,(sockaddr*)&dst, sizeof(dst),  &m_sendHandle._overlapped, NULL) != 0)
 	{
 		if (WSAGetLastError() != WSA_IO_PENDING)
 		{
 			LCE("CUdpSocket::DoSend DoSend failed and ERRCODE!=ERROR_IO_PENDING, socket="<< (unsigned int) m_socket << ", ERRCODE=" << WSAGetLastError());
			m_sendWSABuf.buf = nullptr;
			m_sendWSABuf.len = 0;
			m_onSendHandler = nullptr;
 			return false;
 		}
 	}
	m_sendLock = true;
	return true;
}


bool CUdpSocketImpl::DoRecv(char * buf, unsigned int len, const _OnRecvFromHandler& handler)
{
	if (m_summer == NULL)
	{
		LCF("CUdpSocketImpl::DoRecv IIOServer pointer uninitialize.socket=" << (unsigned int) m_socket);
		return false;
	}
	if (m_nLinkStatus != LS_ESTABLISHED)
	{
		LCF("CUdpSocketImpl::DoRecv socket status != LS_ESTABLISHED. socket="<<(unsigned int) m_socket);
		return false;
	}

	if (len == 0)
	{
		LCF("CUdpSocketImpl::DoRecv length is 0. socket="<<(unsigned int) m_socket);
		return false;
	}

	if (m_recvLock)
	{
		LCF("CUdpSocketImpl::DoRecv  is locking. socket="<<(unsigned int) m_socket);
		return false;
	}

	m_recvWSABuf.buf = buf;
	m_recvWSABuf.len = len;
	m_onRecvHander = handler;

	sizeof(&m_recvFrom, 0, sizeof(m_recvFrom));
	m_recvFromLen = sizeof(m_recvFrom);
	DWORD dwRecv = 0;
	DWORD dwFlag = 0;
	if (WSARecvFrom(m_socket, &m_recvWSABuf, 1, &dwRecv, &dwFlag,(sockaddr*)&m_recvFrom, &m_recvFromLen, &m_recvHandle._overlapped, NULL) != 0)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			LCE("CUdpSocket::DoRecv DoRecv failed and ERRCODE!=ERROR_IO_PENDING, socket="<< (unsigned int) m_socket << ", ERRCODE=" << WSAGetLastError());
			m_recvWSABuf.buf = nullptr;
			m_recvWSABuf.len = 0;
			m_onRecvHander = nullptr;
			return false;
		}
	}
	m_recvLock = true;
	return true;
}

bool CUdpSocketImpl::OnIOCPMessage(BOOL bSuccess, DWORD dwTranceCount, unsigned char cType)
{
	if (cType == tagReqHandle::HANDLE_RECVFROM)
	{
		m_recvLock = false;
		m_recvWSABuf.buf = nullptr;
		m_recvWSABuf.len = 0;
		_OnRecvFromHandler onRecv;
		onRecv.swap(m_onRecvHander);
		if (bSuccess && dwTranceCount > 0)
		{
			onRecv(EC_SUCCESS, inet_ntoa(m_recvFrom.sin_addr), ntohs(m_recvFrom.sin_port), dwTranceCount);
		}
		else
		{
			onRecv(EC_ERROR, "0.0.0.0", 0,0);
		}

	}
	else if (cType == tagReqHandle::HANDLE_SENDTO)
	{
		m_sendLock = false;
		m_sendWSABuf.buf = nullptr;
		m_sendWSABuf.len = 0;
		_OnSendToHandler onSend;
		onSend.swap(m_onSendHandler);
		if (bSuccess && dwTranceCount > 0)
		{
			onSend(EC_SUCCESS);
		}
		else
		{
			onSend(EC_ERROR);
		}
	}
	return true;
}


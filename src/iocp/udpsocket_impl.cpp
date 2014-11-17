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


#include <zsummerX/iocp/iocp_impl.h>
#include <zsummerX/iocp/udpsocket_impl.h>
using namespace zsummer::network;



CUdpSocket::CUdpSocket()
{
	m_socket=INVALID_SOCKET;
	memset(&m_addr, 0, sizeof(m_addr));

	//recv
	memset(&m_recvHandle._overlapped, 0, sizeof(m_recvHandle._overlapped));
	m_recvHandle._type = tagReqHandle::HANDLE_RECVFROM;
	m_recvWSABuf.buf = NULL;
	m_recvWSABuf.len = 0;

	m_nLinkStatus = LS_UNINITIALIZE;

}


CUdpSocket::~CUdpSocket()
{
	if (m_socket != INVALID_SOCKET)
	{
		if (m_onRecvHander)
		{
			LCE("Destruct CUdpSocket Error. socket handle not invalid and some request was not completed. socket=" 
				<< (unsigned int)m_socket << ", m_onRecvHander=" << (bool)m_onRecvHander);
		}
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
	}
}

bool CUdpSocket::Initialize(const ZSummerPtr &summer, const char *localIP, unsigned short localPort)
{
	if (m_socket != INVALID_SOCKET)
	{
		LCF("CUdpSocket: socket is aread used , socket=" << (unsigned int)m_socket);
		return false;
	}	
	if (m_summer)
	{
		LCF("CUdpSocket: socket is aread used, m_summer =" << m_summer.get() << ", socket=" << (unsigned int)m_socket);
		return false;
	}
	m_summer = summer;
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
	
	if (CreateIoCompletionPort((HANDLE)m_socket, m_summer->m_io, (ULONG_PTR)this, 1) == NULL)
	{
		LCE("CUdpSocket::bind socket to IOCP error. socket="<< (unsigned int) m_socket << ", ERRCODE=" << GetLastError());
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
		return false;
	}
	m_nLinkStatus = LS_ESTABLISHED;
	return true;
}





bool CUdpSocket::DoSendTo(char * buf, unsigned int len, const char *dstip, unsigned short dstport)
{
	if (!m_summer)
	{
		LCF("CUdpSocket::DoSend IIOServer pointer uninitialize.socket=" << (unsigned int) m_socket);
		return false;
	}
	if (m_nLinkStatus != LS_ESTABLISHED)
	{
		LCF("CUdpSocket::DoSendto socket status != LS_ESTABLISHED. socket="<<(unsigned int) m_socket);
		return false;
	}

	if (len == 0 || len >1200)
	{
		LCF("CUdpSocket::DoSend length is error. socket="<<(unsigned int) m_socket);
		return false;
	}

	sockaddr_in dst;
	dst.sin_addr.s_addr = inet_addr(dstip);
	dst.sin_port = htons(dstport);
	dst.sin_family = AF_INET;
	sendto(m_socket, buf, len, 0, (sockaddr*)&dst, sizeof(dst));
	return true;
}


bool CUdpSocket::DoRecvFrom(char * buf, unsigned int len, _OnRecvFromHandler&& handler)
{
	if (!m_summer)
	{
		LCF("CUdpSocket::DoRecv IIOServer pointer uninitialize.socket=" << (unsigned int) m_socket);
		return false;
	}
	if (m_nLinkStatus != LS_ESTABLISHED)
	{
		LCF("CUdpSocket::DoRecv socket status != LS_ESTABLISHED. socket="<<(unsigned int) m_socket);
		return false;
	}

	if (len == 0)
	{
		LCF("CUdpSocket::DoRecv length is 0. socket="<<(unsigned int) m_socket);
		return false;
	}

	if (m_onRecvHander)
	{
		LCF("CUdpSocket::DoRecv  is locking. socket="<<(unsigned int) m_socket);
		return false;
	}

	m_recvWSABuf.buf = buf;
	m_recvWSABuf.len = len;


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
			return false;
		}
	}
	m_onRecvHander = std::move(handler);
	m_recvHandle._udpSocket = shared_from_this();
	return true;
}

bool CUdpSocket::OnIOCPMessage(BOOL bSuccess, DWORD dwTranceCount, unsigned char cType)
{
	if (cType == tagReqHandle::HANDLE_RECVFROM)
	{
		std::shared_ptr<CUdpSocket> guad(std::move(m_recvHandle._udpSocket));
		_OnRecvFromHandler onRecv(std::move(m_onRecvHander));
		m_recvWSABuf.buf = nullptr;
		m_recvWSABuf.len = 0;
		
		if (bSuccess && dwTranceCount > 0)
		{
			onRecv(EC_SUCCESS, inet_ntoa(m_recvFrom.sin_addr), ntohs(m_recvFrom.sin_port), dwTranceCount);
		}
		else
		{
			onRecv(EC_ERROR, "0.0.0.0", 0,0);
		}
	}
	return true;
}


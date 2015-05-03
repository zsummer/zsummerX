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
 * Copyright (C) 2010-2015 YaweiZhang <yawei_zhang@foxmail.com>.
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
#include <zsummerX/iocp/tcpaccept_impl.h>
using namespace zsummer::network;

TcpAccept::TcpAccept()
{
	//listen
	memset(&_handle._overlapped, 0, sizeof(_handle._overlapped));
	_server = INVALID_SOCKET;
	memset(&_addr, 0, sizeof(_addr));
	_handle._type = tagReqHandle::HANDLE_ACCEPT;

	//client
	_socket = INVALID_SOCKET;
	memset(_recvBuf, 0, sizeof(_recvBuf));
	_recvLen = 0;
	//status
	_nLinkStatus = LS_UNINITIALIZE;
}
TcpAccept::~TcpAccept()
{
	if (_server != INVALID_SOCKET)
	{
		closesocket(_server);
		_server = INVALID_SOCKET;
	}
	if (_socket != INVALID_SOCKET)
	{
		closesocket(_socket);
		_socket = INVALID_SOCKET;
	}
}

bool TcpAccept::initialize(EventLoopPtr& summer)
{
	_summer = summer;
	return true;
}

bool TcpAccept::openAccept(const char * ip, unsigned short port)
{
	if (!_summer)
	{
		LCF("TcpAccept _summer is nullptr!  ip=" << ip << ", port=" << port);
		assert(0);
		return false;
	}

	if (_server != INVALID_SOCKET)
	{
		LCF("TcpAccept socket is arealy used!  ip=" << ip << ", port=" << port);
		assert(0);
		return false;
	}

	_server = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (_server == INVALID_SOCKET)
	{
		LCF("TcpAccept server can't create socket!   ip=" << ip << ", port=" << port);
		assert(0);
		return false;
	}

	BOOL bReUseAddr = TRUE;
	if (setsockopt(_server, SOL_SOCKET,SO_REUSEADDR, (char*)&bReUseAddr, sizeof(BOOL)) != 0)
	{
		LCW("setsockopt  SO_REUSEADDR ERROR! ERRCODE=" << WSAGetLastError() << "    ip=" << ip << ", port=" << port);
	}

	_addr.sin_family = AF_INET;
	_addr.sin_addr.s_addr = inet_addr(ip);
	_addr.sin_port = htons(port);
	if (bind(_server, (sockaddr *) &_addr, sizeof(_addr)) != 0)
	{
		LCF("server bind err, ERRCODE=" << WSAGetLastError() << "   ip=" << ip << ", port=" << port);
		closesocket(_server);
		_server = INVALID_SOCKET;
		return false;
	}

	if (listen(_server, SOMAXCONN) != 0)
	{
		LCF("server listen err, ERRCODE=" << WSAGetLastError() << "   ip=" << ip << ", port=" << port);
		closesocket(_server);
		_server = INVALID_SOCKET;
		return false;
	}

	if (CreateIoCompletionPort((HANDLE)_server, _summer->_io, (ULONG_PTR)this, 1) == NULL)
	{
		LCF("server bind iocp err, ERRCODE=" << WSAGetLastError() << "   ip=" << ip << ", port=" << port);
		closesocket(_server);
		_server = INVALID_SOCKET;
		return false;
	}
	_nLinkStatus = LS_ESTABLISHED;
	return true;
}

bool TcpAccept::doAccept(const TcpSocketPtr & s, _OnAcceptHandler&& handler)
{
	if (_onAcceptHandler)
	{
		LCF("doAccept err, aready doAccept  ip=" << _ip << ", port=" << _port);
		return false;
	}
	if (_nLinkStatus != LS_ESTABLISHED)
	{
		LCF("doAccept err, link is unestablished.  ip=" << _ip << ", port=" << _port);
		return false;
	}
	
	_client = s;
	_socket = INVALID_SOCKET;
	memset(_recvBuf, 0, sizeof(_recvBuf));
	_recvLen = 0;
	_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP,  NULL, 0, WSA_FLAG_OVERLAPPED);
	if (_socket == INVALID_SOCKET)
	{
		LCF("create client socket err! ERRCODE=" << WSAGetLastError() << " ip=" << _ip << ", port=" << _port);
		return false;
	}

	if (!AcceptEx(_server, _socket, _recvBuf, 0, sizeof(SOCKADDR_IN)+16, sizeof(SOCKADDR_IN)+16, &_recvLen, &_handle._overlapped))
	{
		if (WSAGetLastError() != ERROR_IO_PENDING)
		{
			LCE("do AcceptEx err, ERRCODE=" << WSAGetLastError() << " ip=" << _ip << ", port=" << _port);
			closesocket(_socket);
			_socket = INVALID_SOCKET;
			return false;
		}
	}
	_onAcceptHandler = std::move(handler);
	_handle._tcpAccept = shared_from_this();
	return true;
}
bool TcpAccept::onIOCPMessage(BOOL bSuccess)
{
	std::shared_ptr<TcpAccept> guad( std::move(_handle._tcpAccept));
	_OnAcceptHandler onAccept(std::move(_onAcceptHandler));
	if (bSuccess)
	{
		{
			if (setsockopt(_socket,SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&_server, sizeof(_server)) != 0)
			{
				LCW("SO_UPDATE_ACCEPT_CONTEXT fail!  last err=" << WSAGetLastError()  << " ip=" << _ip << ", port=" << _port);
			}
			BOOL bTrue = TRUE;
			if (setsockopt(_socket,IPPROTO_TCP, TCP_NODELAY, (char*)&bTrue, sizeof(bTrue)) != 0)
			{
				LCW("setsockopt TCP_NODELAY fail!  last err=" << WSAGetLastError()  << " ip=" << _ip << ", port=" << _port);
			}
		}

		sockaddr * paddr1 = NULL;
		sockaddr * paddr2 = NULL;
		int tmp1 = 0;
		int tmp2 = 0;
		GetAcceptExSockaddrs(_recvBuf, _recvLen, sizeof(SOCKADDR_IN)+16, sizeof(SOCKADDR_IN)+16, &paddr1, &tmp1, &paddr2, &tmp2);
		_client->attachSocket(_socket, inet_ntoa(((sockaddr_in*)paddr2)->sin_addr), ntohs(((sockaddr_in*)paddr2)->sin_port));
		onAccept(NEC_SUCCESS, _client);
	}
	else
	{
		LCW("Accept Fail,  retry doAccept ... ip=" << _ip << ", port=" << _port << ", lastError=" << GetLastError());
		onAccept(NEC_ERROR, _client);
	}
	return true;
}

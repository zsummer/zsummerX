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
 * Copyright (C) 2010-2015 YaweiZhang <yawei.zhang@foxmail.com>.
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


TcpSocket::TcpSocket()
{
	g_appEnvironment.addCreatedSocketCount();
	//recv
	memset(&_recvHandle._overlapped, 0, sizeof(_recvHandle._overlapped));
	_recvHandle._type = tagReqHandle::HANDLE_RECV;
	_recvWSABuf.buf = NULL;
	_recvWSABuf.len = 0;

	//send
	memset(&_sendHandle._overlapped, 0, sizeof(_sendHandle._overlapped));
	_sendHandle._type = tagReqHandle::HANDLE_SEND;
	_sendWsaBuf.buf = NULL;
	_sendWsaBuf.len = 0;

	//connect
	memset(&_connectHandle._overlapped, 0, sizeof(_connectHandle._overlapped));
	_connectHandle._type = tagReqHandle::HANDLE_CONNECT;
}


TcpSocket::~TcpSocket()
{
	g_appEnvironment.addClosedSocketCount();
	if (_onConnectHandler || _onRecvHandler || _onSendHandler)
	{
		LCT("TcpSocket::~TcpSocket[" << this << "] Destruct TcpSocket Error. socket handle not invalid and some request was not completed. " << getTcpSocketStatus());
	}	
	if (_socket != INVALID_SOCKET)
	{
		closesocket(_socket);
		_socket = INVALID_SOCKET;
	}
}

std::string TcpSocket::getTcpSocketStatus()
{
	std::stringstream os;
	os << " TcpSocket Status: _summer.use_count()=" << _summer.use_count() << ", _socket=" << _socket
		<< ", _remoteIP=" << _remoteIP << ", _remotePort=" << _remotePort
		<< ", _recvHandle=" << _recvHandle << ", _recvWSABuf[0x" << (void*)_recvWSABuf.buf << "," << _recvWSABuf.len
		<< "], _onRecvHandler=" << (bool)_onRecvHandler << ", _sendHandle=" << _sendHandle
		<< ", _sendWsaBuf[0x" << (void*)_sendWsaBuf.buf << "," << _sendWsaBuf.len << "], _onSendHandler=" << (bool)_onSendHandler
		<< ", _connectHandle=" << _connectHandle << ", _onConnectHandler=" << (bool)_onConnectHandler
		<< ", _nLinkStatus=" << _nLinkStatus << ", Notes: HANDLE_ACCEPT=0, HANDLE_RECV,HANDLE_SEND,HANDLE_CONNECT,HANDLE_RECVFROM,HANDLE_SENDTO=5"
		<< " LS_UNINITIALIZE=0,	LS_WAITLINK,LS_ESTABLISHED,LS_CLOSED"; 
	return os.str();
}

//new socket to connect, or accept established socket
bool TcpSocket::initialize(const EventLoopPtr& summer)
{
	_summer = summer;
	if (_nLinkStatus != LS_UNINITIALIZE)
	{
		_nLinkStatus = LS_ESTABLISHED;
	}
	else
	{
		_nLinkStatus = LS_WAITLINK;
		_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (_socket == INVALID_SOCKET)
		{
			LCE("TcpSocket::doConnect[" << this << "] socket create error! ERRCODE=" << WSAGetLastError() << getTcpSocketStatus());
			return false;
		}
		setNoDelay(_socket);
		SOCKADDR_IN localAddr;
		memset(&localAddr, 0, sizeof(SOCKADDR_IN));
		localAddr.sin_family = AF_INET;
		if (bind(_socket, (sockaddr *)&localAddr, sizeof(SOCKADDR_IN)) != 0)
		{
			LCE("TcpSocket::doConnect[" << this << "] bind local addr error! ERRCODE=" << WSAGetLastError() << getTcpSocketStatus());
			closesocket(_socket);
			_socket = INVALID_SOCKET;
			return false;
		}
	}

	if (CreateIoCompletionPort((HANDLE)_socket, _summer->_io, (ULONG_PTR)this, 1) == NULL)
	{
		LCE("TcpSocket::initialize[" << this << "] bind socket to IOCP error.  ERRCODE=" << GetLastError() << getTcpSocketStatus());
		closesocket(_socket);
		_socket = INVALID_SOCKET;
		return false;
	}
	return true;
}


bool TcpSocket::attachSocket(SOCKET s, std::string remoteIP, unsigned short remotePort)
{

	_socket = s;
	_remoteIP = remoteIP;
	_remotePort = remotePort;
	_nLinkStatus = LS_WAITLINK;
	return true;
}


typedef  BOOL (PASCAL  *ConnectEx)(  SOCKET s,  const struct sockaddr* name,  int namelen,
			 PVOID lpSendBuffer,  DWORD dwSendDataLength,  LPDWORD lpdwBytesSent,
			 LPOVERLAPPED lpOverlapped);

bool TcpSocket::doConnect(const std::string& remoteIP, unsigned short remotePort, _OnConnectHandler && handler)
{
	if (!_summer)
	{
		LCF("TcpSocket::doConnect[" << this << "] _summer pointer uninitialize." << getTcpSocketStatus());
		return false;
	}
	if (_nLinkStatus != LS_WAITLINK)
	{
		LCW("TcpSocket::doConnect[" << this << "] socket already used or not initilize. " << getTcpSocketStatus());
		return false;
	}
	if (_onConnectHandler)
	{
		LCF("TcpSocket::doConnect[" << this << "] socket is connecting." << getTcpSocketStatus());
		return false;
	}
	

	GUID gid = WSAID_CONNECTEX;
	ConnectEx lpConnectEx = NULL;
	DWORD dwSize = 0;
	if (WSAIoctl(_socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &gid, sizeof(gid), &lpConnectEx, sizeof(lpConnectEx), &dwSize, NULL, NULL) != 0)
	{
		LCF("TcpSocket::doConnect[" << this << "] Get ConnectEx pointer err!  ERRCODE= " << WSAGetLastError() << getTcpSocketStatus());
		return false;
	}

	_remoteIP = remoteIP;
	_remotePort = remotePort;
	SOCKADDR_IN remoteAddr;
	remoteAddr.sin_addr.s_addr = inet_addr(_remoteIP.c_str());
	remoteAddr.sin_family = AF_INET;
	remoteAddr.sin_port = htons(_remotePort);
	

	char buf[1];
	DWORD dwLenth = 0;
	if (!lpConnectEx(_socket, (sockaddr *)&remoteAddr, sizeof(remoteAddr), buf, 0, &dwLenth, &_connectHandle._overlapped))
	{
		if (WSAGetLastError() != ERROR_IO_PENDING)
		{
			LCF("TcpSocket::doConnect[" << this << "] doConnect failed and ERRCODE!=ERROR_IO_PENDING, ERRCODE=" << WSAGetLastError() << getTcpSocketStatus());
			return false;
		}

	}
	_onConnectHandler = std::move(handler);
	_connectHandle._tcpSocket = shared_from_this();
	return true;
}




bool TcpSocket::doSend(char * buf, unsigned int len, _OnSendHandler &&handler)
{
	if (_nLinkStatus != LS_ESTABLISHED)
	{
		LCT("TcpSocket::doSend[" << this << "] socket status != LS_ESTABLISHED." << getTcpSocketStatus());
		return false;
	}
	if (!_summer)
	{
		LCF("TcpSocket::doSend[" << this << "] _summer pointer uninitialize." << getTcpSocketStatus());
		return false;
	}
	if (_onSendHandler)
	{
		LCF("TcpSocket::doSend[" << this << "] socket is sending." << getTcpSocketStatus());
		return false;
	}
	if (len == 0)
	{
		LCF("TcpSocket::doSend[" << this << "] length is 0." << getTcpSocketStatus());
		return false;
	}

	_sendWsaBuf.buf = buf;
	_sendWsaBuf.len = len;
	DWORD dwTemp1=0;
	if (WSASend(_socket, &_sendWsaBuf, 1, &dwTemp1, 0, &_sendHandle._overlapped, NULL) != 0)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			LCT("TcpSocket::doSend[" << this << "] doSend failed and ERRCODE!=ERROR_IO_PENDING ERRCODE=" << WSAGetLastError() << getTcpSocketStatus());
			_sendWsaBuf.buf = nullptr;
			_sendWsaBuf.len = 0;
			doClose();
			return false;
		}
	}
	_onSendHandler = std::move(handler);
	_sendHandle._tcpSocket = shared_from_this();
	return true;
}


bool TcpSocket::doRecv(char * buf, unsigned int len, _OnRecvHandler && handler)
{
	if (_nLinkStatus != LS_ESTABLISHED)
	{
		LCT("TcpSocket::doRecv[" << this << "] socket status != LS_ESTABLISHED. " << getTcpSocketStatus());
		return false;
	}
	if (!_summer)
	{
		LCF("TcpSocket::doRecv[" << this << "] _summer pointer uninitialize." << getTcpSocketStatus());
		return false;
	}
	if (_onRecvHandler)
	{
		LCF("TcpSocket::doRecv[" << this << "] socket is recving. " << getTcpSocketStatus());
		return false;
	}
	if (len == 0)
	{
		LCF("TcpSocket::doRecv[" << this << "] length is 0." << getTcpSocketStatus());
		return false;
	}


	_recvWSABuf.buf = buf;
	_recvWSABuf.len = len;
	
	DWORD dwRecv = 0;
	DWORD dwFlag = 0;
	if (WSARecv(_socket, &_recvWSABuf, 1, &dwRecv, &dwFlag, &_recvHandle._overlapped, NULL) != 0)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			LCT("TcpSocket::doRecv[" << this << "] doRecv failed and ERRCODE!=ERROR_IO_PENDING, ERRCODE=" << WSAGetLastError() << getTcpSocketStatus());
			_recvWSABuf.buf = nullptr;
			_recvWSABuf.len = 0;
			doClose();
			return false;
		}
	}
	_onRecvHandler = std::move(handler);
	_recvHandle._tcpSocket = shared_from_this();
	return true;
}

void TcpSocket::onIOCPMessage(BOOL bSuccess, DWORD dwTranceCount, unsigned char cType)
{
	if (cType == tagReqHandle::HANDLE_CONNECT)
	{
		_OnConnectHandler onConnect(std::move(_onConnectHandler));
		std::shared_ptr<TcpSocket> guad(std::move(_connectHandle._tcpSocket));
		if (!onConnect)
		{
			return;
		}
		if (bSuccess)
		{
			_nLinkStatus = LS_ESTABLISHED;
			{
				BOOL bTrue = TRUE;
				if (setsockopt(_socket,IPPROTO_TCP, TCP_NODELAY, (char*)&bTrue, sizeof(bTrue)) != 0)
				{
					LCW("TcpSocket::onIOCPMessage[" << this << "] setsockopt TCP_NODELAY fail!  last err=" << WSAGetLastError() << getTcpSocketStatus());
				}
			}
			onConnect(NetErrorCode::NEC_SUCCESS);
		}
		else
		{
			closesocket(_socket);
			_socket = INVALID_SOCKET;
			onConnect(NetErrorCode::NEC_ERROR);
		}
		return ;
	}


	if (cType == tagReqHandle::HANDLE_SEND)
	{
		_OnSendHandler onSend(std::move(_onSendHandler));
		std::shared_ptr<TcpSocket> guad(std::move(_sendHandle._tcpSocket));
		if (!onSend)
		{
			return;
		}
		if (!bSuccess || _nLinkStatus != LS_ESTABLISHED)
		{
			return;
		}
		onSend(NetErrorCode::NEC_SUCCESS, dwTranceCount);
		return;
	}
	else if (cType == tagReqHandle::HANDLE_RECV)
	{
		std::shared_ptr<TcpSocket> guad(std::move(_recvHandle._tcpSocket));
		_OnRecvHandler onRecv(std::move(_onRecvHandler));
		if (!onRecv)
		{
			return;
		}
		
		if (!bSuccess)
		{
			_nLinkStatus = LS_CLOSED;
			onRecv(NetErrorCode::NEC_REMOTE_HANGUP, dwTranceCount);
			return;
		}
		else if (dwTranceCount == 0)
		{
			_nLinkStatus = LS_CLOSED;
			onRecv(NetErrorCode::NEC_REMOTE_CLOSED, dwTranceCount);
			return;
		}
		onRecv(NetErrorCode::NEC_SUCCESS, dwTranceCount);
		return;
	}

}


bool TcpSocket::doClose()
{
	if (_nLinkStatus == LS_ESTABLISHED || _nLinkStatus == LS_WAITLINK)
	{
		_nLinkStatus = LS_CLOSED;
		closesocket(_socket);
		return true;
	}
	return true;
}

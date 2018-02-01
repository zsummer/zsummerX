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
    _recvHandle._type = ExtendHandle::HANDLE_RECV;
    _recvWSABuf.buf = NULL;
    _recvWSABuf.len = 0;

    //send
    memset(&_sendHandle._overlapped, 0, sizeof(_sendHandle._overlapped));
    _sendHandle._type = ExtendHandle::HANDLE_SEND;
    _sendWsaBuf.buf = NULL;
    _sendWsaBuf.len = 0;

    //connect
    memset(&_connectHandle._overlapped, 0, sizeof(_connectHandle._overlapped));
    _connectHandle._type = ExtendHandle::HANDLE_CONNECT;
}


TcpSocket::~TcpSocket()
{
    g_appEnvironment.addClosedSocketCount();
    if (_onConnectHandler || _onRecvHandler || _onSendHandler)
    {
        LCW("Destruct TcpSocket Error. socket handle not invalid and some request was not completed. " << logSection());
    }    
    if (_socket != INVALID_SOCKET)
    {
        closesocket(_socket);
        _socket = INVALID_SOCKET;
    }
}

std::string TcpSocket::logSection()
{
    std::stringstream os;
    os << "this[0x" << this <<"] _summer.use_count()=" << _summer.use_count() << ", _socket=" << _socket
        << ", _remoteIP=" << _remoteIP << ", _remotePort=" << _remotePort
        << ", _recvHandle=" << _recvHandle << ", _recvWSABuf[0x" << (void*)_recvWSABuf.buf << "," << _recvWSABuf.len
        << "], _onRecvHandler=" << (bool)_onRecvHandler << ", _sendHandle=" << _sendHandle
        << ", _sendWsaBuf[0x" << (void*)_sendWsaBuf.buf << "," << _sendWsaBuf.len << "], _onSendHandler=" << (bool)_onSendHandler
        << ", _connectHandle=" << _connectHandle << ", _onConnectHandler=" << (bool)_onConnectHandler
        << ", _linkStatus=" << _linkStatus << ", Notes: HANDLE_ACCEPT=0, HANDLE_RECV,HANDLE_SEND,HANDLE_CONNECT,HANDLE_RECVFROM,HANDLE_SENDTO=5"
        << " LS_UNINITIALIZE=0,    LS_WAITLINK,LS_ESTABLISHED,LS_CLOSED"; 
    return os.str();
}

bool TcpSocket::setNoDelay()
{
        return zsummer::network::setNoDelay(_socket);
}
//new socket to connect, or accept established socket
bool TcpSocket::initialize(const EventLoopPtr& summer)
{
    if (_linkStatus == LS_UNINITIALIZE)
    {
        _summer = summer;
        _linkStatus = LS_WAITLINK;

    }
    else if (_linkStatus == LS_ATTACHED)
    {
        _summer = summer;
        _linkStatus = LS_ESTABLISHED;
        if (CreateIoCompletionPort((HANDLE)_socket, _summer->_io, (ULONG_PTR)this, 1) == NULL)
        {
            LCE("TcpSocket bind socket to IOCP error.  ERRCODE=" << GetLastError());
            doClose();
            return false;
        }
    }
    else
    {
        LCE("TcpSocket already initialize! " << logSection());
        return false;
    }
    

    return true;
}


bool TcpSocket::attachSocket(SOCKET s, std::string remoteIP, unsigned short remotePort, bool isIPV6)
{
    _socket = s;
    _remoteIP = remoteIP;
    _remotePort = remotePort;
    _linkStatus = LS_ATTACHED;
    _isIPV6 = isIPV6;
    return true;
}


typedef  BOOL (PASCAL  *ConnectEx)(  SOCKET s,  const struct sockaddr* name,  int namelen,
             PVOID lpSendBuffer,  DWORD dwSendDataLength,  LPDWORD lpdwBytesSent,
             LPOVERLAPPED lpOverlapped);

bool TcpSocket::doConnect(const std::string& remoteIP, unsigned short remotePort, _OnConnectHandler && handler)
{
    if (!_summer || _linkStatus != LS_WAITLINK)
    {
        LCF("TcpSocket uninitialize." );
        return false;
    }

    if (_onConnectHandler)
    {
        LCF("TcpSocket already connect." << logSection());
        return false;
    }
    _isIPV6 = remoteIP.find(':') != std::string::npos;
    if (_isIPV6)
    {
        _socket = WSASocket(AF_INET6, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
        if (_socket == INVALID_SOCKET)
        {
            LCE("TcpSocket create error! ERRCODE=" << WSAGetLastError());
            return false;
        }
        SOCKADDR_IN6 localAddr;
        memset(&localAddr, 0, sizeof(localAddr));
        localAddr.sin6_family = AF_INET6;
        if (bind(_socket, (sockaddr *)&localAddr, sizeof(localAddr)) != 0)
        {
            LCE("TcpSocket bind local addr error! ERRCODE=" << WSAGetLastError());
            closesocket(_socket);
            _socket = INVALID_SOCKET;
            return false;
        }
    }
    else
    {
        _socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
        if (_socket == INVALID_SOCKET)
        {
            LCE("TcpSocket create error! ERRCODE=" << WSAGetLastError());
            return false;
        }
        SOCKADDR_IN localAddr;
        memset(&localAddr, 0, sizeof(localAddr));
        localAddr.sin_family = AF_INET;
        if (bind(_socket, (sockaddr *)&localAddr, sizeof(localAddr)) != 0)
        {
            LCE("TcpSocket bind local addr error! ERRCODE=" << WSAGetLastError());
            closesocket(_socket);
            _socket = INVALID_SOCKET;
            return false;
        }
    }
    if (CreateIoCompletionPort((HANDLE)_socket, _summer->_io, (ULONG_PTR)this, 1) == NULL)
    {
        LCE("TcpSocket bind socket to IOCP error.  ERRCODE=" << GetLastError());
        doClose();
        return false;
    }

	if (true)
	{
		int OptionValue = 1;
		DWORD NumberOfBytesReturned = 0;
		DWORD SIO_LOOPBACK_FAST_PATH_A = 0x98000010;

		WSAIoctl(
			_socket,
			SIO_LOOPBACK_FAST_PATH_A,
			&OptionValue,
			sizeof(OptionValue),
			NULL,
			0,
			&NumberOfBytesReturned,
			0,
			0);
	}

    GUID gid = WSAID_CONNECTEX;
    ConnectEx lpConnectEx = NULL;
    DWORD dwSize = 0;
    if (WSAIoctl(_socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &gid, sizeof(gid), &lpConnectEx, sizeof(lpConnectEx), &dwSize, NULL, NULL) != 0)
    {
        LCF("TcpSocket::doConnect[" << this << "] Get ConnectEx pointer err!  ERRCODE= " << WSAGetLastError() );
        return false;
    }

    _remoteIP = remoteIP;
    _remotePort = remotePort;
    if (remoteIP.find(':') != std::string::npos)
    {
        _isIPV6 = true;
        SOCKADDR_IN6 remoteAddr;
        memset(&remoteAddr, 0, sizeof(remoteAddr));
        if (inet_pton(AF_INET6, remoteIP.c_str(), &remoteAddr.sin6_addr) <= 0)
        {
            LCE("ipv6 format error.  remoteIP=" << remoteIP);
            return false;
        }

        remoteAddr.sin6_family = AF_INET6;
        remoteAddr.sin6_port = htons(_remotePort);
        char buf[1];
        DWORD dwBytes = 0;
        if (!lpConnectEx(_socket, (sockaddr *)&remoteAddr, sizeof(remoteAddr), buf, 0, &dwBytes, &_connectHandle._overlapped))
        {
            if (WSAGetLastError() != ERROR_IO_PENDING)
            {
                LCF("TcpSocket doConnect failed and ERRCODE!=ERROR_IO_PENDING, ERRCODE=" << WSAGetLastError());
                return false;
            }

        }
    }
    else
    {
        _isIPV6 = false;
        SOCKADDR_IN remoteAddr;
        memset(&remoteAddr, 0, sizeof(remoteAddr));
        remoteAddr.sin_addr.s_addr = inet_addr(_remoteIP.c_str());
        remoteAddr.sin_family = AF_INET;
        remoteAddr.sin_port = htons(_remotePort);
        char buf[1];
        DWORD dwBytes = 0;
        if (!lpConnectEx(_socket, (sockaddr *)&remoteAddr, sizeof(remoteAddr), buf, 0, &dwBytes, &_connectHandle._overlapped))
        {
            if (WSAGetLastError() != ERROR_IO_PENDING)
            {
                LCF("TcpSocket doConnect failed and ERRCODE!=ERROR_IO_PENDING, ERRCODE=" << WSAGetLastError());
                return false;
            }

        }
    }

    


    _onConnectHandler = std::move(handler);
    _connectHandle._tcpSocket = shared_from_this();
    return true;
}




bool TcpSocket::doSend(char * buf, unsigned int len, _OnSendHandler &&handler)
{
    if (_linkStatus != LS_ESTABLISHED)
    {
        LCW("TcpSocket status != LS_ESTABLISHED.");
        return false;
    }
    if (!_summer)
    {
        LCF("TcpSocket uninitialize." << logSection());
        return false;
    }
    if (_onSendHandler)
    {
        LCF("TcpSocket already send." << logSection());
        return false;
    }
    if (len == 0)
    {
        LCF("TcpSocket param error. length is 0." );
        return false;
    }

    _sendWsaBuf.buf = buf;
    _sendWsaBuf.len = len;
    DWORD dwBytes=0;
    if (WSASend(_socket, &_sendWsaBuf, 1, &dwBytes, 0, &_sendHandle._overlapped, NULL) != 0)
    {
        if (WSAGetLastError() != WSA_IO_PENDING)
        {
            LCW("TcpSocket doSend failed and ERRCODE!=ERROR_IO_PENDING ERRCODE=" << WSAGetLastError() );
            _sendWsaBuf.buf = nullptr;
            _sendWsaBuf.len = 0;
            return false;
        }
    }
    _onSendHandler = std::move(handler);
    _sendHandle._tcpSocket = shared_from_this();
    return true;
}


bool TcpSocket::doRecv(char * buf, unsigned int len, _OnRecvHandler && handler)
{
    if (_linkStatus != LS_ESTABLISHED)
    {
        LCW("TcpSocket status != LS_ESTABLISHED. ");
        return false;
    }
    if (!_summer)
    {
        LCF("TcpSocket uninitialize." << logSection());
        return false;
    }
    if (_onRecvHandler)
    {
        LCF("TcpSocket already recv. " << logSection());
        return false;
    }
    if (len == 0)
    {
        LCF("TcpSocket param error. length is 0." );
        return false;
    }


    _recvWSABuf.buf = buf;
    _recvWSABuf.len = len;
    
    DWORD dwBytes = 0;
    DWORD dwFlag = 0;
    if (WSARecv(_socket, &_recvWSABuf, 1, &dwBytes, &dwFlag, &_recvHandle._overlapped, NULL) != 0)
    {
        if (WSAGetLastError() != WSA_IO_PENDING)
        {
            LCW("TcpSocket doRecv failed and ERRCODE!=ERROR_IO_PENDING, ERRCODE=" << WSAGetLastError() );
            _recvWSABuf.buf = nullptr;
            _recvWSABuf.len = 0;
            return false;
        }
    }
    _onRecvHandler = std::move(handler);
    _recvHandle._tcpSocket = shared_from_this();
    return true;
}

void TcpSocket::onIOCPMessage(BOOL bSuccess, DWORD dwTranceBytes, unsigned char cType)
{
    if (cType == ExtendHandle::HANDLE_CONNECT)
    {
        _OnConnectHandler onConnect(std::move(_onConnectHandler));
        std::shared_ptr<TcpSocket> guard(std::move(_connectHandle._tcpSocket));
        if (!onConnect)
        {
            return;
        }
        if (_linkStatus != LS_WAITLINK)
        {
            return;
        }
        
        if (bSuccess)
        {
            _linkStatus = LS_ESTABLISHED;
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


    if (cType == ExtendHandle::HANDLE_SEND)
    {
        _OnSendHandler onSend(std::move(_onSendHandler));
        std::shared_ptr<TcpSocket> guard(std::move(_sendHandle._tcpSocket));
        if (!onSend)
        {
            return;
        }
        if (!bSuccess || _linkStatus != LS_ESTABLISHED)
        {
            if (_socket != INVALID_SOCKET)
            {
                closesocket(_socket);
                _socket = INVALID_SOCKET;
            }
            return;
        }
        onSend(NetErrorCode::NEC_SUCCESS, dwTranceBytes);
        return;
    }
    else if (cType == ExtendHandle::HANDLE_RECV)
    {
        std::shared_ptr<TcpSocket> guard(std::move(_recvHandle._tcpSocket));
        _OnRecvHandler onRecv(std::move(_onRecvHandler));
        if (!onRecv)
        {
            return;
        }
        if (_linkStatus != LS_ESTABLISHED)
        {
            return;
        }
        
        if (!bSuccess)
        {
            doClose();
            onRecv(NetErrorCode::NEC_ERROR, dwTranceBytes);
            return;
        }
        else if (dwTranceBytes == 0)
        {
            doClose();
            onRecv(NetErrorCode::NEC_REMOTE_CLOSED, dwTranceBytes);
            return;
        }
        onRecv(NetErrorCode::NEC_SUCCESS, dwTranceBytes);
        return;
    }

}


bool TcpSocket::doClose()
{
    if (_linkStatus == LS_ESTABLISHED || _linkStatus == LS_WAITLINK)
    {
        _linkStatus = LS_CLOSED;
        closesocket(_socket);
        _socket = INVALID_SOCKET;
        return true;
    }
    return true;
}

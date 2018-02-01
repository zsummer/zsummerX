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


#include <zsummerX/iocp/iocp_impl.h>
#include <zsummerX/iocp/tcpaccept_impl.h>
using namespace zsummer::network;

TcpAccept::TcpAccept()
{
    //listen
    memset(&_handle._overlapped, 0, sizeof(_handle._overlapped));
    _server = INVALID_SOCKET;
    
    _handle._type = ExtendHandle::HANDLE_ACCEPT;

    //client
    _socket = INVALID_SOCKET;
    memset(_recvBuf, 0, sizeof(_recvBuf));
    _recvLen = 0;
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

std::string TcpAccept::logSection()
{
    std::stringstream os;
    os << " logSecion[0x" << this << "] " << "_summer=" << _summer.use_count() << ", _server=" << _server << ",_ip=" << _ip << ", _port=" << _port <<", _onAcceptHandler=" << (bool)_onAcceptHandler;
    return os.str();
}
bool TcpAccept::initialize(EventLoopPtr& summer)
{
    if (_summer)
    {
        LCF("TcpAccept already initialize! " << logSection());
        return false;
    }
    _summer = summer;
    return true;
}

bool TcpAccept::openAccept(const std::string ip, unsigned short port , bool reuse )
{
    if (!_summer)
    {
        LCF("TcpAccept not inilialize!  ip=" << ip << ", port=" << port << logSection());
        return false;
    }

    if (_server != INVALID_SOCKET)
    {
        LCF("TcpAccept already opened!  ip=" << ip << ", port=" << port << logSection());
        return false;
    }
    _ip = ip;
    _port = port;
    _isIPV6 = _ip.find(':') != std::string::npos;
    if (_isIPV6)
    {
        _server = WSASocket(AF_INET6, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
        setIPV6Only(_server, false);
    }
    else
    {
        _server = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    }
    if (_server == INVALID_SOCKET)
    {
        LCF("create socket error! ");
        return false;
    }


    if (reuse)
    {
        setReuse(_server);
    }
    
    if (_isIPV6)
    {
        SOCKADDR_IN6 addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin6_family = AF_INET6;
        if (ip.empty() || ip == "::")
        {
            addr.sin6_addr = in6addr_any;
        }
        else
        {
            auto ret = inet_pton(AF_INET6, ip.c_str(), &addr.sin6_addr);
            if (ret <= 0)
            {
                LCF("bind ipv6 error, ipv6 format error" << ip);
                closesocket(_server);
                _server = INVALID_SOCKET;
                return false;
            }
        }
        addr.sin6_port = htons(port);
        auto ret = bind(_server, (sockaddr *)&addr, sizeof(addr));
        if (ret != 0)
        {
            LCF("bind ipv6 error, ERRCODE=" << WSAGetLastError());
            closesocket(_server);
            _server = INVALID_SOCKET;
            return false;
        }
    }
    else
    {
        SOCKADDR_IN addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = ip.empty() ? INADDR_ANY : inet_addr(ip.c_str());
        addr.sin_port = htons(port);
        if (bind(_server, (sockaddr *)&addr, sizeof(addr)) != 0)
        {
            LCF("bind error, ERRCODE=" << WSAGetLastError());
            closesocket(_server);
            _server = INVALID_SOCKET;
            return false;
        }
        
    }
	if (true)
	{
		int OptionValue = 1;
		DWORD NumberOfBytesReturned = 0;
		DWORD SIO_LOOPBACK_FAST_PATH_A = 0x98000010;

		WSAIoctl(
			_server,
			SIO_LOOPBACK_FAST_PATH_A,
			&OptionValue,
			sizeof(OptionValue),
			NULL,
			0,
			&NumberOfBytesReturned,
			0,
			0);
	}

    if (listen(_server, SOMAXCONN) != 0)
    {
        LCF("listen error, ERRCODE=" << WSAGetLastError() );
        closesocket(_server);
        _server = INVALID_SOCKET;
        return false;
    }

    if (CreateIoCompletionPort((HANDLE)_server, _summer->_io, (ULONG_PTR)this, 1) == NULL)
    {
        LCF("bind to iocp error, ERRCODE=" << WSAGetLastError() );
        closesocket(_server);
        _server = INVALID_SOCKET;
        return false;
    }
    return true;
}

bool TcpAccept::close()
{
    if (_server != INVALID_SOCKET)
    {
        LCI("TcpAccept::close. socket=" << _server);
        closesocket(_server);
        _server = INVALID_SOCKET;
    }
    return true;
}

bool TcpAccept::doAccept(const TcpSocketPtr & s, _OnAcceptHandler&& handler)
{
    if (_onAcceptHandler)
    {
        LCF("duplicate operation error." << logSection());
        return false;
    }
    if (!_summer || _server == INVALID_SOCKET)
    {
        LCF("TcpAccept not initialize." << logSection());
        return false;
    }
    
    _client = s;
    _socket = INVALID_SOCKET;
    memset(_recvBuf, 0, sizeof(_recvBuf));
    _recvLen = 0;
    unsigned int addrLen = 0;
    if (_isIPV6)
    {
        addrLen = sizeof(SOCKADDR_IN6)+16;
        _socket = WSASocket(AF_INET6, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    }
    else
    {
        addrLen = sizeof(SOCKADDR_IN)+16;
        _socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    }
    if (_socket == INVALID_SOCKET)
    {
        LCF("TcpAccept::doAccept create client socket error! error code=" << WSAGetLastError() );
        return false;
    }
    setNoDelay(_socket);
    if (!AcceptEx(_server, _socket, _recvBuf, 0, addrLen, addrLen, &_recvLen, &_handle._overlapped))
    {
        if (WSAGetLastError() != ERROR_IO_PENDING)
        {
            LCE("TcpAccept::doAccept do AcceptEx error, error code =" << WSAGetLastError() );
            closesocket(_socket);
            _socket = INVALID_SOCKET;
            return false;
        }
    }
    _onAcceptHandler = handler;
    _handle._tcpAccept = shared_from_this();
    return true;
}
bool TcpAccept::onIOCPMessage(BOOL bSuccess)
{
    std::shared_ptr<TcpAccept> guard( std::move(_handle._tcpAccept));
    _OnAcceptHandler onAccept(std::move(_onAcceptHandler));
    if (bSuccess)
    {

        if (setsockopt(_socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&_server, sizeof(_server)) != 0)
        {
            LCW("setsockopt SO_UPDATE_ACCEPT_CONTEXT fail!  last error=" << WSAGetLastError() );
        }
        if (_isIPV6)
        {
            sockaddr * paddr1 = NULL;
            sockaddr * paddr2 = NULL;
            int tmp1 = 0;
            int tmp2 = 0;
            GetAcceptExSockaddrs(_recvBuf, _recvLen, sizeof(SOCKADDR_IN6) + 16, sizeof(SOCKADDR_IN6) + 16, &paddr1, &tmp1, &paddr2, &tmp2);
            char ip[50] = { 0 };
            inet_ntop(AF_INET6, &(((SOCKADDR_IN6*)paddr2)->sin6_addr), ip, sizeof(ip));
            _client->attachSocket(_socket, ip, ntohs(((sockaddr_in6*)paddr2)->sin6_port), _isIPV6);
            onAccept(NEC_SUCCESS, _client);
        }
        else
        {
            sockaddr * paddr1 = NULL;
            sockaddr * paddr2 = NULL;
            int tmp1 = 0;
            int tmp2 = 0;
            GetAcceptExSockaddrs(_recvBuf, _recvLen, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &paddr1, &tmp1, &paddr2, &tmp2);
            _client->attachSocket(_socket, inet_ntoa(((sockaddr_in*)paddr2)->sin_addr), ntohs(((sockaddr_in*)paddr2)->sin_port), _isIPV6);
            onAccept(NEC_SUCCESS, _client);
        }

    }
    else
    {
        closesocket(_socket);
        _socket = INVALID_SOCKET;
        LCW("AcceptEx failed ... , lastError=" << GetLastError() );
        onAccept(NEC_ERROR, _client);
    }
    return true;
}

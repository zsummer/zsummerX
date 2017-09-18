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
#include <zsummerX/iocp/udpsocket_impl.h>
using namespace zsummer::network;



UdpSocket::UdpSocket()
{
    _socket=INVALID_SOCKET;
    memset(&_addr, 0, sizeof(_addr));

    //recv
    memset(&_recvHandle._overlapped, 0, sizeof(_recvHandle._overlapped));
    _recvHandle._type = ExtendHandle::HANDLE_RECVFROM;
    _recvWSABuf.buf = NULL;
    _recvWSABuf.len = 0;

    _linkStatus = LS_UNINITIALIZE;

}


UdpSocket::~UdpSocket()
{
    if (_socket != INVALID_SOCKET)
    {
        if (_onRecvHander)
        {
            LCE("Destruct UdpSocket Error. socket handle not invalid and some request was not completed. socket=" 
                << (unsigned int)_socket << ", _onRecvHander=" << (bool)_onRecvHander);
        }
        closesocket(_socket);
        _socket = INVALID_SOCKET;
    }
}

bool UdpSocket::initialize(const EventLoopPtr &summer, const char *localIP, unsigned short localPort)
{
    if (_socket != INVALID_SOCKET)
    {
        LCF("socket is aread used , socket=" << (unsigned int)_socket);
        return false;
    }    
    if (_summer)
    {
        LCF("socket is aread used, _summer =" << _summer.get() << ", socket=" << (unsigned int)_socket);
        return false;
    }
    _summer = summer;
    _socket = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (_socket == INVALID_SOCKET)
    {
        LCE("create socket  error! ERRCODE=" << WSAGetLastError());
        return false;
    }

    sockaddr_in addr;
    memset(&addr, 0, sizeof(SOCKADDR_IN));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(localIP);
    addr.sin_port = htons(localPort);
    if (bind(_socket, (sockaddr *) &addr, sizeof(sockaddr_in)) != 0)
    {
        LCE("bind local addr error!  socket=" << (unsigned int)_socket << ", ERRCODE=" << WSAGetLastError());
        return false;
    }
    
    if (CreateIoCompletionPort((HANDLE)_socket, _summer->_io, (ULONG_PTR)this, 1) == NULL)
    {
        LCE("socket to IOCP error. socket="<< (unsigned int) _socket << ", ERRCODE=" << GetLastError());
        closesocket(_socket);
        _socket = INVALID_SOCKET;
        return false;
    }
    _linkStatus = LS_ESTABLISHED;
    return true;
}





bool UdpSocket::doSendTo(char * buf, unsigned int len, const char *dstip, unsigned short dstport)
{
    if (!_summer)
    {
        LCF("UdpSocket uninitialize.socket=" << (unsigned int) _socket);
        return false;
    }
    if (_linkStatus != LS_ESTABLISHED)
    {
        LCF("status != LS_ESTABLISHED. socket="<<(unsigned int) _socket);
        return false;
    }

    if (len == 0 || len >1200)
    {
        LCF("param length error. length=" << len << ", socket="<<(unsigned int) _socket);
        return false;
    }

    sockaddr_in dst;
    dst.sin_addr.s_addr = inet_addr(dstip);
    dst.sin_port = htons(dstport);
    dst.sin_family = AF_INET;
    sendto(_socket, buf, len, 0, (sockaddr*)&dst, sizeof(dst));
    return true;
}


bool UdpSocket::doRecvFrom(char * buf, unsigned int len, _OnRecvFromHandler&& handler)
{
    if (!_summer)
    {
        LCF("uninitialize.socket=" << (unsigned int) _socket);
        return false;
    }
    if (_linkStatus != LS_ESTABLISHED)
    {
        LCF("socket status != LS_ESTABLISHED. socket="<<(unsigned int) _socket);
        return false;
    }

    if (len == 0)
    {
        LCF("length is 0. socket="<<(unsigned int) _socket);
        return false;
    }

    if (_onRecvHander)
    {
        LCF("duplicate doRecvFrom. socket="<<(unsigned int) _socket);
        return false;
    }

    _recvWSABuf.buf = buf;
    _recvWSABuf.len = len;


    sizeof(&_recvFrom, 0, sizeof(_recvFrom));
    _recvFromLen = sizeof(_recvFrom);
    DWORD dwBytes = 0;
    DWORD dwFlag = 0;
    if (WSARecvFrom(_socket, &_recvWSABuf, 1, &dwBytes, &dwFlag,(sockaddr*)&_recvFrom, &_recvFromLen, &_recvHandle._overlapped, NULL) != 0)
    {
        if (WSAGetLastError() != WSA_IO_PENDING)
        {
            LCE("doRecv failed and ERRCODE!=ERROR_IO_PENDING, socket="<< (unsigned int) _socket << ", ERRCODE=" << WSAGetLastError());
            _recvWSABuf.buf = nullptr;
            _recvWSABuf.len = 0;
            return false;
        }
    }
    _onRecvHander = std::move(handler);
    _recvHandle._udpSocket = shared_from_this();
    return true;
}

bool UdpSocket::onIOCPMessage(BOOL bSuccess, DWORD dwTranceBytes, unsigned char cType)
{
    if (cType == ExtendHandle::HANDLE_RECVFROM)
    {
        std::shared_ptr<UdpSocket> guard(std::move(_recvHandle._udpSocket));
        _OnRecvFromHandler onRecv(std::move(_onRecvHander));
        _recvWSABuf.buf = nullptr;
        _recvWSABuf.len = 0;
        
        if (bSuccess && dwTranceBytes > 0)
        {
            onRecv(NEC_SUCCESS, inet_ntoa(_recvFrom.sin_addr), ntohs(_recvFrom.sin_port), dwTranceBytes);
        }
        else
        {
            onRecv(NEC_ERROR, "0.0.0.0", 0,0);
        }
    }
    return true;
}


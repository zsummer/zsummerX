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
 * Copyright (C) 2010-2017 YaweiZhang <yawei.zhang@foxmail.com>.
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

#include <zsummerX/epoll/epoll_impl.h>
#include <zsummerX/epoll/udpsocket_impl.h>

using namespace zsummer::network;



UdpSocket::UdpSocket()
{
    _eventData._event.data.ptr = &_eventData;
    _eventData._event.events = 0;
    _eventData._fd = -1;
    _eventData._linkstat = LS_UNINITIALIZE;
    _eventData._type = EventData::REG_UDP_SOCKET;
    _recvLen  = 0;
    _recvBuf = nullptr;
}


UdpSocket::~UdpSocket()
{
    if (_onRecvFromHandler )
    {
        LCF("UdpSocket::~UdpSocket[this0x" << this << "] Destruct UdpSocket Error. socket handle not invalid and some request was not completed. fd="
            << _eventData._fd );
    }
    if (_eventData._fd != InvalidFD)
    {
        ::close(_eventData._fd);
        _eventData._fd = InvalidFD;
    }
}
bool  UdpSocket::initialize(const EventLoopPtr &summer, const char *localIP, unsigned short localPort)
{
    if (_eventData._linkstat == LS_UNINITIALIZE)
    {
        _summer = summer;
        _eventData._fd = socket(AF_INET, SOCK_DGRAM, 0);
        _eventData._linkstat = LS_WAITLINK;
        if (_eventData._fd == InvalidFD)
        {
            LCE("UdpSocket::initialize[this0x" << this << "] create socket fail. this=" << this << ", errno=" << strerror(errno));
            return false;
        }
        setNonBlock(_eventData._fd);
        sockaddr_in    localAddr;
        localAddr.sin_family = AF_INET;
        localAddr.sin_addr.s_addr = inet_addr(localIP);
        localAddr.sin_port = htons(localPort);
        if (bind(_eventData._fd, (sockaddr *) &localAddr, sizeof(localAddr)) != 0)
        {
            LCE("UdpSocket::initialize[this0x" << this << "]: socket bind err, errno=" << strerror(errno));
            ::close(_eventData._fd);
            _eventData._fd = InvalidFD;
            return false;
        }

        _summer->registerEvent(EPOLL_CTL_ADD, _eventData);
        _eventData._udpsocketPtr = shared_from_this();
        _eventData._linkstat = LS_ESTABLISHED;
        return true;
    }


    LCE("UdpSocket::initialize[this0x" << this << "] UdpSocket is aready initialize, _ios not is nullptr. this=" << this);
    return false;
}

bool UdpSocket::doSendTo(char * buf, unsigned int len, const char *dstip, unsigned short dstport)
{
    if (!_summer || _eventData._linkstat != LS_ESTABLISHED)
    {
        LCE("UdpSocket::doSend[this0x" << this << "] IIOServer not bind!");
        return false;
    }
    if (len == 0 || len >1500)
    {
        LCE("UdpSocket::doSend[this0x" << this << "] argument err! len=" << len);
        return false;
    }
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(dstip);
    addr.sin_port = htons(dstport);
    int ret =sendto(_eventData._fd, buf, len, 0, (sockaddr*)&addr, sizeof(addr));
    if (ret == -1 )
    {
        LOGW("sendto error. errno=" << errno);
    }
    
    return true;
}


bool UdpSocket::doRecvFrom(char * buf, unsigned int len, _OnRecvFromHandler&& handler)
{
    if (!_summer || _eventData._linkstat != LS_ESTABLISHED)
    {
        LCE("UdpSocket::doRecv[this0x" << this << "] _summer not bind!");
        return false;
    }
    if (len == 0 || len >1500)
    {
        LCE("UdpSocket::doRecv[this0x" << this << "] argument err! len=" << len);
        return false;
    }
    if (len == 0 )
    {
        LCE("UdpSocket::doRecv[this0x" << this << "] argument err !!!  len==0");
        return false;
    }
    
    if (_recvBuf || _recvLen != 0 || _onRecvFromHandler)
    {
        LCE("UdpSocket::doRecv[this0x" << this << "] (_recvBuf != nullptr || _recvLen != 0) == TRUE");
        return false;
    }

    
    _recvBuf = buf;
    _recvLen = len;
    _eventData._event.events = _eventData._event.events|EPOLLIN;
    _summer->registerEvent(EPOLL_CTL_MOD, _eventData);
    _onRecvFromHandler = std::move(handler);
    return true;
}


bool UdpSocket::onEPOLLMessage(uint32_t event)
{
    if (!_onRecvFromHandler)
    {
        LCE("UdpSocket::onEPOLLMessage[this0x" << this << "] unknown error");
        return false;
    }

    sockaddr_in raddr;
    memset(&raddr, 0, sizeof(raddr));
    socklen_t len = sizeof(raddr);
    int ret = recvfrom(_eventData._fd, _recvBuf, _recvLen, 0, (sockaddr*)&raddr, &len);

    if (event & EPOLLHUP || event & EPOLLERR || ret == 0 ||(ret ==-1 && (errno !=EAGAIN && errno != EWOULDBLOCK)))
    {
        LCE("UdpSocket::onEPOLLMessage[this0x" << this << "] EPOLLHUP  EPOLLERR error. _eventData fd=" << _eventData << ", events=" << event);
        _OnRecvFromHandler onRecv(std::move(_onRecvFromHandler));
        _eventData._event.events = _eventData._event.events&~EPOLLIN;
        _summer->registerEvent(EPOLL_CTL_DEL, _eventData);
        close(_eventData._fd);
        _eventData._linkstat = LS_CLOSED;
        onRecv(NEC_ERROR, "", 0, 0);
        return false;
    }
    else if (ret != -1)
    {
        _OnRecvFromHandler onRecv(std::move(_onRecvFromHandler));
        _eventData._event.events = _eventData._event.events&~EPOLLIN;
        _summer->registerEvent(EPOLL_CTL_MOD, _eventData);
        _recvBuf = nullptr;
        _recvLen = 0;
        onRecv(NEC_SUCCESS, inet_ntoa(raddr.sin_addr), ntohs(raddr.sin_port), ret);
    }
    return true;
}


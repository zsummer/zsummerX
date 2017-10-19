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


#include <zsummerX/epoll/tcpsocket_impl.h>
#include <zsummerX/epoll/tcpaccept_impl.h>


using namespace zsummer::network;




TcpAccept::TcpAccept()
{
    _eventData._event.events = 0;
    _eventData._event.data.ptr = &_eventData;
    _eventData._type = EventData::REG_TCP_ACCEPT;
}


TcpAccept::~TcpAccept()
{
    if (_eventData._fd != InvalidFD)
    {
        ::close(_eventData._fd);
        _eventData._fd = InvalidFD;
    }
}
std::string TcpAccept::logSection()
{
    std::stringstream os;
    os << " TcpAccept:Status _summer=" << _summer.use_count() << ", _listenIP=" << _listenIP
        << ", _listenPort=" << _listenPort << ", _onAcceptHandler=" << (bool)_onAcceptHandler
        << ", _client=" << _client.use_count() << "_eventData=" << _eventData;
    return os.str();
}
bool TcpAccept::initialize(const EventLoopPtr & summer)
{
    _summer = summer;
    _eventData._linkstat = LS_WAITLINK;
    return true;
}


bool TcpAccept::openAccept(const std::string & listenIP, unsigned short listenPort, bool reuse)
{
    if (!_summer || _eventData._linkstat != LS_WAITLINK)
    {
        LCE("TcpAccept::openAccept[this0x" << this << "]  not initialize!" );
        return false;
    }
    _listenIP = listenIP;
    _listenPort = listenPort;
    _isIPV6 = listenIP.find(':') != std::string::npos;
    if (_isIPV6)
    {

        _eventData._fd = socket(AF_INET6, SOCK_STREAM, 0);
    }
    else
    {
        _eventData._fd = socket(AF_INET, SOCK_STREAM, 0);
    }

    if (_eventData._fd == InvalidFD)
    {
        LCF("TcpAccept::openAccept[this0x" << this << "] listen socket create err errno =" << strerror(errno));
        return false;
    }

    setNonBlock(_eventData._fd);

    if (reuse)
    {
        setReuse(_eventData._fd);
    }


    if (_isIPV6)
    {
        sockaddr_in6 addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin6_family = AF_INET6;
        addr.sin6_addr = in6addr_any;
        if (!listenIP.empty() && listenIP != "::")
        {
            inet_pton(AF_INET6, listenIP.c_str(), &addr.sin6_addr);
        }
        addr.sin6_port = htons(listenPort);
        if (bind(_eventData._fd, (sockaddr *)&addr, sizeof(addr)) != 0) {
            LCF("TcpAccept::openAccept[this0x" << this << "] listen socket bind err, errno = " << strerror(errno) );
            ::close(_eventData._fd);
            _eventData._fd = InvalidFD;
            return false;
        }
    }
    else
    {
        sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        if (!listenIP.empty())
        {
            addr.sin_addr.s_addr = inet_addr(listenIP.c_str());
        }
        addr.sin_port = htons(listenPort);
        if (bind(_eventData._fd, (sockaddr *)&addr, sizeof(addr)) != 0) {
            LCF("TcpAccept::openAccept[this0x" << this << "] listen socket bind err, errno=" << strerror(errno));
            ::close(_eventData._fd);
            _eventData._fd = InvalidFD;
            return false;
        }
    }

    if (listen(_eventData._fd, 200) != 0)
    {
        LCF("TcpAccept::openAccept[this0x" << this << "] listen socket listen err, errno=" << strerror(errno) );
        ::close(_eventData._fd);
        _eventData._fd = InvalidFD;
        return false;
    }

    _eventData._tcpacceptPtr = shared_from_this();
    _summer->registerEvent(EPOLL_CTL_ADD, _eventData);
    _eventData._linkstat = LS_ESTABLISHED;
    return true;
}

bool TcpAccept::doAccept(const TcpSocketPtr &s, _OnAcceptHandler &&handle)
{
    if (_onAcceptHandler)
    {
        LCF("TcpAccept::doAccept[this0x" << this << "] err, dumplicate doAccept" << logSection());
        return false;
    }
    if (_eventData._linkstat != LS_ESTABLISHED)
    {
        LCF("TcpAccept::doAccept[this0x" << this << "] err, _linkstat not work state");
        return false;
    }
    
    _eventData._event.events = EPOLLIN;
    _summer->registerEvent(EPOLL_CTL_MOD, _eventData);
    _onAcceptHandler = std::move(handle);
    _client = s;
    return true;
}
void TcpAccept::onEPOLLMessage(uint32_t event)
{
    //
    if (!_onAcceptHandler)
    {
        LCF("TcpAccept::doAccept[this0x" << this << "] err, dumplicate doAccept" << logSection());
        return ;
    }
    if (_eventData._linkstat != LS_ESTABLISHED)
    {
        LCF("TcpAccept::doAccept[this0x" << this << "] err, _linkstat not work state" );
        return ;
    }

    _OnAcceptHandler onAccept(std::move(_onAcceptHandler));
    TcpSocketPtr ps(std::move(_client));
    _eventData._event.events = 0;
    _summer->registerEvent(EPOLL_CTL_MOD, _eventData);

    if (event &  EPOLLERR || event & EPOLLHUP)
    {
        LCE("TcpAccept::doAccept[this0x" << this << "]  EPOLLERR, errno=" << strerror(errno) );
        auto guard = shared_from_this();
        close();
        onAccept(NEC_ERROR, ps);
        return ;
    }
    else if (event & EPOLLIN)
    {
        int fd = 0;
        char remoteIP[50] = { 0 };
        unsigned remotePort = 0;
        if (_isIPV6)
        {
            sockaddr_in6 cltaddr;
            memset(&cltaddr, 0, sizeof(cltaddr));
            socklen_t len = sizeof(cltaddr);
            fd = accept(_eventData._fd, (sockaddr *)&cltaddr, &len);
            inet_ntop(AF_INET6, &cltaddr.sin6_addr, remoteIP, 50);
            remotePort = ntohs(cltaddr.sin6_port);
        }
        else
        {
            sockaddr_in cltaddr;
            memset(&cltaddr, 0, sizeof(cltaddr));
            socklen_t len = sizeof(cltaddr);
            fd = accept(_eventData._fd, (sockaddr *)&cltaddr, &len);
            inet_ntop(AF_INET, &cltaddr.sin_addr, remoteIP, 50);
            remotePort = ntohs(cltaddr.sin_port);
        }

        if (fd == -1)
        {
            if (errno != EAGAIN && errno != EWOULDBLOCK)
            {
                LCE("TcpAccept::doAccept[this0x" << this << "] ERR: accept return -1, errno=" << strerror(errno));
            }
            auto guard = shared_from_this();
            close();
            onAccept(NEC_ERROR, ps);
            return ;
        }

        ps->attachSocket(fd, remoteIP, remotePort, _isIPV6);
        onAccept(NEC_SUCCESS, ps);
        return;
    }
    return ;
}

bool TcpAccept::close()
{
    auto guard = shared_from_this();
    if (_eventData._linkstat != LS_CLOSED)
    {
        LCI("TcpAccept::close. socket=" << _eventData._fd);
        _eventData._linkstat = LS_CLOSED;
        _summer->registerEvent(EPOLL_CTL_DEL, _eventData);
        _onAcceptHandler = nullptr;
        shutdown(_eventData._fd, SHUT_RDWR);
        ::close(_eventData._fd);
        _eventData._fd = InvalidFD;
        _eventData._tcpacceptPtr.reset();
        return true;
    }

    return true;
}


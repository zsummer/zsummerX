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


#include <zsummerX/select/tcpsocket_impl.h>
#include <zsummerX/select/tcpaccept_impl.h>


using namespace zsummer::network;




TcpAccept::TcpAccept()
{
}

TcpAccept::~TcpAccept()
{
    if (_fd != InvalidFD)
    {
        ::close(_fd);
        _fd = InvalidFD;
    }
}
std::string TcpAccept::AcceptSection()
{
    std::stringstream os;
    os << " TcpAccept:Status _summer=" << _summer.use_count() << ", _listenIP=" << _listenIP
        << ", _listenPort=" << _listenPort << ", _onAcceptHandler=" << (bool)_onAcceptHandler
        << ", _client=" << _client.use_count() ;
    return os.str();
}
bool TcpAccept::initialize(const EventLoopPtr &summer)
{
    _summer = summer;
    _linkstat = LS_WAITLINK;
    return true;
}


bool TcpAccept::openAccept(const std::string& listenIP, unsigned short listenPort, bool reuse)
{
    if (!_summer || _linkstat != LS_WAITLINK)
    {
        LCE("TcpAccept::openAccept[this0x" << this << "] accepter not initialize!" << AcceptSection());
        return false;
    }
    _listenIP = listenIP;
    _listenPort = listenPort;
    _isIPV6 = listenIP.find(':') != std::string::npos;
    if (_isIPV6)
    {

        _fd = socket(AF_INET6, SOCK_STREAM, 0);
    }
    else
    {
        _fd = socket(AF_INET, SOCK_STREAM, 0);
    }
    if (_fd == InvalidFD)
    {
        LCF("TcpAccept::openAccept[this0x" << this << "] listen socket create err " << OSTREAM_GET_LASTERROR << ", " << AcceptSection());
        return false;
    }

    setNonBlock(_fd);

    if (reuse)
    {
        setReuse(_fd);
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
        if (bind(_fd, (sockaddr *) &addr, sizeof(addr)) != 0) {
            LCF("TcpAccept::openAccept[this0x" << this << "] listen socket bind err, " << OSTREAM_GET_LASTERROR <<
                ", " << AcceptSection());
            ::close(_fd);
            _fd = InvalidFD;
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
        if (bind(_fd, (sockaddr *) &addr, sizeof(addr)) != 0) {
            LCF("TcpAccept::openAccept[this0x" << this << "] listen socket bind err, " << OSTREAM_GET_LASTERROR <<
                ", " << AcceptSection());
            ::close(_fd);
            _fd = InvalidFD;
            return false;
        }
    }
    if (listen(_fd, 200) != 0)
    {
        LCF("TcpAccept::openAccept[this0x" << this << "] listen socket listen err, " << OSTREAM_GET_LASTERROR << ", " << AcceptSection());
        ::close(_fd);
        _fd = InvalidFD;
        return false;
    }

    _summer->addTcpAccept(_fd, shared_from_this());

    _linkstat = LS_ESTABLISHED;
    return true;
}

bool TcpAccept::doAccept(const TcpSocketPtr &s, _OnAcceptHandler && handle)
{
    if (_onAcceptHandler)
    {
        LCF("TcpAccept::doAccept[this0x" << this << "] err, dumplicate doAccept" << AcceptSection());
        return false;
    }
    if (_linkstat != LS_ESTABLISHED)
    {
        LCF("TcpAccept::doAccept[this0x" << this << "] err, _linkstat not work state" << AcceptSection());
        return false;
    }
    _summer->setEvent(_fd, 0);
    _onAcceptHandler = std::move(handle);
    _client = s;
    
    return true;
}

void TcpAccept::onSelectMessage()
{
    auto guard = shared_from_this();

    if (!_onAcceptHandler)
    {
        LCF("TcpAccept::onSelectMessage[this0x" << this << "] err, dumplicate doAccept" << AcceptSection());
        return ;
    }

    if (_linkstat != LS_ESTABLISHED)
    {
        LCF("TcpAccept::onSelectMessage[this0x" << this << "] err, _linkstat not work state" << AcceptSection());
        return ;
    }
    _OnAcceptHandler onAccept(std::move(_onAcceptHandler));

    TcpSocketPtr ps(std::move(_client));
    _summer->unsetEvent(_fd, 0);
    int fd = 0;
    char remoteIP[50]={0};
    unsigned remotePort = 0;
    if (_isIPV6)
    {
        sockaddr_in6 cltaddr;
        memset(&cltaddr, 0, sizeof(cltaddr));
        socklen_t len = sizeof(cltaddr);
        fd = ::accept(_fd, (sockaddr *) &cltaddr, &len);
        inet_ntop(AF_INET6, &cltaddr.sin6_addr, remoteIP, 50);
        remotePort = ntohs(cltaddr.sin6_port);
    }
    else
    {
        sockaddr_in cltaddr;
        memset(&cltaddr, 0, sizeof(cltaddr));
        socklen_t len = sizeof(cltaddr);
        fd = ::accept(_fd, (sockaddr *) &cltaddr, &len);
        inet_ntop(AF_INET, &cltaddr.sin_addr, remoteIP, 50);
        remotePort = ntohs(cltaddr.sin_port);
    }
    if (fd == -1)
    {
        if (!IS_WOULDBLOCK)
        {
            LCE("TcpAccept::onSelectMessage[this0x" << this << "] ERR: accept return -1, " OSTREAM_GET_LASTERROR << AcceptSection());
        }
        onAccept(NEC_ERROR, ps);
        return ;
    }

    setNonBlock(fd);

    ps->attachSocket(fd, remoteIP, remotePort, _isIPV6);
    onAccept(NEC_SUCCESS, ps);
    
    return ;
}

bool TcpAccept::close()
{
    auto guard = shared_from_this();
    if(_linkstat != LS_CLOSED)
    {
        LCI("TcpAccept::close. socket=" << _fd);
        _linkstat = LS_CLOSED;
        _onAcceptHandler = nullptr;
        if (_fd != InvalidFD)
        {
            _summer->clearSocket(_fd);
            shutdown(_fd, SHUT_RDWR);
            ::close(_fd);
            _fd = InvalidFD;
        }
    }

    return true;
}


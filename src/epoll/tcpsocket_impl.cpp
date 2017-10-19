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

using namespace zsummer::network;



TcpSocket::TcpSocket()
{
    _eventData._event.data.ptr = &_eventData;
    _eventData._event.events = 0;
    _eventData._type = EventData::REG_TCP_SOCKET;
    g_appEnvironment.addCreatedSocketCount();
}


TcpSocket::~TcpSocket()
{
    g_appEnvironment.addClosedSocketCount();
    if (_onRecvHandler || _onSendHandler || _onConnectHandler)
    {
        LCW("TcpSocket::~TcpSocket[this0x" << this << "] Handler status error. " << logSection());
    }
    if (_eventData._fd != InvalidFD)
    {
        ::close(_eventData._fd);
        _eventData._fd = InvalidFD;
    }
}

std::string TcpSocket::logSection()
{
    std::stringstream os;
    os << ";; Status: summer.user_count()=" << _summer.use_count() << ", remoteIP=" << _remoteIP << ", remotePort=" << _remotePort
        << ", _onConnectHandler = " << (bool)_onConnectHandler 
        << ", _onRecvHandler = " << (bool)_onRecvHandler << ", _recvBuf=" << (void*)_recvBuf << ", _recvLen=" << _recvLen 
        << ", _onSendHandler = " << (bool)_onSendHandler << ", _sendBuf=" << (void*)_sendBuf << ", _sendLen=" << _sendLen
        << "; _eventData=" << _eventData;
    return os.str();
}

bool TcpSocket::setNoDelay()
{
    return zsummer::network::setNoDelay(_eventData._fd);
}

bool TcpSocket::initialize(const EventLoopPtr& summer)
{

    if (_eventData._linkstat == LS_ATTACHED)
    {
        _summer = summer;
        _summer->registerEvent(EPOLL_CTL_ADD, _eventData);
        _eventData._tcpSocketPtr = shared_from_this();
        _eventData._linkstat = LS_ESTABLISHED;
        setNonBlock(_eventData._fd);
        return true;
    }
    else if (_eventData._linkstat == LS_UNINITIALIZE)
    {
        _summer = summer;
        _eventData._linkstat = LS_WAITLINK;
        return true;
    }

    LCE("TcpSocket::initialize[this0x" << this << "] fd aready used!" << logSection());
    return true;
}
bool TcpSocket::attachSocket(int fd, const std::string & remoteIP, unsigned short remotePort, bool isIPV6)
{
    _eventData._fd = fd;
    _remoteIP = remoteIP;
    _remotePort = remotePort;
    _isIPV6 = isIPV6;
    _eventData._linkstat = LS_ATTACHED;
    return true;
}




bool TcpSocket::doConnect(const std::string& remoteIP, unsigned short remotePort, _OnConnectHandler && handler)
{
    if (!_summer || _eventData._linkstat != LS_WAITLINK || _onConnectHandler)
    {
        LCE("TcpSocket::doConnect[this0x" << this << "] status error!" );
        return false;
    }
    _remoteIP = remoteIP;
    _remotePort = remotePort;

    _isIPV6 = remoteIP.find(':') != std::string::npos;
    int ret = -1;
    if (_isIPV6)
    {
        _eventData._fd = socket(AF_INET6, SOCK_STREAM, 0);
        if (_eventData._fd == -1)
        {
            LCE("TcpSocket::doConnect[this0x" << this << "] fd create failed!");
            return false;
        }
        setNonBlock(_eventData._fd);
        _remoteIP = remoteIP;
        _remotePort = remotePort;
        sockaddr_in6 addr;
        addr.sin6_family = AF_INET6;
        inet_pton(AF_INET6, remoteIP.c_str(), &addr.sin6_addr);
        addr.sin6_port = htons(_remotePort);
        ret = connect(_eventData._fd, (sockaddr *)&addr, sizeof(addr));
    }
    else
    {

        _eventData._fd = socket(AF_INET, SOCK_STREAM, 0);
        if (_eventData._fd == -1)
        {
            LCE("TcpSocket::doConnect[this0x" << this << "] fd create failed!");
            return false;
        }
        setNonBlock(_eventData._fd);
        _remoteIP = remoteIP;
        _remotePort = remotePort;
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(_remoteIP.c_str());
        addr.sin_port = htons(_remotePort);
        ret = connect(_eventData._fd, (sockaddr *)&addr, sizeof(addr));
    }

    if (ret!=0 && errno != EINPROGRESS)
    {
        LCW("TcpSocket::doConnect[this0x" << this << "] ::connect error. errno=" << strerror(errno) );
        ::close(_eventData._fd);
        _eventData._fd = InvalidFD;
        return false;
    }

    _eventData._event.events = EPOLLOUT;
    _summer->registerEvent(EPOLL_CTL_ADD, _eventData);
    _eventData._tcpSocketPtr = shared_from_this();
    _onConnectHandler = std::move(handler);
    return true;
}


bool TcpSocket::doSend(char * buf, unsigned int len, _OnSendHandler && handler)
{
    LCT("TcpSocket::doSend len=" << len);
    if (_eventData._linkstat != LS_ESTABLISHED)
    {
        LCW("TcpSocket::doSend[this0x" << this << "] _linkstat error!");
        return false;
    }

    if (!_summer)
    {
        LCE("TcpSocket::doSend[this0x" << this << "] _summer not bind!" << logSection());
        return false;
    }

    if (len == 0)
    {
        LCE("TcpSocket::doSend[this0x" << this << "] argument err! len ==0" );
        return false;
    }
    if (_sendBuf != NULL || _sendLen != 0)
    {
        LCE("TcpSocket::doSend[this0x" << this << "] (_sendBuf != NULL || _sendLen != 0) == TRUE" << logSection());
        return false;
    }
    if (_onSendHandler)
    {
        LCE("TcpSocket::doSend[this0x" << this << "] _onSendHandler == TRUE" << logSection());
        return false;
    }

    int ret = 0;
    if (!_floodSendOptimize)
    {
        ret = send(_eventData._fd, buf, len, 0);
    }
    if (ret <= 0)
    {
        _sendBuf = buf;
        _sendLen = len;


        _eventData._event.events = _eventData._event.events|EPOLLOUT;
        _summer->registerEvent(EPOLL_CTL_MOD, _eventData);
        _onSendHandler = std::move(handler);
    }
    else
    {
        LCT("TcpSocket::doSend direct sent=" << ret);
        _OnSendHandler onSend(std::move(handler));
        onSend(NEC_SUCCESS, ret);
    }
    return true;
}


bool TcpSocket::doRecv(char * buf, unsigned int len, _OnRecvHandler && handler, bool daemonRecv)
{
    if (_eventData._linkstat != LS_ESTABLISHED)
    {
        LCW("TcpSocket::doRecv[this0x" << this << "] status error !" );
        return false;
    }

    if (!_summer)
    {
        LCE("TcpSocket::doRecv[this0x" << this << "] _summer not bind!" << logSection());
        return false;
    }

    if (len == 0 )
    {
        LCE("TcpSocket::doRecv[this0x" << this << "] argument err !!!  len==0" );
        return false;
    }
    if (_recvBuf != NULL || _recvLen != 0)
    {
        LCE("TcpSocket::doRecv[this0x" << this << "]  (_recvBuf != NULL || _recvLen != 0) == TRUE" << logSection());
        return false;
    }
    if (_onRecvHandler)
    {
        LCE("TcpSocket::doRecv[this0x" << this << "] (_onRecvHandler) == TRUE" << logSection());
        return false;
    }
    if (_daemonRecv)
    {
        LCE("TcpSocket::doRecv[this0x" << this << "] already open daemon recv" << logSection());
        return false;
    }
    if (daemonRecv)
    {
        _daemonRecv = daemonRecv;
    }
    
    _recvBuf = buf;
    _recvLen = len;
    _recvOffset = 0;

    _eventData._event.events = _eventData._event.events|EPOLLIN;
    _summer->registerEvent(EPOLL_CTL_MOD, _eventData);
    _onRecvHandler = std::move(handler);
    return true;
}


void TcpSocket::onEPOLLMessage(uint32_t event)
{
    NetErrorCode ec = NEC_ERROR;
    if (!_onRecvHandler && !_onSendHandler && !_onConnectHandler)
    {
        LCE("TcpSocket::onEPOLLMessage[this0x" << this << "] unknown error. errno=" << strerror(errno) << logSection());
        return ;
    }
    if (_eventData._linkstat != LS_WAITLINK && _eventData._linkstat != LS_ESTABLISHED)
    {
        LCE("TcpSocket::onEPOLLMessage[this0x" << this << "] _linkstat error. _linkstat=" << _eventData._linkstat << logSection());
        return;
    }

    if (_eventData._linkstat == LS_WAITLINK)
    {
        auto guard = shared_from_this();
        _OnConnectHandler onConnect(std::move(_onConnectHandler));
        if (event & EPOLLERR || event & EPOLLHUP)
        {
            doClose();
            onConnect(NEC_ERROR);
            return;
        }
        else if(event & EPOLLOUT)
        {
            _eventData._event.events = 0;
            _summer->registerEvent(EPOLL_CTL_MOD, _eventData);
            _eventData._linkstat = LS_ESTABLISHED;
            onConnect(NEC_SUCCESS);
            return;
        }
        return ;
    }

    if (_onRecvHandler)
    {
        int ret = -1;
        errno = EAGAIN;
        if (event & EPOLLIN)
        {
            ret = recv(_eventData._fd, _recvBuf + _recvOffset, _recvLen - _recvOffset, 0);
        }
        if (event & EPOLLHUP || event & EPOLLERR || ret == 0 || (ret == -1 && (errno != EAGAIN && errno != EWOULDBLOCK) ) )
        {
            _eventData._event.events = _eventData._event.events &~EPOLLIN;
            _summer->registerEvent(EPOLL_CTL_MOD, _eventData);
            if (ret == 0 || event & EPOLLHUP)
            {
                ec = NEC_REMOTE_CLOSED;
            }
            else
            {
                ec = NEC_ERROR;
            }
            _OnRecvHandler onRecv(std::move(_onRecvHandler));
            doClose();
            onRecv(ec, 0);
            return ;
        }
        else if (ret != -1)
        {
            if (_daemonRecv)
            {
                auto guard = shared_from_this();
                _recvOffset = _onRecvHandler(NEC_SUCCESS,ret);
            }
            else
            {
                _eventData._event.events = _eventData._event.events &~EPOLLIN;
                _summer->registerEvent(EPOLL_CTL_MOD, _eventData);
                auto guard = shared_from_this();
                _OnRecvHandler onRecv(std::move(_onRecvHandler));
                _recvBuf = NULL;
                _recvLen = 0;
                _recvOffset = 0;
                onRecv(NEC_SUCCESS,ret);
            }

            if (_eventData._linkstat == LS_CLOSED)
            {
                return;
            }
        }
    }

    if (_onSendHandler)
    {
        int ret = -1;
        errno = EAGAIN;
        if (event & EPOLLOUT)
        {
            ret = send(_eventData._fd, _sendBuf, _sendLen, 0);
        }

        if (event & EPOLLERR || event & EPOLLHUP || (ret == -1 && (errno != EAGAIN && errno != EWOULDBLOCK)))
        {
            _eventData._event.events = _eventData._event.events &~EPOLLOUT;
            _summer->registerEvent(EPOLL_CTL_MOD, _eventData);
            LCE("TcpSocket::onEPOLLMessage[this0x" << this << "] send error. errno=" << strerror(errno) );
            _onSendHandler = nullptr;
            return ;
        }
        else if (ret != -1)
        {
            auto guard = shared_from_this();
            _eventData._event.events = _eventData._event.events &~EPOLLOUT;
            _summer->registerEvent(EPOLL_CTL_MOD, _eventData);
            _OnSendHandler onSend(std::move(_onSendHandler));
            _sendBuf = NULL;
            _sendLen = 0;
            onSend(NEC_SUCCESS, ret);
            if (_eventData._linkstat == LS_CLOSED)
            {
                return;
            }
        }
    }
    return ;
}


bool TcpSocket::doClose()
{
    if (_eventData._linkstat != LS_CLOSED)
    {
        _eventData._linkstat = LS_CLOSED;
        _summer->registerEvent(EPOLL_CTL_DEL, _eventData);
        if (_eventData._fd != InvalidFD)
        {
            shutdown(_eventData._fd, SHUT_RDWR);
            close(_eventData._fd);
            _eventData._fd = InvalidFD;
        }
        _onConnectHandler = nullptr;
        _onRecvHandler = nullptr;
        _onSendHandler = nullptr;
        _eventData._tcpSocketPtr.reset();
    }
    return true;
}


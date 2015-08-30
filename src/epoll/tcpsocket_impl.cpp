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
        << ", _onRecvHandler = " << (bool)_onRecvHandler << ", _pRecvBuf=" << (void*)_pRecvBuf << ", _iRecvLen=" << _iRecvLen 
        << ", _onSendHandler = " << (bool)_onSendHandler << ", _pSendBuf=" << (void*)_pSendBuf << ", _iSendLen=" << _iSendLen
        << "; _eventData=" << _eventData;
    return os.str();
}

bool TcpSocket::setNoDelay()
{
    return zsummer::network::setNoDelay(_eventData._fd);
}

bool TcpSocket::initialize(const EventLoopPtr& summer)
{
     _summer = summer;
    if (_eventData._linkstat != LS_UNINITIALIZE)
    {
        if (!_summer->registerEvent(EPOLL_CTL_ADD, _eventData))
        {
            LCE("TcpSocket::initialize[this0x" << this << "] socket already used or not initilize." << logSection());
            return false;
        }
        _eventData._linkstat = LS_ESTABLISHED;
    }
    else
    {
        if (_eventData._fd != -1)
        {
            LCE("TcpSocket::initialize[this0x" << this << "] fd aready used!" << logSection());
            return false;
        }
        _eventData._fd = socket(AF_INET, SOCK_STREAM, 0);
        if (_eventData._fd == -1)
        {
            LCE("TcpSocket::initialize[this0x" << this << "] fd create failed!" << logSection());
            return false;
        }
        _eventData._linkstat = LS_WAITLINK;
    }
    setNonBlock(_eventData._fd);

    return true;
}
bool TcpSocket::attachSocket(int s, const std::string & remoteIP, unsigned short remotePort)
{
    _eventData._fd = s;
    _remoteIP = remoteIP;
    _remotePort = remotePort;
    _eventData._linkstat = LS_WAITLINK;
    return true;
}




bool TcpSocket::doConnect(const std::string& remoteIP, unsigned short remotePort, _OnConnectHandler && handler)
{
    if (!_summer)
    {
        LCE("TcpSocket::doConnect[this0x" << this << "] summer not bind!" << logSection());
        return false;
    }
    if (_eventData._linkstat != LS_WAITLINK)
    {
        LCE("TcpSocket::doConnect[this0x" << this << "] _linkstat not LS_WAITLINK!" << logSection());
        return false;
    }

    _eventData._event.events = EPOLLOUT;

    _remoteIP = remoteIP;
    _remotePort = remotePort;
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(_remoteIP.c_str());
    addr.sin_port = htons(_remotePort);
    
    
    int ret = connect(_eventData._fd, (sockaddr *) &addr, sizeof(addr));
    if (ret!=0 && errno != EINPROGRESS)
    {
        LCW("TcpSocket::doConnect[this0x" << this << "] ::connect error. errno=" << strerror(errno) << logSection());
        ::close(_eventData._fd);
        _eventData._fd = InvalidFD;
        return false;
    }
    if (!_summer->registerEvent(EPOLL_CTL_ADD, _eventData))
    {
        LCW("TcpSocket::initialize[this0x" << this << "] socket already used or not initilize." << logSection());
        return false;
    }
    _onConnectHandler = std::move(handler);
    _eventData._tcpSocketConnectPtr = shared_from_this();
    return true;
}


bool TcpSocket::doSend(char * buf, unsigned int len, _OnSendHandler && handler)
{
    if (_eventData._linkstat != LS_ESTABLISHED)
    {
        LCW("TcpSocket::doSend[this0x" << this << "] _linkstat not REG_ESTABLISHED_TCP!" << logSection());
        return false;
    }

    if (!_summer)
    {
        LCE("TcpSocket::doSend[this0x" << this << "] _summer not bind!" << logSection());
        return false;
    }

    if (len == 0)
    {
        LCE("TcpSocket::doSend[this0x" << this << "] argument err! len ==0" << logSection());
        return false;
    }
    if (_pSendBuf != NULL || _iSendLen != 0)
    {
        LCE("TcpSocket::doSend[this0x" << this << "] (_pSendBuf != NULL || _iSendLen != 0) == TRUE" << logSection());
        return false;
    }
    if (_onSendHandler)
    {
        LCE("TcpSocket::doSend[this0x" << this << "] _onSendHandler == TRUE" << logSection());
        return false;
    }

    
    
    _pSendBuf = buf;
    _iSendLen = len;
    

    _eventData._event.events = _eventData._event.events|EPOLLOUT;
    if (!_summer->registerEvent(EPOLL_CTL_MOD, _eventData))
    {
        LCW("TcpSocket::doSend[this0x" << this << "] registerEvent Error" << logSection());
        _pSendBuf = nullptr;
        _iSendLen = 0;
        return false;
    }
    _onSendHandler = std::move(handler);
    _eventData._tcpSocketSendPtr = shared_from_this();
    return true;
}


bool TcpSocket::doRecv(char * buf, unsigned int len, _OnRecvHandler && handler)
{
    if (_eventData._linkstat != LS_ESTABLISHED)
    {
        LCW("TcpSocket::doRecv[this0x" << this << "] type not REG_ESTABLISHED_TCP!" << logSection());
        return false;
    }

    if (!_summer)
    {
        LCE("TcpSocket::doRecv[this0x" << this << "] _summer not bind!" << logSection());
        return false;
    }

    if (len == 0 )
    {
        LCE("TcpSocket::doRecv[this0x" << this << "] argument err !!!  len==0" << logSection());
        return false;
    }
    if (_pRecvBuf != NULL || _iRecvLen != 0)
    {
        LCE("TcpSocket::doRecv[this0x" << this << "]  (_pRecvBuf != NULL || _iRecvLen != 0) == TRUE" << logSection());
        return false;
    }
    if (_onRecvHandler)
    {
        LCE("TcpSocket::doRecv[this0x" << this << "] (_onRecvHandler) == TRUE" << logSection());
        return false;
    }
    
    _pRecvBuf = buf;
    _iRecvLen = len;
    

    _eventData._event.events = _eventData._event.events|EPOLLIN;
    if (!_summer->registerEvent(EPOLL_CTL_MOD, _eventData))
    {
        LCW("TcpSocket::doRecv[this0x" << this << "] registerEvent Error" << logSection());
        _pRecvBuf = nullptr;
        _iRecvLen = 0;
        doClose();
        return false;
    }
    _eventData._tcpSocketRecvPtr = shared_from_this();
    _onRecvHandler = std::move(handler);
    return true;
}


void TcpSocket::onEPOLLMessage(int flag, bool err)
{
    unsigned char linkstat = _eventData._linkstat;
    NetErrorCode ec = NEC_ERROR;

    if (!_onRecvHandler && !_onSendHandler && linkstat != LS_WAITLINK)
    {
        LCE("TcpSocket::onEPOLLMessage[this0x" << this << "] unknown error. errno=" << strerror(errno) << logSection());
        return ;
    }

    if (linkstat == LS_WAITLINK)
    {
        std::shared_ptr<TcpSocket> guard(std::move(_eventData._tcpSocketConnectPtr));
        _OnConnectHandler onConnect(std::move(_onConnectHandler));

        if (flag & EPOLLOUT && !err)
        {
            _eventData._event.events = /*EPOLLONESHOT*/ 0;
            _summer->registerEvent(EPOLL_CTL_MOD, _eventData);
            _eventData._linkstat = LS_ESTABLISHED;
            onConnect(NEC_SUCCESS);
            return;
        }
        else 
        {
            _eventData._linkstat = LS_WAITLINK;
            _summer->registerEvent(EPOLL_CTL_DEL, _eventData);
            onConnect(NEC_ERROR);
            return;
        }
        return ;
    }

    if (flag & EPOLLIN && _onRecvHandler)
    {
        std::shared_ptr<TcpSocket> guard(std::move(_eventData._tcpSocketRecvPtr));
        int ret = recv(_eventData._fd, _pRecvBuf, _iRecvLen, 0);
        _eventData._event.events = _eventData._event.events &~EPOLLIN;

        if (!_summer->registerEvent(EPOLL_CTL_MOD, _eventData))
        {
            LCF("TcpSocket::onEPOLLMessage[this0x" << this << "] connect true & EPOLLMod error.  errno=" << strerror(errno) << logSection());
        }
        if (ret == 0 || 
            (ret == -1 && (errno != EAGAIN && errno != EWOULDBLOCK)) || err)
        {
            ec = NEC_REMOTE_CLOSED;
            _eventData._linkstat = LS_CLOSED;
            if (_onRecvHandler)
            {
                _OnRecvHandler onRecv(std::move(_onRecvHandler));
                onRecv(ec, 0);
            }
            if (!_onSendHandler && !_onRecvHandler)
            {
                if (!_summer->registerEvent(EPOLL_CTL_DEL, _eventData))
                {
                    LCW("TcpSocket::onEPOLLMessage[this0x" << this << "] connect true & EPOLL DEL error.  errno=" << strerror(errno) << logSection());
                }
            }
            return ;
        }
        else if (ret != -1)
        {
            _OnRecvHandler onRecv(std::move(_onRecvHandler));
            _pRecvBuf = NULL;
            _iRecvLen = 0;
            onRecv(NEC_SUCCESS,ret);
        }
    }
    else if (flag & EPOLLOUT && _onSendHandler)
    {
        std::shared_ptr<TcpSocket> guard(std::move(_eventData._tcpSocketSendPtr));

        int ret = send(_eventData._fd, _pSendBuf, _iSendLen, 0);
        _eventData._event.events = _eventData._event.events &~EPOLLOUT;
        if (!_summer->registerEvent(EPOLL_CTL_MOD, _eventData))
        {
            LCF("TcpSocket::onEPOLLMessage[this0x" << this << "] connect true & EPOLLMod error. errno=" << strerror(errno) << logSection());
        }
        
        if ((ret == -1 && (errno != EAGAIN && errno != EWOULDBLOCK)) || _eventData._linkstat == LS_CLOSED || err)
        {
            ec = NEC_ERROR;
            _eventData._linkstat = LS_CLOSED;
            _onSendHandler = nullptr;
            if (!_onSendHandler && !_onRecvHandler)
            {
                if (!_summer->registerEvent(EPOLL_CTL_DEL, _eventData))
                {
                    LCW("TcpSocket::onEPOLLMessage[this0x" << this << "] connect true & EPOLL DEL error.  errno=" << strerror(errno) << logSection());
                }
            }
            return ;
        }
        else if (ret != -1)
        {
            _OnSendHandler onSend(std::move(_onSendHandler));
            _pSendBuf = NULL;
            _iSendLen = 0;
            onSend(NEC_SUCCESS, ret);
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
        _eventData._tcpSocketConnectPtr.reset();
        _eventData._tcpSocketRecvPtr.reset();
        _eventData._tcpSocketSendPtr.reset();
    }
    return true;
}


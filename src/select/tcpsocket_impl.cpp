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


#include <zsummerX/select/tcpsocket_impl.h>

using namespace zsummer::network;




TcpSocket::TcpSocket()
{
    g_appEnvironment.addCreatedSocketCount();
}


TcpSocket::~TcpSocket()
{
    g_appEnvironment.addClosedSocketCount();
    if (_onRecvHandler || _onSendHandler || _onConnectHandler)
    {
        LCW("TcpSocket::~TcpSocket[this0x" << this << "] Handler status error. " << logSection());
    }
    if (_fd != InvalidFD)
    {
        ::close(_fd);
        _fd = InvalidFD;
    }
}

std::string TcpSocket::logSection()
{
    std::stringstream os;
    os << ";; Status: summer.user_count()=" << _summer.use_count() << ", remoteIP=" << _remoteIP << ", remotePort=" << _remotePort
        << ", _onConnectHandler = " << (bool)_onConnectHandler 
        << ", _onRecvHandler = " << (bool)_onRecvHandler << ", _pRecvBuf=" << (void*)_pRecvBuf << ", _iRecvLen=" << _iRecvLen 
        << ", _onSendHandler = " << (bool)_onSendHandler << ", _pSendBuf=" << (void*)_pSendBuf << ", _iSendLen=" << _iSendLen;
    return os.str();
}

bool TcpSocket::setNoDelay()
{
    return zsummer::network::setNoDelay(_fd);
}

bool TcpSocket::initialize(const EventLoopPtr & summer)
{
     _summer = summer;
    if (_linkstat == LS_ATTACHED)
    {
        _summer->addTcpSocket(_fd, shared_from_this());
        _linkstat = LS_ESTABLISHED;
        return true;
    }
    if(_linkstat == LS_UNINITIALIZE)
    {
        _fd = socket(AF_INET, SOCK_STREAM, 0);
        if (_fd == -1)
        {
            LCE("TcpSocket::initialize[this0x" << this << "] fd create failed!" << logSection());
            return false;
        }
        setNonBlock(_fd);
        _linkstat = LS_WAITLINK;
        return true;
    }
    LCE("TcpSocket::initialize error");
    return false;
}

bool TcpSocket::attachSocket(int fd, const std::string& remoteIP, unsigned short remotePort)
{
     _fd = fd;
     _linkstat = LS_ATTACHED;
     _remoteIP = remoteIP;
     _remotePort = remotePort;
    return true;
}




bool TcpSocket::doConnect(const std::string & remoteIP, unsigned short remotePort, _OnConnectHandler && handler)
{

    if (!_summer || _linkstat != LS_WAITLINK)
    {
        LCE("TcpSocket::doConnect[this0x" << this << "] summer not bind!" << logSection());
        return false;
    }
    _summer->addTcpSocket(_fd, shared_from_this());


    _register._wt = true;

    _remoteIP = remoteIP;
    _remotePort = remotePort;
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(_remoteIP.c_str());
    addr.sin_port = htons(_remotePort);
    
    
    int ret = connect(_register._fd, (sockaddr *) &addr, sizeof(addr));
#ifndef WIN32
    if (ret != 0 && errno != EINPROGRESS)
#else
    if (ret != 0 && WSAGetLastError() != WSAEWOULDBLOCK)
#endif
    {
        LCW("TcpSocket::doConnect[this0x" << this << "] ::connect error. " << OSTREAM_GET_LASTERROR << logSection());
        ::close(_register._fd);
        _register._fd = InvalidFD;
        return false;
    }
    
    _register._tcpSocketConnectPtr = shared_from_this();
    if (!_summer->registerEvent(0, _register))
    {
        LCE("TcpSocket::doConnect[this0x" << this << "] registerEvent Error" << logSection());
        ::close(_register._fd);
        _register._fd = InvalidFD;
        _register._tcpSocketConnectPtr.reset();
        return false;
    }
    _onConnectHandler = std::move(handler);
    return true;
}


bool TcpSocket::doSend(char * buf, unsigned int len, _OnSendHandler && handler)
{
    if (_register._linkstat != LS_ESTABLISHED)
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
        LCE("TcpSocket::doSend[this0x" << this << "] _pSendBuf =" << (void *) _pSendBuf << " _iSendLen =" << _iSendLen<< logSection());
        return false;
    }
    if (_onSendHandler)
    {
        LCE("TcpSocket::doSend[this0x" << this << "] _onSendHandler == TRUE" << logSection());
        return false;
    }

    
    
    _pSendBuf = buf;
    _iSendLen = len;
    

    _register._wt = true;
    _register._tcpSocketSendPtr = shared_from_this();
    if (!_summer->registerEvent(1, _register))
    {
        LCW("TcpSocket::doSend[this0x" << this << "] registerEvent Error" << logSection());
        _pSendBuf = nullptr;
        _iSendLen = 0;
        _register._tcpSocketSendPtr.reset();
        doClose();
        return false;
    }
    _onSendHandler = std::move(handler);
    
    return true;
}


bool TcpSocket::doRecv(char * buf, unsigned int len, _OnRecvHandler && handler)
{
    if (_register._linkstat != LS_ESTABLISHED)
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
    

    _register._rd = true;
    _register._tcpSocketRecvPtr = shared_from_this();
    if (!_summer->registerEvent(1, _register))
    {
        LCW("TcpSocket::doRecv[this0x" << this << "] registerEvent Error" << logSection());
        _pRecvBuf = nullptr;
        _iRecvLen = 0;
        _register._tcpSocketRecvPtr.reset();
        return false;
    }
    _onRecvHandler = std::move(handler);
    
    return true;
}


void TcpSocket::onSelectMessage(bool rd, bool wt, bool err)
{
    unsigned char linkstat = _register._linkstat;
    NetErrorCode ec = NEC_ERROR;

    if (!_onRecvHandler && !_onSendHandler && !_onConnectHandler)
    {
        LCE("TcpSocket::onSelectMessage[this0x" << this << "] unknown error. " << OSTREAM_GET_LASTERROR << logSection());
        return ;
    }

    if (linkstat == LS_WAITLINK)
    {
        std::shared_ptr<TcpSocket> guad(std::move(_register._tcpSocketConnectPtr));
        _OnConnectHandler onConnect(std::move(_onConnectHandler));
        int errCode = 0;
        socklen_t len = sizeof(int);
        if (err || getsockopt(_register._fd, SOL_SOCKET, SO_ERROR, (char*)&errCode, &len) != 0 || errCode != 0)
        {
            LOGT("onConnect False. " << OSTREAM_GET_LASTERROR);
            _register._linkstat = LS_WAITLINK;
            _summer->registerEvent(2, _register);
            onConnect(NEC_ERROR);
            return;
        }
        else
        {
            _register._wt =  0;
            _summer->registerEvent(1, _register);
            _register._linkstat = LS_ESTABLISHED;
            onConnect(NEC_SUCCESS);
            return;
        }
        return ;
    }


    if (rd && _onRecvHandler)
    {
        std::shared_ptr<TcpSocket> guad(std::move(_register._tcpSocketRecvPtr));

        int ret = recv(_register._fd, _pRecvBuf, _iRecvLen, 0);
        _register._rd = false;
        if (!_summer->registerEvent(1, _register))
        {
            LCF("TcpSocket::onSelectMessage[this0x" << this << "] connect true & EPOLLMod error.  " << OSTREAM_GET_LASTERROR << logSection());
        }
        if (ret == 0 || (ret == -1 && !IS_WOULDBLOCK))
        {
            ec = NEC_ERROR;
            _register._linkstat = LS_CLOSED;
            if (rd && _onRecvHandler)
            {
                _OnRecvHandler onRecv(std::move(_onRecvHandler));
                onRecv(ec, 0);
            }
            if (!_onSendHandler && !_onRecvHandler)
            {
                if (!_summer->registerEvent(2, _register))
                {
                    LCW("TcpSocket::onSelectMessage[this0x" << this << "] connect true & EPOLL DEL error.  " << OSTREAM_GET_LASTERROR << logSection());
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
        return;
    }
    else if (wt && _onSendHandler)
    {
        std::shared_ptr<TcpSocket> guad(std::move(_register._tcpSocketSendPtr));

        int ret = send(_register._fd, _pSendBuf, _iSendLen, 0);
        _register._wt = false;
        if (!_summer->registerEvent(1, _register))
        {
            LCF("TcpSocket::onSelectMessage[this0x" << this << "] connect true & EPOLLMod error. " << OSTREAM_GET_LASTERROR << logSection());
        }
        
        if ((ret == -1 && !IS_WOULDBLOCK) || _register._linkstat == LS_CLOSED)
        {
            ec = NEC_ERROR;
            _register._linkstat = LS_CLOSED;
            _onSendHandler = nullptr;
            if (!_onSendHandler && !_onRecvHandler)
            {
                if (!_summer->registerEvent(2, _register))
                {
                    LCW("TcpSocket::onSelectMessage[this0x" << this << "] connect true & EPOLL DEL error.  " << OSTREAM_GET_LASTERROR << logSection());
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
    if (_register._linkstat != LS_CLOSED)
    {
        _register._linkstat = LS_CLOSED;
        _summer->registerEvent(2, _register);
        if (_register._fd != InvalidFD)
        {
            shutdown(_register._fd, SHUT_RDWR);
            ::close(_register._fd);
            _register._fd = InvalidFD;
        }
        _onConnectHandler = nullptr;
        _onRecvHandler = nullptr;
        _onSendHandler = nullptr;
        _register._tcpSocketConnectPtr.reset();
        _register._tcpSocketRecvPtr.reset();
        _register._tcpSocketSendPtr.reset();
    }
    

    return true;
}


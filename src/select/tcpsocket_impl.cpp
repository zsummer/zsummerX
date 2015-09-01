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
    if (_linkstat == LS_ATTACHED)
    {
        _summer = summer;
        _summer->addTcpSocket(_fd, shared_from_this());
        _linkstat = LS_ESTABLISHED;
        return true;
    }
    if(_linkstat == LS_UNINITIALIZE)
    {
        _summer = summer;
        _fd = socket(AF_INET, SOCK_STREAM, 0);
        if (_fd == -1)
        {
            LCE("TcpSocket::initialize[this0x" << this << "] fd create failed!" );
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

    if (!_summer || _linkstat != LS_WAITLINK || _onConnectHandler)
    {
        LCE("TcpSocket::doConnect[this0x" << this << "] summer not bind!" );
        return false;
    }

    _remoteIP = remoteIP;
    _remotePort = remotePort;
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(_remoteIP.c_str());
    addr.sin_port = htons(_remotePort);
    
    
    int ret = connect(_fd, (sockaddr *) &addr, sizeof(addr));
    if (ret != 0 && errno != EINPROGRESS)
    {
        LCW("TcpSocket::doConnect[this0x" << this << "] ::connect error. " << OSTREAM_GET_LASTERROR);
        ::close(_fd);
        _fd = InvalidFD;
        return false;
    }
    _onConnectHandler = std::move(handler);
    _summer->setEvent(_fd, 1);
    _summer->addTcpSocket(_fd, shared_from_this());

    return true;
}


bool TcpSocket::doSend(char * buf, unsigned int len, _OnSendHandler && handler)
{
    if (_linkstat != LS_ESTABLISHED)
    {
        LCW("TcpSocket::doSend[this0x" << this << "] _linkstat not REG_ESTABLISHED_TCP!" );
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

    _onSendHandler = std::move(handler);
    _summer->setEvent(_fd, 1);

    return true;
}


bool TcpSocket::doRecv(char * buf, unsigned int len, _OnRecvHandler && handler)
{
    if (_linkstat != LS_ESTABLISHED)
    {
        LCW("TcpSocket::doRecv[this0x" << this << "] type not REG_ESTABLISHED_TCP!" );
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

    _onRecvHandler = std::move(handler);
    _summer->setEvent(_fd, 0);
    
    return true;
}


void TcpSocket::onSelectMessage(bool rd, bool wt, bool err)
{
    NetErrorCode ec = NEC_ERROR;

    if (!_onRecvHandler && !_onSendHandler && !_onConnectHandler)
    {
        LCE("TcpSocket::onSelectMessage[this0x" << this << "] unknown error. " << OSTREAM_GET_LASTERROR << logSection());
        return ;
    }

    if (_linkstat == LS_WAITLINK)
    {
        _OnConnectHandler onConnect(std::move(_onConnectHandler));
        int errCode = 0;
        socklen_t len = sizeof(int);
        if (err || getsockopt(_fd, SOL_SOCKET, SO_ERROR, (char*)&errCode, &len) != 0 || errCode != 0)
        {
            LCW("onConnect False. " << OSTREAM_GET_LASTERROR);
            doClose();
            onConnect(NEC_ERROR);
            return;
        }
        else
        {
            _summer->unsetEvent(_fd, 1);
            _linkstat = LS_ESTABLISHED;
            onConnect(NEC_SUCCESS);
            return;
        }
        return ;
    }


    if (rd && _onRecvHandler)
    {
        int ret = recv(_fd, _pRecvBuf, _iRecvLen, 0);
        if (ret == 0 || (ret == -1 && !IS_WOULDBLOCK))
        {
            _OnRecvHandler onRecv(std::move(_onRecvHandler));
            ec = NEC_ERROR;
            doClose();
            onRecv(ec, 0);
            if (_linkstat == LS_CLOSED)
            {
                return;
            }
        }
        else if (ret != -1)
        {
            auto guard = shared_from_this();
            _summer->unsetEvent(_fd, 0);
            _pRecvBuf = NULL;
            _iRecvLen = 0;
            _OnRecvHandler onRecv(std::move(_onRecvHandler));
            onRecv(NEC_SUCCESS,ret);
            if (_linkstat == LS_CLOSED)
            {
                return;
            }
        }
    }

    if (wt && _onSendHandler)
    {
        int ret = send(_fd, _pSendBuf, _iSendLen, 0);
        if ((ret == -1 && !IS_WOULDBLOCK))
        {
            _summer->unsetEvent(_fd, 1);
        }
        else if (ret != -1)
        {
            auto guard = shared_from_this();
            _OnSendHandler onSend(std::move(_onSendHandler));
            _pSendBuf = NULL;
            _iSendLen = 0;
            _summer->unsetEvent(_fd, 1);
            onSend(NEC_SUCCESS, ret);
            if (_linkstat == LS_CLOSED)
            {
                return;
            }
        }
    }
    return ;
}


bool TcpSocket::doClose()
{
    auto guard = shared_from_this();
    if (_linkstat != LS_CLOSED)
    {
        _linkstat = LS_CLOSED;
        _summer->clearSocket(_fd);
        shutdown(_fd, SHUT_RDWR);
        close(_fd);
        _fd = InvalidFD;
        _onConnectHandler = nullptr;
        _onRecvHandler = nullptr;
        _onSendHandler = nullptr;
    }
    return true;
}


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
        << ", _onRecvHandler = " << (bool)_onRecvHandler << ", _recvBuf=" << (void*)_recvBuf << ", _recvLen=" << _recvLen 
        << ", _onSendHandler = " << (bool)_onSendHandler << ", _sendBuf=" << (void*)_sendBuf << ", _sendLen=" << _sendLen;
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
        _linkstat = LS_WAITLINK;
        return true;
    }
    LCE("TcpSocket::initialize error");
    return false;
}

bool TcpSocket::attachSocket(int fd, const std::string& remoteIP, unsigned short remotePort, bool isIPV6)
{
     _fd = fd;
     _linkstat = LS_ATTACHED;
     _remoteIP = remoteIP;
     _remotePort = remotePort;
     _isIPV6 = isIPV6;
    return true;
}




bool TcpSocket::doConnect(const std::string & remoteIP, unsigned short remotePort, _OnConnectHandler && handler)
{

    if (!_summer || _linkstat != LS_WAITLINK || _onConnectHandler)
    {
        LCE("TcpSocket::doConnect[this0x" << this << "] summer not bind!" );
        return false;
    }
    _isIPV6 = remoteIP.find(':') != std::string::npos;
    int ret = -1;
    if (_isIPV6)
    {
        _fd = socket(AF_INET6, SOCK_STREAM, 0);
        if (_fd == -1)
        {
            LCE("TcpSocket::doConnect[this0x" << this << "] fd create failed!" );
            return false;
        }
        setNonBlock(_fd);
        _remoteIP = remoteIP;
        _remotePort = remotePort;
        sockaddr_in6 addr;
        addr.sin6_family = AF_INET6;
        inet_pton(AF_INET6, remoteIP.c_str(), &addr.sin6_addr);
        addr.sin6_port = htons(_remotePort);
        ret = connect(_fd, (sockaddr *) &addr, sizeof(addr));
    }
    else
    {

        _fd = socket(AF_INET, SOCK_STREAM, 0);
        if (_fd == -1)
        {
            LCE("TcpSocket::doConnect[this0x" << this << "] fd create failed!" );
            return false;
        }
        setNonBlock(_fd);
        _remoteIP = remoteIP;
        _remotePort = remotePort;
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(_remoteIP.c_str());
        addr.sin_port = htons(_remotePort);
        ret = connect(_fd, (sockaddr *) &addr, sizeof(addr));
    }
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
    LCT("TcpSocket::doSend len=" << len);
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
    if (_sendBuf != NULL || _sendLen != 0)
    {
        LCE("TcpSocket::doSend[this0x" << this << "] _sendBuf =" << (void *) _sendBuf << " _sendLen =" << _sendLen<< logSection());
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
        ret = send(_fd, buf, len, 0);
    }
    if (ret <= 0 )
    {
        _sendBuf = buf;
        _sendLen = len;

        _onSendHandler = std::move(handler);
        _summer->setEvent(_fd, 1);
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

    _recvBuf = buf;
    _recvLen = len;
    _recvOffset = 0;
    _onRecvHandler = std::move(handler);
    _summer->setEvent(_fd, 0);
    if (daemonRecv)
    {
        _daemonRecv = daemonRecv;
    }
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
        int ret = recv(_fd, _recvBuf + _recvOffset, _recvLen - _recvOffset, 0);
        if (ret == 0 || (ret == -1 && !IS_WOULDBLOCK))
        {
            _OnRecvHandler onRecv(std::move(_onRecvHandler));
            if (ret == 0)
            {
                ec = NEC_REMOTE_CLOSED;
            }
            else
            {
                ec = NEC_ERROR;
            }
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
            if (_daemonRecv)
            {
                _recvOffset = _onRecvHandler(NEC_SUCCESS,ret);
            }
            else
            {
                _summer->unsetEvent(_fd, 0);
                _recvBuf = NULL;
                _recvLen = 0;
                _OnRecvHandler onRecv(std::move(_onRecvHandler));
                onRecv(NEC_SUCCESS, ret);
            }
            if (_linkstat == LS_CLOSED)
            {
                return;
            }
        }
    }

    if (wt && _onSendHandler)
    {
        int ret = send(_fd, _sendBuf, _sendLen, 0);
        if ((ret == -1 && !IS_WOULDBLOCK))
        {
            _summer->unsetEvent(_fd, 1);
        }
        else if (ret != -1)
        {
            auto guard = shared_from_this();
            _OnSendHandler onSend(std::move(_onSendHandler));
            _sendBuf = NULL;
            _sendLen = 0;
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


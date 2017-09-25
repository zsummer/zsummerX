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

#include <zsummerX/select/select_impl.h>
#include <zsummerX/select/udpsocket_impl.h>

using namespace zsummer::network;



UdpSocket::UdpSocket()
{

}


UdpSocket::~UdpSocket()
{
    if (_fd != InvalidFD)
    {
        ::close(_fd);
        _fd = InvalidFD;
    }
}
bool  UdpSocket::initialize(const EventLoopPtr & summer, const char *localIP, unsigned short localPort)
{
    if (_linkstat == LS_UNINITIALIZE)
    {
        _summer = summer;
        _fd = socket(AF_INET, SOCK_DGRAM, 0);
        _linkstat = LS_WAITLINK;
        if (_fd == InvalidFD)
        {
            LCE("UdpSocket::initialize[this0x" << this << "] create socket fail. this=" << this  << ", " << OSTREAM_GET_LASTERROR);
            return false;
        }
        setNonBlock(_fd);
        sockaddr_in    localAddr;
        localAddr.sin_family = AF_INET;
        localAddr.sin_addr.s_addr = inet_addr(localIP);
        localAddr.sin_port = htons(localPort);
        if (bind(_fd, (sockaddr *) &localAddr, sizeof(localAddr)) != 0)
        {
            LCE("UdpSocket::initialize[this0x" << this << "]: socket bind err, " << OSTREAM_GET_LASTERROR);
            ::close(_fd);
            _fd = InvalidFD;
            return false;
        }

        _summer->addUdpSocket(_fd, shared_from_this());
        _linkstat = LS_ESTABLISHED;
    }

    LCE("UdpSocket::initialize[this0x" << this << "] UdpSocket is aready initialize, _ios not is nullptr. this=" << this);
    return false;

}

bool UdpSocket::doSendTo(char * buf, unsigned int len, const char *dstip, unsigned short dstport)
{
    if (!_summer || _linkstat != LS_ESTABLISHED)
    {
        LCE("UdpSocket::doSend[this0x" << this << "] stat error!");
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
    sendto(_fd, buf, len, 0, (sockaddr*)&addr, sizeof(addr));
    return true;
}


bool UdpSocket::doRecvFrom(char * buf, unsigned int len, _OnRecvFromHandler && handler)
{
    if (!_summer || _linkstat != LS_ESTABLISHED)
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

    _onRecvFromHandler = std::move(handler);

    _summer->setEvent(_fd, 0);
    
    return true;
}


bool UdpSocket::onSelectMessage(int type, bool rd, bool wt)
{
    if (!_onRecvFromHandler)
    {
        LCE("UdpSocket::onSelectMessage[this0x" << this << "] unknown error");
        return false;
    }

    if (rd && _onRecvFromHandler)
    {
        _OnRecvFromHandler onRecv(std::move(_onRecvFromHandler));
        _summer->unsetEvent(_fd, 0);

        sockaddr_in raddr;
        memset(&raddr, 0, sizeof(raddr));
        socklen_t len = sizeof(raddr);
        int ret = recvfrom(_fd, _recvBuf, _recvLen, 0, (sockaddr*)&raddr, &len);

        _recvBuf = nullptr;
        _recvLen = 0;
        if (ret == 0 || (ret ==-1 && !IS_WOULDBLOCK) )
        {
            LCE("UdpSocket::onSelectMessage[this0x" << this << "] recv error. "  << ", ret=" << ret << ", " << OSTREAM_GET_LASTERROR);
            onRecv(NEC_ERROR, "", 0, 0);
            return false;
        }
        if (ret == -1)
        {
            LCE("UdpSocket::onSelectMessage[this0x" << this << "] recv error. " << ", ret=" << ret << ", " << OSTREAM_GET_LASTERROR);
            onRecv(NEC_ERROR, "", 0, 0);
            return false;
        }

        onRecv(NEC_SUCCESS, inet_ntoa(raddr.sin_addr), ntohs(raddr.sin_port), ret);
    }
    
    

    return true;
}


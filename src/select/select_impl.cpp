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
#include <zsummerX/select/tcpsocket_impl.h>
#include <zsummerX/select/tcpaccept_impl.h>
#include <zsummerX/select/udpsocket_impl.h>

using namespace zsummer::network;

bool EventLoop::initialize()
{
    //////////////////////////////////////////////////////////////////////////
    //create socket pair
    sockaddr_in pairAddr;
    memset(&pairAddr, 0, sizeof(pairAddr));
    pairAddr.sin_family = AF_INET;
    pairAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    int acpt = socket(AF_INET, SOCK_STREAM, 0);
    _sockpair[0] = socket(AF_INET, SOCK_STREAM, 0);
    _sockpair[1] = socket(AF_INET, SOCK_STREAM, 0);
    if (_sockpair[0] == -1 || _sockpair[1] == -1 || acpt == -1)
    {
        LCF("ZSummerImpl::initialize[this0x" << this << "] bind sockpair socket error. " );
        return false;
    }
    if (::bind(acpt, (sockaddr*)&pairAddr, sizeof(pairAddr)) == -1)
    {
        LCF("EventLoop::initialize[this0x" << this << "] bind sockpair socket error. " );
        ::close(acpt);
        ::close(_sockpair[0]);
        ::close(_sockpair[1]);
        return false;
    }
    if (listen(acpt, 1) == -1)
    {
        LCF("EventLoop::initialize[this0x" << this << "] listen sockpair socket error. " );
        ::close(acpt);
        ::close(_sockpair[0]);
        ::close(_sockpair[1]);
        return false;
    }
    socklen_t len = sizeof(pairAddr);
    if (getsockname(acpt, (sockaddr*)&pairAddr, &len) != 0)
    {
        LCF("EventLoop::initialize[this0x" << this << "] getsockname sockpair socket error. " );
        ::close(acpt);
        ::close(_sockpair[0]);
        ::close(_sockpair[1]);
        return false;
    }

    if (::connect(_sockpair[0], (sockaddr*)&pairAddr, sizeof(pairAddr)) == -1)
    {
        LCF("EventLoop::initialize[this0x" << this << "] connect sockpair socket error. " );
        ::close(acpt);
        ::close(_sockpair[0]);
        ::close(_sockpair[1]);
        return false;
    }
    _sockpair[1] = accept(acpt, (sockaddr*)&pairAddr, &len);
    if (_sockpair[1] == -1)
    {
        LCF("EventLoop::initialize[this0x" << this << "] accept sockpair socket error. " );
        ::close(acpt);
        ::close(_sockpair[0]);
        ::close(_sockpair[1]);
        return false;
    }
    ::close(acpt);

    setNonBlock(_sockpair[0]);
    setNonBlock(_sockpair[1]);
    setNoDelay(_sockpair[0]);
    setNoDelay(_sockpair[1]);

    FD_ZERO(&_fdreads);
    FD_ZERO(&_fdwrites);
    FD_ZERO(&_fderrors);
    setEvent(_sockpair[1], 0);
    _arrayTcpAccept.resize(MAX_NUMBER_FD);
    _arrayTcpSocket.resize(MAX_NUMBER_FD);
    _arrayUdpSocket.resize(MAX_NUMBER_FD);
    return true;
}

void EventLoop::setEvent(int fd, int op /*0 read, 1 write, 2 error*/)
{
    if (fd >= MAX_NUMBER_FD)
    {
        LCE("EventLoop::setEvent error. fd = " << fd << " greater than MAX_NUMBER_FD = " << MAX_NUMBER_FD);
        return;
    }
    if (fd > _maxfd)
        _maxfd = fd;
    if (op == 0)
        FD_SET(fd, &_fdreads);
    else if(op == 1)
        FD_SET(fd, &_fdwrites);
    else if(op == 2)
        FD_SET(fd, &_fderrors);
}

void EventLoop::unsetEvent(int fd, int op /*0 read, 1 write, 2 error*/)
{
    if (fd >= MAX_NUMBER_FD)
    {
        LCE("EventLoop::setEvent error. fd = " << fd << " greater than MAX_NUMBER_FD = " << MAX_NUMBER_FD);
        return;
    }
    if (op == 0)
        FD_CLR(fd, &_fdreads);
    else if(op == 1)
        FD_CLR(fd, &_fdwrites);
    else if(op == 2)
        FD_CLR(fd, &_fderrors);
}

void EventLoop::addTcpAccept(int fd, TcpAcceptPtr s)
{
    if (fd >= MAX_NUMBER_FD)
    {
        LCE("EventLoop::setEvent error. fd = " << fd << " greater than MAX_NUMBER_FD = " << MAX_NUMBER_FD);
        return;
    }
    _arrayTcpAccept[fd] = s;
}

void EventLoop::addTcpSocket(int fd, TcpSocketPtr s)
{
    if (fd >= MAX_NUMBER_FD)
    {
        LCE("EventLoop::setEvent error. fd = " << fd << " greater than MAX_NUMBER_FD = " << MAX_NUMBER_FD);
        return;
    }
    _arrayTcpSocket[fd] = s;
}

void EventLoop::addUdpSocket(int fd, UdpSocketPtr s)
{
    if (fd >= MAX_NUMBER_FD)
    {
        LCE("EventLoop::setEvent error. fd = " << fd << " greater than MAX_NUMBER_FD = " << MAX_NUMBER_FD);
        return;
    }
    _arrayUdpSocket[fd] = s;
}


void EventLoop::clearSocket(int fd)
{
    if (fd >= MAX_NUMBER_FD)
    {
        LCE("EventLoop::setEvent error. fd = " << fd << " greater than MAX_NUMBER_FD = " << MAX_NUMBER_FD);
        return;
    }
    unsetEvent(fd, 0);
    unsetEvent(fd, 1);
    unsetEvent(fd, 2);
    _arrayTcpAccept[fd].reset();
    _arrayTcpSocket[fd].reset();
    _arrayUdpSocket[fd].reset();
}

void EventLoop::PostMessage(_OnPostHandler &&handle)
{
    _OnPostHandler * pHandler = new _OnPostHandler(std::move(handle));
    bool needNotice = false;
    _postQueueLock.lock();
    if (_postQueue.empty()){ needNotice = true;}
    _postQueue.push_back(pHandler);
    _postQueueLock.unlock();
    if (needNotice)
    {
        char c = '0';
        send(_sockpair[0], &c, 1, 0);
    }
}

std::string EventLoop::logSection()
{
    std::stringstream os;
    _postQueueLock.lock();
    MessageStack::size_type msgSize = _postQueue.size();
    _postQueueLock.unlock();
    os << " logSection:  _sockpair[2]={" << _sockpair[0] << "," << _sockpair[1] << "}"
        << " _postQueue.size()=" << msgSize << ", current total timer=" << _timer.getTimersCount()
        << " _maxfd=" << _maxfd;
    return os.str();
}

void EventLoop::runOnce(bool isImmediately)
{
    fd_set fdr;
    fd_set fdw;
    fd_set fde;
    memcpy(&fdr, &_fdreads, sizeof(_fdreads));
    memcpy(&fdw, &_fdwrites, sizeof(_fdwrites));
    memcpy(&fde, &_fderrors, sizeof(_fderrors));

    timeval tv;
    if (isImmediately)
    {
        tv.tv_sec = 0;
        tv.tv_usec = 0;
    }
    else
    {
        unsigned int ms = _timer.getNextExpireTime();
        tv.tv_sec = ms / 1000;
        tv.tv_usec = (ms % 1000) * 1000;
    }

    int retCount = ::select(_maxfd+1, &fdr, &fdw, &fde, &tv);
    if (retCount == -1)
    {
        return;
    }

    //check timer
    {
        _timer.checkTimer();
        if (retCount == 0) return;//timeout
    }

    if (FD_ISSET(_sockpair[1], &fdr))
    {
        char buf[1000];
        while (recv(_sockpair[1], buf, 1000, 0) > 0);

        MessageStack msgs;
        _postQueueLock.lock();
        msgs.swap(_postQueue);
        _postQueueLock.unlock();

        for (auto pfunc : msgs)
        {
            _OnPostHandler * p = (_OnPostHandler*)pfunc;
            try
            {
                (*p)();
            }
            catch (const std::exception & e)
            {
                LCW("OnPostHandler have runtime_error exception. err=" << e.what());
            }
            catch (...)
            {
                LCW("OnPostHandler have unknown exception.");
            }
            delete p;
        }
    }

    for (int i = 0; i <= _maxfd ; ++i)
    {
        //tagHandle  type
        if (FD_ISSET(i, &fdr) || FD_ISSET(i, &fdw) || FD_ISSET(i, &fde))
        {
            if (_arrayTcpAccept[i])
                _arrayTcpAccept[i]->onSelectMessage();
            if (_arrayTcpSocket[i])
            {
                _arrayTcpSocket[i]->onSelectMessage(FD_ISSET(i, &fdr), FD_ISSET(i, &fdw), FD_ISSET(i, &fde));
            }
            if (_arrayUdpSocket[i])
            {
                _arrayUdpSocket[i]->onSelectMessage(FD_ISSET(i, &fdr), FD_ISSET(i, &fdw), FD_ISSET(i, &fde));
            }
        }
    }
}



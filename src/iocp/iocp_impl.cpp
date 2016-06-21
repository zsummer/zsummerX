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

#include <zsummerX/iocp/iocp_impl.h>
#include <zsummerX/iocp/tcpsocket_impl.h>
#include <zsummerX/iocp/tcpaccept_impl.h>
#include <zsummerX/iocp/udpsocket_impl.h>
using namespace zsummer::network;

bool EventLoop::initialize()
{
    if (_io != NULL)
    {
        LCF("iocp already craeted !  " << logSection());
        return false;
    }
    _io = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);
    if (!_io)
    {
        LCF("CreateIoCompletionPort false!  " );
        return false;
    }
    return true;
}

void EventLoop::post(_OnPostHandler &&h)
{
    _OnPostHandler *ptr = new _OnPostHandler(std::move(h));
    PostQueuedCompletionStatus(_io, 0, PCK_USER_DATA, (LPOVERLAPPED)(ptr));
}

std::string EventLoop::logSection()
{
    std::stringstream os;
    os << "logSection[0x" << this << "]: _iocp=" << (void*)_io << ", current total timer=" << _timer.getTimersCount() <<", next expire time=" << _timer.getNextExpireTime();
    return os.str();
}



void EventLoop::runOnce(bool isImmediately)
{
    if (_io == NULL)
    {
        LCF("iocp handle not initialize. " <<logSection());
        return;
    }

    DWORD dwTranceBytes = 0;
    ULONG_PTR uComKey = NULL;
    LPOVERLAPPED pOverlapped = NULL;

    BOOL bRet = GetQueuedCompletionStatus(_io, &dwTranceBytes, &uComKey, &pOverlapped, isImmediately ? 0 : _timer.getNextExpireTime()/*INFINITE*/);

    _timer.checkTimer();
    if (!bRet && !pOverlapped)
    {
        //TIMEOUT
        return;
    }
    
    //! user post
    if (uComKey == PCK_USER_DATA)
    {
        _OnPostHandler * func = (_OnPostHandler*) pOverlapped;
        try
        {
            (*func)();
        }
        catch (const std::exception & e)
        {
            LCW("when call [post] callback throw one runtime_error. err=" << e.what());
        }
        catch (...)
        {
            LCW("when call [post] callback throw one unknown exception.");
        }
        delete func;
        return;
    }
    
    //! net data
    ExtendHandle & req = *(HandlerFromOverlaped(pOverlapped));
    switch (req._type)
    {
    case ExtendHandle::HANDLE_ACCEPT:
        {
            if (req._tcpAccept)
            {
                req._tcpAccept->onIOCPMessage(bRet);
            }
        }
        break;
    case ExtendHandle::HANDLE_RECV:
    case ExtendHandle::HANDLE_SEND:
    case ExtendHandle::HANDLE_CONNECT:
        {
            if (req._tcpSocket)
            {
                req._tcpSocket->onIOCPMessage(bRet, dwTranceBytes, req._type);
            }
        }
        break;
    case ExtendHandle::HANDLE_SENDTO:
    case ExtendHandle::HANDLE_RECVFROM:
        {
            if (req._udpSocket)
            {
                req._udpSocket->onIOCPMessage(bRet, dwTranceBytes, req._type);
            }
        }
        break;
    default:
        LCE("GetQueuedCompletionStatus undefined type=" << req._type << logSection());
    }
    
}



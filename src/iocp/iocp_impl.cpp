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


void EventLoop::runOnce(bool isImmediately)
{
	if (_io == NULL)
	{
		LCF("EventLoop::runOnce[this0x" << this << "] server not initialize or initialize false." <<logSection());
		return;
	}

	DWORD dwTranceCount = 0;
	ULONG_PTR uComKey = NULL;
	LPOVERLAPPED pOverlapped = NULL;

	BOOL bRet = GetQueuedCompletionStatus(_io, &dwTranceCount, &uComKey, &pOverlapped, isImmediately ? 0 : _timer.getNextExpireTime()/*INFINITE*/);

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
		catch (std::runtime_error e)
		{
			LCW("OnPostHandler have runtime_error exception. err=" << e.what());
		}
		catch (...)
		{
			LCW("OnPostHandler have unknown exception.");
		}
		delete func;
		return;
	}
	
	//! network data
	tagReqHandle & req = *(HandlerFromOverlaped(pOverlapped));
	switch (req._type)
	{
	case tagReqHandle::HANDLE_ACCEPT:
		{
			if (req._tcpAccept)
			{
				req._tcpAccept->onIOCPMessage(bRet);
			}
		}
		break;
	case tagReqHandle::HANDLE_RECV:
	case tagReqHandle::HANDLE_SEND:
	case tagReqHandle::HANDLE_CONNECT:
		{
			if (req._tcpSocket)
			{
				req._tcpSocket->onIOCPMessage(bRet, dwTranceCount, req._type);
			}
		}
		break;
	case tagReqHandle::HANDLE_SENDTO:
	case tagReqHandle::HANDLE_RECVFROM:
		{
			if (req._udpSocket)
			{
				req._udpSocket->onIOCPMessage(bRet, dwTranceCount, req._type);
			}
		}
		break;
	default:
		LCE("EventLoop::runOnce[this0x" << this << "]GetQueuedCompletionStatus undefined type=" << req._type << logSection());
	}
	
}



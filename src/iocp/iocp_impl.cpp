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
 * Copyright (C) 2013 YaweiZhang <yawei_zhang@foxmail.com>.
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

void CZSummerImpl::RunOnce()
{
	if (m_io == NULL)
	{
		LCF("CZSummerImpl::RunOnce[this0x" << this << "] Can't Run Once. server not initialize or initialize false." << GetZSummerImplStatus());
		return;
	}

	DWORD dwTranceCount = 0;
	ULONG_PTR uComKey = NULL;
	LPOVERLAPPED pOverlapped = NULL;

	BOOL bRet = GetQueuedCompletionStatus(m_io, &dwTranceCount, &uComKey, &pOverlapped, m_timer.GetNextExpireTime()/*INFINITE*/);

	//检查定时器超时状态
	{
		m_timer.CheckTimer();
		if (!bRet && !pOverlapped)
		{
			//TIMEOUT
			return;
		}
	}
	
	//! 检查自定义通知
	if (uComKey == PCK_USER_DATA)
	{
		_OnPostHandler * func = (_OnPostHandler*) pOverlapped;
		(*func)();
		delete func;
		return;
	}
	
	//! 处理来自网络的通知
	
	unsigned char type = HandlerFromOverlaped(pOverlapped)->_type;
	switch (type)
	{
	case tagReqHandle::HANDLE_ACCEPT:
		{
			CTcpAcceptImpl *pKey = (CTcpAcceptImpl *) uComKey;
			pKey->OnIOCPMessage(bRet);
		}
		break;
	case tagReqHandle::HANDLE_RECV:
	case tagReqHandle::HANDLE_SEND:
	case tagReqHandle::HANDLE_CONNECT:
		{
			CTcpSocketImpl *pKey = (CTcpSocketImpl *) uComKey;
			pKey->OnIOCPMessage(bRet, dwTranceCount, type);
		}
		break;
	case tagReqHandle::HANDLE_SENDTO:
	case tagReqHandle::HANDLE_RECVFROM:
		{
			CUdpSocketImpl * pKey = (CUdpSocketImpl*) uComKey;
			pKey->OnIOCPMessage(bRet, dwTranceCount, type);
		}
		break;
	default:
		LCE("CZSummerImpl::RunOnce[this0x" << this << "]GetQueuedCompletionStatus undefined type=" << type << GetZSummerImplStatus());
	}
	
}



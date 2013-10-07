/*
 * ZSUMMER_11X License
 * -----------
 * 
 * ZSUMMER_11X is licensed under the terms of the MIT license reproduced below.
 * This means that ZSUMMER_11X is free software and can be used for both academic
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

#include "iocp_impl.h"
#include "tcpsocket_impl.h"
#include "tcpaccept_impl.h"


using namespace zsummer::network;

void CZSummerImpl::RunOnce()
{
	if (m_io == NULL)
	{
		LCF("Can't Run Once. server not initialize or initialize false.");
		return;
	}

	DWORD dwTranceCount = 0;
	ULONG_PTR uComKey = NULL;
	LPOVERLAPPED pOverlapped = NULL;
	unsigned long long nowMs = std::chrono::system_clock::now().time_since_epoch()/std::chrono::milliseconds(1);
	DWORD dwDelayMs = 0;
	dwDelayMs = (DWORD) (m_nextExpire -nowMs);
	if (dwDelayMs > 100)
	{
		dwDelayMs = 100;
	}
	else if (dwDelayMs < 10)
	{
		dwDelayMs = 10;
	}
	BOOL bRet = GetQueuedCompletionStatus(m_io, &dwTranceCount, &uComKey, &pOverlapped, dwDelayMs/*INFINITE*/);

	//检查定时器超时状态
	{
		CheckTimer();
		if (!bRet && !pOverlapped)
		{
			//TIMEOUT
			return;
		}
	}
	
	//! 检查自定义通知
	if (uComKey == PCK_USER_DATA)
	{
		_OnPostHandle * func = (_OnPostHandle*) pOverlapped;
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
//			CUdpSocket * pKey = (CUdpSocket*) uComKey;
//			pKey->OnIOCPMessage(bRet, dwTranceCount, type);
		}
		break;
	default:
		LCE("GetQueuedCompletionStatus undefined type=" << type);
	}
	
}



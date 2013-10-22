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

#include "epoll_impl.h"
#include "tcpaccept_impl.h"
#include "tcpsocket_impl.h"
#include "udpsocket_impl.h"
using namespace zsummer::network;



CZSummerImpl::CZSummerImpl()
{
	m_epoll = -1;
	m_nextExpire = (unsigned long long)-1;
	m_sockpair[0] = 0;
	m_sockpair[1] = 0;
}

CZSummerImpl::~CZSummerImpl()
{

}



bool CZSummerImpl::Initialize()
{
	m_nextExpire = (unsigned long long)-1;

	if (m_epoll != -1)
	{
		LCF("epoll is created !");
		return false;
	}
	m_epoll = epoll_create(200);
	if (m_epoll == -1)
	{
		LCF("create epoll err errno=" << strerror(errno));
		return false;
	}
	{
		if (socketpair(AF_LOCAL, SOCK_STREAM, 0, m_sockpair) != 0)
		{
			LCF("create socketpair.  errno=" << strerror(errno));
			return false;
		}
		SetNonBlock(m_sockpair[0]);
		SetNonBlock(m_sockpair[1]);
		SetNoDelay(m_sockpair[1]);
		SetNoDelay(m_sockpair[1]);
		m_register.reset();
		m_register._ptr = this;
		m_register._type = tagRegister::REG_THREAD;
		m_register._fd = m_sockpair[1];
		m_register._event.events = EPOLLIN;
		if (epoll_ctl(m_epoll, EPOLL_CTL_ADD, m_sockpair[1], &m_register._event) != 0)
		{
			LCF("epoll_ctl add socketpair failed .  errno=" << errno);
			return false;
		}
	}

	return true;
}



void CZSummerImpl::PostMsg(POST_COM_KEY pck, const _OnPostHandler &handle)
{
	_OnPostHandler * pHandler = new _OnPostHandler(handle);
	m_msglock.lock();
	m_msgs.push_back(std::make_pair(pck, pHandler));
	m_msglock.unlock();
	char c='0';
	send(m_sockpair[0], &c, 1, 0);
}


void CZSummerImpl::RunOnce()
{
	int dwDelayMs =0;
	unsigned long long nowMs = std::chrono::system_clock::now().time_since_epoch()/std::chrono::milliseconds(1);
	dwDelayMs = (int) (m_nextExpire -nowMs);
	if (dwDelayMs > 100)
	{
		dwDelayMs = 100;
	}
	else if (dwDelayMs < 10)
	{
		dwDelayMs = 10;
	}

	int retCount = epoll_wait(m_epoll, m_events, 1000,  dwDelayMs);
	if (retCount == -1)
	{
		if (errno != EINTR)
		{
			LCD(" epoll_wait err!  errno=" <<strerror(errno));
			return; //! error
		}
		return;
	}
	//check timer
	//检查定时器超时状态
	{
		CheckTimer();
		if (retCount == 0) return;//timeout
	}


		
	for (int i=0; i<retCount; i++)
	{
		int eventflag = m_events[i].events;
		tagRegister * pReg = (tagRegister *)m_events[i].data.ptr;
		//tagHandle  type
		if (pReg->_type == tagRegister::REG_THREAD)
		{
			char buf[1000];
			while (recv(pReg->_fd, buf, 1000, 0) > 0){}

			MsgVct msgs;
			m_msglock.lock();
			msgs.swap(m_msgs);
			m_msgs.clear();
			m_msglock.unlock();

			for (auto iter = msgs.begin(); iter != msgs.end(); ++iter)
			{
				if (iter->first == PCK_USER_DATA)
				{
					_OnPostHandler * p = (_OnPostHandler*)(iter->second);
					(*p)();
					delete p;
				}
			}
		}
		else if (pReg->_type == tagRegister::REG_ACCEPT)
		{
			CTcpAcceptImpl *pKey = (CTcpAcceptImpl *) pReg->_ptr;
			if (eventflag & EPOLLIN)
			{
				pKey->OnEPOLLMessage(true);
			}
			if (eventflag & EPOLLERR || eventflag & EPOLLHUP)
			{
				pKey->OnEPOLLMessage(false);
			}
		}
		else if (pReg->_type == tagRegister::REG_ESTABLISHED_TCP || pReg->_type == tagRegister::REG_CONNECT)
		{
			CTcpSocketImpl *pKey = (CTcpSocketImpl *) pReg->_ptr;
			pKey->OnEPOLLMessage(pReg->_type, eventflag);
		}
		else if (pReg->_type == tagRegister::REG_ESTABLISHED_UDP)
		{
			CUdpSocketImpl *pKey = (CUdpSocketImpl *) pReg->_ptr;
			pKey->OnEPOLLMessage(pReg->_type, eventflag);
		}
		else
		{
			LCE("check register event type failed !!  type=" << pReg->_type);
		}
			
	}
}



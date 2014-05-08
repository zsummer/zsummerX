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


#include <zsummerX/epoll/epoll_impl.h>
#include <zsummerX/epoll/tcpsocket_impl.h>
#include <zsummerX/epoll/tcpaccept_impl.h>
#include <zsummerX/epoll/udpsocket_impl.h>

using namespace zsummer::network;



CZSummerImpl::CZSummerImpl()
{
	m_epoll = -1;
	m_sockpair[0] = 0;
	m_sockpair[1] = 0;
}

CZSummerImpl::~CZSummerImpl()
{

}



bool CZSummerImpl::Initialize()
{
	

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
		SetNoDelay(m_sockpair[0]);
		SetNoDelay(m_sockpair[1]);

		m_register._event.data.ptr = &m_register;
		m_register._event.events = EPOLLIN;
		m_register._fd = m_sockpair[1];
		m_register._linkstat = LS_ESTABLISHED;
		m_register._ptr = this;
		m_register._type = tagRegister::REG_ZSUMMER;


		if (epoll_ctl(m_epoll, EPOLL_CTL_ADD, m_sockpair[1], &m_register._event) != 0)
		{
			LCF("epoll_ctl add socketpair failed .  errno=" << errno);
			return false;
		}
	}

	return true;
}



void CZSummerImpl::PostMsg(const _OnPostHandler &handle)
{
	_OnPostHandler * pHandler = new _OnPostHandler(handle);
	m_msglock.lock();
	m_msgs.push_back(pHandler);
	m_msglock.unlock();
	char c='0';
	send(m_sockpair[0], &c, 1, 0);
}


void CZSummerImpl::RunOnce()
{
	int retCount = epoll_wait(m_epoll, m_events, 1000,   m_timer.GetNextExpireTime());
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
	{
		m_timer.CheckTimer();
		if (retCount == 0) return;//timeout
	}


	for (int i=0; i<retCount; i++)
	{
		int eventflag = m_events[i].events;
		tagRegister * pReg = (tagRegister *)m_events[i].data.ptr;
		//tagHandle  type
		if (pReg->_type == tagRegister::REG_ZSUMMER)
		{
			char buf[1000];
			while (recv(pReg->_fd, buf, 1000, 0) > 0);

			MsgVct msgs;
			m_msglock.lock();
			msgs.swap(m_msgs);
			m_msglock.unlock();

			for (auto iter = msgs.begin(); iter != msgs.end(); ++iter)
			{
				_OnPostHandler * p = (_OnPostHandler*)(*iter);
				(*p)();
				delete p;
			}
		}
		else if (pReg->_type == tagRegister::REG_TCP_ACCEPT)
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
		else if (pReg->_type == tagRegister::REG_TCP_SOCKET)
		{
			CTcpSocketImpl *pKey = (CTcpSocketImpl *) pReg->_ptr;
			pKey->OnEPOLLMessage(pReg->_type, eventflag);
		}
		else if (pReg->_type == tagRegister::REG_UDP_SOCKET)
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



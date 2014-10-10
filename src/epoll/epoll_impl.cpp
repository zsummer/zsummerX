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
 * Copyright (C) 2013-2014 YaweiZhang <yawei_zhang@foxmail.com>.
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

bool ZSummer::Initialize()
{
	if (m_epoll != InvalideFD)
	{
		LCF("ZSummer::Initialize[this0x"<<this <<"] epoll is created ! " << ZSummerSection());
		return false;
	}
	const int IGNORE_ENVENTS = 100;
	m_epoll = epoll_create(IGNORE_ENVENTS);
	if (m_epoll == InvalideFD)
	{
		LCF("ZSummer::Initialize[this0x" << this << "] create epoll err errno=" << strerror(errno) << ZSummerSection());
		return false;
	}

	if (socketpair(AF_LOCAL, SOCK_STREAM, 0, m_sockpair) != 0)
	{
		LCF("ZSummer::Initialize[this0x" << this << "] create socketpair.  errno=" << strerror(errno) << ZSummerSection());
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
	m_register._type = tagRegister::REG_ZSUMMER;
	if (!RegisterEvent(EPOLL_CTL_ADD, m_register))
	{
		LCF("ZSummer::Initialize[this0x" << this << "] EPOLL_CTL_ADD m_socketpair error. " << ZSummerSection());
		return false;
	}
	
	return true;
}

bool ZSummer::RegisterEvent(int op, tagRegister & reg)
{
	if (epoll_ctl(m_epoll, op, reg._fd, &reg._event) != 0)
	{
		return false;
	}
	return true;
}

void ZSummer::PostMessage(const _OnPostHandler &handle)
{
	_OnPostHandler * pHandler = new _OnPostHandler(handle);
	m_stackMessagesLock.lock();
	m_stackMessages.push_back(pHandler);
	m_stackMessagesLock.unlock();
	char c='0';
	send(m_sockpair[0], &c, 1, 0);
}

std::string ZSummer::ZSummerSection()
{
	std::stringstream os;
	m_stackMessagesLock.lock();
	MessageStack::size_type msgSize = m_stackMessages.size();
	m_stackMessagesLock.unlock();
	os << " ZSummer: m_epoll=" << m_epoll << ", m_sockpair[2]={" << m_sockpair[0] << "," << m_sockpair[1] << "}"
		<< " m_stackMessages.size()=" << msgSize << ", current total timer=" << m_timer.GetTimersCount()
		<< " m_register=" << m_register;
	return os.str();
}

void ZSummer::RunOnce()
{
	int retCount = epoll_wait(m_epoll, m_events, 1000,   m_timer.GetNextExpireTime());
	if (retCount == -1)
	{
		if (errno != EINTR)
		{
			LCW("ZSummer::RunOnce[this0x" << this << "]  epoll_wait err!  errno=" << strerror(errno) << ZSummerSection());
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

			MessageStack msgs;
			m_stackMessagesLock.lock();
			msgs.swap(m_stackMessages);
			m_stackMessagesLock.unlock();

			for (auto pfunc : msgs)
			{
				_OnPostHandler * p = (_OnPostHandler*)pfunc;
				(*p)();
				delete p;
			}
		}
		else if (pReg->_type == tagRegister::REG_TCP_ACCEPT)
		{
			if (eventflag & EPOLLIN)
			{
				if (pReg->_tcpacceptPtr)
				{
					pReg->_tcpacceptPtr->OnEPOLLMessage(true);
				}
			}
			else if (eventflag & EPOLLERR || eventflag & EPOLLHUP)
			{
				if (pReg->_tcpacceptPtr)
				{
					pReg->_tcpacceptPtr->OnEPOLLMessage(false);
				}
			}
		}
		else if (pReg->_type == tagRegister::REG_TCP_SOCKET)
		{
			if (eventflag & EPOLLERR || eventflag & EPOLLHUP)
			{
				if (pReg->_tcpSocketConnectPtr)
				{
					pReg->_tcpSocketConnectPtr->OnEPOLLMessage(EPOLLOUT, true);
				}
				else if (pReg->_tcpSocketRecvPtr)
				{
					pReg->_tcpSocketRecvPtr->OnEPOLLMessage(EPOLLIN, true);
				}
				else if (pReg->_tcpSocketSendPtr)
				{
					pReg->_tcpSocketSendPtr->OnEPOLLMessage(EPOLLOUT, true);
				}
			}
			else if (eventflag & EPOLLIN)
			{
				if (pReg->_tcpSocketRecvPtr)
				{
					pReg->_tcpSocketRecvPtr->OnEPOLLMessage(EPOLLIN, false);
				}
			}
			else if (eventflag & EPOLLOUT)
			{
				if (pReg->_tcpSocketConnectPtr)
				{
					pReg->_tcpSocketConnectPtr->OnEPOLLMessage(EPOLLOUT, false);
				}
				else if (pReg->_tcpSocketSendPtr)
				{
					pReg->_tcpSocketSendPtr->OnEPOLLMessage(EPOLLOUT, false);
				}
			}
		}
		else if (pReg->_type == tagRegister::REG_UDP_SOCKET)
		{
			if (pReg->_udpsocketPtr)
			{
				pReg->_udpsocketPtr->OnEPOLLMessage(pReg->_type, eventflag);
			}
		}
		else
		{
			LCE("ZSummer::RunOnce[this0x" << this << "] check register event type failed !!  type=" << pReg->_type << ZSummerSection());
		}
			
	}
}



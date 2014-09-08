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


#include <zsummerX/select/select_impl.h>
#include <zsummerX/select/tcpsocket_impl.h>
#include <zsummerX/select/tcpaccept_impl.h>
#include <zsummerX/select/udpsocket_impl.h>

using namespace zsummer::network;



CZSummerImpl::CZSummerImpl()
{
}

CZSummerImpl::~CZSummerImpl()
{

}



bool CZSummerImpl::Initialize()
{
	sockaddr_in pairAddr;
	memset(&pairAddr, 0, sizeof(pairAddr));
	pairAddr.sin_family = AF_INET;
	pairAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	SOCKET acpt = socket(AF_INET, SOCK_STREAM, 0);
	m_sockpair[0] = socket(AF_INET, SOCK_STREAM, 0);
	m_sockpair[1] = socket(AF_INET, SOCK_STREAM, 0);
	if (m_sockpair[0] == -1 || m_sockpair[1] == -1 || acpt == -1)
	{
		LCF("CZSummerImpl::Initialize[this0x" << this << "] bind sockpair socket error. " << GetZSummerImplStatus());
		return false;
	}
	if (::bind(acpt, (sockaddr*)&pairAddr, sizeof(pairAddr)) == -1)
	{
		LCF("CZSummerImpl::Initialize[this0x" << this << "] bind sockpair socket error. " << GetZSummerImplStatus());
		closesocket(acpt);
		closesocket(m_sockpair[0]);
		closesocket(m_sockpair[1]);
		return false;
	}
	if (listen(acpt, 1) == -1)
	{
		LCF("CZSummerImpl::Initialize[this0x" << this << "] listen sockpair socket error. " << GetZSummerImplStatus());
		closesocket(acpt);
		closesocket(m_sockpair[0]);
		closesocket(m_sockpair[1]);
		return false;
	}
	socklen_t len = sizeof(pairAddr);
	if (getsockname(acpt, (sockaddr*)&pairAddr, &len) != 0)
	{
		LCF("CZSummerImpl::Initialize[this0x" << this << "] getsockname sockpair socket error. " << GetZSummerImplStatus());
		closesocket(acpt);
		closesocket(m_sockpair[0]);
		closesocket(m_sockpair[1]);
		return false;
	}

	if (::connect(m_sockpair[0], (sockaddr*)&pairAddr, sizeof(pairAddr)) == -1)
	{
		LCF("CZSummerImpl::Initialize[this0x" << this << "] connect sockpair socket error. " << GetZSummerImplStatus());
		closesocket(acpt);
		closesocket(m_sockpair[0]);
		closesocket(m_sockpair[1]);
		return false;
	}
	m_sockpair[1] = accept(acpt, (sockaddr*)&pairAddr, &len);
	if (m_sockpair[1] == -1)
	{
		LCF("CZSummerImpl::Initialize[this0x" << this << "] accept sockpair socket error. " << GetZSummerImplStatus());
		closesocket(acpt);
		closesocket(m_sockpair[0]);
		closesocket(m_sockpair[1]);
		return false;
	}
	closesocket(acpt);

	SetNonBlock(m_sockpair[0]);
	SetNonBlock(m_sockpair[1]);
	SetNoDelay(m_sockpair[0]);
	SetNoDelay(m_sockpair[1]);

	return true;
}

bool CZSummerImpl::RegisterEvent(int op, tagRegister & reg)
{
	if (!reg._ptr)
	{
		return false;
	}
	if (op == 1)
	{
		auto founder = std::find_if(m_poolRegister.begin(), m_poolRegister.end(), [&reg](const tagRegister & r){ return reg._fd == r._fd; });
		if (founder == m_poolRegister.end())
		{
			return false;
		}
		else
		{
			*founder = reg;
			return true;
		}
	}
	else
	{
		auto founder = std::find_if(m_poolRegister.begin(), m_poolRegister.end(), [&reg](const tagRegister & r){ return reg._fd == r._fd; });
		if (founder == m_poolRegister.end())
		{
			if (op == 0)
			{
				if (m_poolRegister.size() >= FD_SETSIZE)
				{
					return false;
				}
				m_poolRegister.push_back(reg);
				return true;
			}
			else
			{
				return false;
			}

		}
		else
		{
			if (op	== 0)
			{
				return false;
			}
			founder->_ptr = nullptr;
		}
	}
	
	return true;
}


void CZSummerImpl::PostMessage(const _OnPostHandler &handle)
{
	_OnPostHandler * pHandler = new _OnPostHandler(handle);
	m_stackMessagesLock.lock();
	m_stackMessages.push_back(pHandler);
	m_stackMessagesLock.unlock();
	char c='0';
	send(m_sockpair[0], &c, 1, 0);
}

std::string CZSummerImpl::GetZSummerImplStatus()
{
	std::stringstream os;
	m_stackMessagesLock.lock();
	MessageStack::size_type msgSize = m_stackMessages.size();
	m_stackMessagesLock.unlock();
	os << " CZSummerImpl Status:  m_sockpair[2]={" << m_sockpair[0] << "," << m_sockpair[1] << "}"
		<< " m_stackMessages.size()=" << msgSize << ", current total timer=" << m_timer.GetTimersCount()
		<< " m_poolRegister=" << m_poolRegister.size();
	return os.str();
}

void CZSummerImpl::RunOnce()
{
	fd_set fdr;
	fd_set fdw;
	FD_ZERO(&fdr);
	FD_ZERO(&fdw);
	for ( auto & r : m_poolRegister)
	{
		if (!r._ptr)
		{
			continue;
		}
		if (r._rd)
		{
			FD_SET(r._fd, &fdr);
		}
		if (r._wt)
		{
			FD_SET(r._fd, &fdw);
		}
	}
	FD_SET(m_sockpair[1], &fdr);
	timeval tv;
	unsigned int ms = m_timer.GetNextExpireTime();
	tv.tv_sec = ms / 1000;
	tv.tv_usec = (ms % 1000) * 1000;
	int retCount = ::select(m_poolRegister.size(), &fdr, &fdw, NULL, &tv);
	if (retCount == -1)
	{
		if (IS_EINTR)
		{
			LCD("CZSummerImpl::RunOnce[this0x" << this << "]  select err!  " << OSTREAM_GET_LASTERROR << ", " << GetZSummerImplStatus());
			return; //! error
		}
		return;
	}

	//check timer
	{
		m_timer.CheckTimer();
		if (retCount == 0) return;//timeout
	}
	if (FD_ISSET(m_sockpair[1], &fdr))
	{
		char buf[1000];
		while (recv(m_sockpair[1], buf, 1000, 0) > 0);

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
	

	for (auto & r : m_poolRegister)
	{
		if (!r._ptr)
		{
			continue;
		}
		//tagHandle  type
		if (r._type == tagRegister::REG_TCP_ACCEPT)
		{
			CTcpAcceptImpl *pKey = (CTcpAcceptImpl *)r._ptr;
			if (FD_ISSET(r._fd, &fdr))
			{
				pKey->OnSelectMessage();
			}
		}
		else if (r._type == tagRegister::REG_TCP_SOCKET)
		{
			CTcpSocketImpl *pKey = (CTcpSocketImpl *)r._ptr;

			bool rd = r._rd && (bool)FD_ISSET(r._fd, &fdr) ;
			bool wt = r._wt && (bool)FD_ISSET(r._fd, &fdw);
			if (rd || wt)
			{
				pKey->OnSelectMessage(r._type, rd, wt);
			}
		}
		else if (r._type == tagRegister::REG_UDP_SOCKET)
		{
			CUdpSocketImpl *pKey = (CUdpSocketImpl *)r._ptr;
			bool rd = r._rd && (bool)FD_ISSET(r._fd, &fdr);
			bool wt = r._wt && (bool)FD_ISSET(r._fd, &fdw);
			if (rd || wt)
			{
				pKey->OnSelectMessage(r._type, rd, wt);
			}
		}
		else
		{
			LCE("CZSummerImpl::RunOnce[this0x" << this << "] check register event type failed !!  type=" << r._type << GetZSummerImplStatus());
		}
	}

	std::remove_if(m_poolRegister.begin(), m_poolRegister.end(), [](const tagRegister & r){return r._ptr == nullptr; });


}



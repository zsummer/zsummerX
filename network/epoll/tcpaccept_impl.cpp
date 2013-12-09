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
#include "tcpaccept_impl.h"
#include "tcpsocket_impl.h"


using namespace zsummer::network;




CTcpAcceptImpl::CTcpAcceptImpl(CZSummer &summer)
{
	m_port = 0;
	m_summer = &summer;
	m_isAcceptLock = false;
	m_register._event.data.ptr = &m_register;
	m_register._event.events = 0;
	m_register._fd = -1;
	m_register._linkstat = LS_UNINITIALIZE;
	m_register._ptr = this;
	m_register._type = tagRegister::REG_TCP_ACCEPT;
}


CTcpAcceptImpl::~CTcpAcceptImpl()
{
	if (m_register._fd != -1)
	{
		close(m_register._fd);
		m_register._fd = -1;
	}
}

bool CTcpAcceptImpl::Initialize()
{
	m_register._linkstat = LS_WAITLINK;
	return true;
}


bool CTcpAcceptImpl::OpenAccept(const char * ip, unsigned short port)
{
	if (m_summer == NULL)
	{
		LCE("m_summer not bind!");
		return false;
	}

	if (m_register._fd != -1)
	{
		LCF("accept fd is aready used!");
		return false;
	}

	m_register._fd = socket(AF_INET, SOCK_STREAM, 0);
	if (m_register._fd == -1)
	{
		LCF("socket create err errno =" << strerror(errno));
		return false;
	}

	SetNonBlock(m_register._fd);



	int bReuse = 1;
	if (setsockopt(m_register._fd, SOL_SOCKET, SO_REUSEADDR,  &bReuse, sizeof(bReuse)) != 0)
	{
		LCW("socket set reuse fail! errno=" << strerror(errno));
	}


	m_addr.sin_family = AF_INET;
	m_addr.sin_addr.s_addr = inet_addr(ip);
	m_addr.sin_port = htons(port);
	if (bind(m_register._fd, (sockaddr *) &m_addr, sizeof(m_addr)) != 0)
	{
		LCF("socket bind err, errno=" << strerror(errno));
		close(m_register._fd);
		m_register._fd = -1;
		return false;
	}

	if (listen(m_register._fd, 200) != 0)
	{
		LCF("socket listen err, errno=" << strerror(errno));
		close(m_register._fd);
		m_register._fd = -1;
		return false;
	}

	if (epoll_ctl(m_summer->m_impl.m_epoll, EPOLL_CTL_ADD, m_register._fd, &m_register._event) != 0)
	{
		LCF(" EPOLL_CTL_ADD err!  errno=" << strerror(errno));
		return false;
	}

	return true;
}
bool CTcpAcceptImpl::DoAccept(CTcpSocketPtr &s, const _OnAcceptHandler &handle)
{
	if (m_isAcceptLock)
	{
		LCF(" socket DoAccept err, dumplicate DoAccept");
		return false;
	}
	m_register._event.events = EPOLLIN;
	if (epoll_ctl(m_summer->m_impl.m_epoll, EPOLL_CTL_MOD, m_register._fd, &m_register._event) != 0)
	{
		LCF("EPOLL_CTL_MOD!  errno=" << strerror(errno));
		return false;
	}
	m_onAcceptHandler = handle;
	m_client = s;
	m_isAcceptLock = true;
	return true;
}
bool CTcpAcceptImpl::OnEPOLLMessage(bool bSuccess)
{
	//
	if (!m_isAcceptLock)
	{
		LCF("unkown error.  errno=" << strerror(errno));
		return false;
	}

	m_isAcceptLock = false;
	_OnAcceptHandler onAccept;
	onAccept.swap(m_onAcceptHandler);
	CTcpSocketPtr ps(m_client);
	m_client.reset();
	m_register._event.events = 0;
	if (epoll_ctl(m_summer->m_impl.m_epoll, EPOLL_CTL_MOD, m_register._fd, &m_register._event) != 0)
	{
		LCF("EPOLL_CTL_MOD!  errno=" << strerror(errno));
	}
	if (!bSuccess)
	{
		LCF("accept EPOLLERR, errno=" << strerror(errno));
		onAccept(EC_ERROR, ps);
		return false;
	}
	else
	{
		LCD("INFO: onaccept on link");
		sockaddr_in cltaddr;
		memset(&cltaddr, 0, sizeof(cltaddr));
		socklen_t len = sizeof(cltaddr);
		int s = accept(m_register._fd, (sockaddr *)&cltaddr, &len);
		if (s == -1)
		{
			if (errno != EAGAIN && errno != EWOULDBLOCK)
			{
				LCE("ERR: accept -1, errno=" << strerror(errno));
			}
			onAccept(EC_ERROR, ps);
			return false;
		}

		SetNonBlock(s);
		SetNoDelay(s);

		ps->m_impl.AttachEstablishedSocket(s,inet_ntoa(cltaddr.sin_addr), ntohs(cltaddr.sin_port));
		onAccept(EC_SUCCESS, ps);
	}
	return true;
}

bool CTcpAcceptImpl::Close()
{
	if (epoll_ctl(m_summer->m_impl.m_epoll, EPOLL_CTL_DEL, m_register._fd, &m_register._event) != 0)
	{
		LCF(" EPOLL_CTL_DEL err!  errno=" << strerror(errno));
	}
	shutdown(m_register._fd, SHUT_RDWR);
	close(m_register._fd);
	m_register._fd = -1;
	return true;
}


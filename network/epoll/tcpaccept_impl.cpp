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
}


CTcpAcceptImpl::~CTcpAcceptImpl()
{

}

bool CTcpAcceptImpl::Initialize()
{
	return true;
}


bool CTcpAcceptImpl::OpenAccept(const char * ip, unsigned short port)
{
	if (m_summer == NULL)
	{
		LCF("ERR: IIOServer not bind!");
		return false;
	}

	if (m_handle._fd != 0)
	{
		LCF("ERR: accept fd is aready used!");
		return false;
	}

	m_handle._fd = socket(AF_INET, SOCK_STREAM, 0);
	if (m_handle._fd == -1)
	{
		LCF("ERR: socket create err errno =" << strerror(errno));
		return false;
	}

	SetNonBlock(m_handle._fd);

	int bReuse = 1;
	if (setsockopt(m_handle._fd, SOL_SOCKET, SO_REUSEADDR,  &bReuse, sizeof(bReuse)) != 0)
	{
		LCW("WAR: socket set reuse fail! errno=" << strerror(errno));
	}


	m_addr.sin_family = AF_INET;
	m_addr.sin_addr.s_addr = inet_addr(ip);
	m_addr.sin_port = htons(port);
	if (bind(m_handle._fd, (sockaddr *) &m_addr, sizeof(m_addr)) != 0)
	{
		LCF("ERR: socket bind err, errno=" << strerror(errno));
		close(m_handle._fd);
		m_handle._fd = 0;
		return false;
	}

	if (listen(m_handle._fd, 200) != 0)
	{
		LCF("ERR: socket listen err, errno=" << strerror(errno));
		close(m_handle._fd);
		m_handle._fd = 0;
		return false;
	}

	return true;
}
bool CTcpAcceptImpl::DoAccept(const _OnAcceptHandler &handle)
{
	if (m_isAcceptLock)
	{
		LCF("ERR: socket DoAccept err, dumplicate DoAccept");
		return false;
	}
	m_onAcceptHandler = handle;
	m_handle._event.events = EPOLLIN;
	m_handle._event.data.ptr = &m_handle;
	m_handle._ptr = this;
	m_handle._type = tagRegister::REG_ACCEPT;
	if (epoll_ctl(m_summer->m_impl.m_epoll, EPOLL_CTL_ADD, m_handle._fd, &m_handle._event) != 0)
	{
		LCF("ERR: epoll ctl  accept err!  errno=" << strerror(errno));
		return false;
	}
	m_isAcceptLock = true;
	return true;
}
bool CTcpAcceptImpl::OnEPOLLMessage(bool bSuccess)
{
	m_isAcceptLock = false;
	if (!bSuccess)
	{
		LCF("ERR: accept EPOLLERR, errno=" << strerror(errno));
		Close();
		return false;
	}
	else
	{
		LCD("INFO: onaccept on link");
		sockaddr_in cltaddr;
		memset(&cltaddr, 0, sizeof(cltaddr));
		socklen_t len = sizeof(cltaddr);
		int s = accept(m_handle._fd, (sockaddr *)&cltaddr, &len);
		if (s == -1)
		{
			if (errno != EAGAIN && errno != EWOULDBLOCK)
			{
				LCE("ERR: accept -1, errno=" << strerror(errno));
				Close();
				return false;
			}
			return true;
		}

		SetNonBlock(s);
		SetNoDelay(s);
		
		CTcpSocketPtr ps(new CTcpSocket(s,inet_ntoa(cltaddr.sin_addr), ntohs(cltaddr.sin_port)));
		
	/*	ps->m_handle._fd = s;
		ps->m_handle._event.data.ptr = & ps->m_handle;
		ps->m_handle._ptr = ps;
		ps->m_handle._type = tagRegister::REG_ESTABLISHED;
		ps->m_bNeedDestroy = true;
		memcpy(&ps->m_addr, &cltaddr, sizeof(ps->m_addr));
		m_cb->OnAccept(ps); */
	}
	return true;
}

bool CTcpAcceptImpl::Close()
{
	if (epoll_ctl(m_summer->m_impl.m_epoll, EPOLL_CTL_DEL, m_handle._fd, &m_handle._event) != 0)
	{
		LCE("ERR: epoll ctl  DEL err!  errno=" << strerror(errno));
	}
	shutdown(m_handle._fd, SHUT_RDWR);
	close(m_handle._fd);
	return true;
}


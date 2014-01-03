/*
 * ZSUMMER License
 * -----------
 * 
 * ZSUMMER is licensed under the terms of the MIT license reproduced below.
 * This means that ZSUMMER is free software and can be used for both academic
 * and commercial purposes at absolutely no cost.
 * 
 * 
 * ===============================================================================
 * 
 * Copyright (C) 2010-2013 YaweiZhang <yawei_zhang@foxmail.com>.
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


#include "Schedule.h"

#include "Process.h"

CSchedule::CSchedule():m_accept( m_summer)
{
	m_iCurProcess = 0;
	m_nMaxClient = 0;
	m_bRunning = false;
}


void CSchedule::Start(std::string ip, unsigned short port,unsigned short type, unsigned int maxClient, unsigned int interval)
{
	m_ip = ip;
	m_port = port;
	m_nMaxClient = maxClient;
	if (!m_summer.Initialize())
	{
		return ;
	}

	for (int i=0; i< 1; i++)
	{
		CProcess * p = new CProcess(type !=1, type == 3 ? 0 : interval, m_ip, m_port);
		if (p->Start())
		{
			m_process.push_back(p);
		}
	}
	
	if (type == 1)
	{
		if (m_accept.OpenAccept(ip.c_str(), port))
		{
			LOGI("open server port [" << port << "] success");
		}
		CTcpSocketPtr s(new zsummer::network::CTcpSocket());
		s->Initialize(m_process[m_iCurProcess]->GetZSummer());
		m_accept.DoAccept(s, std::bind(&CSchedule::OnAccept, this, std::placeholders::_1, std::placeholders::_2, m_process[m_iCurProcess]));
	}
	else
	{
		doConnect(maxClient);
	}


	m_thread = std::thread(std::bind(&CSchedule::Run, this));
}
void CSchedule::Stop()
{
	m_bRunning = false;
}
void CSchedule::Run()
{
	m_bRunning = true;

	while (m_bRunning)
	{
		m_summer.RunOnce();
	}
}

void CSchedule::doConnect(unsigned int maxClient)
{
	for (unsigned int i = 1; i <= maxClient; i++)
	{
		CTcpSocketPtr s(new zsummer::network::CTcpSocket());
		CProcess * proc = m_process[m_iCurProcess];
		s->Initialize(proc->GetZSummer());
		proc->Post(std::bind(&CProcess::RecvSocketPtr, proc, s));
		m_iCurProcess++;
		m_iCurProcess = m_iCurProcess%(int)m_process.size();
		if (i >= 100 && maxClient > 100)
		{
			m_summer.CreateTimer(1000, std::bind(&CSchedule::doConnect, this, maxClient - i));
			break;
		}
	}
}
void CSchedule::OnAccept(zsummer::network::ErrorCode ec, CTcpSocketPtr sockptr, CProcess * process)
{
	if (ec)
	{
		LOGE("OnAccept error. ec=" <<ec );
		return;
	}
	{
		unsigned long long allClosed = 0;
		unsigned long long allOpen = 0;
		for (std::vector<CProcess *>::const_iterator iter = m_process.begin(); iter != m_process.end(); ++iter)
		{
			allClosed += (*iter)->GetTotalClosed();
			allOpen += (*iter)->GetTotalOpen();
		}
		if (allOpen - allClosed > m_nMaxClient )
		{
			LOGW("OnAccept success but current links is too many.");
			return;
		}
	}
	LOGD("OnAccept one socket");
	process->Post(std::bind(&CProcess::RecvSocketPtr, process, sockptr));

	m_iCurProcess++;
	m_iCurProcess = m_iCurProcess%(int)m_process.size();
	CTcpSocketPtr s(new zsummer::network::CTcpSocket());
	s->Initialize(m_process[m_iCurProcess]->GetZSummer());
	m_accept.DoAccept(s, std::bind(&CSchedule::OnAccept, this, std::placeholders::_1, std::placeholders::_2, m_process[m_iCurProcess]));
}


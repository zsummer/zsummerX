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
 * Copyright (C) 2010-2014 YaweiZhang <yawei_zhang@foxmail.com>.
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

CSchedule::CSchedule()
{
	m_summer = zsummer::network::CZSummerPtr(new zsummer::network::CZSummer());
	m_accept = zsummer::network::CTcpAcceptPtr(new zsummer::network::CTcpAccept(m_summer));
}


void CSchedule::Start()
{

	if (!m_summer->Initialize())
	{
		return ;
	}

	for (int i=0; i< 1; i++)
	{
		CProcess * p = new CProcess();
		if (p->Start())
		{
			m_process.push_back(p);
		}
	}
	
	if (g_startType == 0)
	{
		if (m_accept->OpenAccept(g_remoteIP.c_str(), g_remotePort))
		{
			LOGI("open server port [" << g_remotePort << "] success");
		}
		CTcpSocketPtr s(new zsummer::network::CTcpSocket());
		s->Initialize(m_process[m_iCurProcess]->GetZSummer());
		m_accept->DoAccept(s, std::bind(&CSchedule::OnAccept, this, std::placeholders::_1, std::placeholders::_2, m_process[m_iCurProcess]));
	}
	else
	{
		doConnect(g_maxClient);
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
		m_summer->RunOnce();
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
			m_summer->CreateTimer(1000, std::bind(&CSchedule::doConnect, this, maxClient - i));
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
	
	unsigned long long allClosed = 0;
	unsigned long long allOpen = 0;
	for (std::vector<CProcess *>::const_iterator iter = m_process.begin(); iter != m_process.end(); ++iter)
	{
		allClosed += (*iter)->GetTotalClosed();
		allOpen += (*iter)->GetTotalOpen();
	}
	if (allOpen - allClosed > g_maxClient )
	{
		LOGW("OnAccept success but current links is too many.");
	}
	else
	{
		LOGD("OnAccept one socket");
		process->Post(std::bind(&CProcess::RecvSocketPtr, process, sockptr));
	}



	m_iCurProcess++;
	m_iCurProcess = m_iCurProcess%(int)m_process.size();
	CTcpSocketPtr s(new zsummer::network::CTcpSocket());
	s->Initialize(m_process[m_iCurProcess]->GetZSummer());
	m_accept->DoAccept(s, std::bind(&CSchedule::OnAccept, this, std::placeholders::_1, std::placeholders::_2, m_process[m_iCurProcess]));
}


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

#include "Client.h"

CClient::CClient(CProcess &proc, CTcpSocketPtr sockptr):m_process(proc)
{
	m_sockptr = sockptr;
	memset(&m_recving, 0, sizeof(m_recving));
	memset(&m_sending, 0, sizeof(m_sending));
	m_curSendLen = 0;
	DoRecv();
}

CClient::~CClient()
{
	while (!m_sendque.empty())
	{
		delete m_sendque.front();
		m_sendque.pop();
	}
}
void CClient::DoRecv()
{
	m_recving._len = 0;
	m_sockptr->DoRecv(m_recving._orgdata, (int)2, std::bind(&CClient::OnRecv, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
}
void CClient::OnRecv(zsummer::network::ErrorCode ec, int nRecvedLen)
{
	if (ec)
	{
		LOGD("remote socket closed");
		OnClose();
		return;
	}

	m_recving._len += nRecvedLen;

	int needRecv = zsummer::protocol4z::CheckBuffIntegrity(m_recving._orgdata, m_recving._len, _MSG_BUF_LEN);
	if ( needRecv == -1)
	{
		LOGD("killed socket: CheckBuffIntegrity error ");
		m_sockptr->DoClose();
		OnClose();
		return;
	}
	if (needRecv > 0)
	{
		m_sockptr->DoRecv(m_recving._orgdata+m_recving._len, needRecv, std::bind(&CClient::OnRecv, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
		return ;
	}

	//! 解包完成 进行消息处理
	zsummer::protocol4z::ReadStream rs(m_recving._orgdata, m_recving._len);
	try
	{
		MessageEntry(rs);
	}
	catch (std::runtime_error e)
	{
		LOGD("MessageEntry catch one exception: "<< e.what() );
		m_sockptr->DoClose();
		OnClose();
		return ;
	}
	m_process.AddTotalRecvLen(m_recving._len);
	m_process.AddTotalRecvCount(1);
	//! 继续收包
	DoRecv();
}

void CClient::MessageEntry(zsummer::protocol4z::ReadStream & rs)
{
	//协议流异常会被上层捕获并关闭连接
	unsigned short protocolID = 0;
	rs >> protocolID;
	switch (protocolID)
	{
	case 1:
		{
			unsigned long long clientTick = 0;
			std::string text;
			rs >> clientTick >> text;
			Packet *p = new Packet;
			zsummer::protocol4z::WriteStream ws(p->_orgdata, _MSG_BUF_LEN);
			ws << protocolID << clientTick << text;
			p->_len = ws.GetWriteLen();
			DoSend(p);
		}
		break;
	default:
		{
			LOGI("unknown protocol id = " << protocolID);
		}
		break;
	}
}
void CClient::DoSend(Packet * pack)
{
	if (m_sending._len != 0)
	{
		m_sendque.push(pack);
	}
	else
	{
		memcpy(m_sending._orgdata, pack->_orgdata, pack->_len);
		m_sending._len= pack->_len;
		m_curSendLen = 0;
		delete pack;
		m_sockptr->DoSend(m_sending._orgdata, m_sending._len, std::bind(&CClient::OnSend, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
	}
}


void CClient::OnSend(zsummer::network::ErrorCode ec,  int nSentLen)
{
	if (ec)
	{
		LOGD("remote socket closed");
		return ;
	}
	m_curSendLen += nSentLen;
	if (m_curSendLen < m_sending._len)
	{
		m_sockptr->DoSend(m_sending._orgdata+m_curSendLen, m_sending._len - m_curSendLen, std::bind(&CClient::OnSend, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
	}
	else if (m_curSendLen == m_sending._len)
	{
		m_process.AddTotalSendCount(1);
		m_process.AddTotalSendLen(m_curSendLen);
		m_curSendLen = 0;
		if (m_sendque.empty())
		{
			m_sending._len = 0;
		}
		else
		{
			Packet *pack = m_sendque.front();
			m_sendque.pop();
			memcpy(m_sending._orgdata, pack->_orgdata, pack->_len);
			m_sending._len= pack->_len;
			m_curSendLen = 0;
			delete pack;
			m_sockptr->DoSend(m_sending._orgdata, m_sending._len, std::bind(&CClient::OnSend, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
		}
	}
}

void CClient::OnClose()
{
	LOGI("Client Closed!");
	m_process.AddTotalClosed(1);
}


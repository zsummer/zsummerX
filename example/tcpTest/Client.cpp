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

#include "Client.h"
using namespace zsummer::protocol4z;

CClient::CClient(CProcess &proc, CTcpSocketPtr sockptr):m_process(proc)
{
	m_sockptr = sockptr;
	memset(&m_recving, 0, sizeof(m_recving));
}

CClient::~CClient()
{
	while (!m_sendque.empty())
	{
		delete m_sendque.front();
		m_sendque.pop();
	}
	LOGI("~CClient");
	
}

void CClient::Initialize()
{
	
	if (g_startType == 0)
	{
		DoRecv();
		m_bEstablished = true;
		m_process.AddTotalOpen(1);
	}
	else
	{
		m_sockptr->DoConnect(g_remoteIP, g_remotePort, std::bind(& CClient::OnConnected, shared_from_this(), std::placeholders::_1));
	}	
}

void CClient::SendOnce()
{
	if (!m_bEstablished)
	{
		LOGD("client is dead. no send again");
		return;
	}
	DoSend(1, NOW_TIME, g_text);
	if (g_sendType != 0)
	{
		if (g_intervalMs > 0)
		{
			m_process.GetZSummer()->CreateTimer(g_intervalMs, std::bind(&CClient::SendOnce, shared_from_this()));
		}
		else
		{
			if (m_lastDelayTime < 2000)
			{
				m_process.GetZSummer()->Post(std::bind(&CClient::SendOnce, shared_from_this()));
			}
			else
			{
				m_process.GetZSummer()->CreateTimer(100, std::bind(&CClient::SendOnce, shared_from_this()));
			}
		}
	}
}

void CClient::OnConnected(zsummer::network::ErrorCode ec)
{
	if (ec)
	{
		LOGD("OnConnected failed. ec=" << ec);
		return;
	}
	LOGD("OnConnected success");
	DoRecv();
	m_bEstablished = true;
	m_process.AddTotalOpen(1);
	if (g_intervalMs > 0)
	{
		m_process.GetZSummer()->CreateTimer(g_intervalMs, std::bind(&CClient::SendOnce, shared_from_this()));
	}
	else
	{
		SendOnce();
	}
}

void CClient::DoRecv()
{
	m_recving._len = 0;
	bool bSuccess = m_sockptr->DoRecv(m_recving._orgdata, (int)2, std::bind(&CClient::OnRecv, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
	if (!bSuccess)
	{
		OnClose();
	}
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

	std::pair<bool, zsummer::protocol4z::DefaultStreamHeadTraits::Integer> ret = zsummer::protocol4z::CheckBuffIntegrity<zsummer::protocol4z::DefaultStreamHeadTraits>(m_recving._orgdata, m_recving._len, _MSG_BUF_LEN);
	if (!ret.first)
	{
		LOGD("killed socket: CheckBuffIntegrity error ");
		m_sockptr->DoClose();
		OnClose();
		return;
	}
	if (ret.second > 0)
	{
		m_sockptr->DoRecv(m_recving._orgdata + m_recving._len, ret.second, std::bind(&CClient::OnRecv, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
		return ;
	}

	//! 解包完成 进行消息处理
	zsummer::protocol4z::ReadStream<DefaultStreamHeadTraits> rs(m_recving._orgdata, m_recving._len);
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

void CClient::MessageEntry(zsummer::protocol4z::ReadStream<zsummer::protocol4z::DefaultStreamHeadTraits> & rs)
{
	//协议流异常会被上层捕获并关闭连接
	unsigned short protocolID = 0;
	rs >> protocolID;
	switch (protocolID)
	{
	case 1:
		{
			unsigned long long clientTick = 0;
			m_recvTextCache.clear();
			rs >> clientTick >> m_recvTextCache;
			if (g_startType == 0)
			{
				DoSend(protocolID, clientTick, m_recvTextCache.c_str());
			}
			else
			{
				unsigned long long tick = NOW_TIME;
				if (tick < clientTick)
				{
					LOGE("now time[" << tick << "] < last time[" << clientTick << "].");
					throw std::runtime_error("now time < last time");
				}
				tick -= clientTick;
				m_lastDelayTime = tick;
				m_process.AddTotalEcho(1);
				m_process.AddTotalEchoTime(tick);
				if (g_sendType == 0)
				{
					SendOnce();
				}
			}
		}
		break;
	default:
		{
			LOGI("unknown protocol id = " << protocolID);
		}
		break;
	}
}



void CClient::DoSend(unsigned short protocolID, unsigned long long clientTick, const char* text)
{
	zsummer::protocol4z::WriteStream<DefaultStreamHeadTraits> ws;
	ws << protocolID << clientTick << text;
	DoSend(ws.GetStream(), ws.GetStreamLen());
}

void CClient::DoSend(char *buf, unsigned short len)
{
	if (m_sendLen != 0)
	{
		Packet *pack = new Packet;
		memcpy(pack->_orgdata, buf, len);
		pack->_len = len;
		m_sendque.push(pack);
	}
	else
	{
		memcpy(m_sendBuff, buf, len);
		m_sendLen = len;
		m_curSendLen = 0;
		m_sockptr->DoSend(m_sendBuff, m_sendLen, std::bind(&CClient::OnSend, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
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
	if (m_curSendLen < m_sendLen)
	{
		m_sockptr->DoSend(m_sendBuff+m_curSendLen, m_sendLen - m_curSendLen, std::bind(&CClient::OnSend, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
	}
	else if (m_curSendLen == m_sendLen)
	{
		m_process.AddTotalSendCount(1);
		m_process.AddTotalSendLen(m_curSendLen);
		m_curSendLen = 0;
		m_sendLen = 0;
		if (!m_sendque.empty())
		{
			do
			{
				Packet *pack = m_sendque.front();
				if (_SEND_BUF_LEN - m_sendLen < pack->_len)
				{
					break;
				}
				m_sendque.pop();
				memcpy(m_sendBuff+m_sendLen, pack->_orgdata, pack->_len);
				m_sendLen += pack->_len;
				delete pack;

			} while (!m_sendque.empty());
			
			m_sockptr->DoSend(m_sendBuff, m_sendLen, std::bind(&CClient::OnSend, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
		}
	}
}

void CClient::OnClose()
{
	LOGI("Client Closed!");
	m_process.AddTotalClosed(1);
	m_bEstablished = false;
}


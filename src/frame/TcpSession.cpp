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

#include <zsummerX/TcpSession.h>
#include <zsummerX/TcpSessionManager.h>
#include <zsummerX/MessageDispatch.h>
using namespace zsummer::protocol4z;

CTcpSession::CTcpSession()
{

}


CTcpSession::~CTcpSession()
{
	while (!m_sendque.empty())
	{
		delete m_sendque.front();
		m_sendque.pop();
	}
	while (!m_freeCache.empty())
	{
		delete m_freeCache.front();
		m_freeCache.pop();
	}
	LOGI("~CTcpSession");
	
}

void CTcpSession::BindTcpSocketPrt(CTcpSocketPtr sockptr, SessionID sessionID, bool isConnector, AccepterID aID)
{
	CleanSession();
	m_sockptr = sockptr;
	m_sessionID = sessionID;
	m_isConnector = isConnector;
	if (! m_isConnector)
	{
		m_acceptID = aID;
		m_heartbeatID = CTcpSessionManager::getRef().CreateTimer(HEARTBEART_INTERVAL, std::bind(&CTcpSession::OnHeartbeat, shared_from_this()));
	}
}

void CTcpSession::CleanSession(bool isCleanWithoutMsgPack)
{
	m_sockptr.reset();
	m_sessionID = 0;
	m_isConnector = false;
	m_recving.bufflen = 0;
	m_sending.bufflen = 0;
	m_sendingCurIndex = 0;
	if (isCleanWithoutMsgPack)
	{
		while (!m_sendque.empty())
		{
			m_freeCache.push(m_sendque.front());
			m_sendque.pop();
		}
	}
}

void CTcpSession::DoConnect(const tagConnctorConfigTraits & traits)
{
	m_sockptr->DoConnect(traits.remoteIP, traits.remotePort, std::bind(&CTcpSession::OnConnected, shared_from_this(), std::placeholders::_1, traits));
	LOGI("DoConnected : traits=" << traits);
}

void CTcpSession::OnConnected(zsummer::network::ErrorCode ec, const tagConnctorConfigTraits & traits)
{
	if (ec/* && traits.reconnectMaxCount == 0*/)
	{
		LOGE("OnConnected failed. ec=" << ec 
			<< ",  traits=" << traits);
		CTcpSessionManager::getRef().OnConnectorStatus(m_sessionID, false, shared_from_this());
		return;
	}
// 	else if (ec)
// 	{
// 		LOGE("OnConnected failed. ec=" << ec
// 			<< ",  traits=" << traits);
// 		tagConnctorConfigTraits t = traits;
// 		t.reconnectMaxCount--;
// 		CTcpSessionManager::getRef().CreateTimer(traits.reconnectInterval, std::bind(&CTcpSession::DoConnect, shared_from_this(), t));
// 		return;
// 	}
	LOGI("OnConnected success.  traits=" << traits);
	//用户在该回调中发送的第一包会跑到发送堆栈的栈顶. 适用于建立会话后的一些服务状态信息更新的情况, 但如果还有其他包 则会放在栈底.
	CTcpSessionManager::getRef().OnConnectorStatus(m_sessionID, true, shared_from_this());
	if (m_sending.bufflen == 0 && !m_sendque.empty())
	{
		memcpy(m_sending.buff, m_sendque.front()->buff, m_sendque.front()->bufflen);
		m_sending.bufflen = m_sendque.front()->bufflen;
		m_sendingCurIndex = 0;
		m_freeCache.push(m_sendque.front());
		m_sendque.pop();
	}
	m_heartbeatID = CTcpSessionManager::getRef().CreateTimer(HEARTBEART_INTERVAL, std::bind(&CTcpSession::OnHeartbeat, shared_from_this()));
	
	DoRecv();
}

void CTcpSession::DoRecv()
{
	m_recving.bufflen = 0;
	bool recvRet = m_sockptr->DoRecv(m_recving.buff, FrameStreamTraits::HeadLen, std::bind(&CTcpSession::OnRecv, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
	if (!recvRet)
	{
		OnClose();
	}
}
void CTcpSession::Close()
{
	m_sockptr->DoClose();
	m_heartbeatID = 0;
}

void CTcpSession::OnRecv(zsummer::network::ErrorCode ec, int nRecvedLen)
{
	if (ec)
	{
		LOGD("remote socket closed");
		OnClose();
		return;
	}

	m_recving.bufflen += nRecvedLen;

	std::pair<bool, FrameStreamTraits::Integer> ret = zsummer::protocol4z::CheckBuffIntegrity<FrameStreamTraits>(m_recving.buff, m_recving.bufflen, MSG_BUFF_MAX_LEN);
	if (!ret.first)
	{
		LOGD("killed socket: CheckBuffIntegrity error ");
		m_sockptr->DoClose();
		return;
	}
	if (ret.second > 0)
	{
		m_sockptr->DoRecv(m_recving.buff + m_recving.bufflen, ret.second, std::bind(&CTcpSession::OnRecv, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
		return ;
	}


	try
	{
		ReadStreamPack rs(m_recving.buff, m_recving.bufflen);
		ProtocolID protocolID = 0;
		rs >> protocolID;
		if (m_isConnector)
		{
			CMessageDispatcher::getRef().DispatchConnectorMessage(m_sessionID, protocolID, rs);
		}
		else
		{
			CMessageDispatcher::getRef().DispatchSessionMessage(m_acceptID, m_sessionID, protocolID, rs);
		}
		
		
	}
	catch (std::runtime_error e)
	{
		LOGD("MessageEntry catch one exception: "<< e.what() );
		m_sockptr->DoClose();
		OnClose();
		return ;
	}

	DoRecv();
}

void CTcpSession::DoSend(const char *buf, unsigned int len)
{
	if (m_sending.bufflen != 0)
	{
		tagMSGPack *pack = NULL;
		if (m_freeCache.empty())
		{
			pack = new tagMSGPack(0);
		}
		else
		{
			pack = m_freeCache.front();
			m_freeCache.pop();
		}
		
		memcpy(pack->buff, buf, len);
		pack->bufflen = len;
		m_sendque.push(pack);
	}
	else
	{
		memcpy(m_sending.buff, buf, len);
		m_sending.bufflen = len;
		bool sendRet = m_sockptr->DoSend(m_sending.buff, m_sending.bufflen, std::bind(&CTcpSession::OnSend, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
		if (!sendRet)
		{
			OnClose();
		}
	}
}


void CTcpSession::OnSend(zsummer::network::ErrorCode ec,  int nSentLen)
{
	if (ec)
	{
		LOGD("remote socket closed");
		OnClose();
		return ;
	}
	m_sendingCurIndex += nSentLen;
	if (m_sendingCurIndex < m_sending.bufflen)
	{
		bool sendRet = m_sockptr->DoSend(m_sending.buff + m_sendingCurIndex, m_sending.bufflen - m_sendingCurIndex, std::bind(&CTcpSession::OnSend, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
		if (!sendRet)
		{
			OnClose();
		}
		
	}
	else if (m_sendingCurIndex == m_sending.bufflen)
	{
		m_sendingCurIndex = 0;
		m_sending.bufflen = 0;
		if (!m_sendque.empty())
		{
			do
			{
				tagMSGPack *pack = m_sendque.front();
				m_sendque.pop();
				memcpy(m_sending.buff + m_sending.bufflen, pack->buff, pack->bufflen);
				m_sending.bufflen += pack->bufflen;
				pack->bufflen = 0;
				m_freeCache.push(pack);

				if (m_sendque.empty())
				{
					break;
				}
				if (MSG_BUFF_MAX_LEN - m_sending.bufflen < m_sendque.front()->bufflen)
				{
					break;
				}
			} while (true);
			
			bool sendRet = m_sockptr->DoSend(m_sending.buff, m_sending.bufflen, std::bind(&CTcpSession::OnSend, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
			if (!sendRet)
			{
				OnClose();
			}
		}
	}
}

void CTcpSession::OnHeartbeat()
{
	if (m_isConnector)
	{
		CMessageDispatcher::getRef().DispatchOnConnectorHeartbeat(m_sessionID);
	}
	else
	{
		CMessageDispatcher::getRef().DispatchOnSessionHeartbeat(m_acceptID, m_sessionID);
	}
	if (m_heartbeatID != 0)
	{
		m_heartbeatID = CTcpSessionManager::getRef().CreateTimer(HEARTBEART_INTERVAL, std::bind(&CTcpSession::OnHeartbeat, shared_from_this()));
	}
}

void CTcpSession::OnClose()
{
	LOGI("Client Closed!");
	if (m_heartbeatID >0)
	{
		CTcpSessionManager::getRef().CancelTimer(m_heartbeatID);
		m_heartbeatID = 0;
	}
	
	if (m_isConnector)
	{
		CTcpSessionManager::getRef().OnConnectorStatus(m_sessionID, false, shared_from_this());
	}
	else
	{
		CTcpSessionManager::getRef().OnSessionClose(m_acceptID, m_sessionID);
	}
	
}


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

#include <zsummerX/FrameTcpSession.h>
#include <zsummerX/FrameTcpSessionManager.h>
#include <zsummerX/FrameMessageDispatch.h>

using namespace zsummer::proto4z;


CTcpSession::CTcpSession()
{
	std::string key = "zhangyawei_zhang@foxmail.com";
	m_rc4StateRead.MakeSBox(key);
	m_rc4StateWrite.MakeSBox(key);
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
	m_sockptr.reset();
	LOGI("~CTcpSession. global _g_totalCreatedCTcpSocketObjs=" << zsummer::network::_g_totalCreatedCTcpSocketObjs << ", _g_totalClosedCTcpSocketObjs=" << zsummer::network::_g_totalClosedCTcpSocketObjs);
}
void CTcpSession::CleanSession(bool isCleanAllData)
{
	m_sockptr.reset();
	m_sessionID = InvalidSeesionID;
	m_acceptID = InvalidAccepterID;
	m_connectorID = InvalidConnectorID;
	m_heartbeatID = InvalidTimerID;
	std::string key = "zhangyawei_zhang@foxmail.com";
	m_rc4StateRead.MakeSBox(key);
	m_rc4StateWrite.MakeSBox(key);

	m_recving.bufflen = 0;
	m_sending.bufflen = 0;
	m_sendingCurIndex = 0;
	if (isCleanAllData)
	{
		while (!m_sendque.empty())
		{
			m_freeCache.push(m_sendque.front());
			m_sendque.pop();
		}
	}
}

bool CTcpSession::BindTcpSocketPrt(CTcpSocketPtr sockptr, AccepterID aID, SessionID sID, ProtoType pt)
{
	CleanSession(true);
	m_sockptr = sockptr;
	m_sessionID = sID;
	m_acceptID = aID;
	m_protoType = pt;
	if (!DoRecv())
	{
		LOGW("BindTcpSocketPrt Failed.");
		return false;
	}
	m_heartbeatID = CTcpSessionManager::getRef().CreateTimer(HEARTBEART_INTERVAL, std::bind(&CTcpSession::OnHeartbeat, shared_from_this()));
	return true;
}

void CTcpSession::BindTcpConnectorPtr(CTcpSocketPtr sockptr, const std::pair<tagConnctorConfigTraits, tagConnctorInfo> & config)
{
	CleanSession(config.first.reconnectCleanAllData);
	m_sockptr = sockptr;
	m_connectorID = config.first.cID;
	m_protoType = config.first.protoType;

	bool connectRet = m_sockptr->DoConnect(config.first.remoteIP, config.first.remotePort,
		std::bind(&CTcpSession::OnConnected, shared_from_this(), std::placeholders::_1, config));
	if (!connectRet)
	{
		LOGF("DoConnected Failed: traits=" << config.first);
		return ;
	}
	LOGI("DoConnected : traits=" << config.first);
	return ;
}




void CTcpSession::OnConnected(zsummer::network::ErrorCode ec, const std::pair<tagConnctorConfigTraits, tagConnctorInfo> & config)
{
	if (ec)
	{
		LOGE("OnConnected failed. ec=" << ec 
			<< ",  config=" << config.first);
		CTcpSessionManager::getRef().OnConnectorStatus(config.first.cID, false, shared_from_this());
		return;
	}
	LOGI("OnConnected success.  config=" << config.first);
	
	if (!DoRecv())
	{
		OnClose();
		return;
	}
	m_heartbeatID = CTcpSessionManager::getRef().CreateTimer(HEARTBEART_INTERVAL, std::bind(&CTcpSession::OnHeartbeat, shared_from_this()));
	
	//用户在该回调中发送的第一包会跑到发送堆栈的栈顶.
	CTcpSessionManager::getRef().OnConnectorStatus(m_connectorID, true, shared_from_this());
	if (m_sending.bufflen == 0 && !m_sendque.empty())
	{
		MessagePack *tmp = m_sendque.front();
		m_sendque.pop();
		DoSend(tmp->buff, tmp->bufflen);
		tmp->bufflen = 0;
		m_freeCache.push(tmp);
	}
}

bool CTcpSession::DoRecv()
{
	return m_sockptr->DoRecv(m_recving.buff + m_recving.bufflen, SEND_RECV_CHUNK_SIZE - m_recving.bufflen, std::bind(&CTcpSession::OnRecv, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
}

void CTcpSession::Close()
{
	m_sockptr->DoClose();
	CTcpSessionManager::getRef().CancelTimer(m_heartbeatID);
	m_heartbeatID = InvalidTimerID;
}

void CTcpSession::OnRecv(zsummer::network::ErrorCode ec, int nRecvedLen)
{
	if (ec)
	{
		LOGD("remote socket closed");
		OnClose();
		return;
	}
	if (m_bRC4Encryption)
	{
		m_rc4StateRead.RC4Encryption((unsigned char*)m_recving.buff + m_recving.bufflen, nRecvedLen);
	}
	m_recving.bufflen += nRecvedLen;

	//分包
	unsigned int usedIndex = 0;
	do 
	{
		if (m_protoType == PT_TCP)
		{
			auto ret = zsummer::proto4z::CheckBuffIntegrity<FrameStreamTraits>(m_recving.buff + usedIndex, m_recving.bufflen - usedIndex, SEND_RECV_CHUNK_SIZE - usedIndex);
			if (ret.first == zsummer::proto4z::IRT_CORRUPTION
				|| (ret.first == zsummer::proto4z::IRT_SHORTAGE && ret.second + m_recving.bufflen > SEND_RECV_CHUNK_SIZE))
			{
				LOGT("killed socket: CheckBuffIntegrity error ");
				m_sockptr->DoClose();
				OnClose();
				return;
			}
			if (ret.first == zsummer::proto4z::IRT_SHORTAGE)
			{
				break;
			}
			try
			{
				bool bOrgReturn = false;
				if (m_connectorID != InvalidConnectorID)
				{
					bOrgReturn = CMessageDispatcher::getRef().DispatchOrgConnectorMessage(m_connectorID, m_recving.buff + usedIndex, ret.second);
				}
				else if (m_sessionID != InvalidSeesionID && m_acceptID != InvalidAccepterID)
				{
					bOrgReturn = CMessageDispatcher::getRef().DispatchOrgSessionMessage(m_acceptID, m_sessionID, m_recving.buff + usedIndex, ret.second);
				}
				if (!bOrgReturn)
				{
					LOGW("Dispatch Message failed. ");
					continue;
				}

				ReadStreamPack rs(m_recving.buff + usedIndex, ret.second);
				ProtocolID protocolID = 0;
				rs >> protocolID;
				if (m_connectorID != InvalidConnectorID)
				{
					CMessageDispatcher::getRef().DispatchConnectorMessage(m_connectorID, protocolID, rs);
				}
				else if (m_sessionID != InvalidSeesionID && m_acceptID != InvalidAccepterID)
				{
					CMessageDispatcher::getRef().DispatchSessionMessage(m_acceptID, m_sessionID, protocolID, rs);
				}
			}
			catch (std::runtime_error e)
			{
				LOGW("MessageEntry catch one exception: " << e.what());
				m_sockptr->DoClose();
				OnClose();
				return;
			}
			usedIndex += ret.second;
		}
		else
		{
			zsummer::proto4z::HTTPHeadMap head;
			std::string body;
			unsigned int usedLen = 0;
			auto ret = zsummer::proto4z::CheckHTTPBuffIntegrity(m_recving.buff + usedIndex, m_recving.bufflen - usedIndex, SEND_RECV_CHUNK_SIZE - usedIndex,
				head, body, usedLen);
			if (ret == zsummer::proto4z::IRT_CORRUPTION)
			{
				if (m_connectorID != InvalidConnectorID)
				{
					CMessageDispatcher::getRef().DispatchConnectorHTTPMessage(m_connectorID, head, body);
				}
				else if (m_sessionID != InvalidSeesionID && m_acceptID != InvalidAccepterID)
				{
					CMessageDispatcher::getRef().DispatchSessionHTTPMessage(m_acceptID, m_sessionID, head, body);
				}
				LOGT("killed socket: CheckHTTPBuffIntegrity error ");
				m_sockptr->DoClose();
				OnClose();
				return;
			}
			if (ret == zsummer::proto4z::IRT_SHORTAGE)
			{
				break;
			}
			if (m_connectorID != InvalidConnectorID)
			{
				if (!CMessageDispatcher::getRef().DispatchConnectorHTTPMessage(m_connectorID, head, body))
				{
					LOGT("killed socket: DispatchConnectorHTTPMessage error ");
					m_sockptr->DoClose();
					OnClose();
					return;
				}
			}
			else if (m_sessionID != InvalidSeesionID && m_acceptID != InvalidAccepterID)
			{
				if (CMessageDispatcher::getRef().DispatchSessionHTTPMessage(m_acceptID, m_sessionID, head, body))
				{
					LOGT("killed socket: DispatchSessionHTTPMessage error ");
					m_sockptr->DoClose();
					OnClose();
					return;
				}
			}
			usedIndex += usedLen;
		}
		
	} while (true);
	
	
	if (usedIndex > 0)
	{
		m_recving.bufflen = m_recving.bufflen - usedIndex;
		if (m_recving.bufflen > 0)
		{
			memmove(m_recving.buff, m_recving.buff + usedIndex, m_recving.bufflen);
		}
	}
	
	if (!DoRecv())
	{
		OnClose();
	}
}

void CTcpSession::DoSend(const char *buf, unsigned int len)
{
	if (m_bRC4Encryption)
	{
		m_rc4StateWrite.RC4Encryption((unsigned char*)buf, len);
	}
	
	if (m_sending.bufflen != 0)
	{
		MessagePack *pack = NULL;
		if (m_freeCache.empty())
		{
			pack = new MessagePack();
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
			LOGW("Send Failed");
		}
	}
}


void CTcpSession::OnSend(zsummer::network::ErrorCode ec,  int nSentLen)
{
	if (ec)
	{
		LOGD("remote socket closed");
		return ;
	}
	m_sendingCurIndex += nSentLen;
	if (m_sendingCurIndex < m_sending.bufflen)
	{
		bool sendRet = m_sockptr->DoSend(m_sending.buff + m_sendingCurIndex, m_sending.bufflen - m_sendingCurIndex, std::bind(&CTcpSession::OnSend, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
		if (!sendRet)
		{
			LOGW("Send Failed");
			return;
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
				MessagePack *pack = m_sendque.front();
				m_sendque.pop();
				memcpy(m_sending.buff + m_sending.bufflen, pack->buff, pack->bufflen);
				m_sending.bufflen += pack->bufflen;
				pack->bufflen = 0;
				m_freeCache.push(pack);

				if (m_sendque.empty())
				{
					break;
				}
				if (SEND_RECV_CHUNK_SIZE - m_sending.bufflen < m_sendque.front()->bufflen)
				{
					break;
				}
			} while (true);
			
			bool sendRet = m_sockptr->DoSend(m_sending.buff, m_sending.bufflen, std::bind(&CTcpSession::OnSend, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
			if (!sendRet)
			{
				LOGW("Send Failed");
				return;
			}
		}
	}
}

void CTcpSession::OnHeartbeat()
{
	if (m_connectorID != InvalidConnectorID)
	{
		CMessageDispatcher::getRef().DispatchOnConnectorHeartbeat(m_connectorID);
	}
	else if (m_sessionID != InvalidSeesionID && m_acceptID != InvalidAccepterID)
	{
		CMessageDispatcher::getRef().DispatchOnSessionHeartbeat(m_acceptID, m_sessionID);
	}
	else
	{
		LOGE("Unknown heartbeat");
	}
	if (m_heartbeatID == InvalidTimerID)
	{
		return;
	}
	m_heartbeatID = CTcpSessionManager::getRef().CreateTimer(HEARTBEART_INTERVAL, std::bind(&CTcpSession::OnHeartbeat, shared_from_this()));
	
}

void CTcpSession::OnClose()
{
	LOGI("Client Closed!");
	if (m_heartbeatID != InvalidTimerID)
	{
		CTcpSessionManager::getRef().CancelTimer(m_heartbeatID);
		m_heartbeatID = InvalidTimerID;
	}
	
	if (m_connectorID != InvalidConnectorID)
	{
		CTcpSessionManager::getRef().OnConnectorStatus(m_connectorID, false, shared_from_this());
	}
	else if (m_sessionID != InvalidSeesionID && m_acceptID != InvalidAccepterID)
	{
		CTcpSessionManager::getRef().OnSessionClose(m_acceptID, m_sessionID);
	}
}


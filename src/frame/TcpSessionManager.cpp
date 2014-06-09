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

CTcpSessionManager & CTcpSessionManager::getRef()
{
	static CTcpSessionManager m_manager;
	return m_manager;
}
CTcpSessionManager::CTcpSessionManager()
{
	m_summer = std::shared_ptr<zsummer::network::CZSummer>(new zsummer::network::CZSummer());
}

bool CTcpSessionManager::Start()
{
	
	if (!m_summer->Initialize())
	{
		return false;
	}
	return true;
}

void CTcpSessionManager::Stop()
{
	m_bRunning = false;
}

void CTcpSessionManager::Run()
{
	m_bRunning = true;
	while (m_bRunning)
	{
		m_summer->RunOnce();
	}
}

void CTcpSessionManager::OnAcceptNewClient(zsummer::network::ErrorCode ec, CTcpSocketPtr s, CTcpAcceptPtr accepter, AccepterID aID)
{
	if (ec)
	{
		return;
	}
	
	auto iter = m_mapAccepterConfig.find(aID);
	if (iter == m_mapAccepterConfig.end())
	{
		return;
	}

	std::string remoteIP;
	unsigned short remotePort = 0;
	s->GetPeerInfo(remoteIP, remotePort);
	if (m_mapTcpSessionPtr.size() > iter->second.maxSessions)
	{
		LOGW("Too Many Sessions. The new socket will closed. remoteAddress=" << remoteIP << ":" << remotePort << ", Current AcceptID=" << aID
			<< ", localListenIP=" << iter->second.listenIP << ", localListenPort=" << iter->second.listenPort << ", MaxSession=" << iter->second.maxSessions
			<< ", currentSessions=" << m_mapTcpSessionPtr.size() << ", Notes: currentSessions is a global number");
	}
	else
	{
		LOGD("Accept new Sessions. The new socket  remoteAddress=" << remoteIP << ":" << remotePort << ", Current AcceptID=" << aID
			<< ", localListenIP=" << iter->second.listenIP << ", localListenPort=" << iter->second.listenPort << ", MaxSession=" << iter->second.maxSessions
			<< ", currentSessions=" << m_mapTcpSessionPtr.size() << ", Notes: currentSessions is a global number");

		BindEstablishedSocketPtr(s, aID);
	}
	
	CTcpSocketPtr newclient(new zsummer::network::CTcpSocket);
	newclient->Initialize(m_summer);
	accepter->DoAccept(newclient, std::bind(&CTcpSessionManager::OnAcceptNewClient, this, std::placeholders::_1, std::placeholders::_2, accepter,aID));
}

bool CTcpSessionManager::BindEstablishedSocketPtr(CTcpSocketPtr sockptr, AccepterID aID)
{
	m_lastSessionID++;
	CTcpSessionPtr client(new CTcpSession());
	client->BindTcpSocketPrt(sockptr, m_lastSessionID, false, aID);
	m_mapTcpSessionPtr[m_lastSessionID] = client;
	client->DoRecv();

	CMessageDispatcher::getRef().DispatchOnSessionEstablished(m_lastSessionID);
	return true;
}


void CTcpSessionManager::SendOrgConnectorData(SessionID sID, const char * orgData, unsigned int orgDataLen)
{
	std::map<SessionID, CTcpSessionPtr>::iterator iter = m_mapConnectorPtr.find(sID);
	if (iter == m_mapConnectorPtr.end())
	{
		LOGW("SendOrgConnectorData NOT FOUND SessionID. SessionID=" << sID << ", dataLen=" << orgDataLen);
		return;
	}
	iter->second->DoSend(orgData, orgDataLen);
}
void CTcpSessionManager::SendConnectorData(SessionID sID, ProtocolID pID, const char * userData, unsigned int userDataLen)
{
	WriteStreamPack ws;
	ws << pID;
	ws.AppendOriginalData(userData, userDataLen);
	SendOrgConnectorData(sID, ws.GetStream(), ws.GetStreamLen());
}

void CTcpSessionManager::SendOrgSessionData(SessionID sID, const char * orgData, unsigned int orgDataLen)
{
	std::map<SessionID, CTcpSessionPtr>::iterator iter = m_mapTcpSessionPtr.find(sID);
	if (iter == m_mapTcpSessionPtr.end())
	{
		LOGW("SendOrgSessionData NOT FOUND SessionID. SessionID=" << sID << ", dataLen=" << orgDataLen);
		return;
	}
	iter->second->DoSend(orgData, orgDataLen);
}
void CTcpSessionManager::SendSessionData(SessionID sID, ProtocolID pID, const char * userData, unsigned int userDataLen)
{
	WriteStreamPack ws;
	ws << pID;
	ws.AppendOriginalData(userData, userDataLen);
	SendOrgSessionData(sID, ws.GetStream(), ws.GetStreamLen());
}

void CTcpSessionManager::KickSession(SessionID sID)
{
	std::map<SessionID, CTcpSessionPtr>::iterator iter = m_mapTcpSessionPtr.find(sID);
	if (iter == m_mapTcpSessionPtr.end())
	{
		LOGW("KickSession NOT FOUND SessionID. SessionID=" << sID );
		return;
	}
	iter->second->Close();
}

void CTcpSessionManager::BreakConnector(SessionID sID)
{
	std::map<SessionID, CTcpSessionPtr>::iterator iter = m_mapConnectorPtr.find(sID);
	if (iter == m_mapConnectorPtr.end())
	{
		LOGW("BreakConnector NOT FOUND SessionID. SessionID=" << sID);
		return;
	}
	iter->second->Close();
}

void CTcpSessionManager::OnSessionClose(SessionID sID)
{
	m_mapTcpSessionPtr.erase(sID);
	CMessageDispatcher::getRef().DispatchOnSessionDisconnect(sID);
}
SessionID CTcpSessionManager::AddConnector(const tagConnctorConfigTraits &traits)
{
	m_lastConnectorID++;
	m_mapConnectorConfig[m_lastConnectorID] = traits;
	CTcpSocketPtr sockPtr(new zsummer::network::CTcpSocket());
	sockPtr->Initialize(m_summer);
	CTcpSessionPtr sessionPtr(new CTcpSession());
	sessionPtr->BindTcpSocketPrt(sockPtr, m_lastConnectorID, true);
	sessionPtr->DoConnect(traits);
	return m_lastAcceptorID;
}


void CTcpSessionManager::OnConnectorStatus(SessionID connectorID, bool bConnected, CTcpSessionPtr connector)
{
	std::map<SessionID, tagConnctorConfigTraits>::iterator config = m_mapConnectorConfig.find(connectorID);
	if (config == m_mapConnectorConfig.end())
	{
		LOGE("Unkwon Connector. Not Found ConnectorID=" << connectorID);
		return;
	}
	std::map<SessionID, CTcpSessionPtr>::iterator iter = m_mapConnectorPtr.find(connectorID);
	if (iter== m_mapConnectorPtr.end())
	{
		if (!bConnected)
		{
			//reconnect failed.
			LOGE("END Reconnect. connect");
			CMessageDispatcher::getRef().DispatchOnConnectorDisconnect(connectorID);
		}
		else
		{
			m_mapConnectorPtr[connectorID] = connector;
			CMessageDispatcher::getRef().DispatchOnConnectorEstablished(connectorID);

		}
		return;
	}

	if (!bConnected)
	{
		m_mapConnectorPtr.erase(connectorID);
		if (config->second.reconnect)
		{
			CTcpSocketPtr sockPtr(new zsummer::network::CTcpSocket());
			sockPtr->Initialize(m_summer);
			connector->BindTcpSocketPrt(sockPtr, m_lastConnectorID, true);
			connector->DoConnect(config->second);
		}
	}
}
AccepterID CTcpSessionManager::AddAcceptor(const tagAcceptorConfigTraits &traits)
{
	m_lastAcceptorID++;
	m_mapAccepterConfig[m_lastAcceptorID] = traits;
	CTcpAcceptPtr accepter(new zsummer::network::CTcpAccept(m_summer));
	if (!accepter->OpenAccept(traits.listenIP, traits.listenPort))
	{
		return m_lastAcceptorID;
	}
	m_mapAccepterPtr[m_lastAcceptorID] = accepter;
	CTcpSocketPtr newclient(new zsummer::network::CTcpSocket);
	newclient->Initialize(m_summer);
	accepter->DoAccept(newclient, std::bind(&CTcpSessionManager::OnAcceptNewClient, this, std::placeholders::_1, std::placeholders::_2, accepter, m_lastAcceptorID));
	return m_lastAcceptorID;
}

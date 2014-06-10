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
	auto iter = m_mapAccepterConfig.find(aID);
	if (iter == m_mapAccepterConfig.end())
	{
		LOGE("Unknown DoAccept Result Error. ec=" << ec << ", aID=" << aID);
		return;
	}
	if (ec)
	{
		LOGE("DoAccept Result Error. ec=" << ec << ", traits=" << iter->second);
		return;
	}
	


	std::string remoteIP;
	unsigned short remotePort = 0;
	s->GetPeerInfo(remoteIP, remotePort);
	if (!iter->second.whitelistIP.empty())
	{
		bool checkSucess = true;
		for (auto white : iter->second.whitelistIP)
		{
			if (remoteIP.size() >= white.size())
			{
				if (remoteIP.compare(0,white.size(), white) == 0)
				{
					checkSucess = true;
					break;
				}
			}
		}
		LOGI("Accept New Client Check Whitelist " << (checkSucess ? "Sucess. " : "failed. ") << "remoteAdress=" << remoteIP << ":" << remotePort
			<< ", trais=" << iter->second);
		if (!checkSucess)
		{
			CTcpSocketPtr newclient(new zsummer::network::CTcpSocket);
			newclient->Initialize(m_summer);
			accepter->DoAccept(newclient, std::bind(&CTcpSessionManager::OnAcceptNewClient, this, std::placeholders::_1, std::placeholders::_2, accepter, aID));
			return;
		}
	}
	
	
	auto currentSessions = m_mapTcpSessionPtr[iter->second.aID].size();
	if (currentSessions > iter->second.maxSessions)
	{
		LOGW("Accept New Client. Too Many Sessions And The new socket will closed. remoteAddress=" << remoteIP << ":" << remotePort 
			<< ", Aready linked sessions = " << currentSessions << ", trais=" << iter->second);
	}
	else
	{
		LOGD("Accept New Client. Accept new Sessions. The new socket  remoteAddress=" << remoteIP << ":" << remotePort 
			<< ", Aready linked sessions = " << currentSessions << ", trais=" << iter->second);
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
	m_mapTcpSessionPtr[aID][m_lastSessionID] = client;
	client->DoRecv();
	CMessageDispatcher::getRef().DispatchOnSessionEstablished(aID, m_lastSessionID);
	return true;
}


void CTcpSessionManager::SendOrgConnectorData(ConnectorID cID, const char * orgData, unsigned int orgDataLen)
{
	auto iter = m_mapConnectorPtr.find(cID);
	if (iter == m_mapConnectorPtr.end())
	{
		LOGW("SendOrgConnectorData NOT FOUND SessionID. ConnectorID=" << cID << ", dataLen=" << orgDataLen);
		return;
	}
	iter->second->DoSend(orgData, orgDataLen);
}
void CTcpSessionManager::SendConnectorData(ConnectorID cID, ProtocolID pID, const char * userData, unsigned int userDataLen)
{
	WriteStreamPack ws;
	ws << pID;
	ws.AppendOriginalData(userData, userDataLen);
	SendOrgConnectorData(cID, ws.GetStream(), ws.GetStreamLen());
}

void CTcpSessionManager::SendOrgSessionData(AccepterID aID, SessionID sID, const char * orgData, unsigned int orgDataLen)
{
	auto &mapSessionPtr = m_mapTcpSessionPtr[aID];
	auto iter = mapSessionPtr.find(sID);
	if (iter == mapSessionPtr.end())
	{
		LOGW("SendOrgSessionData NOT FOUND SessionID. AccepterID=" << aID << ", SessionID=" << sID);
		return;
	}
	iter->second->DoSend(orgData, orgDataLen);
}
void CTcpSessionManager::SendSessionData(AccepterID aID, SessionID sID, ProtocolID pID, const char * userData, unsigned int userDataLen)
{
	WriteStreamPack ws;
	ws << pID;
	ws.AppendOriginalData(userData, userDataLen);
	SendOrgSessionData(aID, sID, ws.GetStream(), ws.GetStreamLen());
}

void CTcpSessionManager::KickSession(AccepterID aID, SessionID sID)
{
	auto &mapSessionPtr = m_mapTcpSessionPtr[aID];
	auto iter = mapSessionPtr.find(sID);
	if (iter == mapSessionPtr.end())
	{
		LOGW("KickSession NOT FOUND SessionID. AccepterID=" << aID << ", SessionID=" << sID );
		return;
	}
	iter->second->Close();
}

void CTcpSessionManager::BreakConnector(ConnectorID cID)
{
	auto iter = m_mapConnectorPtr.find(cID);
	if (iter == m_mapConnectorPtr.end())
	{
		LOGW("BreakConnector NOT FOUND SessionID. ConnectorID=" << cID);
		return;
	}
	iter->second->Close();
}

void CTcpSessionManager::OnSessionClose(AccepterID aID, SessionID sID)
{
	CMessageDispatcher::getRef().DispatchOnSessionDisconnect(aID, sID);
	m_mapTcpSessionPtr[aID].erase(sID);
}
SessionID CTcpSessionManager::AddConnector(const tagConnctorConfigTraits &traits)
{
	if (m_mapConnectorConfig.find(traits.cID) != m_mapConnectorConfig.end() || traits.cID == InvalidConnectorID)
	{
		LOGE("AddConnector confilict ConnectorID. ConnectorID=" << traits.cID << ", remoteAdress=" << traits.remoteIP << ":" << traits.remotePort);
		return traits.cID;
	}
	m_mapConnectorConfig[traits.cID] = traits;
	CTcpSocketPtr sockPtr(new zsummer::network::CTcpSocket());
	sockPtr->Initialize(m_summer);
	CTcpSessionPtr sessionPtr(new CTcpSession());
	sessionPtr->BindTcpSocketPrt(sockPtr, traits.cID, true);
	sessionPtr->DoConnect(traits);
	return traits.cID;
}


void CTcpSessionManager::OnConnectorStatus(ConnectorID connectorID, bool bConnected, CTcpSessionPtr connector)
{
	std::map<SessionID, tagConnctorConfigTraits>::iterator config = m_mapConnectorConfig.find(connectorID);
	if (config == m_mapConnectorConfig.end())
	{
		LOGE("Unkwon Connector. Not Found ConnectorID=" << connectorID);
		return;
	}
	std::map<SessionID, CTcpSessionPtr>::iterator iter = m_mapConnectorPtr.find(connectorID);
	if (iter != m_mapConnectorPtr.end() && !bConnected)
	{
		CMessageDispatcher::getRef().DispatchOnConnectorDisconnect(connectorID);
		m_mapConnectorPtr.erase(connectorID);
	}

	if (bConnected)
	{
		m_mapConnectorPtr[connectorID] = connector;
		config->second.curReconnectCount = 0;
		CMessageDispatcher::getRef().DispatchOnConnectorEstablished(connectorID);
	}


	if (!bConnected 
		&& config->second.reconnectMaxCount > 0
		&& config->second.curReconnectCount < config->second.reconnectMaxCount)
	{
		config->second.curReconnectCount++;

		CTcpSocketPtr sockPtr(new zsummer::network::CTcpSocket());
		sockPtr->Initialize(m_summer);
		connector->BindTcpSocketPrt(sockPtr, config->second.cID, true);
		CreateTimer(config->second.reconnectInterval, std::bind(&CTcpSession::DoConnect, connector, config->second));
	}
}
AccepterID CTcpSessionManager::AddAcceptor(const tagAcceptorConfigTraits &traits)
{
	if (m_mapAccepterConfig.find(traits.aID) != m_mapAccepterConfig.end() || traits.aID == InvalidAccepterID)
	{
		LOGE("AddAcceptor confilict AccecptID. traits=" << traits);
		return InvalidAccepterID;
	}
	m_mapAccepterConfig[traits.aID] = traits;
	CTcpAcceptPtr accepter(new zsummer::network::CTcpAccept(m_summer));
	if (!accepter->OpenAccept(traits.listenIP.c_str(), traits.listenPort))
	{
		LOGE("AddAcceptor OpenAccept Failed. traits=" << traits);
		return InvalidAccepterID;
	}
	m_mapAccepterPtr[traits.aID] = accepter;
	CTcpSocketPtr newclient(new zsummer::network::CTcpSocket);
	newclient->Initialize(m_summer);
	accepter->DoAccept(newclient, std::bind(&CTcpSessionManager::OnAcceptNewClient, this, std::placeholders::_1, std::placeholders::_2, accepter, traits.aID));
	return  traits.aID;
}

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


#include <zsummerX/frame/FrameTcpSession.h>
#include <zsummerX/frame/FrameTcpSessionManager.h>
#include <zsummerX/frame/FrameMessageDispatch.h>

CTcpSessionManager & CTcpSessionManager::getRef()
{
	static CTcpSessionManager m_manager;
	return m_manager;
}
CTcpSessionManager::CTcpSessionManager()
{
	m_summer = std::shared_ptr<zsummer::network::ZSummer>(new zsummer::network::ZSummer());
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
	Post(std::bind(&CTcpSessionManager::SafeStop, this));
}

void CTcpSessionManager::SafeStop()
{
	m_bRunning = false;

	//at first close the session from accept.
	m_onlineConnectCounts = 0;
	for (auto m : m_mapTcpSessionPtr)
	{
		if (IsConnectID(m.first))
		{
			m_mapConnectorConfig[m.first].second.curReconnectCount = -1;
			m_onlineConnectCounts++;
			continue;
		}
		m.second->Close();
	}
	//if only have the connect session then close all.
	if (m_mapTcpSessionPtr.size() == m_onlineConnectCounts)
	{
		for (auto m : m_mapTcpSessionPtr)
		{
			m.second->Close();
		}
	}
}
void CTcpSessionManager::Run()
{
	m_bRunning = true;
	while (m_bRunning || !m_mapTcpSessionPtr.empty())
	{
		m_summer->RunOnce();
	}
}




AccepterID CTcpSessionManager::AddAcceptor(const tagAcceptorConfigTraits &traits)
{
	m_lastAcceptID++;
	auto & pairConfig = m_mapAccepterConfig[m_lastAcceptID];
	pairConfig.first = traits;
	pairConfig.second.aID = m_lastAcceptID;

	CTcpAcceptPtr accepter(new zsummer::network::CTcpAccept());
	accepter->Initialize(m_summer);
	if (!accepter->OpenAccept(traits.listenIP.c_str(), traits.listenPort))
	{
		LCE("AddAcceptor OpenAccept Failed. traits=" << traits);
		return InvalidAccepterID;
	}
	m_mapAccepterPtr[m_lastAcceptID] = accepter;
	CTcpSocketPtr newclient(new zsummer::network::CTcpSocket);
	accepter->DoAccept(newclient, std::bind(&CTcpSessionManager::OnAcceptNewClient, this, std::placeholders::_1, std::placeholders::_2, accepter, pairConfig.second.aID));
	return m_lastAcceptID;
}

void CTcpSessionManager::OnAcceptNewClient(zsummer::network::ErrorCode ec, CTcpSocketPtr s, CTcpAcceptPtr accepter, AccepterID aID)
{
	if (!m_bRunning)
	{
		LCI("shutdown accepter. aID=" << aID);
		return;
	}
	auto iter = m_mapAccepterConfig.find(aID);
	if (iter == m_mapAccepterConfig.end())
	{
		LCE("Unknown AccepterID aID=" << aID);
		return;
	}
	if (ec)
	{
		LCE("DoAccept Result Error. ec=" << ec << ", traits=" << iter->second.first);
		return;
	}

	std::string remoteIP;
	unsigned short remotePort = 0;
	s->GetPeerInfo(remoteIP, remotePort);
	
	//! check white list
	//! ---------------------
	if (!iter->second.first.whitelistIP.empty())
	{
		bool checkSucess = false;
		for (auto white : iter->second.first.whitelistIP)
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

		if (!checkSucess)
		{
			LCW("Accept New Client Check Whitelist Failed remoteAdress=" << remoteIP << ":" << remotePort
				<< ", trais=" << iter->second.first);

			CTcpSocketPtr newclient(new zsummer::network::CTcpSocket);
			newclient->Initialize(m_summer);
			accepter->DoAccept(newclient, std::bind(&CTcpSessionManager::OnAcceptNewClient, this, std::placeholders::_1, std::placeholders::_2, accepter, aID));
			return;
		}
		else
		{
			LCI("Accept New Client Check Whitelist Success remoteAdress=" << remoteIP << ":" << remotePort
				<< ", trais=" << iter->second.first);
		}
	}
	
	//! check Max Sessions

	if (iter->second.second.currentLinked >= iter->second.first.maxSessions)
	{
		LCW("Accept New Client. Too Many Sessions And The new socket will closed. remoteAddress=" << remoteIP << ":" << remotePort 
			<< ", Aready linked sessions = " << iter->second.second.currentLinked << ", trais=" << iter->second.first);
	}
	else
	{
		LCD("Accept New Client. Accept new Sessions. The new socket  remoteAddress=" << remoteIP << ":" << remotePort 
			<< ", Aready linked sessions = " << iter->second.second.currentLinked << ", trais=" << iter->second.first);
		iter->second.second.currentLinked++;
		iter->second.second.totalAcceptCount++;

		m_lastSessionID = NextSessionID(m_lastSessionID);
		CTcpSessionPtr session(new CTcpSession());
		s->Initialize(m_summer);
		if (session->BindTcpSocketPrt(s, aID, m_lastSessionID, iter->second.first))
		{
			m_mapTcpSessionPtr[m_lastSessionID] = session;
			Post(std::bind(&CMessageDispatcher::DispatchOnSessionEstablished, &CMessageDispatcher::getRef(), m_lastSessionID));
		}
	}
	
	//! accept next socket.
	CTcpSocketPtr newclient(new zsummer::network::CTcpSocket);
	newclient->Initialize(m_summer);
	accepter->DoAccept(newclient, std::bind(&CTcpSessionManager::OnAcceptNewClient, this, std::placeholders::_1, std::placeholders::_2, accepter,aID));
}





void CTcpSessionManager::KickSession(SessionID sID)
{
	auto iter = m_mapTcpSessionPtr.find(sID);
	if (iter == m_mapTcpSessionPtr.end())
	{
		LCW("KickSession NOT FOUND SessionID. SessionID=" << sID);
		return;
	}
	iter->second->Close();
}

void CTcpSessionManager::OnSessionClose(AccepterID aID, SessionID sID)
{
	m_mapAccepterConfig[aID].second.currentLinked--;
	m_mapTcpSessionPtr.erase(sID);
	CMessageDispatcher::getRef().DispatchOnSessionDisconnect(sID);
	if (!m_bRunning && m_mapTcpSessionPtr.size() <= m_onlineConnectCounts)
	{
		CreateTimer(1000, std::bind(&CTcpSessionManager::SafeStop, CTcpSessionManager::getPtr()));
	}
}


SessionID CTcpSessionManager::AddConnector(const tagConnctorConfigTraits & traits)
{
	m_lastConnectID = NextConnectID(m_lastConnectID);
	auto & pairConfig = m_mapConnectorConfig[m_lastConnectID];
	pairConfig.first = traits;
	pairConfig.second.cID = m_lastConnectID;
	CTcpSocketPtr sockPtr(new zsummer::network::CTcpSocket());
	sockPtr->Initialize(m_summer);
	CTcpSessionPtr sessionPtr(new CTcpSession());
	sessionPtr->BindTcpConnectorPtr(sockPtr, pairConfig);
	return m_lastConnectID;
}


void CTcpSessionManager::OnConnect(SessionID cID, bool bConnected, CTcpSessionPtr session)
{
	auto config = m_mapConnectorConfig.find(cID);
	if (config == m_mapConnectorConfig.end())
	{
		LCE("Unkwon Connector. Not Found ConnectorID=" << cID);
		return;
	}
	if (bConnected)
	{
		m_mapTcpSessionPtr[cID] = session;
		config->second.second.curReconnectCount = 0;
		Post(std::bind(&CMessageDispatcher::DispatchOnSessionEstablished, &CMessageDispatcher::getRef(), cID));
		return;
	}


	auto iter = m_mapTcpSessionPtr.find(cID);
	if (!bConnected && iter != m_mapTcpSessionPtr.end())
	{
		m_mapTcpSessionPtr.erase(cID);
		Post(std::bind(&CMessageDispatcher::DispatchOnSessionDisconnect, &CMessageDispatcher::getRef(), cID));
	}

	if (!bConnected
		&& config->second.first.reconnectMaxCount > 0
		&& config->second.second.curReconnectCount < config->second.first.reconnectMaxCount)
	{
		config->second.second.curReconnectCount++;
		config->second.second.totalConnectCount++;

		CTcpSocketPtr sockPtr(new zsummer::network::CTcpSocket());
		sockPtr->Initialize(m_summer);
		CreateTimer(config->second.first.reconnectInterval, std::bind(&CTcpSession::BindTcpConnectorPtr, session, sockPtr, config->second));
		LCW("Try reconnect current count=" << config->second.second.curReconnectCount << ", total reconnect = " << config->second.second.totalConnectCount << ". Traits=" << config->second.first);
	}
	else if (config->second.first.reconnectMaxCount > 0 && m_bRunning)
	{
		LCE("Try Reconnect Failed. End Try. Traits=" << config->second.first);
	}
}


void CTcpSessionManager::SendOrgSessionData(SessionID sID, const char * orgData, unsigned int orgDataLen)
{
	auto iter = m_mapTcpSessionPtr.find(sID);
	if (iter == m_mapTcpSessionPtr.end())
	{
		LCW("SendOrgSessionData NOT FOUND SessionID.  SessionID=" << sID);
		return;
	}
	iter->second->DoSend(orgData, orgDataLen);
	//trace log
	{
		LCT("SendOrgSessionData Len=" << orgDataLen << ",binarydata=" << zsummer::log4z::BinaryBlock(orgData, orgDataLen >= 10 ? 10 : orgDataLen));
	}
}
void CTcpSessionManager::SendSessionData(SessionID sID, ProtoID pID, const char * userData, unsigned int userDataLen)
{
	WriteStreamPack ws;
	ws << pID;
	ws.AppendOriginalData(userData, userDataLen);
	SendOrgSessionData(sID, ws.GetStream(), ws.GetStreamLen());
	//trace log
	{
		LCT("SendSessionData ProtoID=" << pID << ",  userDataLen=" << userDataLen);
	}
}




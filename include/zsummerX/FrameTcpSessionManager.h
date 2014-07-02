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


#ifndef ZSUMMER_TCPSESSION_MANAGER_H_
#define ZSUMMER_TCPSESSION_MANAGER_H_

#include "FrameHeader.h"
class CTcpSession;
typedef  std::shared_ptr<CTcpSession> CTcpSessionPtr;

class CTcpSessionManager
{
private:
	CTcpSessionManager();

public://!获取单例
	static CTcpSessionManager & getRef();
	static CTcpSessionManager * getPtr(){ return &getRef(); };
public:
	bool Start();
	void Stop();
	void Run();

public://!如果多线程调用该类接口 需要通过Post转换调用线程.
	template<class H>
	void Post(const H &h){m_summer->Post(h);}

public://! 创建和取消定时器
	template <class H>
	zsummer::network::TimerID CreateTimer(unsigned int delayms, const H &h){ return m_summer->CreateTimer(delayms, h); }
	bool CancelTimer(unsigned long long timerID){ return m_summer->CancelTimer(timerID); }

public://! 添加Connector或者Acceptor. 不限个数
	SessionID AddConnector(const tagConnctorConfigTraits &traits);
	AccepterID AddAcceptor(const tagAcceptorConfigTraits &traits);

	
public://! 发送消息和主动断开
	void SendOrgConnectorData(ConnectorID cID, const char * orgData, unsigned int orgDataLen);
	void SendConnectorData(ConnectorID cID, ProtocolID pID, const char * userData, unsigned int userDataLen);

	void SendOrgSessionData(AccepterID aID, SessionID sID, const char * orgData, unsigned int orgDataLen);
	void SendSessionData(AccepterID aID, SessionID sID, ProtocolID pID, const char * userData, unsigned int userDataLen);

	void KickSession(AccepterID aID, SessionID sID);
	void BreakConnector(ConnectorID cID);

private:
	friend class CTcpSession;
	void OnSessionClose(AccepterID aID, SessionID sID);
	void OnConnectorStatus(ConnectorID connectorID, bool bConnected, CTcpSessionPtr session);
	void OnAcceptNewClient(zsummer::network::ErrorCode ec, CTcpSocketPtr s, CTcpAcceptPtr accepter, AccepterID aID);
	bool BindEstablishedSocketPtr(CTcpSocketPtr sockptr, AccepterID aID);
private:
	bool  m_bRunning = true;
	SessionID m_lastSessionID = 0;


	CZSummerPtr m_summer;
	std::map<AccepterID, std::map<SessionID, CTcpSessionPtr> > m_mapTcpSessionPtr;
	std::map<ConnectorID, CTcpSessionPtr> m_mapConnectorPtr;
	std::map<ConnectorID, tagConnctorConfigTraits> m_mapConnectorConfig;
	std::map<AccepterID, tagAcceptorConfigTraits> m_mapAccepterConfig;
	std::map<AccepterID, CTcpAcceptPtr> m_mapAccepterPtr;
public:
};

#endif
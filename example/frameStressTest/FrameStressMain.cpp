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

//! zsummer的测试服务模块(对应zsummer底层网络封装的上层设计测试服务) 可视为服务端架构中的 gateway服务/agent服务/前端服务, 特点是高并发高吞吐量
//! main文件


#include <zsummerX/FrameHeader.h>
#include <zsummerX/FrameTcpSessionManager.h>
#include <zsummerX/FrameMessageDispatch.h>
using namespace zsummer::log4z;

std::string g_remoteIP = "0.0.0.0";
unsigned short g_remotePort = 81;
unsigned short g_startType = 0;  //0 listen, 1 connect
unsigned short g_maxClient = 1; //0 echo send, 1 direct send
unsigned short g_sendType = 0; //0 echo send, 1 direct send
unsigned int   g_intervalMs = 0; // send interval

//test protocols, IDs
static const ProtocolID _Heartbeat = 1000;
static const ProtocolID _RequestSequence = 1001;
static const ProtocolID _ResultSequence = 1002;

std::string g_testStr;

unsigned long long g_totalEchoCount = 0;
unsigned long long g_lastEchoCount = 0;
unsigned long long g_totalSendCount = 0;
unsigned long long g_totalRecvCount = 0;

void MonitorFunc()
{
	LOGI("per seconds Echos Count=" << (g_lastEchoCount - g_totalEchoCount) / 5
		<< ", g_totalSendCount[" << g_totalSendCount << "] g_totalRecvCount[" << g_totalRecvCount << "]");
	g_totalEchoCount = g_lastEchoCount;
	CTcpSessionManager::getRef().CreateTimer(5000, MonitorFunc);
};

class CStressClient
{
public:
	CStressClient()
	{
		CMessageDispatcher::getRef().RegisterOnConnectorEstablished(std::bind(&CStressClient::OnConnected,this, std::placeholders::_1));
		CMessageDispatcher::getRef().RegisterOnMyConnectorHeartbeatTimer(std::bind(&CStressClient::Heartbeat, this, std::placeholders::_1));
		CMessageDispatcher::getRef().RegisterConnectorMessage(_Heartbeat, 
			std::bind(&CStressClient::msg_Heartbeat_fun, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
		CMessageDispatcher::getRef().RegisterConnectorMessage(_ResultSequence, 
			std::bind(&CStressClient::msg_ResultSequence_fun, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
		CMessageDispatcher::getRef().RegisterOnConnectorDisconnect(std::bind(&CStressClient::OnConnectDisconnect, this, std::placeholders::_1));
		
	}
	void OnConnected (ConnectorID cID)
	{
		LOGI("OnConnected. ConnectorID=" << cID );
		WriteStreamPack ws;
		ws << _RequestSequence << "hello";
		CTcpSessionManager::getRef().SendOrgConnectorData(cID, ws.GetStream(), ws.GetStreamLen());
		g_totalSendCount++;
		if (g_sendType != 0 && g_intervalMs > 0)
		{
			m_timerID = CTcpSessionManager::getRef().CreateTimer(g_intervalMs, std::bind(&CStressClient::SendFunc, this, cID));
		}
	};
	void OnConnectDisconnect(ConnectorID cID)
	{
		CTcpSessionManager::getRef().CancelTimer(m_timerID);
		m_timerID = InvalidTimerID;
	}

	void Heartbeat(ConnectorID cID)
	{
		time_t now = time(NULL);
		if (now - m_remoteLastHeartbeat > HEARTBEART_INTERVAL / 1000 * 3)
		{
			LOGW("Connector check heartbeat timeout. Connector=" << cID);
			CTcpSessionManager::getRef().BreakConnector(cID);
			return;
		}
		LOGI("send  heartbeat");
		WriteStreamPack ws;
		ws << _Heartbeat;
		CTcpSessionManager::getRef().SendOrgConnectorData(cID, ws.GetStream(), ws.GetStreamLen());
	};
	void msg_Heartbeat_fun (ConnectorID cID, ProtocolID pID, ReadStreamPack & rs)
	{
		LOGI("on remote heartbeat");
		m_remoteLastHeartbeat = time(NULL);
	};
	void msg_ResultSequence_fun(ConnectorID cID, ProtocolID pID, ReadStreamPack & rs)
	{
		std::string msg;
		rs >> msg;
		g_totalRecvCount++;

		if (g_sendType == 0 || g_intervalMs == 0) //echo send
		{
			WriteStreamPack ws;
			ws << _RequestSequence << g_testStr;
			CTcpSessionManager::getRef().SendOrgConnectorData(cID, ws.GetStream(), ws.GetStreamLen());
			g_totalSendCount++;
		}
		g_lastEchoCount++;
	};
	void SendFunc(ConnectorID cID)
	{
		if (g_totalSendCount - g_totalRecvCount < 1000)
		{
			WriteStreamPack ws;
			ws << _RequestSequence << g_testStr;
			CTcpSessionManager::getRef().SendOrgConnectorData(cID, ws.GetStream(), ws.GetStreamLen());
			g_totalSendCount++;
		}
		m_timerID = CTcpSessionManager::getRef().CreateTimer(g_intervalMs, std::bind(&CStressClient::SendFunc, this, cID));
	};
private:
	time_t m_remoteLastHeartbeat = time(NULL);
	zsummer::network::TimerID m_timerID = InvalidTimerID;
};

class CStressServer
{
public:
	CStressServer()
	{
		CMessageDispatcher::getRef().RegisterOnMySessionHeartbeatTimer(std::bind(&CStressServer::Heartbeat, this, std::placeholders::_1, std::placeholders::_2));
		CMessageDispatcher::getRef().RegisterSessionMessage(_Heartbeat, 
			std::bind(&CStressServer::msg_Heartbeat_fun, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
		CMessageDispatcher::getRef().RegisterSessionMessage(_RequestSequence, 
			std::bind(&CStressServer::msg_RequestSequence_fun, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	}
	void Heartbeat(AccepterID aID, SessionID sID)
	{
		time_t now = time(NULL);
		if (now - m_remoteLastHeartbeat > HEARTBEART_INTERVAL / 1000 * 3)
		{
			LOGW("session heartbeat timeout. AccepterID=" << aID << ", SessionID=" << sID);
			CTcpSessionManager::getRef().KickSession(aID, sID);
			return;
		}
		LOGI("send  heartbeat");
		WriteStreamPack ws;
		ws << _Heartbeat;
		CTcpSessionManager::getRef().SendOrgSessionData(aID, sID, ws.GetStream(), ws.GetStreamLen());
	};

	void msg_Heartbeat_fun(AccepterID aID, SessionID sID, ProtocolID pID, ReadStreamPack & rs)
	{
		LOGI("on remote heartbeat");
		m_remoteLastHeartbeat = time(NULL);
	};
	void msg_RequestSequence_fun (AccepterID aID, SessionID sID, ProtocolID pID, ReadStreamPack & rs)
	{
		std::string msg;
		rs >> msg;
		g_totalRecvCount++;
		msg += " echo";
		WriteStreamPack ws;
		ws << _ResultSequence << msg;
		CTcpSessionManager::getRef().SendOrgSessionData(aID, sID, ws.GetStream(), ws.GetStreamLen());
		g_lastEchoCount++;
		g_totalSendCount++;
	};

private:
	time_t m_remoteLastHeartbeat = time(NULL);
};

int main(int argc, char* argv[])
{

#ifndef _WIN32
	//! linux下需要屏蔽的一些信号
	signal( SIGHUP, SIG_IGN );
	signal( SIGALRM, SIG_IGN ); 
	signal( SIGPIPE, SIG_IGN );
	signal( SIGXCPU, SIG_IGN );
	signal( SIGXFSZ, SIG_IGN );
	signal( SIGPROF, SIG_IGN ); 
	signal( SIGVTALRM, SIG_IGN );
	signal( SIGQUIT, SIG_IGN );
	signal( SIGCHLD, SIG_IGN);
#endif
	if (argc == 2 && 
		(strcmp(argv[1], "--help") == 0 
		|| strcmp(argv[1], "/?") == 0))
	{
		cout << "please input like example:" << endl;
		cout << "tcpTest remoteIP remotePort startType maxClient sendType interval" << endl;
		cout << "./tcpTest 0.0.0.0 81 0" << endl;
		cout << "startType: 0 server, 1 client" << endl;
		cout << "maxClient: limit max" << endl;
		cout << "sendType: 0 echo send, 1 direct send" << endl;
		cout << "interval: send once interval" << endl;
		return 0;
	}
	if (argc > 1)
		g_remoteIP = argv[1];
	if (argc > 2)
		g_remotePort = atoi(argv[2]);
	if (argc > 3)
		g_startType = atoi(argv[3]);
	if (argc > 4)
		g_maxClient = atoi(argv[4]);
	if (argc > 5)
		g_sendType = atoi(argv[5]);
	if (argc > 6)
		g_intervalMs = atoi(argv[6]);

	if (g_startType == 0)
		ILog4zManager::GetInstance()->Config("server.cfg");
	else
		ILog4zManager::GetInstance()->Config("client.cfg");
	ILog4zManager::GetInstance()->Start();

	LOGI("g_remoteIP=" << g_remoteIP << ", g_remotePort=" << g_remotePort << ", g_startType=" << g_startType
		<< ", g_maxClient=" << g_maxClient << ", g_sendType=" << g_sendType << ", g_intervalMs=" << g_intervalMs);


	ILog4zManager::GetInstance()->SetLoggerLevel(LOG4Z_MAIN_LOGGER_ID, LOG_LEVEL_INFO);
	ILog4zManager::GetInstance()->SetLoggerDisplay(LOG4Z_MAIN_LOGGER_ID, false);


	CTcpSessionManager::getRef().Start();

	g_testStr.resize(1024, 's');

	CTcpSessionManager::getRef().CreateTimer(5000, MonitorFunc);

	CStressClient client;
	CStressServer server;
	if (g_startType) //client
	{
		for (int i = 0; i < g_maxClient; ++i)
		{
			tagConnctorConfigTraits traits;
			traits.cID = InvalidConnectorID+1+i;
			traits.remoteIP = g_remoteIP;
			traits.remotePort = g_remotePort;
			traits.reconnectInterval = 5000;
			traits.reconnectMaxCount = 50;
			CTcpSessionManager::getRef().AddConnector(traits);
		}
	}
	else
	{
		tagAcceptorConfigTraits traits;
		traits.aID = 1;
		traits.listenPort = g_remotePort;
		traits.maxSessions = g_maxClient;
		//traits.whitelistIP.push_back("127.0.");
		CTcpSessionManager::getRef().AddAcceptor(traits);
	}
	CTcpSessionManager::getRef().Run();

	return 0;
}


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


//! frame模块压力测试
//! frame封装在网络部分使用单例模式, 如果需要使用多线程 需要在业务层由用户开辟线程池处理.
//! 如果需要在zsummerX的网络部分使用多线程 请参考tcpTest实例调用zsummerX的原始接口实现.


#include <zsummerX/FrameHeader.h>
#include <zsummerX/FrameTcpSessionManager.h>
#include <zsummerX/FrameMessageDispatch.h>
#include <unordered_map>
using namespace zsummer::log4z;

//! 默认启动参数
std::string g_remoteIP = "0.0.0.0"; //如果作为服务端来使用 这里是监听地址
unsigned short g_remotePort = 81; //同上
unsigned short g_startType = 0;  //0 作为服务器启动, 1 作为客户端启动
unsigned short g_maxClient = 1; //如果是服务端 这里是限制客户端的个数 超出的会被踢掉, 如果是客户端 这里是启动的客户端总数.
unsigned short g_sendType = 0; //0 ping-pong test, 1 flood test
unsigned int   g_intervalMs = 0; // 如果是flood test, 这里的间隔应该大于0, 单位是毫秒.



//! g_testStr用来在压力测试中发送统一大小的消息.
std::string g_testStr;

//!收发包测试统计数据
unsigned long long g_totalEchoCount = 0;
unsigned long long g_lastEchoCount = 0;
unsigned long long g_totalSendCount = 0;
unsigned long long g_totalRecvCount = 0;
void MonitorFunc()
{
	LOGI("per seconds Echos Count=" << (g_totalEchoCount - g_lastEchoCount) / 5
		<< ", g_totalSendCount[" << g_totalSendCount << "] g_totalRecvCount[" << g_totalRecvCount << "]");
	g_lastEchoCount = g_totalEchoCount;
	CTcpSessionManager::getRef().CreateTimer(5000, MonitorFunc);
};

/*
* 测试代码中定义了4个协议 用来实现心跳机制和echo发包.
* ECHO包是两个协议号 一个用来客户端发送 一个用来服务端回复.
*/
#define C2S_HEARTBEAT ProtocolID(10000)
#define S2C_HEARTBEAT ProtocolID(10000)

#define C2S_ECHO_REQ ProtocolID(10002)
#define S2C_ECHO_ACK ProtocolID(10003)

/*管理连接状态
*在这个测试代码中 把服务端和客户端的心跳管理都放在了这个一个类中.
* 该心跳机制实现策略是:
*  在连接建立事件触发时 初始化心跳时间戳
*  在收到对方心跳消息后更新时间戳
*  在本地心跳定时器触发时检测时间(同时发送心跳给对方)
*  在连接断开事件触发后清除时间戳
*
* 该心跳策略在本测试代码中是单向的 因此上面定义的协议号相同 这个并不影响.
* 如果需要在心跳包携带额外的数据, 例如检测时间延迟, 则可以把心跳包改为REQ-ACK形式(添加一个ACK即可). 在REQ中发送本地时间 在ACK时候检测时间差.
*/
class CStressHeartBeatManager
{
public:
	CStressHeartBeatManager()
	{
		//! 注册事件和消息
		CMessageDispatcher::getRef().RegisterOnConnectorEstablished(std::bind(&CStressHeartBeatManager::OnConnecotrConnected, this,
			std::placeholders::_1));
		CMessageDispatcher::getRef().RegisterOnMyConnectorHeartbeatTimer(std::bind(&CStressHeartBeatManager::OnConnecotrHeartbeatTimer, this,
			std::placeholders::_1));
		CMessageDispatcher::getRef().RegisterOnConnectorDisconnect(std::bind(&CStressHeartBeatManager::OnConnecotrDisconnect, this,
			std::placeholders::_1));
		CMessageDispatcher::getRef().RegisterConnectorMessage(S2C_HEARTBEAT, std::bind(&CStressHeartBeatManager::OnMsgServerHeartbeat, this,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));


		CMessageDispatcher::getRef().RegisterOnSessionEstablished(std::bind(&CStressHeartBeatManager::OnSessionEstablished, this,
			std::placeholders::_1, std::placeholders::_2));
		CMessageDispatcher::getRef().RegisterOnMySessionHeartbeatTimer(std::bind(&CStressHeartBeatManager::OnSessionHeartbeatTimer, this,
			std::placeholders::_1, std::placeholders::_2));
		CMessageDispatcher::getRef().RegisterOnSessionDisconnect(std::bind(&CStressHeartBeatManager::OnSessionDisconnect, this,
			std::placeholders::_1, std::placeholders::_2));
		CMessageDispatcher::getRef().RegisterSessionMessage(C2S_HEARTBEAT, std::bind(&CStressHeartBeatManager::OnMsgConnectorHeartbeat, this,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	}
	
	void OnConnecotrConnected(ConnectorID cID)
	{
		m_connectorHB[cID] = time(NULL);
		LOGI("connect sucess. cID=" << cID);
	}
	void OnConnecotrHeartbeatTimer(ConnectorID cID)
	{
		auto iter = m_connectorHB.find(cID);
		if (iter == m_connectorHB.end() || time(NULL) - iter->second > HEARTBEART_INTERVAL/1000*2)
		{
			LOGI("server  lost. cID=" << cID << ", timeout=" << time(NULL) - iter->second);
			CTcpSessionManager::getRef().BreakConnector(cID);
		}
		WriteStreamPack pack;
		pack << C2S_HEARTBEAT;
		CTcpSessionManager::getRef().SendOrgConnectorData(cID, pack.GetStream(), pack.GetStreamLen());
	}
	void OnConnecotrDisconnect(ConnectorID cID)
	{
		m_connectorHB.erase(cID);
		LOGI("Disconnect. cID=" << cID);
	}

	void OnMsgServerHeartbeat(ConnectorID cID, ProtocolID pID, ReadStreamPack & pack)
	{
		auto iter = m_connectorHB.find(cID);
		if (iter != m_connectorHB.end())
		{
			iter->second = time(NULL);
		}
	}

	void OnSessionEstablished(AccepterID aID, SessionID sID)
	{
		m_sessionHB[sID] = time(NULL);
		LOGI("remote session connected. sID=" << sID);
	}
	void OnSessionHeartbeatTimer(AccepterID aID, SessionID sID)
	{
		auto iter = m_sessionHB.find(sID);
		if (iter == m_sessionHB.end() || time(NULL) - iter->second > HEARTBEART_INTERVAL/1000 * 2)
		{
			LOGI("remote session lost. sID=" << sID << ", timeout=" << time(NULL) - iter->second);
			CTcpSessionManager::getRef().KickSession(aID, sID);
		}
		WriteStreamPack pack;
		pack << S2C_HEARTBEAT;
		CTcpSessionManager::getRef().SendOrgSessionData(aID, sID, pack.GetStream(), pack.GetStreamLen());
	}
	void OnSessionDisconnect(AccepterID aID, SessionID sID)
	{
		LOGI("remote session Disconnect. sID=" << sID );
		m_sessionHB.erase(sID);
	}
	void OnMsgConnectorHeartbeat(AccepterID aID, SessionID sID, ProtocolID pID, ReadStreamPack & pack)
	{
		auto iter = m_sessionHB.find(sID);
		if (iter != m_sessionHB.end())
		{
			iter->second = time(NULL);
		}
	}

protected:

private:
	//! 注册的事件是针对所有session or connector的 因此在事件触发时候 要去查找对应的心跳信息
	std::unordered_map<SessionID, time_t> m_sessionHB;
	std::unordered_map<ConnectorID, time_t> m_connectorHB;
};


/* 测试客户端handler类
* 如果启动客户端用来做ping-pong压力测试, 则流程不存在定时器 流程如下 
*  在connector成功后发送第一个send包(ping), 然后在每次recv的时候(pong)向服务器send包(pong).
* 如果启动客户端用来做flood压力测试, 则每个连接都需要创建一个定时器 流程如下
*  在connector成功后 send定时器启动, 并以指定定时器间隔时间持续send.
*/
class CStressClientHandler
{
public:
	CStressClientHandler()
	{
		CMessageDispatcher::getRef().RegisterOnConnectorEstablished(std::bind(&CStressClientHandler::OnConnected, this, std::placeholders::_1));
		CMessageDispatcher::getRef().RegisterConnectorMessage(S2C_ECHO_ACK,
			std::bind(&CStressClientHandler::msg_ResultSequence_fun, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
		CMessageDispatcher::getRef().RegisterOnConnectorDisconnect(std::bind(&CStressClientHandler::OnConnectDisconnect, this, std::placeholders::_1));
	}

	void OnConnected (ConnectorID cID)
	{
		LOGI("OnConnected. ConnectorID=" << cID );
		WriteStreamPack ws;
		ws << C2S_ECHO_REQ << "client request one REQ.";
		CTcpSessionManager::getRef().SendOrgConnectorData(cID, ws.GetStream(), ws.GetStreamLen());
		g_totalSendCount++;
		if (g_sendType != 0 && g_intervalMs > 0)
		{
			CTcpSessionManager::getRef().CreateTimer(g_intervalMs, std::bind(&CStressClientHandler::SendFunc, this, cID));
			m_sessionStatus[cID] = true;
		}
	};
	void OnConnectDisconnect(ConnectorID cID)
	{
		m_sessionStatus[cID] = false;
	}

	void msg_ResultSequence_fun(ConnectorID cID, ProtocolID pID, ReadStreamPack & rs)
	{
		std::string msg;
		rs >> msg;
		g_totalRecvCount++;
		g_totalEchoCount++;

		if (g_sendType == 0 || g_intervalMs == 0) //echo send
		{
			WriteStreamPack ws;
			ws << C2S_ECHO_REQ << g_testStr;
			CTcpSessionManager::getRef().SendOrgConnectorData(cID, ws.GetStream(), ws.GetStreamLen());
			g_totalSendCount++;
		}
	};
	void SendFunc(ConnectorID cID)
	{
		if (g_totalSendCount - g_totalRecvCount < 10000)
		{
			WriteStreamPack ws;
			ws << C2S_ECHO_REQ << g_testStr;
			CTcpSessionManager::getRef().SendOrgConnectorData(cID, ws.GetStream(), ws.GetStreamLen());
			g_totalSendCount++;
		}
		if (m_sessionStatus[cID])
		{
			CTcpSessionManager::getRef().CreateTimer(g_intervalMs, std::bind(&CStressClientHandler::SendFunc, this, cID));
		}
	};
private:
	std::unordered_map<ConnectorID, bool> m_sessionStatus;
};


/*
* 服务端handler类
* 服务端逻辑较为简单 只要收到消息包 立刻echo回去就可以.
*/
class CStressServerHandler
{
public:
	CStressServerHandler()
	{
		CMessageDispatcher::getRef().RegisterSessionMessage(C2S_ECHO_REQ,
			std::bind(&CStressServerHandler::msg_RequestSequence_fun, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	}

	void msg_RequestSequence_fun (AccepterID aID, SessionID sID, ProtocolID pID, ReadStreamPack & rs)
	{
		std::string msg;
		rs >> msg;
		msg += " echo";
		WriteStreamPack ws;
		ws << S2C_ECHO_ACK << msg;
		CTcpSessionManager::getRef().SendOrgSessionData(aID, sID, ws.GetStream(), ws.GetStreamLen());
		g_totalEchoCount++;
		g_totalSendCount++;
		g_totalRecvCount++;
	};
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



	CTcpSessionManager::getRef().Start();

	g_testStr.resize(1024, 's');

	CTcpSessionManager::getRef().CreateTimer(5000, MonitorFunc);

	//创建心跳管理handler的实例 只要创建即可, 构造函数中会注册对应事件
	CStressHeartBeatManager statusManager;

	//这里创建服务handler和客户端handler 根据启动参数不同添加不同角色.
	CStressClientHandler client;
	CStressServerHandler server;
	if (g_startType) //client
	{
		//添加多个connector.
		for (int i = 0; i < g_maxClient; ++i)
		{
			tagConnctorConfigTraits traits;
			traits.cID = i;
			traits.remoteIP = g_remoteIP;
			traits.remotePort = g_remotePort;
			traits.reconnectInterval = 5000;
			traits.reconnectMaxCount = 50;
			CTcpSessionManager::getRef().AddConnector(traits);
		}
	}
	else
	{
		//添加acceptor
		tagAcceptorConfigTraits traits;
		traits.aID = 1;
		traits.listenPort = g_remotePort;
		traits.maxSessions = g_maxClient;
		//traits.whitelistIP.push_back("127.0.");
		CTcpSessionManager::getRef().AddAcceptor(traits);
	}

	//启动主循环.
	CTcpSessionManager::getRef().Run();

	return 0;
}


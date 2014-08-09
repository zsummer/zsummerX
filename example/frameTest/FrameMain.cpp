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


//! zsummerX的测试文件
//! 该测试提供最简单的监听,建立连接,收发数据的 客户端和服务端实例代码.


#include <zsummerX/FrameHeader.h>
#include <zsummerX/FrameTcpSessionManager.h>
#include <zsummerX/FrameMessageDispatch.h>
using namespace zsummer::log4z;

std::string g_remoteIP = "0.0.0.0";
unsigned short g_remotePort = 81;
unsigned short g_startIsConnector = 0;  //0 listen, 1 connect


int main(int argc, char* argv[])
{
	if (argc == 2 && 
		(strcmp(argv[1], "--help") == 0 
		|| strcmp(argv[1], "/?") == 0))
	{
		cout <<"please input like example:" << endl;
		cout << "tcpTest remoteIP remotePort startType maxClient sendType interval" << endl;
		cout << "./tcpTest 0.0.0.0 81 0" << endl;
		cout << "startType: 0 server, 1 client" << endl;
		return 0;
	}
	if (argc > 1)
	{
		g_remoteIP = argv[1];
	}
	if (argc > 2)
	{
		g_remotePort = atoi(argv[2]);
	}
	if (argc > 3)
	{
		g_startIsConnector = atoi(argv[3]);
	}

	
	if (!g_startIsConnector)
	{
		//! 启动日志服务
		ILog4zManager::GetInstance()->Config("server.cfg");
		ILog4zManager::GetInstance()->Start();
	}
	else
	{
		//! 启动日志服务
		ILog4zManager::GetInstance()->Config("client.cfg");
		ILog4zManager::GetInstance()->Start();
	}
	LOGI("g_remoteIP=" << g_remoteIP << ", g_remotePort=" << g_remotePort << ", g_startIsConnector=" << g_startIsConnector );


	//! step 1. 开启Manager.
	CTcpSessionManager::getRef().Start();


	//! 定义协议号
	static const ProtocolID _RequestSequence = 1001;
	static const ProtocolID _ResultSequence = 1002;


	if (g_startIsConnector) //client
	{
		//响应连接成功事件
		auto connectedfun = [](ConnectorID cID)
		{
			LOGI("send to ConnectorID=" << cID << ", msg=hello");
			WriteStreamPack ws;
			ws << _RequestSequence << "hello";
			CTcpSessionManager::getRef().SendOrgConnectorData(cID, ws.GetStream(), ws.GetStreamLen());
		};

		//响应消息_ResultSequence
		auto msg_ResultSequence_fun = [](ConnectorID cID, ProtocolID pID, ReadStreamPack & rs)
		{
			std::string msg;
			rs >> msg;
			LOGI("recv ConnectorID = " << cID << ", msg = " << msg);
		};

		//! 注册事件和消息
		CMessageDispatcher::getRef().RegisterOnConnectorEstablished(connectedfun); //!注册连接成功处理函数
		CMessageDispatcher::getRef().RegisterConnectorMessage(_ResultSequence, msg_ResultSequence_fun);//!注册消息


		//添加一个connector
		tagConnctorConfigTraits traits;
		traits.cID = 1;
		traits.remoteIP = "127.0.0.1";
		traits.remotePort = 81;
		traits.reconnectInterval = 5000;
		traits.reconnectMaxCount = 0;
		CTcpSessionManager::getRef().AddConnector(traits);

		//! step 2 启动主循环
		CTcpSessionManager::getRef().Run();
	}
	else
	{

		//响应消息_RequestSequence
		OnSessionMessageFunction msg_RequestSequence_fun = [](AccepterID aID, SessionID sID, ProtocolID pID, ReadStreamPack & rs)
		{
			std::string msg;
			rs >> msg;
			LOGI("recv SessionID = " << sID << ", msg = " << msg);
			msg += " echo";
			LOGI("send echo AcceptID = " << aID << ", SessionID = " << sID  << ", msg = " << msg);
			WriteStreamPack ws;
			ws << _ResultSequence << msg;
			CTcpSessionManager::getRef().SendOrgSessionData(aID, sID, ws.GetStream(), ws.GetStreamLen());
			
		};


		CMessageDispatcher::getRef().RegisterSessionMessage(_RequestSequence, msg_RequestSequence_fun); //注册消息

		//添加监听器
		tagAcceptorConfigTraits traits;
		traits.aID = 1;
		traits.listenPort = 81;
		traits.maxSessions = 1;
		traits.whitelistIP.push_back("127.0.");
		CTcpSessionManager::getRef().AddAcceptor(traits);

		//! step 2 启动主循环
		CTcpSessionManager::getRef().Run();
	}
	

	return 0;
}


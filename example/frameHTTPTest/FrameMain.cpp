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

std::string g_remoteIP = "220.181.112.224";
unsigned short g_remotePort = 80;
unsigned short g_startIsConnector = 1;  //0 listen, 1 connect


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


	if (g_startIsConnector) //client
	{
		//响应连接成功事件
		auto connectedfun = [](ConnectorID cID)
		{
			LOGI("send to ConnectorID=" << cID);
			zsummer::proto4z::WriteHTTP wh;
			wh.AddHead("Accept", " text/html, application/xhtml+xml, */*");
			wh.AddHead("Accept-Language", "zh-CN");
			wh.AddHead("User-Agent", "Mozilla/5.0 ");
			wh.AddHead("Accept-Encoding", "utf8");
			wh.AddHead("Host", "www.baidu.com");
			wh.AddHead("DNT", "1");
			wh.AddHead("Connection", "Keep-Alive");
			wh.Get("/");
			CTcpSessionManager::getRef().SendOrgConnectorData(cID, wh.GetStream(), wh.GetStreamLen());
		};

		//响应消息_ResultSequence
		auto msg_ResultSequence_fun = [](ConnectorID cID, const zsummer::proto4z::HTTPHeadMap &head, const std::string & body)
		{
			auto fouder = head.find("RESPONSE");
			if (fouder == head.end())
			{
				LOGE("response error.");
				return false;
			}
			if (fouder->second != "200")
			{
				LOGE("response error. error code=" << fouder->second);
				//CTcpSessionManager::getRef().AddConnector(1);
				return false;
			}
			
			
			LOGI("response success. content=" << body);
			return true;
		};

		//! 注册事件和消息
		CMessageDispatcher::getRef().RegisterOnConnectorEstablished(connectedfun); //!注册连接成功处理函数
		CMessageDispatcher::getRef().RegisterOnConnectorHTTPMessage(msg_ResultSequence_fun);//!注册消息


		//添加一个connector
		tagConnctorConfigTraits traits;
		traits.cID = 1;
		traits.remoteIP = g_remoteIP;
		traits.remotePort = g_remotePort;
		traits.protoType = PT_HTTP;
		traits.reconnectInterval = 5000;
		traits.reconnectMaxCount = 0;
		CTcpSessionManager::getRef().AddConnector(traits);

		//! step 2 启动主循环
		CTcpSessionManager::getRef().Run();
	}
	else
	{

		//响应消息_ResultSequence
		auto msg_ResultSequence_fun = [](AccepterID aID, SessionID sID, const zsummer::proto4z::HTTPHeadMap &head, const std::string & body)
		{
			auto fouder = head.find("GET");
			if (fouder == head.end())
			{
				LOGE("not found GET.");
				return false;
			}
			LOGI("GET  content=" << fouder->second);
			return false;
		};

		//! 注册事件和消息
		CMessageDispatcher::getRef().RegisterOnSessionHTTPMessage(msg_ResultSequence_fun);//!注册消息

		tagAcceptorConfigTraits traits;
		traits.aID = 1;
		traits.listenPort = g_remotePort;
		traits.protoType = PT_HTTP;
		CTcpSessionManager::getRef().AddAcceptor(traits);
		//! step 2 启动主循环
		CTcpSessionManager::getRef().Run();
	}
	

	return 0;
}


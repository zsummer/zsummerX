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


//! frame simple test
//! create connector,  build session, send message, recv message.
//! create accpter, accept session, recv message, send message.



#include <zsummerX/frameX.h>
using namespace zsummer::log4z;

//default param
std::string g_remoteIP = "0.0.0.0";
unsigned short g_remotePort = 8081;
unsigned short g_startIsConnector = 0;  //0 listen, 1 connect


int main(int argc, char* argv[])
{
	if (argc == 2 && 
		(strcmp(argv[1], "--help") == 0 
		|| strcmp(argv[1], "/?") == 0))
	{
		cout <<"please input like example:" << endl;
		cout << "tcpTest remoteIP remotePort startType maxClient sendType interval" << endl;
		cout << "./tcpTest 0.0.0.0 8081 0" << endl;
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
		//! start log4z service
		ILog4zManager::getPtr()->config("server.cfg");
		ILog4zManager::getPtr()->start();
	}
	else
	{
		//! start log4z service
		ILog4zManager::getPtr()->config("client.cfg");
		ILog4zManager::getPtr()->start();
	}
	LOGI("g_remoteIP=" << g_remoteIP << ", g_remotePort=" << g_remotePort << ", g_startIsConnector=" << g_startIsConnector );


	//! step 1. start Manager.
	TcpSessionManager::getRef().start();


	//! define protocol id
	static const ProtoID _RequestSequence = 1001;
	static const ProtoID _ResultSequence = 1002;


	if (g_startIsConnector) //client mod
	{
		//on connect success
		auto connectedfun = [](SessionID cID)
		{
			LOGI("send to ConnectorID=" << cID << ", msg=hello");
			WriteStreamPack ws;
			ws << _RequestSequence << "hello";
			TcpSessionManager::getRef().sendOrgSessionData(cID, ws.getStream(), ws.getStreamLen());
		};

		//process message _ResultSequence
		auto msg_ResultSequence_fun = [](SessionID cID, ProtoID pID, ReadStreamPack & rs)
		{
			std::string msg;
			rs >> msg;
			LOGI("recv ConnectorID = " << cID << ", msg = " << msg);

			//! step 3 stop server
			TcpSessionManager::getRef().stop();
		};

		//! register event and message
		MessageDispatcher::getRef().registerOnSessionEstablished(connectedfun); //!register connect success
		MessageDispatcher::getRef().registerSessionMessage(_ResultSequence, msg_ResultSequence_fun);//!register message for protoID: _ResultSequence

		//add connector
		tagConnctorConfigTraits traits;
		traits.remoteIP = "127.0.0.1";
		traits.remotePort = 81;
		traits.reconnectInterval = 5000;
		traits.reconnectMaxCount = 5;
		TcpSessionManager::getRef().addConnector(traits);

		//! step 2 running
		TcpSessionManager::getRef().run();
	}
	else
	{

		//process message _RequestSequence
		OnMessageFunction msg_RequestSequence_fun = [](SessionID sID, ProtoID pID, ReadStreamPack & rs)
		{
			std::string msg;
			rs >> msg;
			LOGI("recv SessionID = " << sID << ", msg = " << msg);
			msg += " echo";
			LOGI("send echo SessionID = " << sID  << ", msg = " << msg);
			WriteStreamPack ws;
			ws << _ResultSequence << msg;
			TcpSessionManager::getRef().sendOrgSessionData(sID, ws.getStream(), ws.getStreamLen());

			//! step 3 stop server after 1 second.
			TcpSessionManager::getRef().createTimer(1000, [](){TcpSessionManager::getRef().stop(); });
		};


		MessageDispatcher::getRef().registerSessionMessage(_RequestSequence, msg_RequestSequence_fun); //!register message for protoID: _RequestSequence

		//add Acceptor
		tagAcceptorConfigTraits traits;
		traits.listenPort = 8081;
		traits.maxSessions = 1;
		//!traits.whitelistIP.push_back("127.0.");
		TcpSessionManager::getRef().addAcceptor(traits);
		//! step 2 running
		TcpSessionManager::getRef().run();
	}
	

	return 0;
}


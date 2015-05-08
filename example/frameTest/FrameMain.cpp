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
 * Copyright (C) 2010-2015 YaweiZhang <yawei.zhang@foxmail.com>.
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



#include <zsummerX/zsummerX.h>
using namespace zsummer::log4z;
using namespace zsummer::proto4z;
using namespace zsummer::network;

//default param
std::string g_remoteIP = "0.0.0.0";
unsigned short g_remotePort = 8081;
unsigned short g_startIsConnector = 0;  //0 listen, 1 connect

//! define protocol id
static const ProtoID _RequestID = 1001;
static const ProtoID _ResultID = 1002;

bool initEnv(int argc, char* argv[]);
void startServer();
void startClient();

// main
//////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
	if (!initEnv(argc, argv))
	{
		return 0;
	}
	
	if (g_startIsConnector) //client mod
	{
		startClient();
	}
	else
	{
		startServer();
	}
	return 0;
}





//impl
//////////////////////////////////////////////////////////////////////////


bool initEnv(int argc, char* argv[])
{
	if (argc == 2 &&
		(strcmp(argv[1], "--help") == 0
		|| strcmp(argv[1], "/?") == 0))
	{
		std::cout << "please input like example:" << std::endl;
		std::cout << "tcpTest remoteIP remotePort startType maxClient sendType interval" << std::endl;
		std::cout << "./tcpTest 0.0.0.0 8081 0" << std::endl;
		std::cout << "startType: 0 server, 1 client" << std::endl;
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
	LOGI("g_remoteIP=" << g_remoteIP << ", g_remotePort=" << g_remotePort << ", g_startIsConnector=" << g_startIsConnector);
	return true;
}


void startServer()
{
	//! step 1. start Manager.
	SessionManager::getRef().start();
	//process message _RequestSequence
	OnMessageFunction msg_RequestSequence_fun = [](TcpSessionPtr session, ProtoID pID, ReadStream & rs)
	{
		std::string content = rs.getStreamBody();
		LOGI("recv SessionID = " << session->getSessionID() << ", content = " << content);
		content += " ==> echo.";
		SessionManager::getRef().sendSessionData(session->getSessionID(), _ResultID, content.c_str(), (unsigned int)content.length() + 1);

		//! step 3 stop server after 1 second.
		SessionManager::getRef().createTimer(1000, [](){
			SessionManager::getRef().stopAccept();
			SessionManager::getRef().stopClients();
			SessionManager::getRef().stopServers();
			SessionManager::getRef().stop(); });
	};

	MessageDispatcher::getRef().registerSessionMessage(_RequestID, msg_RequestSequence_fun); //!register message for protoID: _RequestSequence

	//add Acceptor
	ListenConfig traits;
	traits._listenPort = g_remotePort;
	SessionManager::getRef().addAcceptor(traits);

	//! step 2 running
	SessionManager::getRef().run();
}

void startClient()
{
	//! step 1. start Manager.
	SessionManager::getRef().start();

	//on connect success
	auto connectedfun = [](TcpSessionPtr session)
	{
		LOGI("on connect. ID=" << session->getSessionID());
		std::string content = "hello";
		SessionManager::getRef().sendSessionData(session->getSessionID(), _RequestID, content.c_str(), (unsigned int)content.length() + 1);
	};

	//process message _ResultID
	auto msg_ResultSequence_fun = [](TcpSessionPtr session, ProtoID pID, ReadStream & rs)
	{
		std::string content = rs.getStreamBody();
		LOGI("recv ConnectorID = " << session->getSessionID() << ", content = " << content);
		//! step 3 stop server
		SessionManager::getRef().stopAccept();
		SessionManager::getRef().stopClients();
		SessionManager::getRef().stopServers();
		SessionManager::getRef().stop();
	};

	//! register event and message
	MessageDispatcher::getRef().registerOnSessionEstablished(connectedfun); //!register connect success
	MessageDispatcher::getRef().registerSessionMessage(_ResultID, msg_ResultSequence_fun);//!register message for protoID: _ResultSequence

	//add connector
	ConnectConfig traits;
	traits._remoteIP = g_remoteIP;
	traits._remotePort = g_remotePort;
	SessionManager::getRef().addConnector(traits);

	//! step 2 running
	SessionManager::getRef().run();
}




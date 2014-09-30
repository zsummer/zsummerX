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




#include <zsummerX/zsummer.h>
#include <zsummerX/tcpaccept.h>
#include <zsummerX/tcpsocket.h>
#include <zsummerX/udpsocket.h>

#include <iostream>
#include <queue>
#include <chrono>
#include <iomanip>
#include <string.h>
#include <signal.h>
#include <log4z/log4z.h>
#include <proto4z/proto4z.h>
using namespace std;
using namespace zsummer::log4z;
using namespace zsummer::network;


std::string g_remoteIP = "0.0.0.0";
unsigned short g_remotePort = 81;
unsigned short g_startType = 0;  //0 listen, 1 connect
bool g_runing = true;
bool g_safeexit = false;
CZSummerPtr summer;
CTcpAcceptPtr accepter;
CTcpSocketPtr ts;
CTcpSocketPtr usedSocket;
int maxAcceptCount = 1;
int curAcceptCount = 0;
char recvBuffer[1024]; //recv buffer
int recvBufferLen = 1024;
char sendBuffer[1024]; //send buffer
int sendBufferLen = 1024;

void Close()
{
	if (usedSocket)
	{
		usedSocket->DoClose();
	}
	g_runing = false;
}
void signalFun(int sig)
{
	if (summer)
	{
		summer->Post(&Close);
	}
}


void OnSocektSend(ErrorCode ec, int recvLength)
{
	if (ec)
	{
		curAcceptCount = 0;
		return;
	}
	if (recvLength != sendBufferLen)
	{
		LOGW("safe warning, need translate remaining data.");
		return; //warning; ignore the warning because this a simple test.
	}
	return;
};

void OnServerSocektRecv(ErrorCode ec, int recvLength)
{
	if (ec != EC_SUCCESS)
	{
		curAcceptCount = 0;
		g_safeexit = true;
		return;
	}
	LOGI("recv client msg len = " << recvLength << ", msg :" << recvBuffer);
	memcpy(sendBuffer, recvBuffer, recvLength);
	sendBufferLen = recvLength;
	bool ret = usedSocket->DoSend(sendBuffer, sendBufferLen, OnSocektSend);// safe-warning: can't call this methon again before receive last DoSend callback. 
	if (!ret)
	{
		g_safeexit = true;
		curAcceptCount = 0;
		return;
	}
	ret = usedSocket->DoRecv(recvBuffer, recvBufferLen, OnServerSocektRecv);
	if (!ret)
	{
		g_safeexit = true;
		curAcceptCount = 0;
		return;
	}
};


void OnAcceptSocket(ErrorCode ec, CTcpSocketPtr s)
{
	if (ec != EC_SUCCESS)
	{
		return;
	}
	if (curAcceptCount >= maxAcceptCount)
	{
		LOGW("max client limit is " << maxAcceptCount << ", curCount is" << curAcceptCount);
		return;
	}
	g_safeexit = false;
	curAcceptCount++;
	usedSocket = s;
	usedSocket->DoRecv(recvBuffer, recvBufferLen, OnServerSocektRecv);
	ts = std::shared_ptr<CTcpSocket>(new CTcpSocket);
	ts->Initialize(summer);
	accepter->DoAccept(ts, OnAcceptSocket);

};


void SendOneMsg()
{
	if (usedSocket && summer && g_runing && !g_safeexit)
	{
		sprintf(sendBuffer, "%s", "hellow");
		sendBufferLen = strlen(sendBuffer) + 1;
		// safe-warning: can't call this methon again before receive last DoSend callback. 
		usedSocket->DoSend(sendBuffer, sendBufferLen, OnSocektSend);
	}
}
void OnClientSocektRecv(ErrorCode ec, int recvLength)
{
	if (ec != EC_SUCCESS)
	{
		g_runing = false;
		g_safeexit = true;
		curAcceptCount = 0;
		return;
	}
	LOGI("recv server msg len = " << recvLength << ", msg :" << recvBuffer);
	summer->CreateTimer(1000, SendOneMsg);
	bool ret = usedSocket->DoRecv(recvBuffer, recvBufferLen, OnClientSocektRecv);
	if (!ret) { g_safeexit = true; return; };
};


void OnConnect(ErrorCode ec)
{
	if (ec != EC_SUCCESS)
	{
		LOGE("connect error");
		return;
	}
	g_safeexit = false;
	usedSocket->DoRecv(recvBuffer, recvBufferLen, OnClientSocektRecv);
	summer->CreateTimer(1000, SendOneMsg);
};


int main(int argc, char* argv[])
{
	signal(SIGINT, signalFun);
	if (argc == 2 && 
		(strcmp(argv[1], "--help") == 0 
		|| strcmp(argv[1], "/?") == 0))
	{
		cout << "please input like example:" << endl;
		cout << "tcpTest remoteIP remotePort startType" << endl;
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
		g_startType = atoi(argv[3]);
	}


	
	if (g_startType == 0)
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

	LOGI("g_remoteIP=" << g_remoteIP << ", g_remotePort=" << g_remotePort << ", g_startType=" << g_startType );


	

	//! 启动调度器
	summer = std::shared_ptr<CZSummer>(new CZSummer);
	summer->Initialize();

	if (g_startType == 0)
	{
		accepter = std::shared_ptr<CTcpAccept>(new CTcpAccept(summer));
		ts = std::shared_ptr<CTcpSocket>(new CTcpSocket);
		if (!accepter->OpenAccept(g_remoteIP.c_str(), g_remotePort))
		{
			return 0;
		}
		ts->Initialize(summer);
		accepter->DoAccept(ts, OnAcceptSocket);
	}
	else
	{
		usedSocket = std::shared_ptr<CTcpSocket>(new CTcpSocket);
		usedSocket->Initialize(summer);
		usedSocket->DoConnect(g_remoteIP.c_str(), g_remotePort, OnConnect);
	}
	
	while (g_runing || !g_safeexit)
	{
		summer->RunOnce();
	}
	//warning: be safe close :  1.all socket call close. 2. all socket receive recv callback. 3 stop runonce and safe exit.
	//current test will  output some safety warning with error log because the test not safe exit. but no problem.

	return 0;
}


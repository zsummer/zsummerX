/*
 * ZSUMMER License
 * -----------
 * 
 * ZSUMMER is licensed under the terms of the MIT license reproduced below.
 * This means that ZSUMMER is free software and can be used for both academic
 * and commercial purposes at absolutely no cost.
 * 
 * 
 * ===============================================================================
 * 
 * Copyright (C) 2013 YaweiZhang <yawei_zhang@foxmail.com>.
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

//! zsummer的测试客户端模块
//! main文件

#include <signal.h>

#include "../../network/zsummer.h"
#include "../../network/tcpaccept.h"
#include "../../network/tcpsocket.h"

using namespace std;



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
	zsummer::log4z::ILog4zManager::GetInstance()->Config("client.cfg");
	zsummer::log4z::ILog4zManager::GetInstance()->Start();
	zsummer::network::CZSummer summer;
	summer.Initialize();
	typedef std::shared_ptr<zsummer::network::CTcpSocket> CTcpSocketPtr;
	typedef std::function<void(zsummer::network::ErrorCode, std::string, unsigned short)> _OnConnectHandler;
	typedef std::function<void(zsummer::network::ErrorCode, char *, unsigned int, unsigned int)> _OnSendHandler;
	typedef _OnSendHandler _OnRecvHandler;
	std::list<CTcpSocketPtr> clients;
	auto onConnect = [](zsummer::network::ErrorCode ec, std::string ip, unsigned short port)
	{
		if (!ec)
		{
			LOGI("connect success, remote ip=" << ip << ", remote port=" << port);
		}
		else
		{
			LOGE("connect error, remote ip=" << ip << ", remote port=" << port);
		}
	};
	auto doConnect = [&]()
	{
		for (int i = 0; i < 10; i++)
		{
			std::string ip="61.135.169.125";
			unsigned short port = 80;
			CTcpSocketPtr s(new zsummer::network::CTcpSocket(ip, port));
			s->Initialize(summer);
			s->DoConnect(std::bind(onConnect, std::placeholders::_1, ip, port));
			clients.push_back(s);
		}
	};
	summer.Post(doConnect);
	do
	{
		summer.RunOnce();
	} while (true);
	return 0;
}


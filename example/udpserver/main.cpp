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
#include "../../depends/log4z/log4z.h"
#include "../../network/zsummer.h"
#include "../../network/udpsocket.h"
#include "../../depends/protocol4z/protocol4z.h"
using namespace std;
using namespace zsummer::network;

//! 消息包缓冲区大小
#define _MSG_BUF_LEN	(1000)

//! 消息包 
struct Packet
{
	char		   _orgdata[_MSG_BUF_LEN];
	unsigned short _len;
	std::string _remoteIP;
	unsigned short _remotePort;
};





int main(int argc, char* argv[])
{

	//! linux下需要屏蔽的一些信号
#ifndef _WIN32
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
	//client 初始化
	typedef std::shared_ptr<Packet> PacketPtr;
	zsummer::log4z::ILog4zManager::GetInstance()->Config("server.cfg");
	zsummer::log4z::ILog4zManager::GetInstance()->Start();
	zsummer::network::CZSummer summer;
	summer.Initialize();
	zsummer::network::CUdpSocket server;
	server.Initialize(summer, "0.0.0.0", 82);
	char recvFromBuf[_MSG_BUF_LEN];
	//statistic
	unsigned long long totalCount = 0;

	//! 接收
	//void (ErrorCode, const char*, unsigned short, int)
	std::function<void (zsummer::network::ErrorCode, const char*, unsigned short, int)>  onRecv = [&](zsummer::network::ErrorCode ec, 
		const char* ip, unsigned short port, int transelate)
	{
		if (ec)
		{
			LOGE ("onRecvFrom Error, EC=" << ec );
			return;
		}
		int ret = zsummer::protocol4z::CheckBuffIntegrity(recvFromBuf, transelate,_MSG_BUF_LEN);
		if (ret == 0)
		{
			//! 解包
			zsummer::protocol4z::ReadStream rs(recvFromBuf, transelate);
			try
			{
				//协议流异常会被上层捕获并关闭连接
				unsigned short protocolID = 0;
				rs >> protocolID;
				switch (protocolID)
				{
				case 1:
					{
						unsigned long long clientTick = 0;
						std::string text;
						rs >> clientTick >> text;
						char buf[_MSG_BUF_LEN];
						zsummer::protocol4z::WriteStream ws(buf, _MSG_BUF_LEN);
						ws << protocolID << clientTick << text;
						server.DoSend(buf, ws.GetWriteLen(), ip, port);
						totalCount++;
					}
					break;
				default:
					{
						LOGI("unknown protocol id = " << protocolID);
					}
					break;
				}
			}
			catch (std::runtime_error e)
			{
				LOGE("MessageEntry catch one exception: "<< e.what() );
			}
		}
		server.DoRecv(recvFromBuf, _MSG_BUF_LEN, onRecv);
	};

	unsigned long long lastTotalCount = 0;
	std::function<void(unsigned long long)> onTimer = [&](unsigned long long)
	{
		LOGI("Current Recv Total Count=" << totalCount << ", speed=" << (totalCount - lastTotalCount)/5);
		lastTotalCount = totalCount;
		summer.CreateTimer(5000, onTimer);
	};
	summer.CreateTimer(5000, onTimer);
	server.DoRecv(recvFromBuf, _MSG_BUF_LEN, onRecv);

	do
	{
		summer.RunOnce();
	} while (true);
	return 0;
}


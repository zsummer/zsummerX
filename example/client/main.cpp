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
#include <log4z/log4z.h>
#include <protocol4z/protocol4z.h>
using namespace std;
using namespace zsummer::network;
//! 消息包缓冲区大小
#define _MSG_BUF_LEN	(2*1024)
std::string g_fillString;
#define  NOW_TIME (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count())
//! 消息包 
struct Packet
{
	char		   _orgdata[_MSG_BUF_LEN];
	unsigned short _len;
	unsigned short _curpos;
	unsigned long long   _reqTime;
};

struct Client
{
public:
	Packet recvData;
	Packet sendData;
	CTcpSocket c;
};
typedef std::shared_ptr<Client> ClientPtr;


//统计
struct tagStatistic
{
	enum TYPE_DELAY
	{
		TD_SEND,
		TD_TOTAL,
		TD_END,
	};
	enum TIME_DELAY
	{
		TM_1MS, // < 1 ms
		TM_5MS, // < 5 ms
		TM_10MS,
		TM_20MS,
		TM_40MS,
		TM_60MS,
		TM_100MS,
		TM_1000MS,
		TM_LOWMS,// error dalay time
		TM_END,
	};
	unsigned long long _delay[TD_END][TM_END];
	inline void addDelayData(TYPE_DELAY td, unsigned long long usedtime)
	{
		if (usedtime <= 1)
		{
			_delay[td][TM_1MS]++;
		}
		else if (usedtime <= 5)
		{
			_delay[td][TM_5MS]++;
		}
		else if (usedtime <= 10)
		{
			_delay[td][TM_10MS]++;
		}
		else if (usedtime <= 20)
		{
			_delay[td][TM_20MS]++;
		}
		else if (usedtime <= 40)
		{
			_delay[td][TM_40MS]++;
		}
		else if (usedtime <= 60)
		{
			_delay[td][TM_60MS]++;
		}
		else if (usedtime <= 100)
		{
			_delay[td][TM_100MS]++;
		}
		else if (usedtime <= 1000)
		{
			_delay[td][TM_1000MS]++;	
		}
		else
		{
			_delay[td][TM_LOWMS]++;
		}
	}
};

bool g_runing = true;
void sig(int)
{
	g_runing = false;
}

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
	signal(SIGINT,sig);
	//client 初始化
	zsummer::log4z::ILog4zManager::GetInstance()->Config("client.cfg");
	zsummer::log4z::ILog4zManager::GetInstance()->Start();

	zsummer::network::CZSummer summer;
	summer.Initialize();
	std::list<ClientPtr> clients;

	g_fillString.resize(1000, 'z');
	//! 统计信息
	tagStatistic statist;
	memset(&statist, 0, sizeof(statist));
	int ping_pong_test = 1;


	//! 发送
	std::function<void(ErrorCode, int,ClientPtr)>  onSend = [&](ErrorCode ec, int ts, ClientPtr c)
	{
		if (ec)
		{
			LOGE ("onSend Error, EC=" << ec );
			return;
		}
		if (c->sendData._curpos + ts < c->sendData._len)
		{
			c->sendData._curpos+= ts;
			c->c.DoSend(c->sendData._orgdata+c->sendData._curpos, c->sendData._len-c->sendData._curpos, std::bind(onSend, std::placeholders::_1, std::placeholders::_2, c));
		}
		else
		{
			c->sendData._curpos+=ts;
			statist.addDelayData(tagStatistic::TD_SEND, NOW_TIME-c->sendData._reqTime);
		}
	};
	//! 请求发送
	auto doSend = [&](ClientPtr c)
	{
		
		c->sendData._reqTime = NOW_TIME;
		zsummer::protocol4z::WriteStream ws(c->sendData._orgdata, _MSG_BUF_LEN);
		ws << (unsigned short) 1; //protocol id
		ws << c->sendData._reqTime; // local tick count
		ws << g_fillString; // append text, fill the length protocol.
		c->sendData._len = ws.GetWriteLen();
		c->sendData._curpos = 0;
		c->c.DoSend(c->sendData._orgdata+c->sendData._curpos, c->sendData._len-c->sendData._curpos, std::bind(onSend, std::placeholders::_1, std::placeholders::_2, c));
	};

	//! 接收
	std::function<void(ErrorCode, int, ClientPtr)>  onRecv = [&](ErrorCode ec, int ts, ClientPtr c)
	{
		if (ec)
		{
			LOGE ("onRecv Error, EC=" << ec );
			return;
		}
		c->recvData._curpos += ts;
		int ret = zsummer::protocol4z::CheckBuffIntegrity(c->recvData._orgdata, c->recvData._curpos,_MSG_BUF_LEN);
		if (ret == -1)
		{
			LOGD("CheckBuffIntegrity fail.");
			c->c.DoClose();
			return ;
		}
		else if (ret > 0)
		{
			auto temp = std::bind(onRecv, std::placeholders::_1, std::placeholders::_2, c);
			c->c.DoRecv(c->recvData._orgdata+c->recvData._curpos, ret, temp);
		}
		else if (ret == 0)
		{
			//! 解包
			zsummer::protocol4z::ReadStream rs(c->recvData._orgdata, c->recvData._curpos);
			try
			{
				//协议流异常会被上层捕获并关闭连接
				unsigned short protocolID = 0;
				rs >> protocolID;
				switch (protocolID)
				{
				case 1:
					{
						unsigned long long localTick = 0;
						std::string text;
						rs >> localTick >> text;
						unsigned long long curTick = NOW_TIME;
						curTick = curTick - localTick;
						statist.addDelayData(tagStatistic::TD_TOTAL, curTick);
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
				c->c.DoClose();
				return ;
			}
			c->recvData._len = 0;
			c->recvData._curpos = 0;
			c->recvData._reqTime = NOW_TIME;
			c->c.DoRecv(c->recvData._orgdata+c->recvData._curpos, 2, std::bind(onRecv, std::placeholders::_1, std::placeholders::_2, c));
			if (ping_pong_test)
			{
				doSend(c);
			}
			else
			{
				summer.CreateTimer(9000 + rand()%1500, std::bind(doSend, c));
			}
		}
	};
	//请求接收
	std::function<void(ClientPtr)> doRecv = [&](ClientPtr c)
	{

		c->recvData._reqTime = NOW_TIME;
		c->recvData._len = 0;
		c->recvData._curpos = 0;
		c->c.DoRecv(c->recvData._orgdata+c->recvData._curpos, 2, std::bind(onRecv, std::placeholders::_1, std::placeholders::_2, c));
	};

	//建立连接响应
	auto onConnect = [&](zsummer::network::ErrorCode ec, ClientPtr c)
	{
		if (!ec)
		{
			LOGI("connect success");
			clients.push_back(c);
			doRecv(c);
			doSend(c);
		}
		else
		{
			LOGE("connect error, ERRORCODE=" << ec);
		}
	};

	int n=1;
	std::string ip;
	unsigned short port = 0;
	//请求建立连接
	std::function<void(int)> doConnect = [&](int n)
	{
		for (int i = 1; i <= n; i++)
		{
			
			ClientPtr c(new Client());
			c->c.Initialize(summer);
			c->c.DoConnect(ip, port, std::bind(onConnect, std::placeholders::_1, c));
			if (i >= 200)
			{
				summer.CreateTimer(1000, std::bind(doConnect, n-i));
				break;
			}
		}
	};
	LOGI("please input ip: ");
	cin >> ip;
	LOGI("please input port: ");
	cin >> port;
	LOGI("please input clients count:");
	cin >> n;
	LOGI("please input is ping-pong-test ");
	cin >> ping_pong_test;
	LOGI("ip[" << ip << "], port[" << port <<"], clients[" << n << "], ping-pong[" 
		<< ping_pong_test << "]. please push any key to continue.");
	getchar();
	getchar();
	summer.Post(std::bind(doConnect, n));

	//定时检测
	std::function<void()> doTimer = [&]()
	{
		LOGI("-- type -- 1MS -- 5MS -- 10MS -- 20MS -- 40MS -- 60MS -- 100MS -- 1S -- LOW MS --");
		for (int i=0; i<tagStatistic::TD_END; i++)
		{
			LOGI("--  " << i << " "
			<< " -- " << statist._delay[i][tagStatistic::TM_1MS]
			<< " -- " << statist._delay[i][tagStatistic::TM_5MS]
			<< " -- " << statist._delay[i][tagStatistic::TM_10MS]
			<< " -- " << statist._delay[i][tagStatistic::TM_20MS]
			<< " -- " << statist._delay[i][tagStatistic::TM_40MS]
			<< " -- " << statist._delay[i][tagStatistic::TM_60MS]
			<< " -- " << statist._delay[i][tagStatistic::TM_100MS]
			<< " -- " << statist._delay[i][tagStatistic::TM_1000MS]
			<< " -- " << statist._delay[i][tagStatistic::TM_LOWMS] << " --");
		}
		summer.CreateTimer(10*1000, doTimer);
	};


	summer.CreateTimer(10*1000, doTimer);
	do
	{
		summer.RunOnce();
	} while (g_runing);
	while (!clients.empty())
	{
		ClientPtr c(clients.front());
		c->c.DoClose();
		clients.pop_front();
	}

	return 0;
}


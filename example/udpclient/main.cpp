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
#include "../../network/udpsocket.h"
#include <log4z/log4z.h>
#include <protocol4z/protocol4z.h>
using namespace std;
using namespace zsummer::network;
//! 消息包缓冲区大小
#define _MSG_BUF_LEN	(1200)
std::string g_fillString;
//! 消息包 
struct Picnic
{
	char		   recvData[_MSG_BUF_LEN];
	char		   sendData[_MSG_BUF_LEN];
	unsigned long long _reqTime;
	zsummer::network::CUdpSocket sock;
};
typedef std::shared_ptr<Picnic> PicnicPtr;

//统计
struct tagStatistic
{
	enum TYPE_DELAY
	{
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
	zsummer::log4z::ILog4zManager::GetInstance()->Config("client.cfg");
	zsummer::log4z::ILog4zManager::GetInstance()->Start();
	zsummer::network::CZSummer summer;
	summer.Initialize();
	g_fillString.resize(1000, 'z');
	int n=1;
	std::string ip;
	unsigned short port = 0;
	LOGI("please input ip: ");
	cin >> ip;
	LOGI("please input port: ");
	cin >> port;
	LOGI("please input clients count:");
	cin >> n;
	LOGI("ip[" << ip << "], port[" << port <<"], clients[" << n << "]. please push any key to continue.");
	getchar();
	getchar();


	//! 统计信息
	tagStatistic statist;
	memset(&statist, 0, sizeof(statist));
	unsigned long long totalSend = 0;
	unsigned long long totalRecv = 0;



	//! 请求发送
	std::function<void(const char*, unsigned short, PicnicPtr)> doSend = [&](const char *remoteIP, unsigned short remotePort, PicnicPtr pic)
	{
		pic->_reqTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		zsummer::protocol4z::WriteStream ws(pic->sendData, _MSG_BUF_LEN);
		ws << (unsigned short) 1; //protocol id
		ws << pic->_reqTime; // local tick count
		ws << g_fillString; // append text, fill the length protocol.
		pic->sock.DoSendTo(pic->sendData, ws.GetWriteLen(), remoteIP, remotePort);
		//summer.CreateTimer(rand()%5000+2500,std::bind(doSend,std::placeholders::_1, ip.c_str(), port, pic));
		totalSend++;

	};

	//! 接收
	std::function<void(ErrorCode, const char*, unsigned short, int, PicnicPtr)>  onRecv = [&](ErrorCode ec,
		const char *remoteIP, unsigned short port, int translate, PicnicPtr pic)
	{
		
		if (ec)
		{
			LOGE ("onRecv Error, EC=" << ec );
			return;
		}
		
		int ret = zsummer::protocol4z::CheckBuffIntegrity(pic->recvData, translate, _MSG_BUF_LEN);
		if (ret == 0)
		{
			//! 解包
			zsummer::protocol4z::ReadStream rs(pic->recvData, translate);
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
						unsigned long long curTick = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
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
			}
			
		}
		totalRecv++;
		pic->sock.DoRecvFrom(pic->recvData, _MSG_BUF_LEN, std::bind(onRecv, 
											std::placeholders::_1,
											std::placeholders::_2,
											std::placeholders::_3,
											std::placeholders::_4,
											pic));
		doSend(ip.c_str(), port, pic);
	};







	for (int i = 0; i < n; i++)
	{
		PicnicPtr pic(new Picnic);
		if (!pic->sock.Initialize(summer, "0.0.0.0", 0))
		{
			LOGI("init udp socket error.");
			continue;
		}
		pic->sock.DoRecvFrom(pic->recvData, _MSG_BUF_LEN, std::bind(onRecv, 
													std::placeholders::_1,
													std::placeholders::_2,
													std::placeholders::_3,
													std::placeholders::_4,
													pic));
		summer.CreateTimer(rand()%1000+3000,std::bind(doSend,ip.c_str(), port, pic));
	}


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
		LOGI("totalSend="<< totalSend << ", totalRecv=" << totalRecv);
		summer.CreateTimer(10*1000, doTimer);
	};
	summer.CreateTimer(10*1000, doTimer);
	do
	{
		summer.RunOnce();
	} while (true);
	return 0;
}


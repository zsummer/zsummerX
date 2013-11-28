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
#include "../../network/tcpaccept.h"
#include "../../network/tcpsocket.h"
#include "../../depends/protocol4z/protocol4z.h"
using namespace std;
using namespace zsummer::network;
//! 消息包缓冲区大小
#define _MSG_BUF_LEN	(8*1024)

//! 消息包 
struct Packet
{
	char		   _orgdata[_MSG_BUF_LEN];
	unsigned short _len;
	unsigned short _curpos;
	unsigned long long   _reqTime;
};

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
	std::list<CTcpSocketPtr> clients;
	typedef std::shared_ptr<Packet> PacketPtr;

	//! 统计信息
	tagStatistic statist;
	memset(&statist, 0, sizeof(statist));


	//! 发送
	std::function<void(ErrorCode, int,PacketPtr,CTcpSocketPtr)>  onSend = [&](ErrorCode ec, int ts, PacketPtr pack, CTcpSocketPtr c)
	{
		if (ec)
		{
			LOGE ("onSend Error, EC=" << ec );
			return;
		}
		if (pack->_curpos + ts < pack->_len)
		{
			pack->_curpos+= ts;
			c->DoSend(pack->_orgdata+pack->_curpos, pack->_len-pack->_curpos, std::bind(onSend, std::placeholders::_1, std::placeholders::_2, pack, c));
		}
		else
		{
			pack->_curpos+=ts;
			statist.addDelayData(tagStatistic::TD_SEND, std::chrono::system_clock::now().time_since_epoch()/std::chrono::milliseconds(1)-pack->_reqTime);
		}
	};
	//! 请求发送
	auto doSend = [&](CTcpSocketPtr c)
	{

		PacketPtr pack(new Packet);
		pack->_reqTime = std::chrono::system_clock::now().time_since_epoch()/std::chrono::milliseconds(1);
		zsummer::protocol4z::WriteStream ws(pack->_orgdata, _MSG_BUF_LEN);
		ws << (unsigned short) 1; //protocol id
		ws << pack->_reqTime; // local tick count
		ws << "100000000000000000000000000"; // append text, fill the length protocol.
		pack->_len = ws.GetWriteLen();
		pack->_curpos = 0;
		c->DoSend(pack->_orgdata+pack->_curpos, pack->_len-pack->_curpos, std::bind(onSend, std::placeholders::_1, std::placeholders::_2, pack, c));
	};

	//! 接收
	std::function<void(ErrorCode, int, PacketPtr,CTcpSocketPtr)>  onRecv = [&](ErrorCode ec, int ts, PacketPtr pack, CTcpSocketPtr c)
	{
		if (ec)
		{
			LOGE ("onRecv Error, EC=" << ec );
			return;
		}
		pack->_curpos += ts;
		int ret = zsummer::protocol4z::CheckBuffIntegrity(pack->_orgdata, pack->_curpos,_MSG_BUF_LEN);
		if (ret == -1)
		{
			LOGD("CheckBuffIntegrity fail.");
			c->DoClose();
			return ;
		}
		else if (ret > 0)
		{
			auto temp = std::bind(onRecv, std::placeholders::_1, std::placeholders::_2, pack, c);
			c->DoRecv(pack->_orgdata+pack->_curpos, ret, temp);
		}
		else if (ret == 0)
		{
			//! 解包
			zsummer::protocol4z::ReadStream rs(pack->_orgdata, pack->_curpos);
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
						unsigned long long curTick = std::chrono::system_clock::now().time_since_epoch()/std::chrono::milliseconds(1);
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
				c->DoClose();
				return ;
			}
			pack->_len = 0;
			pack->_curpos = 0;
			pack->_reqTime = std::chrono::system_clock::now().time_since_epoch()/std::chrono::milliseconds(1);
			c->DoRecv(pack->_orgdata+pack->_curpos, 2, std::bind(onRecv, std::placeholders::_1, std::placeholders::_2, pack, c));
			summer.CreateTimer(1500 + rand()%3000, std::bind(doSend, c));
			//doSend(c);
		}
	};
	//请求接收
	std::function<void(CTcpSocketPtr)> doRecv = [&](CTcpSocketPtr c)
	{
		PacketPtr pack(new Packet);
		pack->_reqTime = std::chrono::system_clock::now().time_since_epoch()/std::chrono::milliseconds(1);
		pack->_len = 0;
		pack->_curpos = 0;
		c->DoRecv(pack->_orgdata+pack->_curpos, 2, std::bind(onRecv, std::placeholders::_1, std::placeholders::_2, pack, c));
	};

	//建立连接响应
	auto onConnect = [&](zsummer::network::ErrorCode ec, CTcpSocketPtr c)
	{
		if (!ec)
		{
			LOGI("connect success");
			//clients.push_back(c);
			doRecv(c);
			doSend(c);
		}
		else
		{
			LOGE("connect error, ERRORCODE=" << ec);
		}
	};
	//请求建立连接
	auto doConnect = [&]()
	{

		for (int i = 0; i < 1; i++)
		{
			std::string ip="127.0.0.1";
			unsigned short port = 81;
			CTcpSocketPtr c(new zsummer::network::CTcpSocket(ip, port));
			c->Initialize(summer);
			c->DoConnect(std::bind(onConnect, std::placeholders::_1, c));
		}
	};
	summer.Post(doConnect);

	//定时检测
	std::function<void(unsigned long long)> doTimer = [&](unsigned long long)
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
	} while (true);
	return 0;
}


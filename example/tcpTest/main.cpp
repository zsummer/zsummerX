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
 * Copyright (C) 2010-2013 YaweiZhang <yawei_zhang@foxmail.com>.
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

//! zsummer的测试服务模块(对应zsummer底层网络封装的上层设计测试服务) 可视为服务端架构中的 gateway服务/agent服务/前端服务, 特点是高并发高吞吐量
//! main文件


#include "header.h"
#include "Schedule.h"
#include "Process.h"
using namespace zsummer::log4z;

std::string g_remoteIP = "0.0.0.0";
unsigned short g_remotePort = 81;
unsigned short g_startType = 0;  //0 listen, 1 connect
unsigned short g_maxClient = 1; //0 echo send, 1 direct send
unsigned short g_sendType = 0; //0 echo send, 1 direct send
unsigned int   g_intervalMs = 0; // send interval

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
	if (argc == 2 && 
		(strcmp(argv[1], "--help") == 0 
		|| strcmp(argv[1], "/?") == 0))
	{
		cout <<"please input like example:" << endl;
		cout << "./tcpTest remoteIP=0.0.0.0 remotePort=81 startType=0 maxClient=1 sendType=0 interval=0" << endl;
		cout <<"startType: 0 server, 1 client" << endl;
		cout <<"maxClient: limite max" << endl;
		cout <<"sendType: 0 echo send, 1 direct send" << endl;
		cout <<"interval: send once interval" << endl;
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
	if (argc > 4)
	{
		g_maxClient = atoi(argv[4]);
	}
	if (argc > 5)
	{
		g_sendType = atoi(argv[5]);
	}
	if (argc > 6)
	{
		g_intervalMs = atoi(argv[6]);
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
	LOGI("g_remoteIP=" << g_remoteIP << ", g_remotePort=" << g_remotePort << ", g_startType=" << g_startType 
		<< ", g_maxClient=" << g_maxClient << ", g_sendType=" << g_sendType << ", g_intervalMs=" << g_intervalMs);



	//! 启动调度器
	CSchedule schedule;
	schedule.Start();


	//main线程用于服务状态统计与输出

	unsigned long long nLast[10] = {0};
	unsigned long long temp[10] = {0};
	for (;;)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(5000));
		memset(&temp, 0, sizeof(temp));

		for (std::vector<CProcess *>::const_iterator iter = schedule.m_process.begin(); iter != schedule.m_process.end(); ++iter)
		{
			temp[0] += (*iter)->GetTotalRecvLen();
			temp[1] += (*iter)->GetTotalSendLen();
			temp[2] += (*iter)->GetTotalRecvCount();
			temp[3] += (*iter)->GetTotalSendCount();
			temp[4] += (*iter)->GetTotalOpen();
			temp[5] += (*iter)->GetTotalClosed();
			temp[6] += (*iter)->GetTotalEcho();
			temp[7] += (*iter)->GetTotalEchoTime();
		}
		LOGD("Linked[" << temp[4] 
			<<"]  Closed[" << temp[5]
			<<"]  Recv[" << (temp[0] - nLast[0])/1024.0/1024.0/5.0
			<<"]M  Send[" << (temp[1] - nLast[1])/1024.0/1024.0/5.0
			<<"]M  RecvSpeed[" << (temp[2] - nLast[2])/5.0
			<<"]  SendSpeed[" << (temp[3] - nLast[3])/5.0
			<<"]  EchoSpeed[" << (temp[6]- nLast[6])/5.0
			<<"]  DelayTime[" << (temp[7]-nLast[7])/1.0/(temp[6]-nLast[6] == 0 ? 1 :temp[6]-nLast[6])
			<<"].");


		memcpy(&nLast, &temp, sizeof(temp));
	}

	schedule.Stop();

	//日志服务不需要显式调用停止接口
	//InterfaceLogger::GetInstance()->Stop();
	return 0;
}


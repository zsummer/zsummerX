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

//! zsummer的测试服务模块(对应zsummer底层网络封装的上层设计测试服务) 可视为服务端架构中的 gateway服务/agent服务/前端服务, 特点是高并发高吞吐量
//! main文件


#include "header.h"
#include "Schedule.h"
#include "Process.h"
#include <thread>
#include <chrono>
using namespace zsummer::log4z;

std::string g_remoteIP = "0.0.0.0";
unsigned short g_remotePort = 8081;
unsigned short g_startType = 0;  //0 listen, 1 connect
unsigned short g_maxClient = 1; //
unsigned int   g_multiThread = 2; //multithread
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
        cout << "please input like example:" << endl;
        cout << "tcpTest remoteIP remotePort startType maxClient sendType interval" << endl;
        cout << "./tcpTest 0.0.0.0 8081 0" << endl;
        cout << "startType: 0 server, 1 client" << endl;
        cout << "maxClient: limit max" << endl;
        cout << "multi-thread: default 2" << endl;
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
        g_multiThread = atoi(argv[5]);
    }
    g_multiThread = g_multiThread < 1 ? 1 : g_multiThread;
    g_multiThread = g_multiThread > 100 ? 100 : g_multiThread;

    if (g_startType == 0)
    {
        //! 启动日志服务
        ILog4zManager::getPtr()->config("server.cfg");
        ILog4zManager::getPtr()->start();
    }
    else
    {
                //! 启动日志服务
        ILog4zManager::getPtr()->config("client.cfg");
        ILog4zManager::getPtr()->start();
    }
    LOGI("g_remoteIP=" << g_remoteIP << ", g_remotePort=" << g_remotePort << ", g_startType=" << g_startType 
        << ", g_maxClient=" << g_maxClient   << ", g_multiThread=" << g_multiThread);



    //! 启动调度器
    CSchedule schedule;
    schedule.start();


    //main线程用于服务状态统计与输出

    unsigned long long nLast[10] = {0};
    unsigned long long temp[10] = {0};
    for (;;)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        memset(&temp, 0, sizeof(temp));

        for (std::vector<CProcess *>::const_iterator iter = schedule._process.begin(); iter != schedule._process.end(); ++iter)
        {
            temp[0] += (*iter)->_nTotalRecvLen;
            temp[1] += (*iter)->_nTotalSendLen;
            temp[2] += (*iter)->_nTotalRecvCount;
            temp[3] += (*iter)->_nTotalSendCount;
            temp[4] += (*iter)->_nTotalOpen;
            temp[5] += (*iter)->_nTotalClosed;
            temp[6] += (*iter)->_nTotalRecvPacket;
            temp[7] += (*iter)->_nTotalSendPacket;
        }
        LOGD("Linked[" << temp[4] 
            <<"]  Closed[" << temp[5]
            <<"]  Recv[" << (temp[0] - nLast[0])/1024.0/1024.0/5.0
            <<"]M  Send[" << (temp[1] - nLast[1])/1024.0/1024.0/5.0
            <<"]M  RecvSpeed[" << (temp[2] - nLast[2])/5.0
            <<"]  SendSpeed[" << (temp[3] - nLast[3])/5.0
            <<"]  RecvPacket[" << (temp[6]- nLast[6])/5.0
            <<"]  SendPacket[" << (temp[7]-nLast[7])/5.0
            <<"].");


        memcpy(&nLast, &temp, sizeof(temp));
    }

    schedule.stop();

    return 0;
}


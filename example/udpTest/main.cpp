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

//! zsummer的测试客户端模块
//! main文件

#include <signal.h>
#include <zsummerX/zsummerX.h>
#include <log4z/log4z.h>
#include <proto4z/proto4z.h>
using namespace std;
using namespace zsummer::network;
using namespace zsummer::proto4z;
//! 消息包缓冲区大小
#define _MSG_BUF_LEN    (1200)
std::string g_fillString;
#define NOW_TIME (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count())
unsigned short g_type;
//! 消息包 
struct Picnic
{
    char           recvData[_MSG_BUF_LEN];
    char           sendData[_MSG_BUF_LEN];
    std::string textCache;
    unsigned long long _reqTime;
    UdpSocketPtr sock;
};
typedef std::shared_ptr<Picnic> PicnicPtr;

unsigned long long g_totalEcho;
unsigned long long g_totalEchoTime;
unsigned long long g_totalSend;
unsigned long long g_totalRecv;

//! 请求发送
void doSend(const char *remoteIP, unsigned short remotePort, unsigned short protoID, unsigned long long clientTick, PicnicPtr pic)
{
    pic->_reqTime = clientTick;
    zsummer::proto4z::WriteStream ws(protoID);
    ws << pic->_reqTime; // local tick count
    ws << g_fillString; // append text, fill the length protocol.
    pic->sock->doSendTo(ws.getStream(), ws.getStreamLen(), remoteIP, remotePort);
    g_totalSend++;
};

void onRecv(NetErrorCode ec, const char *remoteIP, unsigned short remotePort, int translate, PicnicPtr pic)
{
    if (ec)
    {
        LOGE ("onRecv Error, EC=" << ec );
        return;
    }

    auto ret = zsummer::proto4z::checkBuffIntegrity(pic->recvData, translate, _MSG_BUF_LEN, _MSG_BUF_LEN);
    if (ret.first == IRT_SUCCESS)
    {
        //! 解包
        zsummer::proto4z::ReadStream rs(pic->recvData, translate);
        try
        {
            //协议流异常会被上层捕获并关闭连接
            unsigned short protoID = 0;
            rs >> protoID;
            switch (protoID)
            {
            case 1:
                {
                    unsigned long long clientTick = 0;
                    pic->textCache.clear();
                    rs >> clientTick >> pic->textCache;
                    g_totalRecv++;
                    if (g_type == 1)
                    {
                        doSend(remoteIP, remotePort, protoID, clientTick, pic);
                    }
                    else
                    {
                        unsigned long long curTick = NOW_TIME;
                        if (curTick < clientTick)
                        {
                            throw std::runtime_error("curTick < clientTick");
                        }
                        g_totalEcho ++;
                        g_totalEchoTime +=(curTick - clientTick);
                        doSend(remoteIP, remotePort, 1, NOW_TIME, pic);
                    }
                }
                break;
            default:
                {
                    LOGI("unknown protocol id = " << protoID);
                }
                break;
            }
        }
        catch (std::runtime_error e)
        {
            LOGE("MessageEntry catch one exception: "<< e.what() );
        }
        pic->sock->doRecvFrom(pic->recvData, _MSG_BUF_LEN, std::bind(onRecv, 
            std::placeholders::_1,
            std::placeholders::_2,
            std::placeholders::_3,
            std::placeholders::_4,
            pic));
    }
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
    if (argc < 5)
    {
        cout <<"please input like example:" << endl;
        cout <<"./udpTest ip port type maxClient" << endl;
        cout <<"type: 0 server, 1 client" << endl;
        return 0;
    }

    std::string ip = argv[1];
    unsigned short port = atoi(argv[2]);
    g_type = atoi(argv[3]);
    unsigned int maxClient = atoi(argv[4]);
    if (g_type ==1)
    {
        zsummer::log4z::ILog4zManager::getPtr()->config("server.cfg");
        zsummer::log4z::ILog4zManager::getPtr()->start();
    }
    else
    {
        zsummer::log4z::ILog4zManager::getPtr()->config("client.cfg");
        zsummer::log4z::ILog4zManager::getPtr()->start();
    }
    LOGI("ip=" << ip << ", port=" << port << ", type=" << g_type << ", maxClients=" << maxClient);
    
    zsummer::network::EventLoopPtr summer(new zsummer::network::EventLoop());
    summer->initialize();

    g_fillString.resize(1000, 'z');
    g_totalEcho = 0;
    g_totalEchoTime = 0;
    g_totalSend = 0;
    g_totalRecv = 0;

    
    if (g_type == 0)
    {
        PicnicPtr pic(new Picnic());
        pic->sock = std::make_shared<UdpSocket>();
        pic->sock->initialize(summer, ip.c_str(), port);
        pic->sock->doRecvFrom(pic->recvData, _MSG_BUF_LEN, std::bind(onRecv,
            std::placeholders::_1,
            std::placeholders::_2,
            std::placeholders::_3,
            std::placeholders::_4,
            pic));
    }
    else
    {

        for (unsigned int i = 0; i < maxClient; i++)
        {
            PicnicPtr picc(new Picnic);
            picc->sock = std::make_shared<UdpSocket>();
            if (!picc->sock->initialize(summer, "0.0.0.0", 0))
            {
                LOGI("init udp socket error.");
                continue;
            }
            picc->sock->doRecvFrom(picc->recvData, _MSG_BUF_LEN, std::bind(onRecv,
                std::placeholders::_1,
                std::placeholders::_2,
                std::placeholders::_3,
                std::placeholders::_4,
                picc));
            summer->createTimer(rand()%1000+1000,std::bind(doSend,ip.c_str(), port, 1, NOW_TIME, picc));
        }

    }
    
    
    //定时检测
    unsigned long long nLast[4] = {0};

    std::function<void()> doTimer = [&nLast, &doTimer, &summer]()
    {
        LOGI("EchoSpeed[" << (g_totalEcho - nLast[0])/5.0
            << "]  DelayTime[" << (g_totalEchoTime - nLast[1])/1.0/((g_totalEcho - nLast[0]) ==0?1:(g_totalEcho - nLast[0]))
            << "]  SendSpeed[" << (g_totalSend - nLast[2])/5.0 
            << "]  RecvSpeed[" << (g_totalRecv - nLast[3])/ 5.0
            << "].");
        nLast[0] = g_totalEcho;
        nLast[1] = g_totalEchoTime;
        nLast[2] = g_totalSend;
        nLast[3] = g_totalRecv;

        summer->createTimer(5 * 1000, std::bind(doTimer));
    };
    summer->createTimer(5*1000, std::bind(doTimer));
    do
    {
        summer->runOnce();
    } while (true);
    return 0;
}


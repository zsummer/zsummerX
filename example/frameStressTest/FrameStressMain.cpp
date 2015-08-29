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


//! frame module stress test
//! frame module used 'single sodule', if you need use multi-thread then you can create thread at user-code-layer, and you can look the example 'tcpStressTest'.

#include <zsummerX/zsummerX.h>
#include <unordered_map>
#include "TestProto.h"
using namespace zsummer::log4z;
using namespace zsummer::proto4z;
using namespace zsummer::network;
using namespace std::placeholders;

//! default value
std::string g_remoteIP = "0.0.0.0"; 
unsigned short g_remotePort = 8081; 
unsigned short g_startType = 0;  //0 start with server module, 1 start with client module. 
unsigned short g_maxClient = 1; //if start with server it is client limite count. else other it is the start client count.
unsigned short g_sendType = 0; //0 ping-pong test, 1 flooding test
unsigned int   g_intervalMs = 0; // if start client with flood test, it's flooding interval by millionsecond.
unsigned int   g_hightBenchmark = 0;




//!statistic 
unsigned long long g_lastStatInfo[STAT_SIZE] = { 0 };
unsigned long long getStatInfo(int stat)
{
    return SessionManager::getRef()._statInfo[stat] - g_lastStatInfo[stat];
}
//!statistic monitor
void MonitorFunc()
{
    LOGI("send count =" << getStatInfo(STAT_SEND_COUNT) / 5
        << "n/s, send pack = " << getStatInfo(STAT_SEND_PACKS) / 5
        << "n/s, send bytes = " << getStatInfo(STAT_SEND_BYTES) / 5
        << "byte/s, recv count = " << getStatInfo(STAT_RECV_COUNT) / 5
        << "n/s, recv pack = " << getStatInfo(STAT_RECV_PACKS) / 5
        << "n/s, recv bytes = " << getStatInfo(STAT_RECV_BYTES) / 5
        << "byte/s, linked = " << SessionManager::getRef()._statInfo[STAT_LINKED]
        << ", closed = " << SessionManager::getRef()._statInfo[STAT_CLOSED]
        << ", freeBlocks = " << SessionManager::getRef()._statInfo[STAT_FREE_BLOCKS]
        << ", exsitBlocks = " << SessionManager::getRef()._statInfo[STAT_EXIST_BLOCKS]
        );

    memcpy(g_lastStatInfo, SessionManager::getRef()._statInfo, sizeof(g_lastStatInfo));
    SessionManager::getRef().createTimer(5000, MonitorFunc);
};



//client handler
// ping-pong module test: 
//            1. send first message to server when client connect server success.
//            2. send message to server again when receive server echo.
// flooding module test:
//            1. create a timer used to send one message to server when client connect server success.
//            2. create a timer used to send next message again when the last timer trigger and send message.

class CStressClientHandler
{
public:
    CStressClientHandler()
    {
        MessageDispatcher::getRef().registerSessionMessage(ID_EchoPack,
            std::bind(&CStressClientHandler::msg_ResultSequence_fun, this, _1, _2));
    }

    void onConnected(TcpSessionPtr session)
    {
        LOGI("onConnected. ConnectorID=" << session->getSessionID() << ", remoteIP=" << session->getRemoteIP() << ", remotePort=" << session->getRemotePort() );
        SendFunc(session, g_sendType != 0);
    };
    void OnConnectDisconnect(TcpSessionPtr session)
    {
    }

    void msg_ResultSequence_fun(TcpSessionPtr session, ReadStream & rs)
    {
        if (g_hightBenchmark)
        {
            if (g_sendType == 0 || g_intervalMs == 0) //echo send
            {
                session->send(rs.getStream(), rs.getStreamLen());
            }
        }
        else
        {
            EchoPack pack;
            rs >> pack;

            if (g_sendType == 0 || g_intervalMs == 0) //echo send
            {
                WriteStream ws(ID_EchoPack);
                ws << pack;
                session->send(ws.getStream(), ws.getStreamLen());
            }
        }
    };

    void SendFunc(TcpSessionPtr session, bool openTimer)
    {
        if (session->getSendQueSize() < 1000)
        {
            WriteStream ws(ID_EchoPack);
            EchoPack pack;
            TestIntegerData idata;
            idata._char = 'a';
            idata._uchar = 100;
            idata._short = 200;
            idata._ushort = 300;
            idata._int = 400;
            idata._uint = 500;
            idata._i64 = 600;
            idata._ui64 = 700;
            idata._ui128 = 255;

            TestFloatData fdata;
            fdata._float = (float)123123.111111;
            fdata._double = 12312312.88888;

            TestStringData sdata;
            sdata._string = "abcdefg";

            pack._iarray.push_back(idata);
            pack._iarray.push_back(idata);
            pack._farray.push_back(fdata);
            pack._farray.push_back(fdata);
            pack._sarray.push_back(sdata);
            pack._sarray.push_back(sdata);

            pack._imap.insert(std::make_pair("123", idata));
            pack._imap.insert(std::make_pair("223", idata));
            pack._fmap.insert(std::make_pair("323", fdata));
            pack._fmap.insert(std::make_pair("423", fdata));
            pack._smap.insert(std::make_pair("523", sdata));
            pack._smap.insert(std::make_pair("623", sdata));
            ws << pack;
            session->send(ws.getStream(), ws.getStreamLen());
            if (openTimer)
            {
                SessionManager::getRef().createTimer(g_intervalMs, std::bind(&CStressClientHandler::SendFunc, this, session, !session->isInvalidSession()));
            }
        }
        else if (openTimer)
        {
            SessionManager::getRef().createTimer(20, std::bind(&CStressClientHandler::SendFunc, this, session, !session->isInvalidSession()));
        }
        


    };

};


/*
* server handler
* echo data when receive client message.
*/
class CStressServerHandler
{
public:
    CStressServerHandler()
    {
        MessageDispatcher::getRef().registerSessionMessage(ID_EchoPack,
            std::bind(&CStressServerHandler::msg_RequestSequence_fun, this, _1, _2));
    }

    void msg_RequestSequence_fun (TcpSessionPtr session, ReadStream & rs)
    {
        if (g_hightBenchmark)
        {
            session->send(rs.getStream(), rs.getStreamLen());
        }
        else
        {
            EchoPack pack;
            rs >> pack;
            WriteStream ws(ID_EchoPack);
            ws << pack;
            session->send(ws.getStream(), ws.getStreamLen());
        }
    };
};


void sigFun(int sig)
{
    SessionManager::getRef().stopAccept();
    SessionManager::getRef().stopClients();
}

void onAllClientsStoped()
{
    LOGA("onAllClientsStoped");
    SessionManager::getRef().stopServers();
}

void onAllServersStoped()
{
    LOGA("onAllServersStoped");
    SessionManager::getRef().stop();
}

int main(int argc, char* argv[])
{

#ifndef _WIN32
    //! ignore some signal
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
    signal(SIGINT, sigFun);
    if (argc == 2 && 
        (strcmp(argv[1], "--help") == 0 
        || strcmp(argv[1], "/?") == 0))
    {
        std::cout << "please input like example:" << std::endl;
        std::cout << "tcpTest remoteIP remotePort startType maxClient sendType interval" << std::endl;
        std::cout << "./tcpTest 0.0.0.0 8081 0" << std::endl;
        std::cout << "startType: 0 server, 1 client" << std::endl;
        std::cout << "maxClient: limit max" << std::endl;
        std::cout << "sendType: 0 echo send, 1 direct send" << std::endl;
        std::cout << "interval: send once interval" << std::endl;
        std::cout << "high benchmark: 0 close, 1 open" << std::endl;
        return 0;
    }
    if (argc > 1)
        g_remoteIP = argv[1];
    if (argc > 2)
        g_remotePort = atoi(argv[2]);
    if (argc > 3)
        g_startType = atoi(argv[3]);
        if (g_startType != 0) g_maxClient = 1;
    if (argc > 4)
        g_maxClient = atoi(argv[4]);
    if (argc > 5)
        g_sendType = atoi(argv[5]);
    if (argc > 6)
        g_intervalMs = atoi(argv[6]);
    if (argc > 7)
        g_hightBenchmark = atoi(argv[7]);

    if (g_startType == 0)
        ILog4zManager::getPtr()->config("server.cfg");
    else
        ILog4zManager::getPtr()->config("client.cfg");
    ILog4zManager::getPtr()->start();

    LOGI("g_remoteIP=" << g_remoteIP << ", g_remotePort=" << g_remotePort << ", g_startType=" << g_startType
        << ", g_maxClient=" << g_maxClient << ", g_sendType=" << g_sendType << ", g_intervalMs=" << g_intervalMs << ", hightBenchmark=" << g_hightBenchmark);


    ILog4zManager::getPtr()->setLoggerLevel(LOG4Z_MAIN_LOGGER_ID, LOG_LEVEL_INFO);


    SessionManager::getRef().setStopClientsHandler(onAllClientsStoped);
    SessionManager::getRef().setStopServersHandler(onAllServersStoped);
    SessionManager::getRef().start();


    SessionManager::getRef().createTimer(5000, MonitorFunc);


    //build client and server
    if (g_startType) //client
    {
        CStressClientHandler client;
        //add connector.
        for (int i = 0; i < g_maxClient; ++i)
        {
            SessionID cID = SessionManager::getRef().addConnecter(g_remoteIP, g_remotePort);
            SessionManager::getRef().getConnecterOptions(cID)._reconnects = 15;
            SessionManager::getRef().getConnecterOptions(cID)._connectPulseInterval = 100000;
            SessionManager::getRef().getConnecterOptions(cID)._onSessionLinked = std::bind(&CStressClientHandler::onConnected, &client, _1);
            SessionManager::getRef().openConnecter(cID);
        }
        //running
        SessionManager::getRef().run();
    }
    else
    {
        CStressServerHandler server;
        AccepterID aID = SessionManager::getRef().addAccepter("0.0.0.0", g_remotePort);
        SessionManager::getRef().openAccepter(aID);
        //running
        SessionManager::getRef().run();
    }



    return 0;
}


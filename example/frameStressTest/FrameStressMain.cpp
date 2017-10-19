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
 * Copyright (C) 2010-2017 YaweiZhang <yawei.zhang@foxmail.com>.
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
unsigned short g_startClient = 0;  //0 start with server module, 1 start with client module. 
unsigned short g_concClient = 1; //if start with server it is client limite count. else other it is the start client count.
unsigned short g_concExtraSend = 0; //concurrent ping-pong test count(flooding test)
unsigned int   g_intervalSend = 0; // if start client with flood test, it's flooding interval by millionsecond.
unsigned int   g_hightBenchmarkLevel = 0; //0 serialize full data, 1 serialize simple data,  2 serialize ellipsis  




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
        << ", socket created = " << zsummer::network::g_appEnvironment.getCreatedSocketCount()
        << ", socket destroyed = " << zsummer::network::g_appEnvironment.getClosedSocketCount()
        << ", session created = " << SessionManager::getRef()._statInfo[STAT_SESSION_CREATED]
        << ", session destroyed = " << SessionManager::getRef()._statInfo[STAT_SESSION_DESTROYED]
        << ", session linked = " << SessionManager::getRef()._statInfo[STAT_SESSION_LINKED]
        << ", session closed = " << SessionManager::getRef()._statInfo[STAT_SESSION_CLOSED]
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
    }

    void onLinked(const TcpSessionPtr & session)
    {
        LOGI("onLinked. ConnectorID=" << session->getSessionID() << ", remoteIP=" << session->getRemoteIP() << ", remotePort=" << session->getRemotePort() );
        session->setUserParamInteger(0, 0);
        EchoPack ep;
        initEchoPack(ep, g_hightBenchmarkLevel == 0);
        WriteStream ws(EchoPack::getProtoID());
        ws << ep;
        session->send(ws.getStream(), ws.getStreamLen());
    };
    void onLost(const TcpSessionPtr & session)
    {
        LOGI("onLost. ConnectorID=" << session->getSessionID() << ", remoteIP=" << session->getRemoteIP() << ", remotePort=" << session->getRemotePort());
    }

    void onMessage(const TcpSessionPtr & session, ReadStream & rs)
    {
        if (g_intervalSend > 0)
        {
            SessionManager::getRef().createTimer(g_intervalSend, std::bind(&CStressClientHandler::delaySend, this, session));
            return;
        }

        if (g_hightBenchmarkLevel >= 2)
        {
            session->send(rs.getStream(), rs.getStreamLen());
            if (g_concExtraSend > 0) // concurrence rule via limit memory 
            {
                unsigned int limitQue = (600 * 1024 * 1024 / SESSION_BLOCK_SIZE) / g_concClient;
                const unsigned int limitQue2 = session->getOptions()._maxSendListCount;
                limitQue = limitQue > limitQue2 ? limitQue2 : limitQue;
                limitQue = limitQue > g_concExtraSend ? g_concExtraSend : limitQue;
                auto up = session->getUserParamInteger(0);
                if (up < limitQue ) // 0 == 0 
                {
                    session->setUserParamInteger(0, up + 1);
                    session->send(rs.getStream(), rs.getStreamLen());
                }
            }
        }
        else
        {
            EchoPack pack;
            rs >> pack;
            WriteStream ws(EchoPack::getProtoID());
            ws << pack;
            session->send(ws.getStream(), ws.getStreamLen());

            if (g_concExtraSend > 0)
            {
                unsigned int limitQue = (600 * 1024 * 1024 / SESSION_BLOCK_SIZE) / g_concClient;
                const unsigned int limitQue2 = session->getOptions()._maxSendListCount;
                limitQue = limitQue > limitQue2 ? limitQue2 : limitQue;
                limitQue = limitQue > g_concExtraSend ? g_concExtraSend : limitQue;
                auto up = session->getUserParamInteger(0);
                if (up < limitQue) // 0 == 0 
                {
                    session->setUserParamInteger(0, up + 1);
                    session->send(ws.getStream(), ws.getStreamLen());
                }
            }
        }
    }

    void initEchoPack(EchoPack & pack, bool isFullData)
    {
        IntegerData idata;
        idata._char = 'a';
        idata._uchar = 100;
        idata._short = 200;
        idata._ushort = 300;
        idata._int = 400;
        idata._uint = 500;
        idata._i64 = 600;
        idata._ui64 = 700;

        pack._iarray.clear();
        pack._iarray.push_back(idata);
        pack._iarray.push_back(idata);


        FloatData fdata;
        fdata._float = (float)123123.111111;
        fdata._double = 12312312.88888;

        pack._farray.clear();
        pack._farray.push_back(fdata);
        pack._farray.push_back(fdata);

        if (isFullData)
        {
            StringData sdata;
            sdata._string = "abcdefg";
            pack._sarray.push_back(sdata);
            pack._sarray.push_back(sdata);


            pack._imap.insert(std::make_pair("123", idata));
            pack._imap.insert(std::make_pair("223", idata));
            pack._fmap.insert(std::make_pair("323", fdata));
            pack._fmap.insert(std::make_pair("423", fdata));
            pack._smap.insert(std::make_pair("523", sdata));
            pack._smap.insert(std::make_pair("623", sdata));
        }
    }
    void delaySend(const TcpSessionPtr & session)
    {
        thread_local static char buff[SESSION_BLOCK_SIZE];
        thread_local static zsummer::proto4z::Integer buffSize = 0;
        if (buffSize == 0)
        {
            EchoPack ep;
            initEchoPack(ep, g_hightBenchmarkLevel == 0);
            WriteStream ws(EchoPack::getProtoID());
            ws << ep;
            buffSize = ws.getStreamLen();
            memcpy(buff, ws.getStream(), ws.getStreamLen());
        }
        if (session->isInvalidSession())
        {
            LOGE("InvalidSession termination send data");
            return;
        }

        if (g_hightBenchmarkLevel >= 2)
        {
            session->send(buff, buffSize);
        }
        else
        {
            thread_local static EchoPack ep; 
            if (ep._farray.empty())
            {
                initEchoPack(ep, g_hightBenchmarkLevel == 0);
            }
            WriteStream ws(EchoPack::getProtoID());
            ws << ep;
            session->send(ws.getStream(), ws.getStreamLen());
        }
    }
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
    }
    void onLinked(const TcpSessionPtr & session)
    {
        LOGI("onLinked. sessionID=" << session->getSessionID() << ", remoteIP=" << session->getRemoteIP() << ", remotePort=" << session->getRemotePort());
    };
    void onLost(const TcpSessionPtr & session)
    {
        LOGI("onLost. sessionID=" << session->getSessionID() << ", remoteIP=" << session->getRemoteIP() << ", remotePort=" << session->getRemotePort());
    }
    void onMessage(const TcpSessionPtr & session, ReadStream & rs)
    {
        //LOGD("onMessage");
        if (g_hightBenchmarkLevel >= 2)
        {
            session->send(rs.getStream(), rs.getStreamLen());
        }
        else
        {
            EchoPack pack;
            rs >> pack;
            WriteStream ws(EchoPack::getProtoID());
            ws << pack;
            session->send(ws.getStream(), ws.getStreamLen());
        }
    };
};


void sigFun(int sig)
{
    SessionManager::getRef().stop(); 
    SessionManager::getRef().post(std::bind([](){
        SessionManager::getRef().stopAccept();
        SessionManager::getRef().kickClientSession();
        SessionManager::getRef().kickConnect(); }));
}

void testfunction(std::shared_ptr<int> s)
{
    (*s)++;
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
        std::cout << "frameStressTest remoteIP remotePort isClient concurrentClient concurrentExtraSend sendInterval highbenchmarkLevel" << std::endl;
        std::cout << "./tcpTest 0.0.0.0 8081 0" << std::endl;
        std::cout << "isClient: 0 server, 1 client" << std::endl;
        std::cout << "concurrentClient: default 1" << std::endl;
        std::cout << "concurrentSend: default 0" << std::endl;
        std::cout << "interval: send interval" << std::endl;
        std::cout << "highbenchmarkLevel: 0 full , 1 simple, 2 direct" << std::endl;
        return 0;
    }
    if (argc > 1)
        g_remoteIP = argv[1];
    if (argc > 2)
        g_remotePort = atoi(argv[2]);
    if (argc > 3)
        g_startClient = atoi(argv[3]);
        if (g_startClient != 0) g_concClient = 1;
    if (argc > 4)
        g_concClient = atoi(argv[4]);
    if (argc > 5)
        g_concExtraSend = atoi(argv[5]);
    if (argc > 6)
        g_intervalSend = atoi(argv[6]);
    if (argc > 7)
        g_hightBenchmarkLevel = atoi(argv[7]);

    if (g_startClient == 0)
        ILog4zManager::getPtr()->config("server.cfg");
    else
        ILog4zManager::getPtr()->config("client.cfg");
    ILog4zManager::getPtr()->start();

    LOGI("g_remoteIP=" << g_remoteIP <<", orgin=" << getHostByName(g_remoteIP) << ", g_remotePort=" << g_remotePort << ", g_startClient=" << g_startClient
        << ", g_concClient=" << g_concClient << ", g_concExtraSend=" << g_concExtraSend << ", g_intervalSend=" << g_intervalSend << ", hightBenchmark=" << g_hightBenchmarkLevel);




    SessionManager::getRef().start();

    std::shared_ptr<int> sp = std::make_shared<int>();
    auto getTick = []() {return (unsigned long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); };
    auto now = getTick();
    for (size_t i = 0; i < 100*10000; i++)
    {
        std::shared_ptr<int> sp2 = sp;
        (*sp2)++;
    }
    LOGD("shared_ptr " << *sp << "  100*10000=" << getTick() - now);
    now = getTick();
    auto x = std::bind(testfunction, sp);
    for (size_t i = 0; i < 100 * 10000; i++)
    {
        auto y = std::bind(testfunction, sp);
        y();
    }
    LOGD("function  " << *sp << " 100*10000=" << getTick() - now);



    SessionManager::getRef().createTimer(5000, MonitorFunc);

    //build client and server
    if (g_startClient) //client
    {
        CStressClientHandler client;
        //add connector.
        for (int i = 0; i < g_concClient; ++i)
        {
            SessionID cID = SessionManager::getRef().addConnecter(g_remoteIP, g_remotePort);
            SessionManager::getRef().getConnecterOptions(cID)._reconnects = 0;
            SessionManager::getRef().getConnecterOptions(cID)._connectPulseInterval = 4000;
            SessionManager::getRef().getConnecterOptions(cID)._maxSendListCount = 20000;
            if (g_concExtraSend == 0) SessionManager::getRef().getConnecterOptions(cID)._floodSendOptimize = false;
            
            SessionManager::getRef().getConnecterOptions(cID)._onSessionLinked = std::bind(&CStressClientHandler::onLinked, &client, _1);
            SessionManager::getRef().getConnecterOptions(cID)._onSessionClosed = std::bind(&CStressClientHandler::onLost, &client, _1);
            SessionManager::getRef().getConnecterOptions(cID)._onBlockDispatch = [&client](TcpSessionPtr   session, const char * begin, unsigned int len)
            {
                ReadStream rs(begin, len);
                client.onMessage(session, rs);
            };
            SessionManager::getRef().openConnecter(cID);
        }
        //running
        SessionManager::getRef().run();
    }
    else
    {
        CStressServerHandler server;
        AccepterID aID = SessionManager::getRef().addAccepter(g_remoteIP, g_remotePort);
        SessionManager::getRef().getAccepterOptions(aID)._sessionOptions._maxSendListCount = 40000;
        if (g_concExtraSend == 0) SessionManager::getRef().getAccepterOptions(aID)._sessionOptions._floodSendOptimize = false;
        SessionManager::getRef().getAccepterOptions(aID)._sessionOptions._onSessionLinked = std::bind(&CStressServerHandler::onLinked, &server, _1);
        SessionManager::getRef().getAccepterOptions(aID)._sessionOptions._onSessionClosed = std::bind(&CStressServerHandler::onLost, &server, _1);
        SessionManager::getRef().getAccepterOptions(aID)._sessionOptions._onBlockDispatch = [&server](TcpSessionPtr   session, const char * begin, unsigned int len)
        {
            ReadStream rs(begin, len);
            server.onMessage(session, rs);
        };
        SessionManager::getRef().openAccepter(aID);
        //running
        SessionManager::getRef().run();
    }



    return 0;
}


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
 * Copyright (C) 2010-2016 YaweiZhang <yawei.zhang@foxmail.com>.
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


//! frame simple test
//! create connector,  build session, send message, recv message.
//! create accpter, accept session, recv message, send message.



#include <zsummerX/zsummerX.h>
using namespace zsummer::proto4z;
using namespace zsummer::network;


constexpr static int kTunnelListen = 0;
constexpr static int kTunnelConnect = 1;

std::string g_input_ip = "";
unsigned short g_input_port = 5426;
int g_input_mod = kTunnelListen;


std::string g_output_ip = "";
unsigned short g_output_port = 5427;
int g_output_mod = kTunnelListen;




int initEnv(int argc, char* argv[]);
void startServer();
void startClient();

void sigFun(int sig)
{
    SessionManager::getRef().createTimer(1000, std::bind([]() {
        SessionManager::getRef().stopAccept();
        SessionManager::getRef().kickClientSession();
        SessionManager::getRef().kickConnect(); 
        SessionManager::getRef().stop();
    }), false);
}


// main
//////////////////////////////////////////////////////////////////////////
void TestSessionUserParam();
int main(int argc, char* argv[])
{
#ifndef _WIN32
    //! ignore some signal
    signal(SIGHUP, SIG_IGN);
    signal(SIGALRM, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGXCPU, SIG_IGN);
    signal(SIGXFSZ, SIG_IGN);
    signal(SIGPROF, SIG_IGN);
    signal(SIGVTALRM, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
#endif
    signal(SIGINT, sigFun);
    int ret = initEnv(argc, argv);
    if (ret != 0)
    {
        return ret;
    }
    LogInfo() << "init Env finish";


    if (!SessionManager::getRef().start())
    {
        LogError() << "start net manager error";
        return -1;
    }

    constexpr static size_t kIndexTargetSession = 0;
    constexpr static int kIndexTmpBuffer = 1;
    constexpr static int kTmpBuffer = 1024 * 1024 * 50;

    auto OnRawPacketCheckResult = [](const char* begin, unsigned int len, unsigned int bound, unsigned int blockLimit)
        {
            return RawPacketCheckResult(BCT_SUCCESS, len);
        };

    auto OnSessionBlock = [](TcpSessionPtr session, const char* begin, unsigned int len)
        {
            unsigned long long target_session_id = session->getUserParamInteger(kIndexTargetSession);
            if (!session->isUserParamInited(kIndexTargetSession))
            {
                LogError() << "has error";
                return;
            }
            TcpSessionPtr tgr_session = SessionManager::getRef().getTcpSession(target_session_id);
            if (!tgr_session)
            {
                LogError() << "has error";
                return;
            }
            tgr_session->send(begin, len);
        };

    auto OnSessionLinked = [](const TcpSessionPtr& session)
        {

        };

    auto OnSessionClosed = [](const TcpSessionPtr& session)
        {

        };



    if (g_input_mod  == 0)
    {
        AccepterID aID = SessionManager::getRef().addAccepter(g_input_ip, g_input_port);
        SessionManager::getRef().getAccepterOptions(aID)._sessionOptions._onRawPacketCheck = OnRawPacketCheckResult;
        SessionManager::getRef().getAccepterOptions(aID)._sessionOptions._onRawPacketProc = OnSessionBlock;
        SessionManager::getRef().getAccepterOptions(aID)._sessionOptions._onSessionLinked = OnSessionLinked; 
        SessionManager::getRef().getAccepterOptions(aID)._sessionOptions._onSessionClosed = OnSessionClosed;
        SessionManager::getRef().getAccepterOptions(aID)._setReuse = true;
        SessionManager::getRef().openAccepter(aID);
    }
    if (g_output_mod == 0)
    {
        AccepterID aID = SessionManager::getRef().addAccepter(g_output_ip, g_output_port);
        SessionManager::getRef().getAccepterOptions(aID)._sessionOptions._onRawPacketCheck = OnRawPacketCheckResult;
        SessionManager::getRef().getAccepterOptions(aID)._sessionOptions._onRawPacketProc = OnSessionBlock;
        SessionManager::getRef().getAccepterOptions(aID)._sessionOptions._onSessionLinked = OnSessionLinked;
        SessionManager::getRef().getAccepterOptions(aID)._sessionOptions._onSessionClosed = OnSessionClosed;
        SessionManager::getRef().getAccepterOptions(aID)._setReuse = true;
        SessionManager::getRef().openAccepter(aID);
    }

    if (g_input_mod == 1)
    {
        SessionID cID = SessionManager::getRef().addConnecter(g_input_ip, g_input_port);
        SessionManager::getRef().getConnecterOptions(cID)._onReconnectEnd = OnSessionClosed;
        SessionManager::getRef().getConnecterOptions(cID)._onSessionLinked = OnSessionLinked;
        SessionManager::getRef().getConnecterOptions(cID)._onRawPacketCheck = OnRawPacketCheckResult;
        SessionManager::getRef().getConnecterOptions(cID)._onRawPacketProc = OnSessionBlock;
    }
    if (g_output_mod == 1)
    {
        SessionID cID = SessionManager::getRef().addConnecter(g_output_ip, g_output_port);
        SessionManager::getRef().getConnecterOptions(cID)._onReconnectEnd = OnSessionClosed;
        SessionManager::getRef().getConnecterOptions(cID)._onSessionLinked = OnSessionLinked;
        SessionManager::getRef().getConnecterOptions(cID)._onRawPacketCheck = OnRawPacketCheckResult;
        SessionManager::getRef().getConnecterOptions(cID)._onRawPacketProc = OnSessionBlock;
    }



    SessionManager::getRef().run();
    return 0;
}





//impl
//////////////////////////////////////////////////////////////////////////


int initEnv(int argc, char* argv[])
{
    if (argc <= 6)
    {
        //if (argc == 2 &&
        //    (strcmp(argv[1], "--help") == 0
        //        || strcmp(argv[1], "/?") == 0))
        {
            std::cout << "please input like example:" << std::endl;
            std::cout << "./tunnel 0 0.0.0.0 5425 1 127.0.0.1 5425" << std::endl;
            std::cout << "listen 5425 and trans data to after " << std::endl;
            std::cout << "0 listen, 1 connect;  host port" << std::endl;
            return 0;
        }
    }

    g_input_mod = atoi(argv[1]);
    g_input_ip = argv[2];
    g_input_port = atoi(argv[3]);

    g_output_mod = atoi(argv[1]);
    g_output_ip = argv[2];
    g_output_port = atoi(argv[3]);

    int ret = FNLog::FastStartDefaultLogger();
    if (ret != 0)
    {
        return ret;
    }
    LogInfo() << argv;
    return 0;
}


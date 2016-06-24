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


//! zsummerX  http proto test



#include <zsummerX/zsummerX.h>
using namespace zsummer::log4z;
using namespace zsummer::network;

std::string g_remoteIP = "0.0.0.0";
unsigned short g_remotePort = 8081;
unsigned short g_startIsConnector = 0;  //0 listen, 1 connect, 3 connect cnblogs


int main(int argc, char* argv[])
{
    if (argc == 2 && 
        (strcmp(argv[1], "--help") == 0 
        || strcmp(argv[1], "/?") == 0))
    {
        std::cout << "please input like example:" << std::endl;
        std::cout << "tcpTest remoteIP remotePort startType maxClient sendType interval" << std::endl;
        std::cout << "./tcpTest 0.0.0.0 8081 0" << std::endl;
        std::cout << "startType: 0 server, 1 client" << std::endl;
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
        g_startIsConnector = atoi(argv[3]);
    }

    
    if (!g_startIsConnector)
    {
        //! start log4z 
        ILog4zManager::getPtr()->config("server.cfg");
        ILog4zManager::getPtr()->start();
    }
    else
    {
        //! start log4z 
        ILog4zManager::getPtr()->config("client.cfg");
        ILog4zManager::getPtr()->start();
    }
    LOGI("g_remoteIP=" << g_remoteIP << ", g_remotePort=" << g_remotePort << ", g_startIsConnector=" << g_startIsConnector );


    //! step 1. start frame manager.
    SessionManager::getRef().start();


    if (g_startIsConnector == 1) //client
    {
        //callback when connect success.
        auto OnSessionLinked = [](TcpSessionPtr session)
        {
//            std::string jsonString = R"---(data={%22uid%22:10001,%22session%22:%220192023a7bbd73250516f069df18b500%22})---";
            std::string jsonString = R"---(data=)---";
            jsonString += zsummer::proto4z::urlEncode(std::string(R"---({"uid":10001,"session":"0192023a7bbd73250516f069df18b500"})---"));

            LOGI("send to ConnectorID=" << session->getSessionID());
            zsummer::proto4z::WriteHTTP wh;
//             wh.addHead("Accept", " text/html, application/xhtml+xml, */*");
//             wh.addHead("Accept-Language", "zh-CN");
//             wh.addHead("User-Agent", "Mozilla/5.0 ");
            wh.addHead("Content-Type", "application/x-www-form-urlencoded");
//            wh.addHead("Accept-Encoding", "utf8");
            wh.addHead("Host", g_remoteIP);
//            wh.addHead("DNT", "1");
//            wh.addHead("Connection", "Keep-Alive");
            wh.post("/user/oauth", jsonString);
            session->send(wh.getStream(), wh.getStreamLen());
        };

        //callback when receive http data
        auto OnHTTPBlock = [](TcpSessionPtr session, const std::string & method, const std::string &methodLine, const std::map<std::string,std::string> &head, const std::string & body)
        {
            if (methodLine != "200")
            {
                LOGI("response false. commond=" << method << ", commondvalue=" << methodLine);
                session->close();
                return ;
            }
            LOGI("response success. commond=" << method << ", commondvalue=" << methodLine << ", content=" << body);
            session->close();            //step 3. stop
            //SessionManager::getRef().stop();
            return ;
        };


        SessionID cID = SessionManager::getRef().addConnecter(g_remoteIP, g_remotePort);
        SessionManager::getRef().getConnecterOptions(cID)._protoType = PT_HTTP;
        SessionManager::getRef().getConnecterOptions(cID)._onHTTPBlockDispatch = OnHTTPBlock;
        SessionManager::getRef().getConnecterOptions(cID)._onSessionLinked = OnSessionLinked;
        SessionManager::getRef().openConnecter(cID);


        //! step 2 running
        SessionManager::getRef().run();
    }
    else if( g_startIsConnector == 0)
    {

        //result when receive http data
        auto OnHTTPBlock = [](TcpSessionPtr session, const std::string & method, const std::string &methodLine, 
            const std::map<std::string, std::string> &head, const std::string & body)
        {
            LOGI("recv request. commond=" << method << ", commondvalue=" << methodLine);
            zsummer::proto4z::WriteHTTP wh;
            wh.addHead("Accept", " text/html, application/xhtml+xml, */*");
            wh.addHead("Accept-Language", "zh-CN");
            wh.addHead("User-Agent", "Mozilla/5.0 ");
            wh.addHead("Accept-Encoding", "utf8");
            wh.addHead("Host", "www.baidu.com");
            wh.addHead("DNT", "1");
            wh.addHead("Connection", "Keep-Alive");
            wh.response("200", "What's your name ?");
            session->send(wh.getStream(), wh.getStreamLen());
            SessionManager::getRef().createTimer(2000, std::bind(&SessionManager::kickSession, SessionManager::getPtr(), session->getSessionID()));
            //step 3. stop server.
        //    SessionManager::getRef().createTimer(1000,std::bind(&SessionManager::stop, SessionManager::getPtr()));
            return ;
        };


        AccepterID aID = SessionManager::getRef().addAccepter(g_remoteIP, g_remotePort);
        SessionManager::getRef().getAccepterOptions(aID)._sessionOptions._protoType = PT_HTTP;
        SessionManager::getRef().getAccepterOptions(aID)._sessionOptions._onHTTPBlockDispatch = OnHTTPBlock;
        SessionManager::getRef().openAccepter(aID);
        //! step 2 running
        SessionManager::getRef().run();
    }
    else 
    {
        auto OnSessionLinked = [](TcpSessionPtr session)
        {
            LOGI("send to ConnectorID=" << session->getSessionID());
            zsummer::proto4z::WriteHTTP wh;
            wh.addHead("Host", "www.cnblogs.com");
            wh.get("/test.php");
            session->send(wh.getStream(), wh.getStreamLen());
        };

        //callback when receive http data
        auto OnHTTPBlock = [](TcpSessionPtr session, const std::string & method, const std::string &methodLine, const std::map<std::string, std::string> &head, const std::string & body)
        {
            if (methodLine != "200")
            {
                LOGI("response false. commond=" << method << ", commondvalue=" << methodLine);
                session->close();
                return;
            }
            LOGI("response success. commond=" << method << ", commondvalue=" << methodLine << ", content=" << body);
            //session->close();            //step 3. stop
                                         //SessionManager::getRef().stop();
            return;
        };

        SessionID cID = SessionManager::getRef().addConnecter("104.238.150.38", 80);
        SessionManager::getRef().getConnecterOptions(cID)._protoType = PT_HTTP;
        SessionManager::getRef().getConnecterOptions(cID)._onHTTPBlockDispatch = OnHTTPBlock;
        SessionManager::getRef().getConnecterOptions(cID)._onSessionLinked = OnSessionLinked;
        SessionManager::getRef().openConnecter(cID);


        //! step 2 running
        SessionManager::getRef().run();
    }

    return 0;
}


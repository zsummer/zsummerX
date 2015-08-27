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


#include <zsummerX/frame/session.h>
#include <zsummerX/frame/manager.h>
#include <zsummerX/frame/dispatch.h>
using namespace zsummer::network;
using namespace zsummer::proto4z;



SessionManager & SessionManager::getRef()
{
    static SessionManager _manager;
    return _manager;
}
SessionManager::SessionManager()
{
    memset(_statInfo, 0, sizeof(_statInfo));
    _summer = std::make_shared<EventLoop>();
}

bool SessionManager::start()
{
    if (!_summer->initialize())
    {
        return false;
    }
    _openTime = time(NULL);
    return true;
}

void SessionManager::stop()
{
    _running = false;
}

void SessionManager::stopAccept()
{
    _stopAccept = false;
}



void SessionManager::stopClients()
{
    _stopClients = 1;
}

void SessionManager::stopServers()
{
    _stopServers = 1;
}

void SessionManager::setStopClientsHandler(std::function<void()> handler)
{
    _funClientsStop = handler;
}
void SessionManager::setStopServersHandler(std::function<void()> handler)
{
    _funServerStop = handler;
}

bool SessionManager::run()
{
    while (_running || !_mapTcpSessionPtr.empty())
    {
        runOnce(false);
    }
    return false;
}

bool SessionManager::runOnce(bool isImmediately)
{
    if (_running || !_mapTcpSessionPtr.empty())
    {
        if (_stopClients == 1)
        {
            _stopClients = 2;
            int count = 0;
            for (auto kv : _mapTcpSessionPtr)
            {
                if (isSessionID(kv.first))
                {
                    kickSession(kv.first);
                    count++;
                }
            }
            LCW("SessionManager::kickAllClients() [" << count << "], session[" << _mapTcpSessionPtr.size() << "] success.");
            if (count == 0 && _funClientsStop)
            {
                auto temp = std::move(_funClientsStop);
                temp();
                _stopClients = 3;
            }
        }
        if (_stopServers == 1)
        {
            int count = 0;
            _stopServers = 2;
            for (auto &kv : _mapTcpSessionPtr)
            {
                if (isConnectID(kv.first))
                {
                    kv.second->getTraits()._reconnectMaxCount = 0;
                    kickSession(kv.first);
                    count++;
                }
                
            }

            LCW("SessionManager::kickAllConnect() [" << count << "], session[" << _mapTcpSessionPtr.size() << "] success.");
            if (count == 0 && _funServerStop)
            {
                auto temp = std::move(_funServerStop);
                temp();
                _stopServers = 3;
            }
        }
        _summer->runOnce(isImmediately);
        return true;
    }
    return false;
}


AccepterID SessionManager::addAccepter(const std::string & listenIP, unsigned short listenPort)
{
    _lastAcceptID = nextSessionID(_lastAcceptID);
    auto & extend = _mapAccepterExtend[_lastAcceptID];
    extend._aID = _lastAcceptID;
    extend._listenIP = listenIP;
    extend._listenPort = listenPort;
    extend._sessionTraits._pulseInterval = 5 * 1000;
    extend._sessionTraits._checkHTTPBlock = DefaultCheckHTTPBlock;
    extend._sessionTraits._dispatchHTTP = DefaultDispatchHTTPMessage;
    extend._sessionTraits._checkTcpBlock = DefaultCheckBlock;
    extend._sessionTraits._dispatchBlock = DefaultDispatchBlock;
    extend._sessionTraits._createBlock = DefaultCreateBlock;
    extend._sessionTraits._freeBlock = DefaultFreeBlock;
    return _lastAcceptID;
}

AccepterExtend & SessionManager::getAccepterExtend(AccepterID aID)
{
    if (aID == InvalidAccepterID)
    {
        throw std::runtime_error("AccepterID can not be InvalidAccepterID");
    }
    return _mapAccepterExtend[aID];
}

bool SessionManager::openAccepter(AccepterID aID)
{
    auto founder = _mapAccepterExtend.find(aID);
    if (founder == _mapAccepterExtend.end())
    {
        LCE("openAccepter error. not found the Accepter ID extend info. aID=" << aID);
        return false;
    }

    if (founder->second._accepter)
    {
        LCE("openAccepter error. already opened. extend info=" << founder->second);
        return false;
    }
    if (founder->second._listenIP.empty())
    {
        LCE("openAccepter error. no listen IP. extend info=" << founder->second);
        return false;
    }
    
    TcpAcceptPtr accepter = std::make_shared<TcpAccept>();
    if (!accepter->initialize(_summer))
    {
        LCE("openAccept error. extend info=" << founder->second);
        return false;
    }
    if (!accepter->openAccept(founder->second._listenIP, founder->second._listenPort))
    {
        LCE("openAccept error. extend info=" << founder->second);
        return false;
    }
    if (!accepter->doAccept(std::make_shared<TcpSocket>(), 
        std::bind(&SessionManager::onAcceptNewClient, this, std::placeholders::_1, std::placeholders::_2, accepter, founder->second._aID)))
    {
        LCE("openAccept error. extend info=" << founder->second);
        return false;
    }
    founder->second._accepter = accepter;
    return true;
}


AccepterID SessionManager::getAccepterID(SessionID sID)
{
    if (!isSessionID(sID))
    {
        return InvalidAccepterID;
    }
    auto founder = _mapTcpSessionPtr.find(sID);
    if (founder == _mapTcpSessionPtr.end())
    {
        return InvalidAccepterID;
    }
    return founder->second->getAcceptID();
}


void SessionManager::onAcceptNewClient(zsummer::network::NetErrorCode ec, const TcpSocketPtr& s, const TcpAcceptPtr &accepter, AccepterID aID)
{
    if (!_running ||  _stopAccept)
    {
        LCI("shutdown accepter. aID=" << aID);
        return;
    }
    auto founder = _mapAccepterExtend.find(aID);
    if (founder == _mapAccepterExtend.end())
    {
        LCE("Unknown AccepterID aID=" << aID);
        return;
    }
    if (ec)
    {
        LCE("doAccept Result Error. ec=" << ec << ", extend=" << founder->second);
        
        auto &&handler = std::bind(&SessionManager::onAcceptNewClient, this, std::placeholders::_1, std::placeholders::_2, accepter, aID);
        auto timer = [accepter, handler]()
        {
            accepter->doAccept(std::make_shared<TcpSocket>(), std::move(handler));
        };
        createTimer(5000, std::move(timer));
        return;
    }

    std::string remoteIP;
    unsigned short remotePort = 0;
    s->getPeerInfo(remoteIP, remotePort);
    
    //! check white list
    //! ---------------------
    if (!founder->second._whitelistIP.empty())
    {
        bool checkSucess = false;
        for (auto white : founder->second._whitelistIP)
        {
            if (remoteIP.size() >= white.size())
            {
                if (remoteIP.compare(0,white.size(), white) == 0)
                {
                    checkSucess = true;
                    break;
                }
            }
        }

        if (!checkSucess)
        {
            LCW("Accept New Client Check Whitelist Failed remoteAdress=" << remoteIP << ":" << remotePort
                << ", extend=" << founder->second);
            accepter->doAccept(std::make_shared<TcpSocket>(), std::bind(&SessionManager::onAcceptNewClient, this, std::placeholders::_1, std::placeholders::_2, accepter, aID));
            return;
        }
        else
        {
            LCI("Accept New Client Check Whitelist Success remoteAdress=" << remoteIP << ":" << remotePort
                << ", extend=" << founder->second);
        }
    }
    
    //! check Max Sessions

    if (founder->second._currentLinked >= founder->second._maxSessions)
    {
        LCW("Accept New Client. Too Many Sessions And The new socket will closed. extend=" << founder->second << "] total TcpSocket object count =[create:" << g_appEnvironment.getCreatedSocketCount() << ", close:" << g_appEnvironment.getClosedSocketCount() << "], total TcpSession object count =[create:"
            << g_appEnvironment.getCreatedSessionCount() << ", close:" << g_appEnvironment.getClosedSessionCount()
            << "]");
    }
    else
    {
        LCD("Accept New Client. Accept new Sessions. The new socket  remoteAddress=" << remoteIP << ":" << remotePort 
            << ", Aready linked sessions = " << founder->second._currentLinked << ", extend=" << founder->second);
        founder->second._currentLinked++;
        founder->second._totalAcceptCount++;

        _lastSessionID = nextSessionID(_lastSessionID);
        s->initialize(_summer);

        TcpSessionPtr session = std::make_shared<TcpSession>();
        session->getTraits() = founder->second._sessionTraits;
        session->setEventLoopPtr(_summer);
        if (session->bindTcpSocketPrt(s, aID, _lastSessionID))
        {
            _mapTcpSessionPtr[_lastSessionID] = session;
            //MessageDispatcher::getRef().dispatchOnSessionEstablished(session);
        }
    }
    
    //! accept next socket.
    accepter->doAccept(std::make_shared<TcpSocket>(), std::bind(&SessionManager::onAcceptNewClient, this, std::placeholders::_1, std::placeholders::_2, accepter, aID));
}



std::string SessionManager::getRemoteIP(SessionID sID)
{
    auto founder = _mapTcpSessionPtr.find(sID);
    if (founder == _mapTcpSessionPtr.end())
    {
        return "closed session";
    }
    else
    {
        return founder->second->getRemoteIP();
    }
    return "";
}
unsigned short SessionManager::getRemotePort(SessionID sID)
{
    auto founder = _mapTcpSessionPtr.find(sID);
    if (founder == _mapTcpSessionPtr.end())
    {
        return -1;
    }
    else
    {
        return founder->second->getRemotePort();
    }
    return 0;
}

void SessionManager::kickSession(SessionID sID)
{
    auto iter = _mapTcpSessionPtr.find(sID);
    if (iter == _mapTcpSessionPtr.end())
    {
        LCW("kickSession NOT FOUND SessionID. SessionID=" << sID);
        return;
    }
    iter->second->close();
}

void SessionManager::onSessionClose(TcpSessionPtr session)
{
    _mapTcpSessionPtr.erase(session->getSessionID());
    if (session->getAcceptID() != InvalidAccepterID)
    {
        _mapAccepterExtend[session->getAcceptID()]._currentLinked--;
        _mapAccepterExtend[session->getAcceptID()]._totalAcceptCount++;
    }

    //MessageDispatcher::getRef().dispatchOnSessionDisconnect(session);

    if (_stopClients == 2 || _stopServers == 2)
    {
        int clients = 0;
        int servers = 0;
        for (auto & kv : _mapTcpSessionPtr)
        {
            if (isSessionID(kv.first))
            {
                clients++;
            }
            else if (isConnectID(kv.first))
            {
                servers++;
            }
            else
            {
                LCE("error. invalid session id in _mapTcpSessionPtr");
            }
        }
        if (_stopClients == 2)
        {
            _stopClients = 3;
            if (_funClientsStop)
            {
                auto temp = std::move(_funClientsStop);
                temp();
            }
        }
        if (_stopServers == 2)
        {
            _stopServers = 3;
            if (_funServerStop)
            {
                auto temp = std::move(_funServerStop);
                temp();
            }
        }
    }
    
}

SessionID SessionManager::addConnecter(const std::string & remoteIP, unsigned short remotePort)
{
    _lastConnectID = nextConnectID(_lastConnectID);
    TcpSessionPtr & session = _mapTcpSessionPtr[_lastConnectID];
    session = std::make_shared<TcpSession>();
    
   session->getTraits()._pulseInterval = 20 * 1000;
   session->getTraits()._checkHTTPBlock = DefaultCheckHTTPBlock;
   session->getTraits()._dispatchHTTP = DefaultDispatchHTTPMessage;
   session->getTraits()._checkTcpBlock = DefaultCheckBlock;
   session->getTraits()._dispatchBlock = DefaultDispatchBlock;
   session->getTraits()._createBlock = DefaultCreateBlock;
   session->getTraits()._freeBlock = DefaultFreeBlock;

    session->setEventLoopPtr(_summer);
    session->setSessionID(_lastConnectID);
    session->setRemoteIP(remoteIP);
    session->setRemotePort(remotePort);
    return _lastConnectID;
}

SessionTraits & SessionManager::getConnecterExtend(SessionID cID)
{
    auto founder = _mapTcpSessionPtr.find(cID);
    if (founder == _mapTcpSessionPtr.end())
    {
        throw std::runtime_error("getConnecterExtend error.");
    }
    return founder->second->getTraits();
}

bool SessionManager::openConnecter(SessionID cID)
{
    auto founder = _mapTcpSessionPtr.find(cID);
    if (founder == _mapTcpSessionPtr.end())
    {
        LCE("openConnecter error");
        return false;
    }
    founder->second->connect();
    return true;
}

TcpSessionPtr SessionManager::getTcpSession(SessionID sID)
{
    auto founder = _mapTcpSessionPtr.find(sID);
    if (founder != _mapTcpSessionPtr.end())
    {
        return founder->second;
    }
    return nullptr;
}



void SessionManager::sendSessionData(SessionID sID, const char * orgData, unsigned int orgDataLen)
{
    auto iter = _mapTcpSessionPtr.find(sID);
    if (iter == _mapTcpSessionPtr.end())
    {
        LCW("sendSessionData NOT FOUND SessionID.  SessionID=" << sID);
        return;
    }

    iter->second->send(orgData, orgDataLen);
    //trace log
    {
        LCT("sendSessionData Len=" << orgDataLen << ",binarydata=" << zsummer::log4z::Log4zBinary(orgData, orgDataLen >= 10 ? 10 : orgDataLen));
    }
}




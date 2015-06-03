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
                _funClientsStop();
                _funClientsStop = nullptr;
                _stopClients = 3;
            }
        }
        if (_stopServers == 1)
        {
            _stopServers = 2;
            for (auto &kv : _mapConnectorConfig)
            {
                kv.second.first._reconnectMaxCount = 0;
            }
            int count = 0;
            for (auto kv : _mapTcpSessionPtr)
            {
                if (isConnectID(kv.first))
                {
                    kickSession(kv.first);
                    count++;
                }
            }
            LCW("SessionManager::kickAllConnect() [" << count << "], session[" << _mapTcpSessionPtr.size() << "] success.");
            if (count == 0 && _funServerStop)
            {
                _funServerStop();
                _funServerStop = nullptr;
                _stopServers = 3;
            }
        }
        _summer->runOnce(isImmediately);
        return true;
    }
    return false;
}



AccepterID SessionManager::addAcceptor(const ListenConfig &traits)
{
    _lastAcceptID++;
    auto & pairConfig = _mapAccepterConfig[_lastAcceptID];
    pairConfig.first = traits;
    pairConfig.second._aID = _lastAcceptID;

    TcpAcceptPtr accepter = std::make_shared<TcpAccept>();
    accepter->initialize(_summer);
    if (!accepter->openAccept(traits._listenIP.c_str(), traits._listenPort))
    {
        LCE("addAcceptor openAccept Failed. traits=" << traits);
        return InvalidAccepterID;
    }
    _mapAccepterPtr[_lastAcceptID] = accepter;
    accepter->doAccept(std::make_shared<TcpSocket>(), std::bind(&SessionManager::onAcceptNewClient, this, std::placeholders::_1, std::placeholders::_2, accepter, pairConfig.second._aID));
    return _lastAcceptID;
}

bool SessionManager::getAcceptorConfig(AccepterID aID, std::pair<ListenConfig, ListenInfo> & config)
{
    auto founder = _mapAccepterConfig.find(aID);
    if (founder != _mapAccepterConfig.end())
    {
        config = founder->second;
        return true;
    }
    return false;
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
    auto iter = _mapAccepterConfig.find(aID);
    if (iter == _mapAccepterConfig.end())
    {
        LCE("Unknown AccepterID aID=" << aID);
        return;
    }
    if (ec)
    {
        LCE("doAccept Result Error. ec=" << ec << ", traits=" << iter->second.first);
        
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
    if (!iter->second.first._whitelistIP.empty())
    {
        bool checkSucess = false;
        for (auto white : iter->second.first._whitelistIP)
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
                << ", trais=" << iter->second.first);
            accepter->doAccept(std::make_shared<TcpSocket>(), std::bind(&SessionManager::onAcceptNewClient, this, std::placeholders::_1, std::placeholders::_2, accepter, aID));
            return;
        }
        else
        {
            LCI("Accept New Client Check Whitelist Success remoteAdress=" << remoteIP << ":" << remotePort
                << ", trais=" << iter->second.first);
        }
    }
    
    //! check Max Sessions

    if (iter->second.second._currentLinked >= iter->second.first._maxSessions)
    {
        LCW("Accept New Client. Too Many Sessions And The new socket will closed. remoteAddress=" << remoteIP << ":" << remotePort 
            << ", Aready linked sessions = " << iter->second.second._currentLinked << ", trais=[" << iter->second.first << "] total TcpSocket object count =[create:" << g_appEnvironment.getCreatedSocketCount() << ", close:" << g_appEnvironment.getClosedSocketCount() << "], total TcpSession object count =[create:"
            << g_appEnvironment.getCreatedSessionCount() << ", close:" << g_appEnvironment.getClosedSessionCount()
            << "]");
    }
    else
    {
        LCD("Accept New Client. Accept new Sessions. The new socket  remoteAddress=" << remoteIP << ":" << remotePort 
            << ", Aready linked sessions = " << iter->second.second._currentLinked << ", trais=" << iter->second.first);
        iter->second.second._currentLinked++;
        iter->second.second._totalAcceptCount++;

        _lastSessionID = nextSessionID(_lastSessionID);
        s->initialize(_summer);

        TcpSessionPtr session = std::make_shared<TcpSession>();
        if (session->bindTcpSocketPrt(s, aID, _lastSessionID, iter->second.first))
        {
            _mapTcpSessionPtr[_lastSessionID] = session;
            _totalAcceptCount++;
            MessageDispatcher::getRef().dispatchOnSessionEstablished(session);
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

void SessionManager::onSessionClose(AccepterID aID, SessionID sID, const TcpSessionPtr &session)
{
    _mapTcpSessionPtr.erase(sID);
    _mapAccepterConfig[aID].second._currentLinked--;
    _totalAcceptClosedCount++;
    MessageDispatcher::getRef().dispatchOnSessionDisconnect(session);
    if (isSessionID(sID) && _stopClients == 2 && _funClientsStop)
    {
        int count = 0;
        for (auto & kv: _mapTcpSessionPtr)
        {
            if (isSessionID(kv.second->getSessionID()))
            {
                count++;
            }
        }
        if (count == 0)
        {
            _funClientsStop();
            _funClientsStop = nullptr;
            _stopClients = 3;
        }
    }
}


SessionID SessionManager::addConnector(const ConnectConfig & traits)
{
    _lastConnectID = nextConnectID(_lastConnectID);
    auto & pairConfig = _mapConnectorConfig[_lastConnectID];
    pairConfig.first = traits;
    pairConfig.second._cID = _lastConnectID;
    TcpSocketPtr sockPtr = std::make_shared<TcpSocket>();
    sockPtr->initialize(_summer);

    TcpSessionPtr sessionPtr = std::make_shared<TcpSession>();
    sessionPtr->bindTcpConnectorPtr(sockPtr, pairConfig);
    return _lastConnectID;
}

bool SessionManager::getConnectorConfig(SessionID sID, std::pair<ConnectConfig, ConnectInfo> & config)
{
    auto founder = _mapConnectorConfig.find(sID);
    if (founder != _mapConnectorConfig.end())
    {
        config = founder->second;
        return true;
    }
    return false;
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

void SessionManager::onConnect(SessionID cID, bool bConnected, const TcpSessionPtr &session)
{
    auto config = _mapConnectorConfig.find(cID);
    if (config == _mapConnectorConfig.end())
    {
        LCE("Unkwon Connector. Not Found ConnectorID=" << cID);
        return;
    }
    if (bConnected)
    {
        _mapTcpSessionPtr[cID] = session;
        config->second.second._curReconnectCount = 0;
        _totalConnectCount++;
        MessageDispatcher::getRef().dispatchOnSessionEstablished(session);
        return;
    }


    auto iter = _mapTcpSessionPtr.find(cID);
    if (!bConnected && iter != _mapTcpSessionPtr.end())
    {
        _mapTcpSessionPtr.erase(cID);
        _totalConnectClosedCount++;
        MessageDispatcher::getRef().dispatchOnSessionDisconnect(session);
        if (_stopServers == 2 && _funServerStop)
        {
            int count = 0;
            for (auto & kv : _mapTcpSessionPtr)
            {
                if (isConnectID(kv.second->getSessionID()))
                {
                    count++;
                }
            }
            if (count == 0)
            {
                _funServerStop();
                _funServerStop = nullptr;
                _stopServers = 3;
            }
        }
    }

    if (!bConnected
        && config->second.first._reconnectMaxCount > 0
        && config->second.second._curReconnectCount < config->second.first._reconnectMaxCount)
    {
        config->second.second._curReconnectCount++;
        config->second.second._totalConnectCount++;

        TcpSocketPtr sockPtr = std::make_shared<TcpSocket>();
        sockPtr->initialize(_summer);
        createTimer(config->second.first._reconnectInterval, std::bind(&TcpSession::bindTcpConnectorPtr, session, sockPtr, config->second));
        LCW("Try reconnect current count=" << config->second.second._curReconnectCount << ", total reconnect = " << config->second.second._totalConnectCount << ". Traits=" << config->second.first);
        return;//try reconnect
    }
    
    if (config->second.first._reconnectMaxCount > 0 && _running)
    {
        LCE("Try Reconnect Failed. End Try. Traits=" << config->second.first);
    }
    //connect faild . clean data
    _mapConnectorConfig.erase(config);
}


void SessionManager::sendSessionData(SessionID sID, const char * orgData, unsigned int orgDataLen)
{
    auto iter = _mapTcpSessionPtr.find(sID);
    if (iter == _mapTcpSessionPtr.end())
    {
        LCW("sendSessionData NOT FOUND SessionID.  SessionID=" << sID);
        return;
    }

    iter->second->doSend(orgData, orgDataLen);
    //trace log
    {
        LCT("sendSessionData Len=" << orgDataLen << ",binarydata=" << zsummer::log4z::Log4zBinary(orgData, orgDataLen >= 10 ? 10 : orgDataLen));
    }
}
void SessionManager::sendSessionData(SessionID sID, ProtoID pID, const char * userData, unsigned int userDataLen)
{
    WriteStream ws(pID);
    ws.appendOriginalData(userData, userDataLen);
    sendSessionData(sID, ws.getStream(), ws.getStreamLen());
    //trace log
    {
        LCT("sendSessionData ProtoID=" << pID << ",  userDataLen=" << userDataLen);
    }
}




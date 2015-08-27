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

using namespace zsummer::proto4z;
using namespace zsummer::network;



TcpSession::TcpSession()
{
    zsummer::network::g_appEnvironment.addCreatedSessionCount();
    LCI("TcpSession. total TcpSocket object count =[create:" << zsummer::network::g_appEnvironment.getCreatedSocketCount() << ", close:" << zsummer::network::g_appEnvironment.getClosedSocketCount() << "], total TcpSession object count =[create:"
        << zsummer::network::g_appEnvironment.getCreatedSessionCount() << ", close:" << zsummer::network::g_appEnvironment.getClosedSessionCount()
        << "]");
    _recving = (SessionBlock*)malloc(sizeof(SessionBlock)+SESSION_BLOCK_SIZE);
    _recving->len = 0;
    _recving->bound = SESSION_BLOCK_SIZE;
    _sending = (SessionBlock*)malloc(sizeof(SessionBlock)+SESSION_BLOCK_SIZE);
    _sending->len = 0;
    _sending->bound = SESSION_BLOCK_SIZE;
}

TcpSession::~TcpSession()
{
    zsummer::network::g_appEnvironment.addClosedSessionCount();
    while (!_sendque.empty())
    {
        _traits._freeBlock(_sendque.front());
        _sendque.pop();
    }
    _sockptr.reset();
    free (_recving);
    _recving = nullptr;
    free (_sending);
    _sending = nullptr;
    LCI("~TcpSession. total TcpSocket object count =[create:" << zsummer::network::g_appEnvironment.getCreatedSocketCount() << ", close:" << zsummer::network::g_appEnvironment.getClosedSocketCount() << "], total TcpSession object count =[create:"
        << zsummer::network::g_appEnvironment.getCreatedSessionCount() << ", close:" << zsummer::network::g_appEnvironment.getClosedSessionCount()
        << "]");
}


bool TcpSession::bindTcpSocketPrt(const TcpSocketPtr &sockptr, AccepterID aID, SessionID sID)
{
    _sockptr = sockptr;
    _acceptID = aID;
    _sessionID = sID;
    _sockptr->getPeerInfo(_remoteIP, _remotePort);
    _status = 2;
    if (!doRecv())
    {
        LCW("bindTcpSocketPrt Failed.");
        return false;
    }
    if (_traits._eventSessionBuild)
    {
        _traits._eventSessionBuild(shared_from_this());
    }
    _pulseTimerID = SessionManager::getRef().createTimer(_traits._pulseInterval, std::bind(&TcpSession::onPulseTimer, shared_from_this()));
    SessionManager::getRef()._statInfo[STAT_LINKED]++;
    return true;
}

void TcpSession::connect()
{
    _pulseTimerID = SessionManager::getRef().createTimer(_traits._pulseInterval, std::bind(&TcpSession::onPulseTimer, shared_from_this()));
    _status = 1;
    reconnect();
}

void TcpSession::reconnect()
{
    //!
    if (_sockptr)
    {
        //socket在close之后不应该再有任何回调.
        //socket close前只有recv回调才会收到close通知.
        //需要改底层
        _sockptr->doClose();
        _sockptr.reset();
    }
    _recving->len = 0;
    _sending->len = 0;
    _sendingCurIndex = 0;
    _rc4StateRead.makeSBox(_traits._rc4TcpEncryption);
    _rc4StateWrite.makeSBox(_traits._rc4TcpEncryption);
    _bFirstRecvData = true;

    if (_traits._reconnectCleanAllData)
    {
        while (!_sendque.empty())
        {
            _traits._freeBlock(_sendque.front());
            _sendque.pop();
        }
    }
    _sockptr = std::make_shared<TcpSocket>();
    if (!_sockptr->initialize(_eventLoop))
    {
        LCE("connect init error");
        return;
    }
    if (!_sockptr->doConnect(_remoteIP, _remotePort, std::bind(&TcpSession::onConnected, shared_from_this(), std::placeholders::_1)))
    {
        LCE("connect error");
        return;
    }
}



void TcpSession::onConnected(zsummer::network::NetErrorCode ec)
{
    if (ec)
    {
        LCW("onConnected error. ec=" << ec << ",  cID=" << _sessionID);
        return;
    }
    LCI("onConnected success. cID=" << _sessionID);
    _status = 2;

    if (!doRecv())
    {
        _status = 1;
        return;
    }

    if (!_sendque.empty())
    {
        send(nullptr, 0);
    }
    if (_traits._eventSessionBuild)
    {
        _traits._eventSessionBuild(shared_from_this());
    }
}

bool TcpSession::doRecv()
{
    return _sockptr->doRecv(_recving->begin + _recving->len, _recving->bound - _recving->len, std::bind(&TcpSession::onRecv, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
}

void TcpSession::close()
{
    LCW("TcpSession to close socket. sID= " << _sessionID );
    if (_sockptr)
    {
        _sockptr->doClose();
        _sockptr.reset();
        //onClose();
        if (isConnectID(_sessionID) && _curReconnectCount < _traits._reconnectMaxCount)
        {
            _status = 1;
        }
        else
        {
            _status = 3;
            SessionManager::getRef().post(std::bind(&TcpSession::onClose, shared_from_this()));
        }
    }
}

void TcpSession::onRecv(zsummer::network::NetErrorCode ec, int nRecvedLen)
{
    if (ec)
    {
        LCD("socket closed. ec=" << ec);
        onClose();
        return;
    }
    _recving->len += nRecvedLen;
    SessionManager::getRef()._statInfo[STAT_RECV_COUNT]++;
    SessionManager::getRef()._statInfo[STAT_RECV_BYTES] += nRecvedLen;
    // skip encrypt the flash policy data if that open flash policy.
    // skip encrypt when the rc4 encrypt sbox is empty.
    {
        do 
        {
            //process something when recv first data.
            // flash policy process
            const char * flashPolicyRequestString = "<policy-file-request/>"; //string length is 23 byte, contain null-terminator character.
            unsigned int flashPolicyRequestSize = 23;
            if (_bFirstRecvData 
                && _traits._openFlashPolicy 
                && _acceptID != InvalidAccepterID 
                && _recving->len == flashPolicyRequestSize 
                && memcmp(flashPolicyRequestString, _recving->begin, flashPolicyRequestSize) == 0)
            {
                _recving->len = 0;
                const char * flashPolicyResponseString = R"---(<cross-domain-policy><allow-access-from domain="*" to-ports="*"/></cross-domain-policy>)---";
                unsigned int flashPolicyResponseSize = (unsigned int)strlen(flashPolicyResponseString) + 1;
                send(flashPolicyResponseString, flashPolicyResponseSize);
                _bFirstRecvData = false;
            }
            else if (_bFirstRecvData)
            {
                //do other something.

                //do other something end.
                _bFirstRecvData = false;
            }
            
            if (_traits._rc4TcpEncryption.empty() || _recving->len == 0)
            {
                break;
            }
            
            unsigned int needEncry = nRecvedLen;
            if (_recving->len < (unsigned int)nRecvedLen)
            {
                needEncry = _recving->len;
            }
            _rc4StateRead.encryption((unsigned char*)_recving->begin + _recving->len - needEncry, needEncry);
        } while (0);
        
    }

    //分包
    unsigned int usedIndex = 0;
    do 
    {
        if (_traits._protoType == PT_TCP)
        {
            auto ret = _traits._checkTcpBlock(_recving->begin + usedIndex, _recving->len - usedIndex, _recving->bound - usedIndex);
            if (ret.first == zsummer::proto4z::IRT_CORRUPTION)
            {
                LCW("killed socket: checkBuffIntegrity error ");
                _sockptr->doClose();
                onClose();
                return;
            }
            if (ret.first == zsummer::proto4z::IRT_SHORTAGE)
            {
                break;
            }
            try
            {
                SessionManager::getRef()._statInfo[STAT_RECV_PACKS]++;
                _traits._dispatchBlock(shared_from_this(), _recving->begin + usedIndex, ret.second);
            }
            catch (std::runtime_error e)
            {
                LCW("MessageEntry catch one exception: " << e.what());
                _sockptr->doClose();
                onClose();
                return;
            }
            usedIndex += ret.second;
        }
        else
        {
            std::string body;
            unsigned int usedLen = 0;
            auto ret = _traits._checkHTTPBlock(_recving->begin + usedIndex,
                _recving->len - usedIndex,
                _recving->bound - usedIndex,
                _httpHadHeader, _httpIsChunked, _httpCommonLine, _httpHeader,
                body);

            if (ret.first == zsummer::proto4z::IRT_CORRUPTION)
            {
                LCT("killed http socket: checkHTTPBuffIntegrity error sID=" << _sessionID);
                _sockptr->doClose();
                onClose();
                return;
            }
            if (ret.first == zsummer::proto4z::IRT_SHORTAGE)
            {
                break;
            }

            if (!_httpHadHeader)
            {
                _httpHadHeader = true;
            }
            SessionManager::getRef()._statInfo[STAT_RECV_PACKS]++;
            _traits._dispatchHTTP(shared_from_this(), _httpCommonLine, _httpHeader, body);
            usedIndex += ret.second;
        }
        
    } while (true);
    
    
    if (usedIndex > 0)
    {
        _recving->len= _recving->len - usedIndex;
        if (_recving->len > 0)
        {
            memmove(_recving->begin, _recving->begin + usedIndex, _recving->len);
        }
    }
    
    if (!doRecv())
    {
        onClose();
    }
}

void TcpSession::send(const char *buf, unsigned int len)
{
    if (len > SESSION_BLOCK_SIZE)
    {
        LCE("error.  too large block send.  len=" << len);
        return;
    }
    SessionBlock * b = nullptr;
    if (len == 0 && _status == 2 && !_sendque.empty())
    {
        b = _sendque.front();
        _sendque.pop();
        buf = b->begin;
        len = b->len;
        SessionManager::getRef()._statInfo[STAT_SEND_QUES]--;
    }
    else if (len == 0)
    {
        return;
    }
    //push to send queue
    if (!_sendque.empty() || _status != 2)
    {
        SessionBlock * sb = _traits._createBlock();
        if (sb->bound < len)
        {
            _traits._freeBlock(sb);
            LCE("error.  too large block send.  len=" << len);
            return;
        }
        memcpy(sb->begin, buf, len);
        sb->len = len;
        _sendque.push(sb);
        SessionManager::getRef()._statInfo[STAT_SEND_QUES]++;
    }
    //send direct
    else
    {
        memcpy(_sending->begin, buf, len);
        _sending->len = len;
        if (b)
        {
            _traits._freeBlock(b);
            b = nullptr;
        }
        if (!_traits._rc4TcpEncryption.empty())
        {
            _rc4StateWrite.encryption((unsigned char*)_sending->begin, len);
        }
        SessionManager::getRef()._statInfo[STAT_SEND_COUNT]++;
        SessionManager::getRef()._statInfo[STAT_SEND_PACKS]++;
        bool sendRet = _sockptr->doSend(_sending->begin, _sending->len, std::bind(&TcpSession::onSend, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
        if (!sendRet)
        {
            LCE("Send Failed");
        }
    }
}


void TcpSession::onSend(zsummer::network::NetErrorCode ec, int nSentLen)
{
    if (ec)
    {
        LCD("remote socket closed");
        return ;
    }

    _sendingCurIndex += nSentLen;
    SessionManager::getRef()._statInfo[STAT_SEND_BYTES] += nSentLen;
    if (_sendingCurIndex < _sending->len)
    {
        bool sendRet = _sockptr->doSend(_sending->begin + _sendingCurIndex, _sending->len - _sendingCurIndex, std::bind(&TcpSession::onSend, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
        if (!sendRet)
        {
            LCW("Send Failed");
            return;
        }
        SessionManager::getRef()._statInfo[STAT_SEND_COUNT]++;
    }
    else if (_sendingCurIndex == _sending->len)
    {
        _sendingCurIndex = 0;
        _sending->len = 0;
        if (!_sendque.empty())
        {
            do
            {
                SessionBlock *sb = _sendque.front();
                _sendque.pop();
                SessionManager::getRef()._statInfo[STAT_SEND_QUES]--;
                memcpy(_sending->begin + _sending->len, sb->begin, sb->len);
                _sending->len += sb->len;
                _traits._freeBlock(sb);
                SessionManager::getRef()._statInfo[STAT_SEND_PACKS]++;
                if (_sendque.empty())
                {
                    break;
                }
                if (_sending->bound - _sending->len < _sendque.front()->len)
                {
                    break;
                }
            } while (true);
            _rc4StateWrite.encryption((unsigned char *)_sending->begin, _sending->len);
            bool sendRet = _sockptr->doSend(_sending->begin, _sending->len, std::bind(&TcpSession::onSend, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
            if (!sendRet)
            {
                LCW("Send Failed");
                return;
            }
            SessionManager::getRef()._statInfo[STAT_SEND_COUNT]++;
        }
    }
}

void TcpSession::onPulseTimer()
{
    if (_status == 3)
    {
        return;
    }
    
    if (_status == 1)
    {
        if (_traits._reconnectMaxCount == 0 || _curReconnectCount >= _traits._reconnectMaxCount)
        {
            _status = 3;
            onClose();
        }
        else
        {
            reconnect();
            _curReconnectCount++;
            if (_traits._eventPulse)
            {
                _traits._eventPulse(shared_from_this());
            }
            _pulseTimerID = SessionManager::getRef().createTimer(_traits._reconnectInterval, std::bind(&TcpSession::onPulseTimer, shared_from_this()));
        }
    }
    else if (_status == 2)
    {
        if (_traits._eventPulse)
        {
            _traits._eventPulse(shared_from_this());
        }
        _pulseTimerID = SessionManager::getRef().createTimer(_traits._pulseInterval, std::bind(&TcpSession::onPulseTimer, shared_from_this()));
    }
}

void TcpSession::onClose()
{
    LCI("Client Closed!");
    _sockptr.reset();
    _status = 3;
    if (isConnectID(_sessionID) && _curReconnectCount < _traits._reconnectMaxCount)
    {
        _status = 1;
        return;
    }
    if (_traits._eventSessionClose)
    {
        _traits._eventSessionClose(shared_from_this());
    }
    SessionManager::getRef().onSessionClose(shared_from_this());
    if (isSessionID(_sessionID))
    {
        SessionManager::getRef()._statInfo[STAT_CLOSED]++;
    }
    
}

Any TcpSession::setUserParam(int index, const Any &any)
{
    if (_param.size() <= index)
    {
        _param.assign(index + 1 - _param.size(), Any(0));
    }
    _param[index] = any;
    return any;
}
Any TcpSession::getUserParam(int index)
{
    if (index >= _param.size())
    {
        return Any(0);
    }
    return _param[index];
}


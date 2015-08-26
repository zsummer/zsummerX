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
    _recving = (SessionBlock*)malloc(sizeof(SessionBlock)+1024 * 100);
    _recving->len = 0;
    _recving->bound = 100 * 1024;
    _sending = (SessionBlock*)malloc(sizeof(SessionBlock)+1024 * 100);
    _sending->len = 0;
    _sending->bound = 100 * 1024;
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
    _pulseTimerID = SessionManager::getRef().createTimer(_traits._pulseInterval, std::bind(&TcpSession::onPulseTimer, shared_from_this()));
    return true;
}

void TcpSession::connect()
{
    _pulseTimerID = SessionManager::getRef().createTimer(_traits._pulseInterval, std::bind(&TcpSession::onPulseTimer, shared_from_this()));
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
    _recving.bufflen = 0;
    _sending.bufflen = 0;
    _sendingCurIndex = 0;
    _rc4StateRead.makeSBox(_traits._rc4TcpEncryption);
    _rc4StateWrite.makeSBox(_traits._rc4TcpEncryption);
    _bFirstRecvData = true;

    if (_traits._reconnectCleanAllData)
    {
        while (!_sendque.empty())
        {
            _freeCache.push(_sendque.front());
            _sendque.pop();
        }
    }
    _sockptr = std::make_shared<TcpSocket>();
    if (_sockptr->initialize(_eventLoop))
    {
        throw std::runtime_error("connect init error");
    }
    if (!_sockptr->doConnect(_remoteIP, _remotePort, std::bind(&TcpSession::onConnected, shared_from_this(), std::placeholders::_1)))
    {
        throw std::runtime_error("connect error");
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

    if (!_sendque.empty())
    {
        MessageSendPack *tmp = _sendque.front();
        _sendque.pop();
        send(tmp->buff, tmp->bufflen);
        tmp->bufflen = 0;
        _freeCache.push(tmp);
    }
    if (!doRecv())
    {
        _status = 1;
        return;
    }
}

bool TcpSession::doRecv()
{
    return _sockptr->doRecv(_recving.buff + _recving.bufflen, MAX_BUFF_SIZE - _recving.bufflen, std::bind(&TcpSession::onRecv, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
}

void TcpSession::close()
{
    LCW("TcpSession to close socket. sID= " << _sessionID );
    if (_sockptr)
    {
        _sockptr->doClose();
    }
    if (_pulseTimerID != zsummer::network::InvalidTimerID)
    {
        SessionManager::getRef().cancelTimer(_pulseTimerID);
        _pulseTimerID = zsummer::network::InvalidTimerID;
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
    _recving.bufflen += nRecvedLen;

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
                && _recving.bufflen == flashPolicyRequestSize 
                && memcmp(flashPolicyRequestString, _recving.buff, flashPolicyRequestSize) == 0)
            {
                _recving.bufflen = 0;
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
            
            if (_traits._rc4TcpEncryption.empty() || _recving.bufflen == 0)
            {
                break;
            }
            
            unsigned int needEncry = nRecvedLen;
            if (_recving.bufflen < (unsigned int)nRecvedLen)
            {
                needEncry = _recving.bufflen;
            }
            _rc4StateRead.encryption((unsigned char*)_recving.buff + _recving.bufflen - needEncry, needEncry);
        } while (0);
        
    }

    //分包
    unsigned int usedIndex = 0;
    do 
    {
        if (_traits._protoType == PT_TCP)
        {
            auto ret = _traits._checkTcpIntegrity(_recving.buff + usedIndex, _recving.bufflen - usedIndex, MAX_BUFF_SIZE - usedIndex);
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
                _traits._dispatchBlock(shared_from_this(), _recving.buff + usedIndex, ret.second);
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
            auto ret = _traits._checkHTTPIntegrity(_recving.buff + usedIndex,
                _recving.bufflen - usedIndex,
                MAX_BUFF_SIZE - usedIndex,
                _httpHadHeader, _httpIsChunked, _httpCommonLine, _httpHeader,
                body, usedLen);

            if (ret == zsummer::proto4z::IRT_CORRUPTION)
            {
                LCT("killed http socket: checkHTTPBuffIntegrity error sID=" << _sessionID);
                _sockptr->doClose();
                onClose();
                return;
            }
            if (ret == zsummer::proto4z::IRT_SHORTAGE)
            {
                break;
            }
            if (!_httpHadHeader)
            {
                _httpHadHeader = true;
            }
            _traits._dispatchHTTP(shared_from_this(), _httpCommonLine, _httpHeader, body);
            usedIndex += usedLen;
        }
        
    } while (true);
    
    
    if (usedIndex > 0)
    {
        _recving.bufflen = _recving.bufflen - usedIndex;
        if (_recving.bufflen > 0)
        {
            memmove(_recving.buff, _recving.buff + usedIndex, _recving.bufflen);
        }
    }
    
    if (!doRecv())
    {
        onClose();
    }
}

void TcpSession::send(const char *buf, unsigned int len)
{
    if (len > MAX_SEND_PACK_SIZE)
    {
        LCE("Send Message failed!  send len = " << len << ", max send pack len=" << MAX_SEND_PACK_SIZE);
        return;
    }



    if (_status != 2 || _sending.bufflen != 0)
    {
        MessageSendPack *pack = NULL;
        if (_freeCache.empty())
        {
            pack = new MessageSendPack();
        }
        else
        {
            pack = _freeCache.front();
            _freeCache.pop();
        }
        
        memcpy(pack->buff, buf, len);
        pack->bufflen = len;
        _sendque.push(pack);
    }
    else
    {
        if (!_traits._rc4TcpEncryption.empty())
        {
            _rc4StateWrite.encryption((unsigned char*)buf, len);
        }
        memcpy(_sending.buff, buf, len);
        _sending.bufflen = len;
        bool sendRet = _sockptr->doSend(_sending.buff, _sending.bufflen, std::bind(&TcpSession::onSend, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
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
    if (_sendingCurIndex < _sending.bufflen)
    {
        bool sendRet = _sockptr->doSend(_sending.buff + _sendingCurIndex, _sending.bufflen - _sendingCurIndex, std::bind(&TcpSession::onSend, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
        if (!sendRet)
        {
            LCW("Send Failed");
            return;
        }
    }
    else if (_sendingCurIndex == _sending.bufflen)
    {
        _sendingCurIndex = 0;
        _sending.bufflen = 0;
        if (!_sendque.empty())
        {
            do
            {
                SessionBlock *block = _sendque.front();
                _sendque.pop();
                memcpy(_sending.buff + _sending.bufflen, block->begin, block->len);
                _sending.bufflen += pack->bufflen;
                pack->bufflen = 0;
                _freeCache.push(pack);

                if (_sendque.empty())
                {
                    break;
                }
                if (MAX_BUFF_SIZE - _sending.bufflen < _sendque.front()->bufflen)
                {
                    break;
                }
            } while (true);
            _rc4StateWrite.encryption((unsigned char *)_sending.buff, _sending.bufflen);
            
            bool sendRet = _sockptr->doSend(_sending.buff, _sending.bufflen, std::bind(&TcpSession::onSend, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
            if (!sendRet)
            {
                LCW("Send Failed");
                return;
            }
        }
    }
}

void TcpSession::onPulseTimer()
{
    if (_status == 1)
    {
        if (_traits._reconnectMaxCount == 0 || _curReconnectCount > _traits._reconnectMaxCount)
        {
            onClose();
        }
        else
        {
            reconnect();
            _curReconnectCount++;
            _pulseTimerID = SessionManager::getRef().createTimer(_traits._reconnectInterval, std::bind(&TcpSession::onPulseTimer, shared_from_this()));
        }
    }
    else if (_status == 2)
    {
        _pulseTimerID = SessionManager::getRef().createTimer(_traits._pulseInterval, std::bind(&TcpSession::onPulseTimer, shared_from_this()));
    }
}

void TcpSession::onClose()
{
    LCI("Client Closed!");
    _sockptr.reset();
    _status = 3;
    SessionManager::getRef().onSessionClose(_acceptID, _sessionID, shared_from_this());
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


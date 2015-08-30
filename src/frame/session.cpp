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
    SessionManager::getRef()._statInfo[STAT_SESSION_CREATED]++;
    _recving = (SessionBlock*)malloc(sizeof(SessionBlock)+SESSION_BLOCK_SIZE);
    _recving->len = 0;
    _recving->bound = SESSION_BLOCK_SIZE;
    _sending = (SessionBlock*)malloc(sizeof(SessionBlock)+SESSION_BLOCK_SIZE);
    _sending->len = 0;
    _sending->bound = SESSION_BLOCK_SIZE;
}

TcpSession::~TcpSession()
{
    SessionManager::getRef()._statInfo[STAT_SESSION_DESTROYED]++;
    while (!_sendque.empty())
    {
        _options._freeBlock(_sendque.front());
        _sendque.pop();
    }
    if (_sockptr)
    {
        _sockptr->doClose();
        _sockptr.reset();
    }

    free (_recving);
    _recving = nullptr;
    free (_sending);
    _sending = nullptr;
}


void TcpSession::connect()
{
    _pulseTimerID = SessionManager::getRef().createTimer(isConnectID(_sessionID) ? _options._connectPulseInterval : _options._sessionPulseInterval, std::bind(&TcpSession::onPulse, shared_from_this()));
    _status = 1;
    reconnect();
}

void TcpSession::reconnect()
{
    if (_sockptr)
    {
        _sockptr->doClose();
        _sockptr.reset();
    }
    _recving->len = 0;
    _sending->len = 0;
    _sendingCurIndex = 0;
    _rc4StateRead.makeSBox(_options._rc4TcpEncryption);
    _rc4StateWrite.makeSBox(_options._rc4TcpEncryption);
    _bFirstRecvData = true;

    while (_options._reconnectClean && !_sendque.empty())
    {
        _options._freeBlock(_sendque.front());
        _sendque.pop();
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

bool TcpSession::attatch(const TcpSocketPtr &sockptr, AccepterID aID, SessionID sID)
{
    _sockptr = sockptr;
    _acceptID = aID;
    _sessionID = sID;
    _sockptr->getPeerInfo(_remoteIP, _remotePort);
    if (_options._setNoDelay)
    {
        sockptr->setNoDelay();
    }
    _status = 2;
    _pulseTimerID = SessionManager::getRef().createTimer(_options._connectPulseInterval, std::bind(&TcpSession::onPulse, shared_from_this()));
    SessionManager::getRef()._statInfo[STAT_SESSION_LINKED]++;

    if (_options._onSessionLinked)
    {
        _options._onSessionLinked(shared_from_this());
    }

    if (!doRecv())
    {
        close();
        return false;
    }
    return true;
}

void TcpSession::onConnected(zsummer::network::NetErrorCode ec)
{
    if (ec)
    {
        LCW("onConnected error. ec=" << ec << ",  cID=" << _sessionID);
        return;
    }
    LCI("onConnected success. cID=" << _sessionID);

    if (!doRecv())
    {
        return;
    }
    _status = 2;
    _reconnects = 0;
    if (_options._setNoDelay)
    {
        _sockptr->setNoDelay();
    }
    if (_options._onSessionLinked)
    {
        _options._onSessionLinked(shared_from_this());
    }
    SessionManager::getRef()._statInfo[STAT_SESSION_LINKED]++;
    if (!_sendque.empty())
    {
        send(nullptr, 0);
    }
}

bool TcpSession::doRecv()
{
    if (!_sockptr)
    {
        return false;
    }
    return _sockptr->doRecv(_recving->begin + _recving->len, _recving->bound - _recving->len, std::bind(&TcpSession::onRecv, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
}

void TcpSession::close()
{
    LCW("TcpSession to close socket. sID= " << _sessionID );
    if (_sockptr)
    {
        if (_status == 2)
        {
            SessionManager::getRef()._statInfo[STAT_SESSION_CLOSED]++;
            if (_options._onSessionClosed)
            {
                _options._onSessionClosed(shared_from_this());
            }
        }
        _status = 3;
        _sockptr->doClose();
        _sockptr.reset();
        if (isConnectID(_sessionID) && _reconnects < _options._reconnects)
        {
            _status = 1;
        }
        else
        {
            SessionManager::getRef().removeSession(shared_from_this());
        }
        return;
    }
    LCE("TcpSession::close invalid");
}

void TcpSession::onRecv(zsummer::network::NetErrorCode ec, int nRecvedLen)
{
    if (ec)
    {
        LCD("socket closed. ec=" << ec);
        close();
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
                && _options._openFlashPolicy 
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
            
            if (_options._rc4TcpEncryption.empty() || _recving->len == 0)
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
        if (_options._protoType == PT_TCP)
        {
            auto ret = _options._onBlockCheck(_recving->begin + usedIndex, _recving->len - usedIndex, _recving->bound - usedIndex, _recving->bound);
            if (ret.first == BCT_CORRUPTION)
            {
                LCW("killed socket: _onBlockCheck error ");
                close();
                return;
            }
            if (ret.first == BCT_SHORTAGE)
            {
                break;
            }
            try
            {
                SessionManager::getRef()._statInfo[STAT_RECV_PACKS]++;
                _options._onBlockDispatch(shared_from_this(), _recving->begin + usedIndex, ret.second);
            }
            catch (std::runtime_error e)
            {
                LCW("MessageEntry catch one exception: " << e.what());
                close();
                return;
            }
            usedIndex += ret.second;
        }
        else
        {
            std::string body;
            auto ret = _options._onHTTPBlockCheck(_recving->begin + usedIndex,
                _recving->len - usedIndex,
                _recving->bound - usedIndex,
                _httpHadHeader, _httpIsChunked, _httpCommonLine, _httpHeader,
                body);

            if (ret.first == BCT_CORRUPTION)
            {
                LCE("killed http socket: _onHTTPBlockCheck error sID=" << _sessionID);
                close();
                return;
            }
            if (ret.first == BCT_SHORTAGE)
            {
                break;
            }

            if (!_httpHadHeader)
            {
                _httpHadHeader = true;
            }
            SessionManager::getRef()._statInfo[STAT_RECV_PACKS]++;
            _options._onHTTPBlockDispatch(shared_from_this(), _httpCommonLine, _httpHeader, body);
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
        close();
    }
}

void TcpSession::send(const char *buf, unsigned int len)
{
    if (len > _sending->bound)
    {
        LCE("send error.  too large block than sending block bound.  len=" << len);
        return;
    }

    if (len == 0)
    {
        if (_status == 2 && _sending->len == 0 && !_sendque.empty())
        {
            SessionBlock *sb = _sendque.front();
            memcpy(_sending->begin, sb->begin, sb->len);
            _sending->len = sb->len;
            _options._freeBlock(sb);
            if (!_options._rc4TcpEncryption.empty())
            {
                _rc4StateWrite.encryption((unsigned char*)_sending->begin, _sending->len);
            }
            bool sendRet = _sockptr->doSend(_sending->begin, _sending->len, std::bind(&TcpSession::onSend, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
            if (!sendRet)
            {
                LCE("send error from first connect to send dirty block");
            }
        }
        return;
    }

    //push to send queue
    if (!_sendque.empty() || _status != 2 || _sending->len != 0)
    {
        SessionBlock * sb = _options._createBlock();
        if (sb->bound < len)
        {
            _options._freeBlock(sb);
            LCE("send error.  too large block than session block.  len=" << len);
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
        if (!_options._rc4TcpEncryption.empty())
        {
            _rc4StateWrite.encryption((unsigned char*)_sending->begin, _sending->len);
        }
        SessionManager::getRef()._statInfo[STAT_SEND_COUNT]++;
        SessionManager::getRef()._statInfo[STAT_SEND_PACKS]++;
        bool sendRet = _sockptr->doSend(_sending->begin, _sending->len, std::bind(&TcpSession::onSend, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
        if (!sendRet)
        {
            LCE("send error ");
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
        SessionManager::getRef()._statInfo[STAT_SEND_COUNT]++;
        bool sendRet = _sockptr->doSend(_sending->begin + _sendingCurIndex, _sending->len - _sendingCurIndex, std::bind(&TcpSession::onSend, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
        if (!sendRet)
        {
            LCW("send error from onSend");
            return;
        }
        return;
    }
    if (_sendingCurIndex == _sending->len)
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
                _options._freeBlock(sb);
                SessionManager::getRef()._statInfo[STAT_SEND_PACKS]++;
                if (_sendque.empty())
                {
                    break;
                }
                if (_sending->bound - _sending->len < _sendque.front()->len)
                {
                    break;
                }
                if (!_options._joinSmallBlock)
                {
                    break;
                }
                
            } while (true);

            if (!_options._rc4TcpEncryption.empty())
            {
                _rc4StateWrite.encryption((unsigned char *)_sending->begin, _sending->len);
            }
            SessionManager::getRef()._statInfo[STAT_SEND_COUNT]++;
            bool sendRet = _sockptr->doSend(_sending->begin, _sending->len, std::bind(&TcpSession::onSend, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
            if (!sendRet)
            {
                LCW("send error from next queue block.");
                return;
            }
            return;
        }
    }
}

void TcpSession::onPulse()
{
    if (_status == 3)
    {
        return;
    }
    
    if (_status == 1)
    {
        if (_reconnects >= _options._reconnects)
        {
            close();
        }
        else
        {
            reconnect();
            _reconnects++;
            if (_options._onSessionPulse)
            {
                _options._onSessionPulse(shared_from_this());
            }
            _pulseTimerID = SessionManager::getRef().createTimer(isConnectID(_sessionID) ? _options._connectPulseInterval : _options._sessionPulseInterval, std::bind(&TcpSession::onPulse, shared_from_this()));
        }
    }
    else if (_status == 2)
    {
        if (_options._onSessionPulse)
        {
            _options._onSessionPulse(shared_from_this());
        }
        _pulseTimerID = SessionManager::getRef().createTimer(isConnectID(_sessionID) ? _options._connectPulseInterval : _options._sessionPulseInterval, std::bind(&TcpSession::onPulse, shared_from_this()));
    }
}


Any TcpSession::setUserParam(int index, const Any &any)
{
    if (_param.size() <= (unsigned int)index)
    {
        _param.assign(index + 1 - _param.size(), Any(0));
    }
    _param[index] = any;
    return any;
}
Any TcpSession::getUserParam(int index)
{
    if ((unsigned int)index >= _param.size())
    {
        return Any(0);
    }
    return _param[index];
}


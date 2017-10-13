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

#include <zsummerX/frame/session.h>
#include <zsummerX/frame/manager.h>
#include <algorithm>

using namespace zsummer::proto4z;
using namespace zsummer::network;
using std::min;
using std::max;


TcpSession::TcpSession()
{
    SessionManager::getRef()._statInfo[STAT_SESSION_CREATED]++;
    _recving = (SessionBlock*)malloc(sizeof(SessionBlock)+SESSION_BLOCK_SIZE);
    _recving->len = 0;
    _recving->bound = SESSION_BLOCK_SIZE;
    _sending = (SessionBlock*)malloc(sizeof(SessionBlock)+SESSION_BLOCK_SIZE);
    _sending->len = 0;
    _sending->bound = SESSION_BLOCK_SIZE;
    _param.reserve(100);
}

TcpSession::~TcpSession()
{
    SessionManager::getRef()._statInfo[STAT_SESSION_DESTROYED]++;
    while (!_sendque.empty())
    {
        _options._freeBlock(_sendque.front());
        _sendque.pop_front();
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
    if (_status == 0)
    {
        _pulseTimerID = SessionManager::getRef().createTimer(_options._connectPulseInterval, std::bind(&TcpSession::onPulse, shared_from_this()));
        _status = 1;
        reconnect();
    }
    else
    {
        LCE("can't connect on a old session.  please use addConnect try again.");
    }
}

void TcpSession::reconnect()
{
    if (_sockptr)
    {
        _sockptr->doClose();
        _sockptr.reset();
    }
    _sockptr = std::make_shared<TcpSocket>();
    if (!_sockptr->initialize(_eventLoop))
    {
        LCE("connect init error");
        return;
    }
    _recving->len = 0;
    _sending->len = 0;
    _sendingLen = 0;
    _rc4StateRead.makeSBox(_options._rc4TcpEncryption);
    _rc4StateWrite.makeSBox(_options._rc4TcpEncryption);
    _bFirstRecvData = true;

    while (_options._reconnectClean && !_sendque.empty())
    {
        _options._freeBlock(_sendque.front());
        _sendque.pop_front();
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
#ifndef WIN32
    sockptr->setFloodSendOptimize(_options._floodSendOptimize);
#endif
    _status = 2;
    _pulseTimerID = SessionManager::getRef().createTimer(_options._sessionPulseInterval, std::bind(&TcpSession::onPulse, shared_from_this()));
    SessionManager::getRef()._statInfo[STAT_SESSION_LINKED]++;

    if (_options._onSessionLinked)
    {
        try
        {
            _options._onSessionLinked(shared_from_this());
        }
        catch (const std::exception & e)
        {
            LOGE("TcpSession::attatch _onSessionLinked error. e=" << e.what());
        }
        catch (...)
        {
            LCW("TcpSession::attatch _onSessionLinked catch one unknown exception.");
        }
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
    LCI("onConnected success. sessionID=" << _sessionID);

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
#ifndef WIN32
    _sockptr->setFloodSendOptimize(_options._floodSendOptimize);
#endif
    if (_options._onSessionLinked)
    {
        try
        {
            _options._onSessionLinked(shared_from_this());
        }
        catch (const std::exception & e)
        {
            LOGE("TcpSession::onConnected error. e=" << e.what());
        }
        catch (...)
        {
            LCW("TcpSession::onConnected catch one unknown exception.");
        }
    }
    SessionManager::getRef()._statInfo[STAT_SESSION_LINKED]++;
    if (!_sendque.empty())
    {
        send(nullptr, 0);
    }
}

bool TcpSession::doRecv()
{
    LCT("TcpSession::doRecv sessionID=" << getSessionID() );
    if (!_sockptr)
    {
        return false;
    }
#ifndef WIN32
    return _sockptr->doRecv(_recving->begin + _recving->len, _recving->bound - _recving->len, std::bind(&TcpSession::onRecv, shared_from_this(), std::placeholders::_1, std::placeholders::_2), true);
#else
    return _sockptr->doRecv(_recving->begin + _recving->len, _recving->bound - _recving->len, std::bind(&TcpSession::onRecv, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
#endif
}

void TcpSession::close()
{
    if (_status != 3 && _status != 0)
    {
        if (_sockptr)
        {
            _sockptr->doClose();
            _sockptr.reset();
        }
        LCD("TcpSession to close socket. sID= " << _sessionID);
        if (_status == 2)
        {
            SessionManager::getRef()._statInfo[STAT_SESSION_CLOSED]++;
            if (_options._onSessionClosed)
            {
                SessionManager::getRef().post(std::bind(_options._onSessionClosed, shared_from_this()));
            }
        }

        if (isConnectID(_sessionID) && _reconnects < _options._reconnects)
        {
            _status = 1;
            LCD("TcpSession already closed. try reconnect ... sID= " << _sessionID);
        }
        else
        {
            _status = 3;
            SessionManager::getRef().post(std::bind(&SessionManager::removeSession, SessionManager::getPtr(), shared_from_this()));
            LCI("TcpSession remove self from manager. sID= " << _sessionID);
        }
        return;
    }
    LCW("TcpSession::close closing. sID=" << _sessionID);
}

unsigned int TcpSession::onRecv(zsummer::network::NetErrorCode ec, int received)
{
    LCT("TcpSession::onRecv sessionID=" << getSessionID() << ", received=" << received);
    if (ec)
    {
        _lastRecvError = ec;
        if (_lastRecvError == NEC_REMOTE_CLOSED)
        {
            LCI("socket closed.  remote close. sID=" << _sessionID);
        }
        else
        {
            LCI("socket closed.  socket error(or win rst). sID=" << _sessionID);
        }
        close();
        return 0 ;
    }
    _recving->len += received;
    SessionManager::getRef()._statInfo[STAT_RECV_COUNT]++;
    SessionManager::getRef()._statInfo[STAT_RECV_BYTES] += received;
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
                //do other something.

                //do other something end.
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
            
            unsigned int needEncry = received;
            if (_recving->len < (unsigned int)received)
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
            OnBlockCheckResult ret;
            try
            {
                ret = _options._onBlockCheck(_recving->begin + usedIndex, _recving->len - usedIndex, _recving->bound - usedIndex, _recving->bound);
            }
            catch (const std::exception & e)
            {
                LCW("MessageEntry _onBlockCheck catch one exception: " << e.what()  << ",  offset = " << usedIndex << ", len = " << _recving->len << ", bound = " << _recving->bound
                    << ", bindata(max 500byte) :"
                    << zsummer::log4z::Log4zBinary(_recving->begin+usedIndex, min(_recving->len - usedIndex, (unsigned int)500)));
                close();
                return 0;
            }
            catch (...)
            {
                LCW("MessageEntry _onBlockCheck catch one unknown exception.  offset=" << usedIndex << ",  len=" << _recving->len << ", bound=" << _recving->bound
                    << "bindata(max 500byte) :"
                    << zsummer::log4z::Log4zBinary(_recving->begin + usedIndex, min(_recving->len - usedIndex, (unsigned int)500)));
                close();
                return 0;
            }
            if (ret.first == BCT_CORRUPTION)
            {
                LCW("killed socket: _onBlockCheck error.  offset=" << usedIndex << ",  len=" << _recving->len << ", bound=" << _recving->bound
                    << "bindata(max 500byte) :"
                    << zsummer::log4z::Log4zBinary(_recving->begin + usedIndex, min(_recving->len - usedIndex, (unsigned int)500)));
                close();
                return 0;
            }
            if (ret.first == BCT_SHORTAGE)
            {
                break;
            }
            try
            {
                SessionManager::getRef()._statInfo[STAT_RECV_PACKS]++;
                LCT("TcpSession::onRecv _onBlockDispatch(sessionID=" << getSessionID() << ", offset=" << usedIndex
                    <<", len=" << ret.second);
                _options._onBlockDispatch(shared_from_this(), _recving->begin + usedIndex, ret.second);
            }
            catch (const std::exception & e)
            {
                LCW("MessageEntry _onBlockDispatch catch one exception: " << e.what() << ", bindata(max 500byte) :"
                    << zsummer::log4z::Log4zBinary(_recving->begin + usedIndex, min(ret.second, (unsigned int)500)));
            }
            catch (...)
            {
                LCW("MessageEntry _onBlockDispatch catch one unknown exception, bindata(max 500byte) :"
                    << zsummer::log4z::Log4zBinary(_recving->begin + usedIndex, min(ret.second, (unsigned int)500)));
            }
            usedIndex += ret.second;
        }
        else
        {
            std::string body;
            bool isFirstRead = _httpHeader.empty();
            OnBlockCheckResult ret;
            try
            {
                ret = _options._onHTTPBlockCheck(_recving->begin + usedIndex,
                    _recving->len - usedIndex,
                    _recving->bound - usedIndex,
                    _httpIsChunked, _httpMethod, _httpMethodLine, _httpHeader,
                    body);
            }
            catch (const std::exception & e)
            {
                LCW("MessageEntry _onHTTPBlockCheck catch one exception: " << e.what() << ", bindata(max 500byte) :"
                    << zsummer::log4z::Log4zBinary(_recving->begin + usedIndex, min(_recving->len - usedIndex, (unsigned int)500)));
                close();
                return 0;
            }
            catch (...)
            {
                LCW("MessageEntry _onHTTPBlockCheck catch one unknown exception, bindata(max 500byte) :"
                    << zsummer::log4z::Log4zBinary(_recving->begin + usedIndex, min(_recving->len - usedIndex, (unsigned int)500)));
                close();
                return 0;
            }


            if (ret.first == BCT_CORRUPTION)
            {
                LCE("killed http socket: _onHTTPBlockCheck error sID=" << _sessionID << ", bindata(max 500byte) :"
                    << zsummer::log4z::Log4zBinary(_recving->begin + usedIndex, min(_recving->len - usedIndex, (unsigned int)500)));
                close();
                return 0;
            }
            if (ret.first == BCT_SHORTAGE)
            {
                if (isFirstRead)
                {
                    _httpMethod.clear();
                    _httpMethodLine.clear();
                    _httpHeader.clear();
                    _httpIsChunked = false;
                }
                break;
            }


            SessionManager::getRef()._statInfo[STAT_RECV_PACKS]++;
            try
            {
                _options._onHTTPBlockDispatch(shared_from_this(), _httpMethod, _httpMethodLine, _httpHeader, body);

            }
            catch (const std::exception & e)
            {
                LCW("MessageEntry _onHTTPBlockDispatch catch one exception: " << e.what() << ", bindata(max 500byte) :"
                    << zsummer::log4z::Log4zBinary(_recving->begin + usedIndex, min(_recving->len - usedIndex, (unsigned int)500)));
            }
            catch (...)
            {
                LCW("MessageEntry _onHTTPBlockDispatch catch one unknown exception, bindata(max 500byte) :"
                    << zsummer::log4z::Log4zBinary(_recving->begin + usedIndex, min(_recving->len - usedIndex, (unsigned int)500)));
            }

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
# ifndef WIN32
    return _recving->len;
#else
    if (!doRecv())
    {
        close();
    }
    return 0;
#endif

}

void TcpSession::send(const char *buf, unsigned int len)
{
    LCT("TcpSession::send sessionID=" << getSessionID() << ", len=" << len);
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
            _sendque.pop_front();
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
        if (_sendque.size() >= _options._maxSendListCount)
        {
            close();
            return;
        }
        
        SessionBlock * sb = _options._createBlock();
        if (sb->bound < len)
        {
            _options._freeBlock(sb);
            LCE("send error.  too large block than session block.  len=" << len);
            return;
        }
        memcpy(sb->begin, buf, len);
        sb->len = len;
        _sendque.push_back(sb);
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
            LCW("send error ");
        }
    }
}


void TcpSession::onSend(zsummer::network::NetErrorCode ec, int sent)
{

    LCT("TcpSession::onSend session id=" << getSessionID() << ", sent=" << sent);
    if (ec)
    {
        LCD("remote socket closed");
        return ;
    }

    _sendingLen += sent;
    SessionManager::getRef()._statInfo[STAT_SEND_BYTES] += sent;
    if (_sendingLen < _sending->len)
    {
        SessionManager::getRef()._statInfo[STAT_SEND_COUNT]++;
        bool sendRet = _sockptr->doSend(_sending->begin + _sendingLen, _sending->len - _sendingLen, std::bind(&TcpSession::onSend, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
        if (!sendRet)
        {
            LCW("send error from onSend");
            return;
        }
        return;
    }
    if (_sendingLen == _sending->len)
    {
        _sendingLen = 0;
        _sending->len = 0;
        if (!_sendque.empty())
        {
            do
            {
                SessionBlock *sb = _sendque.front();
                _sendque.pop_front();
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
            if (_options._onReconnectEnd)
            {
                try
                {
                    _options._onReconnectEnd(shared_from_this());
                }
                catch (const std::exception & e)
                {
                    LCE("_options._onReconnectEnd catch excetion=" << e.what());
                }
                catch (...)
                {
                    LCE("_options._onReconnectEnd catch excetion");
                }
            }
            close();
        }
        else
        {
            reconnect();
            _reconnects++;
            _pulseTimerID = SessionManager::getRef().createTimer(_options._connectPulseInterval, std::bind(&TcpSession::onPulse, shared_from_this()));
        }
    }
    else if (_status == 2)
    {
        if (_options._onSessionPulse)
        {
            try
            {
                _options._onSessionPulse(shared_from_this());
            }
            catch (const std::exception & e)
            {
                LCW("TcpSession::onPulse catch one exception: " << e.what());
            }
            catch (...)
            {
                LCW("TcpSession::onPulse catch one unknown exception: ");
            }
        }
        _pulseTimerID = SessionManager::getRef().createTimer(isConnectID(_sessionID) ? _options._connectPulseInterval : _options._sessionPulseInterval, std::bind(&TcpSession::onPulse, shared_from_this()));
    }
}




TupleParam & TcpSession::autoTupleParamImpl(size_t index)
{
    if (index > 100)
    {
        LOGW("user param is too many. trace=" << zsummer::proto4z::proto4z_traceback());
    }
    if (_param.size() <= index)
    {
        _param.insert(_param.end(), index - _param.size() + 1, std::make_tuple(false, 0.0, 0, ""));
    }
    return _param[index];
}

const TupleParam & TcpSession::peekTupleParamImpl(size_t index) const
{
    const static TupleParam _invalid = std::make_tuple( false, 0.0, 0, "" );
    if (index > 100)
    {
        LOGW("user param is too many. trace=" << zsummer::proto4z::proto4z_traceback());
    }
    if (_param.size() <= index )
    {
        LOGW("get user param error. not inited. trace=" << zsummer::proto4z::proto4z_traceback());
        return _invalid;
    }
    return _param.at(index);
}
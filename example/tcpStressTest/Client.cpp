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

#include "Client.h"
using namespace zsummer::proto4z;

CClient::CClient(CProcess &proc, TcpSocketPtr sockptr):_process(proc)
{
    _sockptr = sockptr;
    memset(&_recving, 0, sizeof(_recving));
}

CClient::~CClient()
{
    while (!_sendque.empty())
    {
        delete _sendque.front();
        _sendque.pop();
    }
    LOGI("~CClient");
    
}

void CClient::initialize()
{
    
    if (g_startType == 0)
    {
        doRecv();
        _bEstablished = true;
        _process.AddTotalOpen(1);
    }
    else
    {
        _sockptr->doConnect(g_remoteIP, g_remotePort, std::bind(& CClient::onConnected, shared_from_this(), std::placeholders::_1));
    }    
}

void CClient::SendOnce()
{
    if (!_bEstablished)
    {
        LOGD("client is dead. no send again");
        return;
    }
    doSend(1, NOW_TIME, g_text);
    if (g_sendType != 0)
    {
        if (g_intervalMs > 0)
        {
            _process.GetZSummer()->createTimer(g_intervalMs, std::bind(&CClient::SendOnce, shared_from_this()));
        }
        else
        {
            if (_lastDelayTime < 2000)
            {
                _process.GetZSummer()->post(std::bind(&CClient::SendOnce, shared_from_this()));
            }
            else
            {
                _process.GetZSummer()->createTimer(100, std::bind(&CClient::SendOnce, shared_from_this()));
            }
        }
    }
}

void CClient::onConnected(zsummer::network::NetErrorCode ec)
{
    if (ec)
    {
        LOGD("onConnected failed. ec=" << ec);
        return;
    }
    LOGD("onConnected success");
    doRecv();
    _bEstablished = true;
    _process.AddTotalOpen(1);
    if (g_intervalMs > 0)
    {
        _process.GetZSummer()->createTimer(g_intervalMs, std::bind(&CClient::SendOnce, shared_from_this()));
    }
    else
    {
        SendOnce();
    }
}

void CClient::doRecv()
{
    _recving._len = 0;
    bool bSuccess = _sockptr->doRecv(_recving._orgdata, (int)2, std::bind(&CClient::onRecv, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
    if (!bSuccess)
    {
        onClose();
    }
}

unsigned int CClient::onRecv(zsummer::network::NetErrorCode ec, int nRecvedLen)
{
    if (ec)
    {
        LOGD("remote socket closed");
        onClose();
        return 0;
    }

    _recving._len += nRecvedLen;

    auto ret = zsummer::proto4z::checkBuffIntegrity(_recving._orgdata, _recving._len, _MSG_BUF_LEN, _MSG_BUF_LEN);
    if (ret.first == zsummer::proto4z::IRT_CORRUPTION)
    {
        LOGD("killed socket: checkBuffIntegrity error ");
        _sockptr->doClose();
        onClose();
        return 0;
    }
    if (ret.first == zsummer::proto4z::IRT_SHORTAGE)
    {
        _sockptr->doRecv(_recving._orgdata + _recving._len, ret.second, std::bind(&CClient::onRecv, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
        return 0;
    }

    //! 解包完成 进行消息处理
    zsummer::proto4z::ReadStream rs(_recving._orgdata, _recving._len);
    try
    {
        MessageEntry(rs);
    }
    catch (const std::exception & e)
    {
        LOGD("MessageEntry catch one exception: "<< e.what() );
        _sockptr->doClose();
        onClose();
        return 0;
    }
    _process.AddTotalRecvLen(_recving._len);
    _process.AddTotalRecvCount(1);
    //! 继续收包
    doRecv();
    return 0;
}

void CClient::MessageEntry(zsummer::proto4z::ReadStream & rs)
{
    //协议流异常会被上层捕获并关闭连接
    unsigned short protoID = rs.getProtoID();
    switch (protoID)
    {
    case 1:
        {
            unsigned long long clientTick = 0;
            _recvTextCache.clear();
            rs >> clientTick >> _recvTextCache;
            if (g_startType == 0)
            {
                doSend(protoID, clientTick, _recvTextCache);
            }
            else
            {
                unsigned long long tick = NOW_TIME;
                if (tick < clientTick)
                {
                    LOGE("now time[" << tick << "] < last time[" << clientTick << "].");
                    throw std::runtime_error("now time < last time");
                }
                tick -= clientTick;
                _lastDelayTime = tick;
                _process.AddTotalEcho(1);
                _process.AddTotalEchoTime(tick);
                if (g_sendType == 0)
                {
                    SendOnce();
                }
            }
        }
        break;
    default:
        {
            LOGI("unknown protocol id = " << protoID);
        }
        break;
    }
}



void CClient::doSend(unsigned short protoID, unsigned long long clientTick, const std::string& text)
{
    zsummer::proto4z::WriteStream ws(protoID);
    ws << clientTick << text;
    doSend(ws.getStream(), ws.getStreamLen());
}

void CClient::doSend(char *buf, unsigned short len)
{
    if (_sendLen != 0)
    {
        Packet *pack = new Packet;
        memcpy(pack->_orgdata, buf, len);
        pack->_len = len;
        _sendque.push(pack);
    }
    else
    {
        memcpy(_sendBuff, buf, len);
        _sendLen = len;
        _curSendLen = 0;
        _sockptr->doSend(_sendBuff, _sendLen, std::bind(&CClient::onSend, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
    }
}


void CClient::onSend(zsummer::network::NetErrorCode ec,  int nSentLen)
{
    if (ec)
    {
        LOGD("remote socket closed");
        return ;
    }
    _curSendLen += nSentLen;
    if (_curSendLen < _sendLen)
    {
        _sockptr->doSend(_sendBuff+_curSendLen, _sendLen - _curSendLen, std::bind(&CClient::onSend, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
    }
    else if (_curSendLen == _sendLen)
    {
        _process.AddTotalSendCount(1);
        _process.AddTotalSendLen(_curSendLen);
        _curSendLen = 0;
        _sendLen = 0;
        if (!_sendque.empty())
        {
            do
            {
                Packet *pack = _sendque.front();
                if (_SEND_BUF_LEN - _sendLen < pack->_len)
                {
                    break;
                }
                _sendque.pop();
                memcpy(_sendBuff+_sendLen, pack->_orgdata, pack->_len);
                _sendLen += pack->_len;
                delete pack;

            } while (!_sendque.empty());
            
            _sockptr->doSend(_sendBuff, _sendLen, std::bind(&CClient::onSend, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
        }
    }
}

void CClient::onClose()
{
    LOGI("Client Closed!");
    _process.AddTotalClosed(1);
    _bEstablished = false;
}


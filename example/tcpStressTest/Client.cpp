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
#ifndef WIN32
        _sockptr->setFloodSendOptimize(false);
#endif
        doRecv();
        _bEstablished = true;
        _process._nTotalOpen++;
    }
    else
    {
        _sockptr->doConnect(g_remoteIP, g_remotePort, std::bind(& CClient::onConnected, shared_from_this(), std::placeholders::_1));
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
#ifndef WIN32
    _sockptr->setFloodSendOptimize(false);
#endif
    
    doRecv();
    _bEstablished = true;
    _process._nTotalOpen++;
    zsummer::proto4z::WriteStream ws(1);
    ws << g_text;
    doSend(ws.getStream(), ws.getStreamLen());
}

void CClient::doRecv()
{
    bool bSuccess = _sockptr->doRecv(_recving._orgdata + _recving._offset, _MSG_BUF_LEN - _recving._offset, std::bind(&CClient::onRecv, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
    if (!bSuccess)
    {
        onClose();
    }
}

unsigned int CClient::onRecv(zsummer::network::NetErrorCode ec, int nRecvedLen)
{
    if (ec)
    {
        LOGW("remote socket closed");
        onClose();
        return 0;
    }
    _process._nTotalRecvCount++;
    _recving._offset += nRecvedLen;
    unsigned int readed = 0;
    while (true)
    {
        auto ret = zsummer::proto4z::checkBuffIntegrity(_recving._orgdata + readed, _recving._offset - readed, _MSG_BUF_LEN - readed, _MSG_BUF_LEN - readed);
        if (ret.first == zsummer::proto4z::IRT_CORRUPTION)
        {
            LOGE("killed socket: checkBuffIntegrity error readed =" << readed << ", len=" << _recving._offset << ", _MSG_BUF_LEN=" << _MSG_BUF_LEN << ", nRecvedLen=" << nRecvedLen);
            _sockptr->doClose();
            onClose();
            return 0;
        }
        if (ret.first == zsummer::proto4z::IRT_SHORTAGE)
        {
            break;
        }

        //! 解包完成 进行消息处理
        zsummer::proto4z::ReadStream rs(_recving._orgdata + readed, ret.second);
        try
        {
            MessageEntry(rs);
        }
        catch (const std::exception & e)
        {
            LOGD("MessageEntry catch one exception: " << e.what());
            _sockptr->doClose();
            onClose();
            return 0;
        }
        readed += ret.second;
        _process._nTotalRecvPacket++;
        _process._nTotalRecvLen += ret.second;
    }
    if (readed < _recving._offset)
    {
        memmove(_recving._orgdata, _recving._orgdata + readed, _recving._offset - readed);
        _recving._offset -= readed;
    }
    else
    {
        _recving._offset = 0;
    }
    //! 继续收包
    doRecv();
    return 0;
}

void CClient::MessageEntry(zsummer::proto4z::ReadStream & rs)
{
    doSend(rs.getStream(), rs.getStreamLen());
}


void CClient::doSend(const char *buf, unsigned int len)
{
    if (_needSendLen != 0)
    {
        Packet *pack = new Packet;
        memcpy(pack->_orgdata, buf, len);
        pack->_offset = len;
        _sendque.push(pack);
    }
    else
    {
        memcpy(_sending._orgdata, buf, len);
        _sending._offset = 0;
        _needSendLen = len;
        _sockptr->doSend(_sending._orgdata, len, std::bind(&CClient::onSend, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
        _process._nTotalSendPacket++;
    }
}


void CClient::onSend(zsummer::network::NetErrorCode ec,  int nSentLen)
{
    if (ec)
    {
        LOGD("remote socket closed");
        return ;
    }
    _process._nTotalSendCount++;
    _process._nTotalSendLen += nSentLen;
    _sending._offset += nSentLen;
    if (_sending._offset < _needSendLen)
    {
        _sockptr->doSend(_sending._orgdata + _sending._offset, _needSendLen - _sending._offset, std::bind(&CClient::onSend, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
    }
    else if (_sending._offset == _needSendLen)
    {
        
        _sending._offset = 0;
        _needSendLen = 0;
        if (!_sendque.empty())
        {
            do
            {
                Packet *pack = _sendque.front();
                if (_MSG_BUF_LEN - _sending._offset < pack->_offset)
                {
                    break;
                }
                _sendque.pop();
                memcpy(_recving._orgdata+_recving._offset, pack->_orgdata, pack->_offset);
                _needSendLen += pack->_offset;
                delete pack;
                _process._nTotalSendPacket++;
            } while (!_sendque.empty());
            
            _sockptr->doSend(_sending._orgdata + _sending._offset, _needSendLen - _sending._offset, std::bind(&CClient::onSend, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
        }
    }
}

void CClient::onClose()
{
    LOGI("Client Closed!");
    _process._nTotalClosed++;
    _bEstablished = false;
}


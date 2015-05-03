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

#include <zsummerX/frame/config.h>
#include <zsummerX/frame/dispatch.h>

using namespace zsummer::proto4z;
using namespace zsummer::network;

MessageDispatcher & MessageDispatcher::getRef()
{
	static MessageDispatcher _manager;
	return _manager;
}


bool MessageDispatcher::dispatchPreSessionMessage(TcpSessionPtr session, const char * blockBegin, zsummer::proto4z::Integer blockSize)
{
    if (_vctPreSessionDispatch.empty())
    {
        return true;
    }
    
    try
    {
        for (auto & fun : _vctPreSessionDispatch)
        {
            LCT("Entry dispatchOrgSessionMessage  SessionID=" << session->getSessionID() << ", blockSize=" << blockSize);
            if (!fun(session, blockBegin, blockSize))
            {
                return false;
            }
        }
    }
    catch (std::runtime_error e)
    {
        LCE("Leave dispatchOrgSessionMessage With Runtime Error:  SessionID=" << session->getSessionID() << ", Error Message=\"" << e.what() << "\"");
        return false;
    }
    catch (...)
    {
        LCE("Leave dispatchOrgSessionMessage With Unknown Runtime Error: SessionID=" << session->getSessionID());
        return false;
    }
    return true;
}




void MessageDispatcher::dispatchSessionMessage(TcpSessionPtr  session, ProtoID pID, zsummer::proto4z::ReadStream & msg)
{
    MapProtoDispatch::iterator iter = _mapSessionDispatch.find(pID);
    if ((iter == _mapSessionDispatch.end() || iter->second.empty()) && _vctDefaultSessionDispatch.empty())
    {
        LCW("Entry dispatchSessionMessage[" << pID << "] Failed: UNKNOWN ProtoID.  SessionID=" << session->getSessionID() << ", ProtoID=" << pID);
        return;
    }
    try
    {
        if (iter != _mapSessionDispatch.end() && !iter->second.empty())
        {
            for (auto & fun : iter->second)
            {
                LCT("Entry dispatchSessionMessage[" << pID << "]  SessionID=" << session->getSessionID());
                msg.resetMoveCursor();
                fun(session,  pID, msg);
            }
        }
        else
        {
            for (auto & fun : _vctDefaultSessionDispatch)
            {
                msg.resetMoveCursor();
                fun(session, pID, msg);
            }
        }
        LCT("Leave dispatchSessionMessage[" << pID << "]  SessionID=" << session->getSessionID());
    }
    catch (std::runtime_error e)
    {
        LCE("Leave dispatchSessionMessage[" << pID << "] With Runtime Error: SessionID=" << session->getSessionID() << ", rsLen=" << msg.getStreamLen() << ", Error Message=\"" << e.what() << "\"");
    }
    catch (...)
    {
        LCE("Leave dispatchSessionMessage[" << pID << "] With Unknown Runtime Error: SessionID=" << session->getSessionID());
    }
}


void MessageDispatcher::dispatchOnSessionEstablished(TcpSessionPtr  session)
{
    if (_vctOnSessionEstablished.empty())
    {
        return;
    }
    for (auto &fun : _vctOnSessionEstablished)
    {
        try
        {
            LCT("Entry dispatchOnSessionEstablished SessionID=" << session->getSessionID());
            fun(session);
            LCT("Leave dispatchOnSessionEstablished SessionID=" << session->getSessionID());
        }
        
        catch (std::runtime_error e)
        {
            LCE("Leave dispatchOnSessionEstablished Runtime Error: SessionID=" << session->getSessionID() << ", Error Message=\"" << e.what() << "\"");
        }
        catch (...)
        {
            LCE("Leave dispatchOnSessionEstablished Unknown Runtime Error: SessionID=" << session->getSessionID());
        }
    }
}

void MessageDispatcher::dispatchOnSessionDisconnect(TcpSessionPtr  session)
{
    if (_vctOnSessionDisconnect.empty())
    {
        return;
    }
    for (auto & fun : _vctOnSessionDisconnect)
    {
        try
        {
            LCT("Entry dispatchOnSessionDisconnect SessionID=" << session->getSessionID());
            fun(session);
            LCT("Leave dispatchOnSessionDisconnect SessionID=" << session->getSessionID());
        }
        catch (std::runtime_error e)
        {
            LCE("Leave dispatchOnSessionDisconnect Runtime Error: SessionID=" << session->getSessionID() << ", Error Message=\"" << e.what() << "\"");
        }
        catch (...)
        {
            LCE("Leave dispatchOnSessionDisconnect Unknown Runtime Error: SessionID=" << session->getSessionID());
        }
    }
}


void MessageDispatcher::dispatchOnSessionPulse(TcpSessionPtr  session, unsigned int pulseInterval)
{
    if (_vctOnSessionPulse.empty())
    {
        return;
    }
    for (auto & fun : _vctOnSessionPulse)
    {
        try
        {
            LCT("Entry dispatchOnSessionPulse SessionID=" << session->getSessionID());
            fun(session, pulseInterval);
            LCT("Leave dispatchOnSessionPulse SessionID=" << session->getSessionID());
        }
        catch (std::runtime_error e)
        {
            LCE("Leave dispatchOnSessionPulse Runtime Error: SessionID=" << session->getSessionID() << ", Error Message=\"" << e.what() << "\"");
        }
        catch (...)
        {
            LCE("Leave dispatchOnSessionPulse Unknown Runtime Error: SessionID=" << session->getSessionID());
        }
    }
}


bool  MessageDispatcher::dispatchSessionHTTPMessage(TcpSessionPtr session, const zsummer::proto4z::PairString & commonLine, const zsummer::proto4z::HTTPHeadMap &head, const std::string & body)
{
    try
    {
        for (auto & fun : _vctSessionHTTPMessage)
        {
            LCT("Entry dispatchSessionHTTPMessage  SessionID=" << session->getSessionID() << ", commond=" << commonLine.first << ", commondValue=" << commonLine.second
                << ", head count=" << head.size() << ", bodySize=" << body.length());
            fun(session, commonLine, head, body);
            LCT("Leave dispatchSessionHTTPMessage  SessionID=" << session->getSessionID());
        }
        return true;
    }
    catch (std::runtime_error e)
    {
        LCE("Leave dispatchSessionHTTPMessage With Runtime Error: SessionID=" << session->getSessionID() << ", Error Message=\"" << e.what() << "\"");
    }
    catch (...)
    {
        LCE("Leave dispatchSessionHTTPMessage With Unknown Runtime Error: SessionID=" << session->getSessionID());
    }
    return false;
}

















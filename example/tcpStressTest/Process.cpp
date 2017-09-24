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

#include "Process.h"
#include "Client.h"

CProcess::CProcess() :_nTotalRecvLen(0), _nTotalSendLen(0), _nTotalRecvCount(0), _nTotalSendCount(0), _nTotalRecvPacket(0), _nTotalSendPacket(0), _nTotalOpen(0), _nTotalClosed(0)
{
    _summer = zsummer::network::EventLoopPtr(new zsummer::network::EventLoop());


}

bool CProcess::start()
{
    if (!_summer->initialize())
    {
        return false;
    }
    
    std::thread t(std::bind(&CProcess::run, this));
    _thread.swap(t);
    return true;
}

void CProcess::stop()
{
    _running = false;
}

void CProcess::run()
{
    _running = true;
    while (_running)
    {
        _summer->runOnce();
    }
}


void CProcess::RecvSocketPtr(std::shared_ptr<zsummer::network::TcpSocket> sockptr)
{
    std::shared_ptr<CClient> client(new CClient(*this, sockptr));
    client->initialize();
}


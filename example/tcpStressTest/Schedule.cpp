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


#include "Schedule.h"

#include "Process.h"

CSchedule::CSchedule()
{
    _summer = zsummer::network::EventLoopPtr(new zsummer::network::EventLoop());
    _accept = zsummer::network::TcpAcceptPtr(new zsummer::network::TcpAccept());
    _accept->initialize(_summer);
}


void CSchedule::start()
{

    if (!_summer->initialize())
    {
        return ;
    }

    for (unsigned int i=0; i< g_multiThread; i++)
    {
        CProcess * p = new CProcess();
        if (p->start())
        {
            _process.push_back(p);
        }
    }
    
    if (g_startType == 0)
    {
        if (_accept->openAccept(g_remoteIP.c_str(), g_remotePort))
        {
            LOGI("open server port [" << g_remotePort << "] success");
        }
        TcpSocketPtr s(new zsummer::network::TcpSocket());
        
        _accept->doAccept(s, std::bind(&CSchedule::OnAccept, this, std::placeholders::_1, std::placeholders::_2));
    }
    else
    {
        doConnect(g_maxClient);
    }


    _thread = std::thread(std::bind(&CSchedule::run, this));
}
void CSchedule::stop()
{
    _running = false;
}
void CSchedule::run()
{
    _running = true;

    while (_running)
    {
        _summer->runOnce();
    }
}

void CSchedule::doConnect(unsigned int maxClient)
{
    for (unsigned int i = 1; i <= maxClient; i++)
    {
        TcpSocketPtr s(new zsummer::network::TcpSocket());
        CProcess * proc = _process[_iCurProcess];
        s->initialize(proc->GetZSummer());
        proc->post(std::bind(&CProcess::RecvSocketPtr, proc, s));
        _iCurProcess++;
        _iCurProcess = _iCurProcess%(int)_process.size();
        if (i >= 100 && maxClient > 100)
        {
            _summer->createTimer(1000, std::bind(&CSchedule::doConnect, this, maxClient - i));
            break;
        }
    }
}
void CSchedule::OnAccept(zsummer::network::NetErrorCode ec, TcpSocketPtr sockptr)
{
    if (ec)
    {
        LOGE("OnAccept error. ec=" <<ec );
        return;
    }
    

    LOGD("OnAccept one socket");
    _iCurProcess++;
    _iCurProcess = _iCurProcess % (int)_process.size();
    CProcess * process = _process[_iCurProcess];
    sockptr->initialize(process->GetZSummer());
    process->post(std::bind(&CProcess::RecvSocketPtr, process, sockptr));


    TcpSocketPtr s(new zsummer::network::TcpSocket());
    _accept->doAccept(s, std::bind(&CSchedule::OnAccept, this, std::placeholders::_1, std::placeholders::_2));
}


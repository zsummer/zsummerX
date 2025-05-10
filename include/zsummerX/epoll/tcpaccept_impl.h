﻿/*
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
* Copyright (C) 2010-2017 YaweiZhang <yawei.zhang@foxmail.com>.
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

#if !defined(ZSUMMERX_TCPACCEPT_IMPL_H_) && !defined(WIN32) && !defined(__APPLE__) && !defined(USE_SELECT_IMPL)
#define ZSUMMERX_TCPACCEPT_IMPL_H_

#include "common_impl.h"
#include "epoll_impl.h"
#include "tcpsocket_impl.h"

namespace zsummer
{
    namespace network
    {
        class TcpAccept : public std::enable_shared_from_this<TcpAccept>
        {
        public:
            TcpAccept();
            ~TcpAccept();
            bool initialize(const EventLoopPtr &summer);
            bool openAccept(const std::string & listenIP, unsigned short listenPort, bool reuse = true);
            bool doAccept(const TcpSocketPtr &s, _OnAcceptHandler &&handle);
            void onEPOLLMessage(uint32_t event);
            bool close();

        private:
            std::string logSection();
        private:
            EventLoopPtr         _summer;
            std::string        _listenIP;
            short            _listenPort = 0;
            bool _isIPV6 = false;


            EventData _eventData;
            _OnAcceptHandler _onAcceptHandler;
            TcpSocketPtr  _client;

        };
        using TcpAcceptPtr = std::shared_ptr<TcpAccept>;
    }
}


#endif


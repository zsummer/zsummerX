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




#ifndef ZSUMMER_TCP_SESSION_H_
#define ZSUMMER_TCP_SESSION_H_
#include "config.h"
#include <rc4/rc4_encryption.h>

namespace zsummer
{
    namespace network
    {
        class TcpSession : public std::enable_shared_from_this<TcpSession>
        {
        public:
            TcpSession();
            ~TcpSession();
            void send(const char *buf, unsigned int len);
            void close();
            void connect();
            void reconnect();
            bool bindTcpSocketPrt(const TcpSocketPtr &sockptr, AccepterID aID, SessionID sID);
            inline SessionTraits & getTraits(){ return _traits; }
        private:
            void cleanSession(bool isCleanAllData, const std::string &rc4TcpEncryption);

            bool doRecv();

            void onConnected(zsummer::network::NetErrorCode ec);

            void onRecv(zsummer::network::NetErrorCode ec, int nRecvedLen);

            void onSend(zsummer::network::NetErrorCode ec, int nSentLen);

            void onPulseTimer();

            void onClose();

        public:
            inline void setEventLoopPtr(EventLoopPtr el){ _eventLoop = el; }
            inline SessionID getAcceptID(){ return _acceptID; }
            inline SessionID getSessionID(){ return _sessionID; }
            inline void setSessionID(SessionID sID){ _sessionID = sID; }
            inline bool isInvalidSession(){ return !_sockptr; }
            inline const std::string & getRemoteIP(){ return _remoteIP; }
            inline void setRemoteIP(const std::string &remoteIP){ _remoteIP = remoteIP; }
            inline unsigned short getRemotePort(){ return _remotePort; }
            inline void setRemotePort(unsigned short remotePort){ _remotePort = remotePort; }
            Any setUserParam(int index, const Any &any);
            Any getUserParam(int index);
        private:
            SessionTraits _traits;
            EventLoopPtr _eventLoop;
            TcpSocketPtr  _sockptr;
            std::string _remoteIP;
            unsigned short _remotePort = 0;
            int _status = 0; //0 uninit, 1 connecting, 2 session established, 3  died
   
            //
            SessionID _sessionID = InvalidSeesionID;
            AccepterID _acceptID = InvalidAccepterID;
            zsummer::network::TimerID _pulseTimerID = zsummer::network::InvalidTimerID;

            //! 
            SessionBlock* _recving = nullptr;
            SessionBlock* _sending = nullptr;
            unsigned int _sendingCurIndex = 0;

            //! send data queue
            std::queue<SessionBlock *> _sendque;
            unsigned long long _totalConnectCount = 0;
            unsigned long long _curReconnectCount = 0;

            //! rc encrypt
            RC4Encryption _rc4StateRead;
            RC4Encryption _rc4StateWrite;

            //!
            bool _bFirstRecvData = true;


            //! http status data
            bool _httpHadHeader = false;
            bool _httpIsChunked = false;
            PairString  _httpCommonLine;
            MapString _httpHeader;
            
            //! user param
            std::vector<Any> _param;
            

        };
        using TcpSessionPtr = std::shared_ptr<TcpSession>;
    }
}


#endif



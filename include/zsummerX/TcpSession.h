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
 * Copyright (C) 2010-2014 YaweiZhang <yawei_zhang@foxmail.com>.
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
#include "FrameHeader.h"


struct tagMSGPack
{
	tagMSGPack(unsigned int l){ bufflen = l; }
	char buff[MSG_BUFF_MAX_LEN];
	unsigned int bufflen;
};



class CTcpSession : public std::enable_shared_from_this<CTcpSession>
{
public:
	CTcpSession();
	~CTcpSession();
	void BindTcpSocketPrt(CTcpSocketPtr sockptr, SessionID sessionID, bool isConnector, AccepterID aID=0);
	
	void DoConnect(const tagConnctorConfigTraits & traits);
	void DoRecv();
	void DoSend(const char *buf, unsigned int len);
	void Close();
private:
	void CleanSession(bool isCleanWithoutMsgPack = true);

	void OnConnected(zsummer::network::ErrorCode ec, const tagConnctorConfigTraits & traits);

	
	void OnRecv(zsummer::network::ErrorCode ec, int nRecvedLen);

	
	void OnSend(zsummer::network::ErrorCode ec,  int nSentLen);

	
	void OnClose();


	CTcpSocketPtr  m_sockptr;


	SessionID m_sessionID;
	bool m_isConnector = false;
	AccepterID m_acceptID = 0;
	
	//! 读包
	tagMSGPack m_recving = {0};


	tagMSGPack m_sending = { 0 };
	unsigned int m_sendingCurIndex = 0;
	//! 写包队列
	std::queue<tagMSGPack *> m_sendque;
	std::queue<tagMSGPack*> m_freeCache;
};

#endif



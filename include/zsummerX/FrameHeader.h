/*
 * ZSUMMER License
 * -----------
 * 
 * ZSUMMER is licensed under the terms of the MIT license reproduced below.
 * This means that ZSUMMER is free software and can be used for both academic
 * and commercial purposes at absolutely no cost.
 * 
 * 
 * ===============================================================================
 * 
 * Copyright (C) 2010-2013 YaweiZhang <yawei_zhang@foxmail.com>.
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

//! zsummer的测试服务模块(对应zsummer底层网络封装的上层设计测试服务) 可视为服务端架构中的 gateway服务/agent服务/前端服务, 特点是高并发高吞吐量
//! 公共头文件

#ifndef ZSUMMER_HEADER_H_
#define ZSUMMER_HEADER_H_

#include <zsummerX/zsummer.h>
#include <zsummerX/tcpaccept.h>
#include <zsummerX/tcpsocket.h>
#include <zsummerX/udpsocket.h>

#include <iostream>
#include <queue>
#include <iomanip>
#include <string.h>
#include <signal.h>
#include <log4z/log4z.h>
#include <protocol4z/protocol4z.h>
using namespace std;

//服务端内部底层公共结构

//监听器配置
struct tagAcceptorConfigTraits
{
	char listenIP[20];
	unsigned short listenPort;
	unsigned int maxSessions;
};

//连接器配置
struct tagConnctorConfigTraits
{
	char remoteIP[20];
	unsigned short remotePort;
	bool reconnect;
	unsigned int reconnectInterval;//million seconds;
	unsigned int reconnectMaxCount; // try count
};



//类型定义
//----------------------------------------
class CTcpSession;
class CWorkManager;

typedef std::shared_ptr<zsummer::network::CTcpSocket> CTcpSocketPtr;
typedef std::shared_ptr<zsummer::network::CTcpAccept> CTcpAcceptPtr;
typedef std::shared_ptr<zsummer::network::CZSummer> CZSummerPtr;
typedef std::shared_ptr<CTcpSession> CTcpSessionPtr;
typedef std::shared_ptr<CWorkManager> CWorkManagerPtr;


typedef unsigned int SessionID;
typedef unsigned int AccepterID;

typedef unsigned int ProtocolID;







//public method
#define  NOW_TIME (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count())

#define MSG_BUFF_MAX_LEN 64*1024 //底层通讯最大协议长度

struct FrameStreamTraits
{
	typedef unsigned int Integer; //User-Defined Integer Type must in [unsigned char, unsigned short, unsigned int, unsigned long long].
	const static Integer PreOffset = 0; //User-Defined 
	const static Integer PostOffset = 0; //User-Defined 
	const static Integer MaxPackLen = (Integer)MSG_BUFF_MAX_LEN; //User-Defined. example:  Integer = unsigned short(-1) ==>(65535)
	const static bool	 PackLenIsContainHead = true; //User-Defined 
	const static zsummer::protocol4z::ZSummer_EndianType EndianType = zsummer::protocol4z::LittleEndian;//User-Defined 
	const static Integer IntegerTypeSize = sizeof(Integer); // Don't Touch. PackLenSize and sizeof(Integer) must be equal. 
	const static Integer HeadLen = PreOffset + IntegerTypeSize + PostOffset; //Don't Touch. Head Length.
};

typedef zsummer::protocol4z::ReadStream<FrameStreamTraits> ReadStreamPack;
typedef zsummer::protocol4z::WriteStream<FrameStreamTraits> WriteStreamPack;

typedef std::function < void(SessionID, ProtocolID, ReadStreamPack &) > OnMessageFunction;

typedef std::function < void(SessionID) > OnSessionEstablished;
typedef std::function < void(SessionID) > OnConnectorEstablished;

typedef std::function < void(SessionID) > OnSessionDisconnect;
typedef std::function < void(SessionID) > OnConnectorDisconnect;







#endif


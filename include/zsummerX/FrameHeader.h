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

//! frame封装在网络部分使用单例模式, 如果需要使用多线程 需要在业务层由用户开辟线程池处理, 并需要配合CTcpSessionManager的Post接口.
//! 如果需要在zsummerX的网络部分使用多线程 请参考tcpTest实例调用zsummerX的原始接口实现.


//! ID类型和无效状态数值定义
typedef unsigned int SessionID;
const SessionID InvalidSeesionID = -1;
typedef unsigned int AccepterID;
const AccepterID InvalidAccepterID = -1;
typedef unsigned int ConnectorID;
const ConnectorID InvalidConnectorID = -1;
typedef unsigned short ProtocolID;
const ProtocolID InvalidProtocolID = -1;



//! 监听器配置
struct tagAcceptorConfigTraits
{
	AccepterID aID = InvalidAccepterID;
	std::string listenIP = "0.0.0.0";
	unsigned short listenPort = 81;
	unsigned int maxSessions = 5000;
	std::vector<std::string> whitelistIP;
};



//连接器配置
struct tagConnctorConfigTraits
{
	ConnectorID cID = InvalidConnectorID;
	std::string remoteIP = "127.0.0.1";
	unsigned short remotePort = 81;
	unsigned int reconnectMaxCount = 0; // try reconnect max count
	unsigned int reconnectInterval =5000; //million seconds;
	bool         reconnectCleanAllData = true ;//clean all data when reconnect;
	unsigned int curReconnectCount = 0; // interval use
};




//类型定义
//----------------------------------------
class CTcpSession;

typedef std::shared_ptr<zsummer::network::CTcpSocket> CTcpSocketPtr;
typedef std::shared_ptr<zsummer::network::CTcpAccept> CTcpAcceptPtr;
typedef std::shared_ptr<zsummer::network::CZSummer> CZSummerPtr;
typedef std::shared_ptr<CTcpSession> CTcpSessionPtr;


//public method
#define  NOW_TIME (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count())

//接收包缓冲大小
const unsigned int SEND_RECV_CHUNK_SIZE = 64 * 1024 -1;



//心跳间隔
const unsigned int HEARTBEART_INTERVAL = 30000; //毫秒


//包头特性
struct FrameStreamTraits
{
	typedef unsigned short Integer;
	const static Integer PreOffset = 0;  
	const static Integer PostOffset = 0;  
	const static Integer MaxPackLen = (Integer)16*1024; 
	const static bool	 PackLenIsContainHead = true; 
	const static zsummer::protocol4z::ZSummer_EndianType EndianType = zsummer::protocol4z::LittleEndian; 
	const static Integer IntegerTypeSize = sizeof(Integer);
	const static Integer HeadLen = PreOffset + IntegerTypeSize + PostOffset; 
};




typedef zsummer::protocol4z::ReadStream<FrameStreamTraits> ReadStreamPack;
typedef zsummer::protocol4z::WriteStream<FrameStreamTraits> WriteStreamPack;

typedef std::function < void(AccepterID, SessionID, ProtocolID, ReadStreamPack &) > OnSessionMessageFunction;
typedef std::function < void(ConnectorID, ProtocolID, ReadStreamPack &) > OnConnectorMessageFunction;


typedef std::function < void(AccepterID, SessionID) > OnSessionEstablished;
typedef std::function < void(ConnectorID) > OnConnectorEstablished;

typedef std::function < void(AccepterID, SessionID) > OnSessionDisconnect;
typedef std::function < void(ConnectorID) > OnConnectorDisconnect;

typedef std::function < void(AccepterID, SessionID) > OnMySessionHeartbeatTimer;
typedef std::function < void(ConnectorID) > OnMyConnectorHeartbeatTimer;


//! print log
template<class OS>
OS & operator <<(OS & os, const tagAcceptorConfigTraits & traits)
{
	os << "[AccepterID=" << traits.aID << "; listenIP=" << traits.listenIP << "; listenPort=" << traits.listenPort
		<< "; maxSessions=" << traits.maxSessions << "; whitelistIP=";
	for (auto x : traits.whitelistIP)
	{
		os << x << ",";
	}
	os << "]";
	return os;
}

//! print log
template<class OS>
OS & operator <<(OS & os, const tagConnctorConfigTraits & traits)
{
	os << "[ConnectorID=" << traits.cID << "; remoteIP=" << traits.remoteIP << "; remotePort=" << traits.remotePort
		<< "; reconnectMaxCount=" << traits.reconnectMaxCount << "; reconnectInterval=" << traits.reconnectInterval;
	return os;
}

#endif


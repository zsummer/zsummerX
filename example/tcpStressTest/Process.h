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


//! zsummer的测试服务模块(对应zsummer底层网络封装的上层设计测试服务) 可视为服务端架构中的 gateway服务/agent服务/前端服务, 特点是高并发高吞吐量
//! IOServer头文件 该类负责处理Schedule抛过来的client socket. 每个IOServer独立一个线程.

#ifndef ZSUMMER_PROCESS_H_
#define ZSUMMER_PROCESS_H_

#include "header.h"

class CProcess
{
public:
	CProcess();
	//! 启动与停止
	bool start();
	void stop();
	//! 线程
	void run();
	//!
	inline zsummer::network::ZSummerPtr GetZSummer(){return _summer;}
private:
	zsummer::network::ZSummerPtr _summer;
	std::thread	 _thread;
	bool  _running = true;


public:
	template<class H>
	void post(const H &h)
	{
		_summer->post(h);
	}
	//接收一个socket
	void RecvSocketPtr(std::shared_ptr<zsummer::network::TcpSocket> sockptr);

	//all use
public:
	inline unsigned long long GetTotalRecvLen() {return _nTotalRecvLen;}
	inline void AddTotalRecvLen(unsigned long long len) { _nTotalRecvLen += len;}

	inline unsigned long long GetTotalSendLen() {return _nTotalSendLen;}
	inline void AddTotalSendLen(unsigned long long len) { _nTotalSendLen += len;}

	inline unsigned long long GetTotalRecvCount() {return _nTotalRecvCount;}
	inline void AddTotalRecvCount(unsigned long long len) { _nTotalRecvCount += len;}

	inline unsigned long long GetTotalSendCount() {return _nTotalSendCount;}
	inline void AddTotalSendCount(unsigned long long len) { _nTotalSendCount += len;}
		
	inline unsigned long long GetTotalOpen() {return _nTotalOpen;}
	inline void AddTotalOpen(unsigned long long len) { _nTotalOpen += len;}
	inline unsigned long long GetTotalClosed() {return _nTotalClosed;}
	inline void AddTotalClosed(unsigned long long len) { _nTotalClosed += len;}
private:
	unsigned long long  _nTotalRecvLen = 0;
	unsigned long long  _nTotalSendLen = 0;
	unsigned long long  _nTotalRecvCount = 0;
	unsigned long long  _nTotalSendCount = 0;
	unsigned long long  _nTotalOpen = 0;
	unsigned long long  _nTotalClosed = 0;
	


	//only connects use
public:
	inline unsigned long long GetTotalEcho() {return _nTotalEcho;}
	inline void AddTotalEcho(unsigned long long len) { _nTotalEcho += len;}

	inline unsigned long long GetTotalEchoTime() {return _nTotalEchoTime;}
	inline void AddTotalEchoTime(unsigned long long len) { _nTotalEchoTime += len;}
private:
	unsigned long long _nTotalEcho = 0;
	unsigned long long _nTotalEchoTime = 0;
	




};

#endif
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
#include "../../network/tcpaccept.h"
#include "../../network/zsummer.h"
#include "../../network/tcpsocket.h"
#include "../../network/udpsocket.h"
#include <iostream>
#include <queue>
#include <iomanip>
#include <string.h>
#include <signal.h>
#include <log4z/log4z.h>
#include <protocol4z/protocol4z.h>
using namespace std;



//! 消息包缓冲区大小
#define _MSG_BUF_LEN	(2*1024)


//! 消息包 
struct Packet
{
	unsigned short _len;
	char		   _orgdata[_MSG_BUF_LEN];
};
typedef std::shared_ptr<Packet> PacketPtr;
typedef std::shared_ptr<zsummer::network::CTcpSocket> CTcpSocketPtr;


//! 服务端总连入连接数和总关闭连接数
extern int g_nTotalLinked;



#endif


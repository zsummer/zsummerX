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

//! zsummer的测试服务模块(对应zsummer底层网络封装的上层设计测试服务) 可视为服务端架构中的 gateway服务/agent服务/前端服务, 特点是高并发高吞吐量
//! 公共头文件

#ifndef ZSUMMER_HEADER_H_
#define ZSUMMER_HEADER_H_

#include <zsummerX/zsummerX.h>

#include <iostream>
#include <queue>
#include <iomanip>
#include <string.h>
#include <atomic>
#include <signal.h>
#include <log4z/log4z.h>
#include <proto4z/proto4z.h>
using namespace std;



//! 消息包缓冲区大小
#define _MSG_BUF_LEN    (20*1024)


//! 消息包 
struct Packet
{
    unsigned int _offset = 0;
    char           _orgdata[_MSG_BUF_LEN];
};

using PacketPtr = std::shared_ptr<Packet>;
using TcpSocketPtr = zsummer::network::TcpSocketPtr;

static std::string g_text = "zsummerX:test text.........  zsummerX:test text.........  " //64
                      "zsummerX:test text.........  zsummerX:test text.........  " //
                      "zsummerX:test text.........  zsummerX:test text.........  " //
                      "zsummerX:test text.........  zsummerX:test text.........  " //
                      "zsummerX:test text.........  zsummerX:test text.........  " //
                      "zsummerX:test text.........  zsummerX:test text.........  " //
                      "zsummerX:test text.........  zsummerX:test text.........  " //
                      "zsummerX:test text.........  zsummerX:test text.........  " // *8
                      "zsummerX:test text.........  zsummerX:test text.........  " //
                      "zsummerX:test text.........  zsummerX:test text.........  " //
                      "zsummerX:test text.........  zsummerX:test text.........  " //
                      "zsummerX:test text.........  zsummerX:test text.........  " //
                      "zsummerX:test text.........  zsummerX:test text.........  " //
                      "zsummerX:test text.........  zsummerX:test text.........  " //
                      "zsummerX:test text.........  zsummerX:test text.........  " //
                      "zsummerX:test text.........  zsummerX:test text.........  "; // *16 =1024


#define  NOW_TIME (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count())

extern std::string g_remoteIP;
extern unsigned short g_remotePort;
extern unsigned short g_startType;  //0 listen, 1 connect
extern unsigned short g_maxClient; //
extern unsigned int   g_multiThread ; // 0 full, 1 send 


#endif


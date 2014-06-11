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
 * Copyright (C) 2013 YaweiZhang <yawei_zhang@foxmail.com>.
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





#ifndef _ZSUMMERX_TCPSOCKET_H_
#define _ZSUMMERX_TCPSOCKET_H_
#include <atomic>
#ifdef WIN32
#include "iocp/tcpsocket_impl.h"
#else
#include "epoll/tcpsocket_impl.h"
#endif
namespace zsummer
{
	namespace network
	{
		extern std::atomic_uint _g_totalCreatedCTcpSocketObjs;
		extern std::atomic_uint _g_totalClosedCTcpSocketObjs;


		class CTcpSocket
		{
		public:
			CTcpSocket():m_userData(0)
			{
				_g_totalCreatedCTcpSocketObjs++;
			}
			~CTcpSocket()
			{
				_g_totalClosedCTcpSocketObjs++;
			}

			//! 初始化,  把当前socket绑定到指定的zsummer上.
			inline bool Initialize(CZSummerPtr summer)
			{
				return m_impl.Initialize(summer);
			}

			//! handle: void(zsummer::network::ErrorCode);
			//! ErrorCode: 0为成功. 其他为失败, 错误码见枚举定义处.
			template<typename H>
			inline bool DoConnect(std::string remoteIP, unsigned short remotePort, const H &h)
			{
				return m_impl.DoConnect(remoteIP, remotePort, h);
			}

			//!handle: void(ErrorCode, int)
			//! ErrorCode: 0为成功. 其他为失败, 错误码见枚举定义处.
			//! int: 发送掉的字节长度, 如果没有全部发送出去 要调用该接口把残留的数据继续发出去.
			template<typename H>
			inline bool DoSend(char * buf, unsigned int len, const H &h)
			{
				return m_impl.DoSend(buf, len, h);
			}

			//! 发起Recv请求.
			//! buf: 要发送的缓冲区地址
			//! len: 要发送的最大长度
			//!
			//! handle: void(ErrorCode, int)
			//! ErrorCode: 0为成功. 其他为失败, 错误码见枚举定义处.
			//! int: 接收到的字节长度.
			template<typename H>
			inline bool DoRecv(char * buf, unsigned int len, const H &h)
			{
				return m_impl.DoRecv(buf, len, h);
			}
			//! 获取远端IP和PORT
			inline bool GetPeerInfo(std::string& remoteIP, unsigned short &remotePort)
			{
				return m_impl.GetPeerInfo(remoteIP, remotePort);
			}
			//! 关闭现有的一个连接.
			//! 该接口调用后并不取消之前发出的请求, 但已经发出的请求操作将会已失败返回.
			inline bool DoClose()
			{
				return m_impl.DoClose();
			}

			//! 该接口提供给用户一个64位的数据块功用户自由使用, 该userData可以保存用户的一个ID或者指针或者任意想保存的数据.
			inline void SetUserData(unsigned long long userData){ m_userData = userData;}
			inline unsigned long long GetUserData(){ return m_userData;}

			friend class CTcpAcceptImpl;
		private:
			CTcpSocketImpl m_impl;
			unsigned long long m_userData;
		};
		typedef std::shared_ptr<CTcpSocket> CTcpSocketPtr;
	};


	
};























#endif












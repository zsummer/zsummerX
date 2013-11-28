/*
 * ZSUMMER_11X License
 * -----------
 * 
 * ZSUMMER_11X is licensed under the terms of the MIT license reproduced below.
 * This means that ZSUMMER_11X is free software and can be used for both academic
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





#ifndef _ZSUMMER_11X_TCPSOCKET_H_
#define _ZSUMMER_11X_TCPSOCKET_H_
#ifdef WIN32
#include "iocp/tcpsocket_impl.h"
#else
#include "epoll/tcpsocket_impl.h"
#endif
namespace zsummer
{
	namespace network
	{
		class CTcpSocket
		{
		public:
			/*
			* 构造函数: 1. 作为connect使用, 构造函数填写server端IP和port, 然后执行Initialize和DoConnect即可发起到服务器的连接.
			*			2. 内部使用, attach一个已有的established状态的原始socket.
			*/
#ifdef WIN32
			CTcpSocket(std::string remoteIP, unsigned short remotePort):m_impl(INVALID_SOCKET, remoteIP, remotePort), m_userData(0){}
			CTcpSocket(SOCKET s, std::string remoteIP, unsigned short remotePort):m_impl(s, remoteIP, remotePort), m_userData(0){}
#else
			CTcpSocket(std::string remoteIP, unsigned short remotePort):m_impl(-1, remoteIP, remotePort), m_userData(0){}
			CTcpSocket(int fd, std::string remoteIP, unsigned short remotePort):m_impl(fd, remoteIP, remotePort), m_userData(0){}
#endif // WIN32
			~CTcpSocket(){}

			//! 初始化,  把当前socket绑定到指定的zsummer上.
			inline bool Initialize(CZSummer & summer)
			{
				return m_impl.Initialize(summer);
			}

			//! 发起连接. Hander原型: void(zsummer::network::ErrorCode);
			//! ErrorCode: 0为成功. 其他为失败, 错误码见枚举定义处.
			template<typename H>
			inline bool DoConnect(const H &h)
			{
				return m_impl.DoConnect(h);
			}

			//!发起Send请求. Hander原型: void(ErrorCode, int)
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
			//! Hander原型: void(ErrorCode, int)
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
	};
	
};























#endif












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

#include <zsummerX/select/select_impl.h>
#include <zsummerX/select/udpsocket_impl.h>

using namespace zsummer::network;



UdpSocket::UdpSocket()
{
	_register._type = tagRegister::REG_UDP_SOCKET;
	_iRecvLen  = 0;
	_pRecvBuf = nullptr;
}


UdpSocket::~UdpSocket()
{
	if (_register._fd != -1)
	{
		if (_onRecvFromHandler )
		{
			LCE("UdpSocket::~UdpSocket[this0x" << this << "] Destruct UdpSocket Error. socket handle not invalid and some request was not completed. fd="
				<< _register._fd );
		}
		closesocket(_register._fd);
		_register._fd = -1;
	}
}
bool  UdpSocket::initialize(const EventLoopPtr & summer, const char *localIP, unsigned short localPort)
{
	if (_summer)
	{
		LCE("UdpSocket::initialize[this0x" << this << "] UdpSocket is aready initialize, _ios not is nullptr. this=" << this);
		return false;
	}
	if (_register._fd != -1)
	{
		LCE("UdpSocket::initialize[this0x" << this << "] UdpSocket is aready initialize, _fd not is -1. this=" << this << ", fd=" << _register._fd);
		return false;
	}
	_summer = summer;
	_register._fd = socket(AF_INET, SOCK_DGRAM, 0);
	_register._linkstat = LS_WAITLINK;
	if (_register._fd == -1)
	{
		LCE("UdpSocket::initialize[this0x" << this << "] create socket fail. this=" << this  << ", " << OSTREAM_GET_LASTERROR);
		return false;
	}
	setNonBlock(_register._fd);
	sockaddr_in	localAddr;
	localAddr.sin_family = AF_INET;
	localAddr.sin_addr.s_addr = inet_addr(localIP);
	localAddr.sin_port = htons(localPort);
	if (bind(_register._fd, (sockaddr *) &localAddr, sizeof(localAddr)) != 0)
	{
		LCE("UdpSocket::initialize[this0x" << this << "]: socket bind err, " << OSTREAM_GET_LASTERROR);
		closesocket(_register._fd);
		_register._fd = -1;
		return false;
	}
	if (!_summer->registerEvent(0, _register))
	{
		LCF("UdpSocket::initialize[this0x" << this << "] EPOLL_CTL_ADD error. _register =" << _register << ", " << OSTREAM_GET_LASTERROR);
		return false;
	}
	_register._linkstat = LS_ESTABLISHED;
	return true;
}

bool UdpSocket::doSendTo(char * buf, unsigned int len, const char *dstip, unsigned short dstport)
{
	if (!_summer)
	{
		LCE("UdpSocket::doSend[this0x" << this << "] IIOServer not bind!");
		return false;
	}
	if (len == 0 || len >1500)
	{
		LCE("UdpSocket::doSend[this0x" << this << "] argument err! len=" << len);
		return false;
	}
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(dstip);
	addr.sin_port = htons(dstport);
	sendto(_register._fd, buf, len, 0, (sockaddr*)&addr, sizeof(addr));
	return true;
}


bool UdpSocket::doRecvFrom(char * buf, unsigned int len, _OnRecvFromHandler && handler)
{
	if (!_summer)
	{
		LCE("UdpSocket::doRecv[this0x" << this << "] _summer not bind!");
		return false;
	}
	if (len == 0 || len >1500)
	{
		LCE("UdpSocket::doRecv[this0x" << this << "] argument err! len=" << len);
		return false;
	}
	if (len == 0 )
	{
		LCE("UdpSocket::doRecv[this0x" << this << "] argument err !!!  len==0");
		return false;
	}
	
	if (_pRecvBuf || _iRecvLen != 0 || _onRecvFromHandler)
	{
		LCE("UdpSocket::doRecv[this0x" << this << "] (_pRecvBuf != nullptr || _iRecvLen != 0) == TRUE");
		return false;
	}

	
	_pRecvBuf = buf;
	_iRecvLen = len;
	_register._rd = true;
	_register._udpsocketPtr = shared_from_this();
	if (!_summer->registerEvent(1, _register))
	{
		LCF("UdpSocket::doRecv[this0x" << this << "] EPOLLMod error. _register=" << _register << ", " << OSTREAM_GET_LASTERROR);
		_onRecvFromHandler = nullptr;
		_pRecvBuf = nullptr;
		_iRecvLen = 0;
		_register._udpsocketPtr.reset();
		return false;
	}
	_onRecvFromHandler = std::move(handler);
	
	return true;
}


bool UdpSocket::onSelectMessage(int type, bool rd, bool wt)
{
	std::shared_ptr<UdpSocket> guad(std::move(_register._udpsocketPtr));


	if (!_onRecvFromHandler)
	{
		LCE("UdpSocket::onSelectMessage[this0x" << this << "] unknown error");
		return false;
	}

	if (rd && _onRecvFromHandler)
	{
		_OnRecvFromHandler onRecv(std::move(_onRecvFromHandler));
		_register._rd = false;

		if (!_summer->registerEvent(1, _register))
		{
			LCF("UdpSocket::onSelectMessage[this0x" << this << "] EPOLLMod error. _register=" << _register << ", " << OSTREAM_GET_LASTERROR);
			return false;
		}

		sockaddr_in raddr;
		memset(&raddr, 0, sizeof(raddr));
		socklen_t len = sizeof(raddr);
		int ret = recvfrom(_register._fd, _pRecvBuf, _iRecvLen, 0, (sockaddr*)&raddr, &len);

		_pRecvBuf = nullptr;
		_iRecvLen = 0;
		if (ret == 0 || (ret ==-1 && !IS_WOULDBLOCK) )
		{
			LCE("UdpSocket::onSelectMessage[this0x" << this << "] recv error.  _register=" << _register << ", ret=" << ret << ", " << OSTREAM_GET_LASTERROR);
			onRecv(EC_ERROR, "", 0, 0);
			return false;
		}
		if (ret == -1)
		{
			LCE("UdpSocket::onSelectMessage[this0x" << this << "] recv error.  _register=" << _register << ", ret=" << ret << ", " << OSTREAM_GET_LASTERROR);
			onRecv(EC_ERROR, "", 0, 0);
			return false;
		}

		onRecv(EC_SUCCESS, inet_ntoa(raddr.sin_addr), ntohs(raddr.sin_port), ret);
	}
	
	

	return true;
}


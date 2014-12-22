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
 * Copyright (C) 2013-2014 YaweiZhang <yawei_zhang@foxmail.com>.
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


#include <zsummerX/epoll/tcpsocket_impl.h>
#include <zsummerX/epoll/tcpaccept_impl.h>


using namespace zsummer::network;




TcpAccept::TcpAccept()
{
	_register._event.events = 0;
	_register._event.data.ptr = &_register;
	_register._type = tagRegister::REG_TCP_ACCEPT;
}


TcpAccept::~TcpAccept()
{
	if (_register._fd != InvalideFD)
	{
		::close(_register._fd);
		_register._fd = InvalideFD;
	}
}
std::string TcpAccept::GetAcceptImplStatus()
{
	std::stringstream os;
	os << " TcpAccept:Status _summer=" << _summer.use_count() << ", _listenIP=" << _listenIP
		<< ", _listenPort=" << _listenPort << ", _onAcceptHandler=" << (bool)_onAcceptHandler
		<< ", _client=" << _client.use_count() << "_register=" << _register;
	return os.str();
}
bool TcpAccept::initialize(const ZSummerPtr & summer)
{
	_summer = summer;
	_register._linkstat = LS_WAITLINK;
	return true;
}


bool TcpAccept::openAccept(const std::string & listenIP, unsigned short listenPort)
{
	if (!_summer)
	{
		LCE("TcpAccept::openAccept[this0x" << this << "] _summer not bind!" << GetAcceptImplStatus());
		return false;
	}

	if (_register._fd != InvalideFD)
	{
		LCF("TcpAccept::openAccept[this0x" << this << "] accept fd is aready used!" << GetAcceptImplStatus());
		return false;
	}

	_register._fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_register._fd == InvalideFD)
	{
		LCF("TcpAccept::openAccept[this0x" << this << "] listen socket create err errno =" << strerror(errno) << GetAcceptImplStatus());
		return false;
	}

	setNonBlock(_register._fd);



	int bReuse = 1;
	if (setsockopt(_register._fd, SOL_SOCKET, SO_REUSEADDR,  &bReuse, sizeof(bReuse)) != 0)
	{
		LCW("TcpAccept::openAccept[this0x" << this << "] listen socket set reuse fail! errno=" << strerror(errno) << GetAcceptImplStatus());
	}


	_addr.sin_family = AF_INET;
	_addr.sin_addr.s_addr = inet_addr(listenIP.c_str());
	_addr.sin_port = htons(listenPort);
	if (bind(_register._fd, (sockaddr *) &_addr, sizeof(_addr)) != 0)
	{
		LCF("TcpAccept::openAccept[this0x" << this << "] listen socket bind err, errno=" << strerror(errno) << GetAcceptImplStatus());
		::close(_register._fd);
		_register._fd = InvalideFD;
		return false;
	}

	if (listen(_register._fd, 200) != 0)
	{
		LCF("TcpAccept::openAccept[this0x" << this << "] listen socket listen err, errno=" << strerror(errno) << GetAcceptImplStatus());
		::close(_register._fd);
		_register._fd = InvalideFD;
		return false;
	}

	if (!_summer->registerEvent(EPOLL_CTL_ADD, _register))
	{
		LCF("TcpAccept::openAccept[this0x" << this << "] listen socket register error." << GetAcceptImplStatus());
		return false;
	}
	_register._linkstat = LS_ESTABLISHED;
	return true;
}

bool TcpAccept::doAccept(const TcpSocketPtr &s, _OnAcceptHandler &&handle)
{
	if (_onAcceptHandler)
	{
		LCF("TcpAccept::doAccept[this0x" << this << "] err, dumplicate doAccept" << GetAcceptImplStatus());
		return false;
	}
	if (_register._linkstat != LS_ESTABLISHED)
	{
		LCF("TcpAccept::doAccept[this0x" << this << "] err, _linkstat not work state" << GetAcceptImplStatus());
		return false;
	}
	
	_register._event.events = EPOLLIN;
	if (!_summer->registerEvent(EPOLL_CTL_MOD, _register))
	{
		LCF("TcpAccept::doAccept[this0x" << this << "] err, _linkstat not work state" << GetAcceptImplStatus());
		return false;
	}

	_onAcceptHandler = std::move(handle);
	_client = s;
	_register._tcpacceptPtr = shared_from_this();
	return true;
}
void TcpAccept::onEPOLLMessage(bool bSuccess)
{
	//
	if (!_onAcceptHandler)
	{
		LCF("TcpAccept::doAccept[this0x" << this << "] err, dumplicate doAccept" << GetAcceptImplStatus());
		return ;
	}
	if (_register._linkstat != LS_ESTABLISHED)
	{
		LCF("TcpAccept::doAccept[this0x" << this << "] err, _linkstat not work state" << GetAcceptImplStatus());
		return ;
	}
	std::shared_ptr<TcpAccept> guad(std::move(_register._tcpacceptPtr));
	_OnAcceptHandler onAccept(std::move(_onAcceptHandler));
	TcpSocketPtr ps(std::move(_client));
	_register._event.events = 0;
	_summer->registerEvent(EPOLL_CTL_MOD, _register);

	if (!bSuccess)
	{
		LCF("TcpAccept::doAccept[this0x" << this << "]  EPOLLERR, errno=" << strerror(errno) << GetAcceptImplStatus());
		onAccept(EC_ERROR, ps);
		return ;
	}
	else
	{
		sockaddr_in cltaddr;
		memset(&cltaddr, 0, sizeof(cltaddr));
		socklen_t len = sizeof(cltaddr);
		int s = accept(_register._fd, (sockaddr *)&cltaddr, &len);
		if (s == -1)
		{
			if (errno != EAGAIN && errno != EWOULDBLOCK)
			{
				LCE("TcpAccept::doAccept[this0x" << this << "] ERR: accept return -1, errno=" << strerror(errno) << GetAcceptImplStatus());
			}
			onAccept(EC_ERROR, ps);
			return ;
		}

		ps->attachSocket(s,inet_ntoa(cltaddr.sin_addr), ntohs(cltaddr.sin_port));
		onAccept(EC_SUCCESS, ps);
	}
	return ;
}

bool TcpAccept::close()
{
	_onAcceptHandler = nullptr;
	_summer->registerEvent(EPOLL_CTL_DEL, _register);
	shutdown(_register._fd, SHUT_RDWR);
	::close(_register._fd);
	_register._fd = InvalideFD;
	return true;
}


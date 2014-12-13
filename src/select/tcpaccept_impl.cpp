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


#include <zsummerX/select/tcpsocket_impl.h>
#include <zsummerX/select/tcpaccept_impl.h>


using namespace zsummer::network;




TcpAcceptImpl::TcpAcceptImpl()
{
	_register._type = tagRegister::REG_TCP_ACCEPT;
}

TcpAcceptImpl::~TcpAcceptImpl()
{
	if (_register._fd != InvalideFD)
	{
		closesocket(_register._fd);
		_register._fd = InvalideFD;
	}
}
std::string TcpAcceptImpl::AcceptSection()
{
	std::stringstream os;
	os << " TcpAcceptImpl:Status _summer=" << _summer.use_count() << ", _listenIP=" << _listenIP
		<< ", _listenPort=" << _listenPort << ", _onAcceptHandler=" << (bool)_onAcceptHandler
		<< ", _client=" << _client.use_count() << "_register=" << _register;
	return os.str();
}
bool TcpAcceptImpl::initialize(const ZSummerPtr &summer)
{
	_summer = summer;
	_register._linkstat = LS_WAITLINK;
	return true;
}


bool TcpAcceptImpl::openAccept(const std::string& listenIP, unsigned short listenPort)
{
	if (!_summer)
	{
		LCE("TcpAcceptImpl::openAccept[this0x" << this << "] _summer not bind!" << AcceptSection());
		return false;
	}

	if (_register._fd != InvalideFD)
	{
		LCF("TcpAcceptImpl::openAccept[this0x" << this << "] accept fd is aready used!" << AcceptSection());
		return false;
	}

	_register._fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_register._fd == InvalideFD)
	{
		LCF("TcpAcceptImpl::openAccept[this0x" << this << "] listen socket create err " << OSTREAM_GET_LASTERROR << ", " << AcceptSection());
		return false;
	}

	setNonBlock(_register._fd);



	int bReuse = 1;
	if (setsockopt(_register._fd, SOL_SOCKET, SO_REUSEADDR,  (char *)&bReuse, sizeof(bReuse)) != 0)
	{
		LCW("TcpAcceptImpl::openAccept[this0x" << this << "] listen socket set reuse fail! " << OSTREAM_GET_LASTERROR << ", " << AcceptSection());
	}


	_addr.sin_family = AF_INET;
	_addr.sin_addr.s_addr = inet_addr(listenIP.c_str());
	_addr.sin_port = htons(listenPort);
	if (bind(_register._fd, (sockaddr *) &_addr, sizeof(_addr)) != 0)
	{
		LCF("TcpAcceptImpl::openAccept[this0x" << this << "] listen socket bind err, " << OSTREAM_GET_LASTERROR << ", " << AcceptSection());
		closesocket(_register._fd);
		_register._fd = InvalideFD;
		return false;
	}

	if (listen(_register._fd, 200) != 0)
	{
		LCF("TcpAcceptImpl::openAccept[this0x" << this << "] listen socket listen err, " << OSTREAM_GET_LASTERROR << ", " << AcceptSection());
		closesocket(_register._fd);
		_register._fd = InvalideFD;
		return false;
	}

	if (!_summer->registerEvent(0, _register))
	{
		LCF("TcpAcceptImpl::openAccept[this0x" << this << "] listen socket register error." << AcceptSection());
		return false;
	}
	_register._linkstat = LS_ESTABLISHED;
	return true;
}

bool TcpAcceptImpl::doAccept(const TcpSocketPtr &s, _OnAcceptHandler && handle)
{
	if (_onAcceptHandler)
	{
		LCF("TcpAcceptImpl::doAccept[this0x" << this << "] err, dumplicate doAccept" << AcceptSection());
		return false;
	}
	if (_register._linkstat != LS_ESTABLISHED)
	{
		LCF("TcpAcceptImpl::doAccept[this0x" << this << "] err, _linkstat not work state" << AcceptSection());
		return false;
	}
	
	_register._rd = true;
	_register._tcpacceptPtr = shared_from_this();
	if (!_summer->registerEvent(1, _register))
	{
		LCF("TcpAcceptImpl::doAccept[this0x" << this << "] err, _linkstat not work state" << AcceptSection());
		_register._tcpacceptPtr.reset();
		return false;
	}

	_onAcceptHandler = std::move(handle);
	_client = s;
	
	return true;
}
void TcpAcceptImpl::onSelectMessage()
{
	std::shared_ptr<TcpAcceptImpl> guard(std::move(_register._tcpacceptPtr));

	if (!_onAcceptHandler)
	{
		LCF("TcpAcceptImpl::onSelectMessage[this0x" << this << "] err, dumplicate doAccept" << AcceptSection());
		return ;
	}
	if (_register._linkstat != LS_ESTABLISHED)
	{
		LCF("TcpAcceptImpl::onSelectMessage[this0x" << this << "] err, _linkstat not work state" << AcceptSection());
		return ;
	}

	_OnAcceptHandler onAccept(std::move(_onAcceptHandler));
	TcpSocketPtr ps(std::move(_client));

	_register._rd = false;
	_summer->registerEvent(1,_register);

	sockaddr_in cltaddr;
	memset(&cltaddr, 0, sizeof(cltaddr));
	socklen_t len = sizeof(cltaddr);
	SOCKET s = ::accept(_register._fd, (sockaddr *)&cltaddr, &len);
	if (s == -1)
	{
		if (!IS_WOULDBLOCK)
		{
			LCE("TcpAcceptImpl::onSelectMessage[this0x" << this << "] ERR: accept return -1, " OSTREAM_GET_LASTERROR << AcceptSection());
		}
		onAccept(EC_ERROR, ps);
		return ;
	}

	setNonBlock(s);
	setNoDelay(s);

	ps->attachSocket(s,inet_ntoa(cltaddr.sin_addr), ntohs(cltaddr.sin_port));
	onAccept(EC_SUCCESS, ps);
	
	return ;
}

bool TcpAcceptImpl::close()
{
	_onAcceptHandler = nullptr;
	_summer->registerEvent(2, _register);
	shutdown(_register._fd, SHUT_RDWR);
	closesocket(_register._fd);
	_register._fd = InvalideFD;
	return true;
}


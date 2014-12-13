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

using namespace zsummer::network;




TcpSocketImpl::TcpSocketImpl()
{
	g_appEnvironment.addCreatedSocketCount();
	_register._type = tagRegister::REG_TCP_SOCKET;
}


TcpSocketImpl::~TcpSocketImpl()
{
	g_appEnvironment.addClosedSocketCount();
	if (_onRecvHandler || _onSendHandler || _onConnectHandler)
	{
		LCT("TcpSocketImpl::~TcpSocketImpl[this0x" << this << "] Handler status error. " << socketSection());
	}
	if (_register._fd != InvalideFD)
	{
		closesocket(_register._fd);
		_register._fd = InvalideFD;
	}
}

std::string TcpSocketImpl::socketSection()
{
	std::stringstream os;
	os << ";; Status: summer.user_count()=" << _summer.use_count() << ", remoteIP=" << _remoteIP << ", remotePort=" << _remotePort
		<< ", _onConnectHandler = " << (bool)_onConnectHandler 
		<< ", _onRecvHandler = " << (bool)_onRecvHandler << ", _pRecvBuf=" << (void*)_pRecvBuf << ", _iRecvLen=" << _iRecvLen 
		<< ", _onSendHandler = " << (bool)_onSendHandler << ", _pSendBuf=" << (void*)_pSendBuf << ", _iSendLen=" << _iSendLen
		<< "; _register=" << _register; 
	return os.str();
}

bool TcpSocketImpl::initialize(const ZSummerPtr & summer)
{
	 _summer = summer;
	if (_register._linkstat != LS_UNINITIALIZE)
	{
		if (!_summer->registerEvent(0, _register))
		{
			LCE("TcpSocketImpl::initialize[this0x" << this << "] socket already used or not initilize." << socketSection());
			return false;
		}
		_register._linkstat = LS_ESTABLISHED;
	}
	else
	{
		if (_register._fd != -1)
		{
			LCE("TcpSocketImpl::doConnect[this0x" << this << "] fd aready used!" << socketSection());
			return false;
		}
		_register._fd = socket(AF_INET, SOCK_STREAM, 0);
		if (_register._fd == -1)
		{
			LCE("TcpSocketImpl::doConnect[this0x" << this << "] fd create failed!" << socketSection());
			return false;
		}
		setNonBlock(_register._fd);
		setNoDelay(_register._fd);
		_register._linkstat = LS_WAITLINK;
	}



	return true;
}
bool TcpSocketImpl::attachSocket(SOCKET s, const std::string& remoteIP, unsigned short remotePort)
{
	 _register._fd = s;
	 _remoteIP = remoteIP;
	 _remotePort = remotePort;
	 _register._linkstat = LS_WAITLINK;
	return true;
}




bool TcpSocketImpl::doConnect(const std::string & remoteIP, unsigned short remotePort, _OnConnectHandler && handler)
{
	if (!_summer)
	{
		LCE("TcpSocketImpl::doConnect[this0x" << this << "] summer not bind!" << socketSection());
		return false;
	}
	if (_register._linkstat != LS_WAITLINK)
	{
		LCE("TcpSocketImpl::doConnect[this0x" << this << "] _linkstat not LS_WAITLINK!" << socketSection());
		return false;
	}

	_register._wt = true;

	_remoteIP = remoteIP;
	_remotePort = remotePort;
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(_remoteIP.c_str());
	addr.sin_port = htons(_remotePort);
	
	
	int ret = connect(_register._fd, (sockaddr *) &addr, sizeof(addr));
#ifndef WIN32
	if (ret != 0 && errno != EINPROGRESS)
#else
	if (ret != 0 && WSAGetLastError() != WSAEWOULDBLOCK)
#endif
	{
		LCT("TcpSocketImpl::doConnect[this0x" << this << "] ::connect error. " << OSTREAM_GET_LASTERROR << socketSection());
		closesocket(_register._fd);
		_register._fd = InvalideFD;
		return false;
	}
	
	_register._tcpSocketConnectPtr = shared_from_this();
	if (!_summer->registerEvent(0, _register))
	{
		LCE("TcpSocketImpl::doConnect[this0x" << this << "] registerEvent Error" << socketSection());
		closesocket(_register._fd);
		_register._fd = InvalideFD;
		_register._tcpSocketConnectPtr.reset();
		return false;
	}
	_onConnectHandler = std::move(handler);
	return true;
}


bool TcpSocketImpl::doSend(char * buf, unsigned int len, _OnSendHandler && handler)
{
	if (_register._linkstat != LS_ESTABLISHED)
	{
		LCT("TcpSocketImpl::doSend[this0x" << this << "] _linkstat not REG_ESTABLISHED_TCP!" << socketSection());
		return false;
	}

	if (!_summer)
	{
		LCE("TcpSocketImpl::doSend[this0x" << this << "] _summer not bind!" << socketSection());
		return false;
	}
	if (len == 0)
	{
		LCE("TcpSocketImpl::doSend[this0x" << this << "] argument err! len ==0" << socketSection());
		return false;
	}
	if (_pSendBuf != NULL || _iSendLen != 0)
	{
		LCE("TcpSocketImpl::doSend[this0x" << this << "] _pSendBuf =" << (void *) _pSendBuf << " _iSendLen =" << _iSendLen<< socketSection());
		return false;
	}
	if (_onSendHandler)
	{
		LCE("TcpSocketImpl::doSend[this0x" << this << "] _onSendHandler == TRUE" << socketSection());
		return false;
	}

	
	
	_pSendBuf = buf;
	_iSendLen = len;
	

	_register._wt = true;
	_register._tcpSocketSendPtr = shared_from_this();
	if (!_summer->registerEvent(1, _register))
	{
		LCT("TcpSocketImpl::doSend[this0x" << this << "] registerEvent Error" << socketSection());
		_pSendBuf = nullptr;
		_iSendLen = 0;
		_register._tcpSocketSendPtr.reset();
		doClose();
		return false;
	}
	_onSendHandler = std::move(handler);
	
	return true;
}


bool TcpSocketImpl::doRecv(char * buf, unsigned int len, _OnRecvHandler && handler)
{
	if (_register._linkstat != LS_ESTABLISHED)
	{
		LCT("TcpSocketImpl::doRecv[this0x" << this << "] type not REG_ESTABLISHED_TCP!" << socketSection());
		return false;
	}

	if (!_summer)
	{
		LCE("TcpSocketImpl::doRecv[this0x" << this << "] _summer not bind!" << socketSection());
		return false;
	}

	if (len == 0 )
	{
		LCE("TcpSocketImpl::doRecv[this0x" << this << "] argument err !!!  len==0" << socketSection());
		return false;
	}
	if (_pRecvBuf != NULL || _iRecvLen != 0)
	{
		LCE("TcpSocketImpl::doRecv[this0x" << this << "]  (_pRecvBuf != NULL || _iRecvLen != 0) == TRUE" << socketSection());
		return false;
	}
	if (_onRecvHandler)
	{
		LCE("TcpSocketImpl::doRecv[this0x" << this << "] (_onRecvHandler) == TRUE" << socketSection());
		return false;
	}
	
	_pRecvBuf = buf;
	_iRecvLen = len;
	

	_register._rd = true;
	_register._tcpSocketRecvPtr = shared_from_this();
	if (!_summer->registerEvent(1, _register))
	{
		LCT("TcpSocketImpl::doRecv[this0x" << this << "] registerEvent Error" << socketSection());
		_pRecvBuf = nullptr;
		_iRecvLen = 0;
		_register._tcpSocketRecvPtr.reset();
		return false;
	}
	_onRecvHandler = std::move(handler);
	
	return true;
}


void TcpSocketImpl::onSelectMessage(bool rd, bool wt, bool err)
{
	unsigned char linkstat = _register._linkstat;
	ErrorCode ec = EC_ERROR;

	if (!_onRecvHandler && !_onSendHandler && !_onConnectHandler)
	{
		LCE("TcpSocketImpl::onSelectMessage[this0x" << this << "] unknown error. " << OSTREAM_GET_LASTERROR << socketSection());
		return ;
	}

	if (linkstat == LS_WAITLINK)
	{
		std::shared_ptr<TcpSocketImpl> guad(std::move(_register._tcpSocketConnectPtr));
		_OnConnectHandler onConnect(std::move(_onConnectHandler));
		int errCode = 0;
		socklen_t len = sizeof(int);
		if (err || getsockopt(_register._fd, SOL_SOCKET, SO_ERROR, (char*)&errCode, &len) != 0 || errCode != 0)
		{
			LOGT("onConnect False. " << OSTREAM_GET_LASTERROR);
			_register._linkstat = LS_WAITLINK;
			_summer->registerEvent(2, _register);
			onConnect(EC_ERROR);
			return;
		}
		else
		{
			_register._wt =  0;
			_summer->registerEvent(1, _register);
			_register._linkstat = LS_ESTABLISHED;
			onConnect(EC_SUCCESS);
			return;
		}
		return ;
	}


	if (rd && _onRecvHandler)
	{
		std::shared_ptr<TcpSocketImpl> guad(std::move(_register._tcpSocketRecvPtr));

		int ret = recv(_register._fd, _pRecvBuf, _iRecvLen, 0);
		_register._rd = false;
		if (!_summer->registerEvent(1, _register))
		{
			LCF("TcpSocketImpl::onSelectMessage[this0x" << this << "] connect true & EPOLLMod error.  " << OSTREAM_GET_LASTERROR << socketSection());
		}
		if (ret == 0 || (ret == -1 && !IS_WOULDBLOCK))
		{
			ec = EC_ERROR;
			_register._linkstat = LS_CLOSED;
			if (rd && _onRecvHandler)
			{
				_OnRecvHandler onRecv(std::move(_onRecvHandler));
				onRecv(ec, 0);
			}
			if (!_onSendHandler && !_onRecvHandler)
			{
				if (!_summer->registerEvent(2, _register))
				{
					LCW("TcpSocketImpl::onSelectMessage[this0x" << this << "] connect true & EPOLL DEL error.  " << OSTREAM_GET_LASTERROR << socketSection());
				}
			}
			return ;
		}
		else if (ret != -1)
		{
			_OnRecvHandler onRecv(std::move(_onRecvHandler));
			_pRecvBuf = NULL;
			_iRecvLen = 0;
			onRecv(EC_SUCCESS,ret);
		}
		return;
	}
	else if (wt && _onSendHandler)
	{
		std::shared_ptr<TcpSocketImpl> guad(std::move(_register._tcpSocketSendPtr));

		int ret = send(_register._fd, _pSendBuf, _iSendLen, 0);
		_register._wt = false;
		if (!_summer->registerEvent(1, _register))
		{
			LCF("TcpSocketImpl::onSelectMessage[this0x" << this << "] connect true & EPOLLMod error. " << OSTREAM_GET_LASTERROR << socketSection());
		}
		
		if ((ret == -1 && !IS_WOULDBLOCK) || _register._linkstat == LS_CLOSED)
		{
			ec = EC_ERROR;
			_register._linkstat = LS_CLOSED;
			_onSendHandler = nullptr;
			if (!_onSendHandler && !_onRecvHandler)
			{
				if (!_summer->registerEvent(2, _register))
				{
					LCW("TcpSocketImpl::onSelectMessage[this0x" << this << "] connect true & EPOLL DEL error.  " << OSTREAM_GET_LASTERROR << socketSection());
				}
			}
			return ;
		}
		else if (ret != -1)
		{
			_OnSendHandler onSend(std::move(_onSendHandler));
			_pSendBuf = NULL;
			_iSendLen = 0;
			onSend(EC_SUCCESS, ret);
		}
	}
	return ;
}


bool TcpSocketImpl::doClose()
{
	if (_register._fd != InvalideFD)
	{
		shutdown(_register._fd, SHUT_RDWR);
	}
	return true;
}


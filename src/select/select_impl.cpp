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


#include <zsummerX/select/select_impl.h>
#include <zsummerX/select/tcpsocket_impl.h>
#include <zsummerX/select/tcpaccept_impl.h>
#include <zsummerX/select/udpsocket_impl.h>

using namespace zsummer::network;

bool EventLoop::initialize()
{
	//////////////////////////////////////////////////////////////////////////
	//create socket pair
	sockaddr_in pairAddr;
	memset(&pairAddr, 0, sizeof(pairAddr));
	pairAddr.sin_family = AF_INET;
	pairAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	SOCKET acpt = socket(AF_INET, SOCK_STREAM, 0);
	_sockpair[0] = socket(AF_INET, SOCK_STREAM, 0);
	_sockpair[1] = socket(AF_INET, SOCK_STREAM, 0);
	if (_sockpair[0] == -1 || _sockpair[1] == -1 || acpt == -1)
	{
		LCF("ZSummerImpl::initialize[this0x" << this << "] bind sockpair socket error. " << logSection());
		return false;
	}
	if (::bind(acpt, (sockaddr*)&pairAddr, sizeof(pairAddr)) == -1)
	{
		LCF("EventLoop::initialize[this0x" << this << "] bind sockpair socket error. " << logSection());
		closesocket(acpt);
		closesocket(_sockpair[0]);
		closesocket(_sockpair[1]);
		return false;
	}
	if (listen(acpt, 1) == -1)
	{
		LCF("EventLoop::initialize[this0x" << this << "] listen sockpair socket error. " << logSection());
		closesocket(acpt);
		closesocket(_sockpair[0]);
		closesocket(_sockpair[1]);
		return false;
	}
	socklen_t len = sizeof(pairAddr);
	if (getsockname(acpt, (sockaddr*)&pairAddr, &len) != 0)
	{
		LCF("EventLoop::initialize[this0x" << this << "] getsockname sockpair socket error. " << logSection());
		closesocket(acpt);
		closesocket(_sockpair[0]);
		closesocket(_sockpair[1]);
		return false;
	}

	if (::connect(_sockpair[0], (sockaddr*)&pairAddr, sizeof(pairAddr)) == -1)
	{
		LCF("EventLoop::initialize[this0x" << this << "] connect sockpair socket error. " << logSection());
		closesocket(acpt);
		closesocket(_sockpair[0]);
		closesocket(_sockpair[1]);
		return false;
	}
	_sockpair[1] = accept(acpt, (sockaddr*)&pairAddr, &len);
	if (_sockpair[1] == -1)
	{
		LCF("EventLoop::initialize[this0x" << this << "] accept sockpair socket error. " << logSection());
		closesocket(acpt);
		closesocket(_sockpair[0]);
		closesocket(_sockpair[1]);
		return false;
	}
	closesocket(acpt);

	setNonBlock(_sockpair[0]);
	setNonBlock(_sockpair[1]);
	setNoDelay(_sockpair[0]);
	setNoDelay(_sockpair[1]);

	return true;
}

bool EventLoop::registerEvent(int op, tagRegister & reg)
{
	if (op == 1)
	{
		auto founder = std::find_if(_poolRegister.begin(), _poolRegister.end(), [&reg](const tagRegister & r){ return reg._fd == r._fd; });
		if (founder == _poolRegister.end())
		{
			LOGE("EventLoop::registerEvent not found register. op=" << op << ", reg._type=" << reg._type);
			return false;
		}
		else
		{
			*founder = reg;
			return true;
		}
	}
	else
	{
		auto founder = std::find_if(_poolRegister.begin(), _poolRegister.end(), [&reg](const tagRegister & r){ return reg._fd == r._fd; });
		if (founder == _poolRegister.end())
		{
			if (op == 0)
			{
				if (_poolRegister.size() >= FD_SETSIZE)
				{
					LOGE("EventLoop::registerEvent add. register is too many. op=" << op << ", reg=" << reg);
					return false;
				}
				_poolRegister.push_back(reg);
				return true;
			}
			else
			{
				LOGE("EventLoop::registerEvent del. register not found. op=" << op << ", reg=" << reg);
				return false;
			}

		}
		else
		{
			if (op	== 0)
			{
				LOGE("EventLoop::registerEvent add. register aready exsit. op=" << op << ", reg=" << reg);
				return false;
			}
			_poolRegister.erase(founder);
		}
	}
	return true;
}


void EventLoop::PostMessage(_OnPostHandler &&handle)
{
	_OnPostHandler * pHandler = new _OnPostHandler(std::move(handle));
	_stackMessagesLock.lock();
	_stackMessages.push_back(pHandler);
	_stackMessagesLock.unlock();
	char c='0';
	send(_sockpair[0], &c, 1, 0);
}

std::string EventLoop::logSection()
{
	std::stringstream os;
	_stackMessagesLock.lock();
	MessageStack::size_type msgSize = _stackMessages.size();
	_stackMessagesLock.unlock();
	os << " logSection:  _sockpair[2]={" << _sockpair[0] << "," << _sockpair[1] << "}"
		<< " _stackMessages.size()=" << msgSize << ", current total timer=" << _timer.GetTimersCount()
		<< " _poolRegister=" << _poolRegister.size();
	return os.str();
}

void EventLoop::runOnce(bool isImmediately)
{
	int nfds = 0;
	fd_set fdr;
	fd_set fdw;
	fd_set fde;
	FD_ZERO(&fdr);
	FD_ZERO(&fdw);
	FD_ZERO(&fde);
	for (auto & r : _poolRegister)
	{
		if (r._rd)
		{
			FD_SET(r._fd, &fdr);
			if (r._fd > nfds)
			{
				nfds = (int)r._fd;
			}
			
		}
		if (r._wt)
		{
			FD_SET(r._fd, &fdw);
			if (r._type == tagRegister::REG_TCP_SOCKET && r._linkstat == LS_WAITLINK)
			{
				FD_SET(r._fd, &fde);
			}
			if (r._fd > nfds)
			{
				nfds = (int)r._fd;
			}
		}
	}
	FD_SET(_sockpair[1], &fdr);
	if (_sockpair[1] > nfds)
	{
		nfds = (int)_sockpair[1];
	}

	timeval tv;
	if (isImmediately)
	{
		tv.tv_sec = 0;
		tv.tv_usec = 0;
	}
	else
	{
		unsigned int ms = _timer.getNextExpireTime();
		tv.tv_sec = ms / 1000;
		tv.tv_usec = (ms % 1000) * 1000;
	}

	int retCount = ::select(nfds+1, &fdr, &fdw, &fde, &tv);
	if (retCount == -1)
	{
		if (IS_EINTR)
		{
			LCT("EventLoop::runOnce[this0x" << this << "]  select err!  " << OSTREAM_GET_LASTERROR << ", " << logSection());
			return; //! error
		}
		return;
	}

	//check timer
	{
		_timer.checkTimer();
		if (retCount == 0) return;//timeout
	}

	if (FD_ISSET(_sockpair[1], &fdr))
	{
		char buf[1000];
		while (recv(_sockpair[1], buf, 1000, 0) > 0);

		MessageStack msgs;
		_stackMessagesLock.lock();
		msgs.swap(_stackMessages);
		_stackMessagesLock.unlock();

		for (auto pfunc : msgs)
		{
			_OnPostHandler * p = (_OnPostHandler*)pfunc;
			try
			{
				(*p)();
			}
			catch (std::runtime_error e)
			{
				LCW("OnPostHandler have runtime_error exception. err=" << e.what());
			}
			catch (...)
			{
				LCW("OnPostHandler have unknown exception.");
			}
			delete p;
		}
	}
	

	PoolReggister poolRegister = _poolRegister;
	for (auto & r : poolRegister)
	{
		//tagHandle  type
		if (r._type == tagRegister::REG_TCP_ACCEPT)
		{
			if (FD_ISSET(r._fd, &fdr))
			{
				if (r._tcpacceptPtr)
				{
					r._tcpacceptPtr->onSelectMessage();
				}
			}
		}
		else if (r._type == tagRegister::REG_TCP_SOCKET)
		{
			bool rd = r._rd && FD_ISSET(r._fd, &fdr) != 0 ;
			bool wt = r._wt && FD_ISSET(r._fd, &fdw) != 0;
			if (FD_ISSET(r._fd, &fde) != 0)
			{
				if (r._rd && r._tcpSocketRecvPtr)
				{
					r._tcpSocketRecvPtr->onSelectMessage(true, false, true);
				}
				if (r._wt)
				{
					if (r._tcpSocketSendPtr)
					{
						r._tcpSocketSendPtr->onSelectMessage(false, true, true);
					}
					if (r._tcpSocketConnectPtr)
					{
						r._tcpSocketConnectPtr->onSelectMessage(false, true, true);
					}
				}
			}
			else
			{
				if (rd && r._tcpSocketRecvPtr)
				{
					r._tcpSocketRecvPtr->onSelectMessage(true, false, false);
				}
				if (wt)
				{
					if (r._tcpSocketSendPtr)
					{
						r._tcpSocketSendPtr->onSelectMessage(false, true, false);
					}
					if (r._tcpSocketConnectPtr)
					{
						r._tcpSocketConnectPtr->onSelectMessage(false, true, false);
					}
				}
			}
		}
		else if (r._type == tagRegister::REG_UDP_SOCKET)
		{
			bool rd = r._rd && FD_ISSET(r._fd, &fdr) != 0;
			bool wt = r._wt && FD_ISSET(r._fd, &fdw) != 0;
			if (rd || wt)
			{
				if (r._udpsocketPtr)
				{
					r._udpsocketPtr->onSelectMessage(r._type, rd, wt);
				}
			}
		}
		else
		{
			
			LCE("ZSummerImpl::runOnce[this0x" << this << "] check register event type failed !!  type=" << r << logSection());
		}
	}



}



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

#include <zsummerX/frame/session.h>
#include <zsummerX/frame/manager.h>
#include <zsummerX/frame/dispatch.h>

using namespace zsummer::proto4z;
using namespace zsummer::network;



TcpSession::TcpSession()
{
	zsummer::network::g_appEnvironment.addCreatedSessionCount();
	LCI("TcpSession. total TcpSocket object count =[create:" << zsummer::network::g_appEnvironment.getCreatedSocketCount() << ", close:" << zsummer::network::g_appEnvironment.getClosedSocketCount() << "], total TcpSession object count =[create:"
		<< zsummer::network::g_appEnvironment.getCreatedSessionCount() << ", close:" << zsummer::network::g_appEnvironment.getClosedSessionCount()
		<< "]");
}

TcpSession::~TcpSession()
{
	zsummer::network::g_appEnvironment.addClosedSessionCount();
	while (!_sendque.empty())
	{
		delete _sendque.front();
		_sendque.pop();
	}
	while (!_freeCache.empty())
	{
		delete _freeCache.front();
		_freeCache.pop();
	}
	_sockptr.reset();
	LCI("~TcpSession. total TcpSocket object count =[create:" << zsummer::network::g_appEnvironment.getCreatedSocketCount() << ", close:" << zsummer::network::g_appEnvironment.getClosedSocketCount() << "], total TcpSession object count =[create:"
		<< zsummer::network::g_appEnvironment.getCreatedSessionCount() << ", close:" << zsummer::network::g_appEnvironment.getClosedSessionCount()
		<< "]");
}



void TcpSession::cleanSession(bool isCleanAllData, const std::string &rc4TcpEncryption)
{
	_sockptr.reset();
	_sessionID = InvalidSeesionID;
	_acceptID = InvalidAccepterID;
	_pulseTimerID = zsummer::network::InvalidTimerID;

	_recving.bufflen = 0;
	_sending.bufflen = 0;
	_sendingCurIndex = 0;

	_rc4Encrypt = rc4TcpEncryption;
	_rc4StateRead.makeSBox(_rc4Encrypt);
	_rc4StateWrite.makeSBox(_rc4Encrypt);

	_bFirstRecvData = true;
	_bOpenFlashPolicy = false;

	if (isCleanAllData)
	{
		while (!_sendque.empty())
		{
			_freeCache.push(_sendque.front());
			_sendque.pop();
		}
	}
}

bool TcpSession::bindTcpSocketPrt(const TcpSocketPtr &sockptr, AccepterID aID, SessionID sID, const ListenConfig &traits)
{
	
	cleanSession(true, traits._rc4TcpEncryption);
	_sockptr = sockptr;
	_sessionID = sID;
	_acceptID = aID;
	_protoType = traits._protoType;
	_pulseInterval = traits._pulseInterval;
	_bOpenFlashPolicy = traits._openFlashPolicy;
	_sockptr->getPeerInfo(_remoteIP, _remotePort);
	if (!doRecv())
	{
		LCW("bindTcpSocketPrt Failed.");
		return false;
	}
	if (traits._pulseInterval > 0)
	{
		_pulseTimerID = SessionManager::getRef().createTimer(traits._pulseInterval, std::bind(&TcpSession::onPulseTimer, shared_from_this()));
	}
	return true;
}

void TcpSession::bindTcpConnectorPtr(const TcpSocketPtr &sockptr, const std::pair<ConnectConfig, ConnectInfo> & config)
{
	cleanSession(config.first._reconnectCleanAllData, config.first._rc4TcpEncryption);
	_sockptr = sockptr;
	_sessionID = config.second._cID;
	_protoType = config.first._protoType;
	_pulseInterval = config.first._pulseInterval;
	_remoteIP = config.first._remoteIP;
	_remotePort = config.first._remotePort;
	bool connectRet = _sockptr->doConnect(config.first._remoteIP, config.first._remotePort,
		std::bind(&TcpSession::onConnected, shared_from_this(), std::placeholders::_1));
	if (!connectRet)
	{
		LCE("DoConnected Failed: traits=" << config.first);
		return ;
	}
	LCI("DoConnected : traits=" << config.first);
	return ;
}




void TcpSession::onConnected(zsummer::network::NetErrorCode ec)
{
	std::pair<ConnectConfig, ConnectInfo> config;
	if (!SessionManager::getRef().getConnectorConfig(getSessionID(), config))
	{
		LCE("onConnected can not found the connect configure. ec=" << ec << ",  connectID=" << getSessionID());
	}
	if (ec)
	{
		LCW("onConnected failed. ec=" << ec << ",  config=" << config.first);
		_sockptr.reset();
		SessionManager::getRef().onConnect(getSessionID(), false, shared_from_this());
		return;
	}

	LCI("onConnected success.  config=" << config.first);
	SessionManager::getRef().onConnect(_sessionID, true, shared_from_this());
	if (_sending.bufflen == 0 && !_sendque.empty())
	{
		MessageSendPack *tmp = _sendque.front();
		_sendque.pop();
		doSend(tmp->buff, tmp->bufflen);
		tmp->bufflen = 0;
		_freeCache.push(tmp);
	}

	if (!doRecv())
	{
		onClose();
		return;
	}
	if (_pulseInterval > 0)
	{
		_pulseTimerID = SessionManager::getRef().createTimer(config.first._pulseInterval, std::bind(&TcpSession::onPulseTimer, shared_from_this()));
	}
}

bool TcpSession::doRecv()
{
	return _sockptr->doRecv(_recving.buff + _recving.bufflen, MAX_BUFF_SIZE - _recving.bufflen, std::bind(&TcpSession::onRecv, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
}

void TcpSession::close()
{
	LCW("TcpSession to close socket. sID= " << _sessionID );
	if (_sockptr)
	{
		_sockptr->doClose();
	}
	if (_pulseTimerID != zsummer::network::InvalidTimerID)
	{
		SessionManager::getRef().cancelTimer(_pulseTimerID);
		_pulseTimerID = zsummer::network::InvalidTimerID;
	}
}

void TcpSession::onRecv(zsummer::network::NetErrorCode ec, int nRecvedLen)
{
	if (ec)
	{
		LCD("socket closed");
		onClose();
		return;
	}
	_recving.bufflen += nRecvedLen;
	SessionManager::getRef()._totalRecvCount++;
	SessionManager::getRef()._totalRecvBytes += nRecvedLen;
	// skip encrypt the flash policy data if that open flash policy.
	// skip encrypt when the rc4 encrypt sbox is empty.
	{
		do 
		{
			//process something when recv first data.
			// flash policy process
			const char * flashPolicyRequestString = "<policy-file-request/>"; //string length is 23 byte, contain null-terminator character.
			unsigned int flashPolicyRequestSize = 23;
			if (_bFirstRecvData && _bOpenFlashPolicy && _acceptID != InvalidAccepterID && _recving.bufflen == flashPolicyRequestSize)
			{
				std::string tmp;
				tmp.assign(_recving.buff, flashPolicyRequestSize-1);
				if (tmp.compare(flashPolicyRequestString) == 0)
				{
					_recving.bufflen -= flashPolicyRequestSize;
					memmove(_recving.buff, _recving.buff + flashPolicyRequestSize, _recving.bufflen);

					const char * flashPolicyResponseString = R"---(<cross-domain-policy><allow-access-from domain="*" to-ports="*"/></cross-domain-policy>)---";
					unsigned int flashPolicyResponseSize = (unsigned int)strlen(flashPolicyResponseString)+1;
					doSend(flashPolicyResponseString, flashPolicyResponseSize);
				}
				_bFirstRecvData = false;
			}
			else if (_bFirstRecvData)
			{
				//do other something.

				//do other something end.
				_bFirstRecvData = false;
			}
			
			if (_rc4Encrypt.empty() || _recving.bufflen == 0)
			{
				break;
			}
			
			unsigned int needEncry = nRecvedLen;
			if (_recving.bufflen < (unsigned int)nRecvedLen)
			{
				needEncry = _recving.bufflen;
			}
			_rc4StateRead.encryption((unsigned char*)_recving.buff + _recving.bufflen - needEncry, needEncry);
		} while (0);
		
	}

	//分包
	unsigned int usedIndex = 0;
	do 
	{
		if (_protoType == PT_TCP)
		{
			auto ret = zsummer::proto4z::checkBuffIntegrity(_recving.buff + usedIndex, _recving.bufflen - usedIndex, MAX_BUFF_SIZE - usedIndex, MAX_BUFF_SIZE);
			if (ret.first == zsummer::proto4z::IRT_CORRUPTION)
			{
				LCW("killed socket: checkBuffIntegrity error ");
				_sockptr->doClose();
				onClose();
				return;
			}
			if (ret.first == zsummer::proto4z::IRT_SHORTAGE)
			{
				break;
			}
			SessionManager::getRef()._totalRecvMessages++;
			try
			{
				bool preCheck = MessageDispatcher::getRef().dispatchPreSessionMessage(shared_from_this(), _recving.buff + usedIndex, ret.second);
				if (!preCheck)
				{
					LCW("Dispatch Message failed. ");
				}
				else
				{
					ReadStream rs(_recving.buff + usedIndex, ret.second);
					ProtoID protoID = rs.getProtoID();
					MessageDispatcher::getRef().dispatchSessionMessage(shared_from_this(), protoID, rs);
				}
			}
			catch (std::runtime_error e)
			{
				LCW("MessageEntry catch one exception: " << e.what());
				_sockptr->doClose();
				onClose();
				return;
			}
			usedIndex += ret.second;
		}
		else
		{
			std::string body;
			unsigned int usedLen = 0;
			auto ret = zsummer::proto4z::checkHTTPBuffIntegrity(_recving.buff + usedIndex, 
				_recving.bufflen - usedIndex, 
				MAX_BUFF_SIZE - usedIndex,
				_httpHadHeader, _httpIsChunked, _httpCommonLine, _httpHeader,
				body, usedLen);
			if (ret == zsummer::proto4z::IRT_CORRUPTION)
			{
				LCT("killed http socket: checkHTTPBuffIntegrity error sID=" << _sessionID);
				_sockptr->doClose();
				onClose();
				return;
			}
			if (ret == zsummer::proto4z::IRT_SHORTAGE)
			{
				break;
			}
			if (!_httpHadHeader)
			{
				_httpHadHeader = true;
			}
			SessionManager::getRef()._totalRecvHTTPCount++;
			MessageDispatcher::getRef().dispatchSessionHTTPMessage(shared_from_this(), _httpCommonLine, _httpHeader, body);
			usedIndex += usedLen;
		}
		
	} while (true);
	
	
	if (usedIndex > 0)
	{
		_recving.bufflen = _recving.bufflen - usedIndex;
		if (_recving.bufflen > 0)
		{
			memmove(_recving.buff, _recving.buff + usedIndex, _recving.bufflen);
		}
	}
	
	if (!doRecv())
	{
		onClose();
	}
}

void TcpSession::doSend(const char *buf, unsigned int len)
{
	if (len > MAX_SEND_PACK_SIZE)
	{
		LCE("Send Message failed!  send len = " << len << ", max send pack len=" << MAX_SEND_PACK_SIZE);
		return;
	}
	if (!_sockptr)
	{
		LCW("Send Message failed!  socket is invalid.! ");
		return;
	}
	
	
	if (!_rc4Encrypt.empty())
	{
		_rc4StateWrite.encryption((unsigned char*)buf, len);
	}
    SessionManager::getRef()._totalSendMessages++;
    SessionManager::getRef()._totalSendBytes += len;
	if (_sending.bufflen != 0)
	{
		MessageSendPack *pack = NULL;
		if (_freeCache.empty())
		{
			pack = new MessageSendPack();
		}
		else
		{
			pack = _freeCache.front();
			_freeCache.pop();
		}
		
		memcpy(pack->buff, buf, len);
		pack->bufflen = len;
		_sendque.push(pack);
	}
	else
	{
		memcpy(_sending.buff, buf, len);
		_sending.bufflen = len;
		bool sendRet = _sockptr->doSend(_sending.buff, _sending.bufflen, std::bind(&TcpSession::onSend, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
		if (!sendRet)
		{
			LCW("Send Failed");
		}
	}
}


void TcpSession::onSend(zsummer::network::NetErrorCode ec, int nSentLen)
{
	if (ec)
	{
		LCD("remote socket closed");
		return ;
	}
    SessionManager::getRef()._totalSendCount++;
	_sendingCurIndex += nSentLen;
	if (_sendingCurIndex < _sending.bufflen)
	{
		bool sendRet = _sockptr->doSend(_sending.buff + _sendingCurIndex, _sending.bufflen - _sendingCurIndex, std::bind(&TcpSession::onSend, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
		if (!sendRet)
		{
			LCW("Send Failed");
			return;
		}
		
	}
	else if (_sendingCurIndex == _sending.bufflen)
	{
		_sendingCurIndex = 0;
		_sending.bufflen = 0;
		if (!_sendque.empty())
		{
			do
			{
				MessageSendPack *pack = _sendque.front();
				_sendque.pop();
				memcpy(_sending.buff + _sending.bufflen, pack->buff, pack->bufflen);
				_sending.bufflen += pack->bufflen;
				pack->bufflen = 0;
				_freeCache.push(pack);

				if (_sendque.empty())
				{
					break;
				}
				if (MAX_BUFF_SIZE - _sending.bufflen < _sendque.front()->bufflen)
				{
					break;
				}
			} while (true);
			
			bool sendRet = _sockptr->doSend(_sending.buff, _sending.bufflen, std::bind(&TcpSession::onSend, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
			if (!sendRet)
			{
				LCW("Send Failed");
				return;
			}
		}
	}
}

void TcpSession::onPulseTimer()
{
	MessageDispatcher::getRef().dispatchOnSessionPulse(shared_from_this(), _pulseInterval);
	if (_pulseTimerID == zsummer::network::InvalidTimerID || _pulseInterval == 0)
	{
		return;
	}
	_pulseTimerID = SessionManager::getRef().createTimer(_pulseInterval, std::bind(&TcpSession::onPulseTimer, shared_from_this()));
}

void TcpSession::onClose()
{
	LCI("Client Closed!");
	_sockptr.reset();
	if (_pulseTimerID != zsummer::network::InvalidTimerID)
	{
		SessionManager::getRef().cancelTimer(_pulseTimerID);
		_pulseTimerID = zsummer::network::InvalidTimerID;
	}
	
	if (isConnectID(_sessionID))
	{
		SessionManager::getRef().onConnect(_sessionID, false, shared_from_this());
	}
	else
	{
		SessionManager::getRef().onSessionClose(_acceptID, _sessionID, shared_from_this());
	}
}


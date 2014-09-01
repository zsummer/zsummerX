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
 * Copyright (C) 2010-2014 YaweiZhang <yawei_zhang@foxmail.com>.
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




#ifndef ZSUMMER_MSG_MANAGER_H_
#define ZSUMMER_MSG_MANAGER_H_
#include <zsummerX/FrameHeader.h>


class CMessageDispatcher
{
private:
	CMessageDispatcher(){}
	typedef std::unordered_map<ProtocolID, OnSessionMessageFunction> MapSessionDispatch;
	typedef std::unordered_map<ProtocolID, OnConnectorMessageFunction> MapConnectorDispatch;
public:
	static CMessageDispatcher & getRef();
	static CMessageDispatcher * getPtr(){ return &getRef(); }
	~CMessageDispatcher(){};

	//message
	inline void RegisterSessionOrgMessage(const OnSessionOrgMessageFunction & msgfun){ m_funOrgSessionDispatch = msgfun; }
	inline void RegisterSessionMessage(ProtocolID protocolID, const OnSessionMessageFunction & msgfun){ m_mapSessionDispatch[protocolID] = msgfun; }
	inline void RegisterSessionDefaultMessage(const OnSessionMessageFunction & msgfun){ m_funDefaultSessionDispatch = msgfun; }

	inline void RegisterConnectorOrgMessage(const OnConnectorOrgMessageFunction & msgfun){ m_funOrgConnectorDispatch = msgfun; }
	inline void RegisterConnectorMessage(ProtocolID protocolID, const OnConnectorMessageFunction & msgfun){ m_mapConnectorDispatch[protocolID] = msgfun; }
	inline void RegisterConnectorDefaultMessage(const OnConnectorMessageFunction & msgfun){ m_funDefaultConnectorDispatch = msgfun; }

	//event
	inline void RegisterOnSessionEstablished(const OnSessionEstablished & fun){m_vctOnSessionEstablished.push_back(fun); }
	inline void RegisterOnSessionDisconnect(const OnSessionDisconnect & fun){ m_vctOnSessionDisconnect.push_back(fun); }
	inline void RegisterOnConnectorEstablished(const OnConnectorEstablished & fun){ m_vctOnConnectorEstablished.push_back(fun); }
	inline void RegisterOnConnectorDisconnect(const OnConnectorDisconnect & fun) { m_vctOnConnectorDisconnect.push_back(fun); }

	//heartbeat
	inline void RegisterOnMySessionHeartbeatTimer(const OnMySessionHeartbeatTimer & fun) { m_vctOnSessionHeartbeat.push_back(fun); }
	inline void RegisterOnMyConnectorHeartbeatTimer(const OnMyConnectorHeartbeatTimer &fun){ m_vctOnConnectorHeartbeat.push_back(fun); }

	//http
	inline void RegisterOnSessionHTTPMessage(const OnSessionHTTPMessageFunction & fun) { m_funSessionHTTPMessage = fun; }
	inline void RegisterOnConnectorHTTPMessage(const OnConnectorHTTPMessageFunction & fun) { m_funConnectorHTTPMessage = fun; }
public:
	inline bool DispatchOrgSessionMessage(AccepterID aID, SessionID sID, const char * blockBegin, FrameStreamTraits::Integer blockSize);
	inline void DispatchSessionMessage(AccepterID aID, SessionID sID, ProtocolID pID, ReadStreamPack & msg);
	inline bool DispatchOrgConnectorMessage(ConnectorID cID, const char * blockBegin, FrameStreamTraits::Integer blockSize);
	inline void DispatchConnectorMessage(ConnectorID cID, ProtocolID pID, ReadStreamPack & msg);
	inline void DispatchOnSessionEstablished(AccepterID aID, SessionID sID);
	inline void DispatchOnSessionDisconnect(AccepterID aID, SessionID sID);
	inline void DispatchOnConnectorEstablished(ConnectorID cID);
	inline void DispatchOnConnectorDisconnect(ConnectorID cID);
	inline void DispatchOnSessionHeartbeat(AccepterID aID, SessionID sID);
	inline void DispatchOnConnectorHeartbeat(ConnectorID cID);
	inline bool DispatchSessionHTTPMessage(AccepterID aID, SessionID sID, const zsummer::proto4z::HTTPHeadMap &head, const std::string & body);
	inline bool DispatchConnectorHTTPMessage(ConnectorID cID, const zsummer::proto4z::HTTPHeadMap &head, const std::string & body);

	private:
		//!对具体的每个消息进行注册handler回调处理
		MapSessionDispatch m_mapSessionDispatch;
		//!对消息的原始封包进行注册handler回调处理, 如果回调返回值为false 直接跳过本次封包的其他dispatch.
		//!该处理可针对所有/部分消息进行一些统一的消息处理 比如转发,屏蔽等.
		OnSessionOrgMessageFunction m_funOrgSessionDispatch;
		//!对于在原始封包处理中没有匹配到handler的消息 将统一dispatch到该handler中.
		OnSessionMessageFunction m_funDefaultSessionDispatch;

		//!对具体的每个消息进行注册handler回调处理
		MapConnectorDispatch m_mapConnectorDispatch;
		//!对消息的原始封包进行注册handler回调处理, 如果回调返回值为false 直接跳过本次封包的其他dispatch.
		//!该处理可针对所有/部分消息进行一些统一的消息处理 比如转发,屏蔽等.
		OnConnectorOrgMessageFunction m_funOrgConnectorDispatch;
		//!对于在原始封包处理中没有匹配到handler的消息 将统一dispatch到该handler中.
		OnConnectorMessageFunction m_funDefaultConnectorDispatch;

		std::vector<OnSessionEstablished> m_vctOnSessionEstablished;
		std::vector<OnSessionDisconnect> m_vctOnSessionDisconnect;
		std::vector<OnConnectorEstablished> m_vctOnConnectorEstablished;
		std::vector<OnConnectorDisconnect> m_vctOnConnectorDisconnect;

		std::vector<OnMySessionHeartbeatTimer> m_vctOnSessionHeartbeat;
		std::vector<OnMyConnectorHeartbeatTimer> m_vctOnConnectorHeartbeat;

		OnSessionHTTPMessageFunction m_funSessionHTTPMessage;
		OnConnectorHTTPMessageFunction m_funConnectorHTTPMessage;
};




inline bool CMessageDispatcher::DispatchOrgSessionMessage(AccepterID aID, SessionID sID, const char * blockBegin, FrameStreamTraits::Integer blockSize)
{
	if (m_funOrgSessionDispatch == nullptr)
	{
		return true;
	}

	try
	{
		LOGT("Entry DispatchOrgSessionMessage AccepterID=" << aID << ", SessionID=" << sID << ", blockSize=" << blockSize);
		if (!m_funOrgSessionDispatch(aID, sID, blockBegin, blockSize))
		{
			return false;
		}
	}
	catch (std::runtime_error e)
	{
		LOGE("Leave OnOrgSessionMessage With Runtime Error: AccepterID=" << aID << ", SessionID=" << sID << ", Error Message=\"" << e.what() << "\"");
		return false;
	}
	catch (...)
	{
		LOGE("Leave OnOrgSessionMessage With Unknown Runtime Error: AccepterID=" << aID << ", SessionID=" << sID);
		return false;
	}
	return true;
}

inline void CMessageDispatcher::DispatchSessionMessage(AccepterID aID, SessionID sID, ProtocolID pID, ReadStreamPack & msg)
{
	MapSessionDispatch::iterator iter = m_mapSessionDispatch.find(pID);
	if (iter == m_mapSessionDispatch.end() && m_funDefaultSessionDispatch == nullptr)
	{
		LOGE("Entry OnSessionMessage[" << pID << "] Failed: UNKNOWN ProtocolID. AccepterID=" << aID << ", SessionID=" << sID << ", ProtocolID=" << pID);
		return;
	}
	try
	{
		LOGT("Entry OnSessionMessage[" << pID << "] AccepterID=" << aID << ", SessionID=" << sID);
		if (iter != m_mapSessionDispatch.end())
		{
			(iter->second)(aID, sID, pID, msg);
		}
		else
		{
			m_funDefaultSessionDispatch(aID, sID, pID, msg);
		}
		LOGT("Leave OnSessionMessage[" << pID << "] AccepterID=" << aID << ", SessionID=" << sID);
	}
	catch (std::runtime_error e)
	{
		LOGE("Leave OnSessionMessage[" << pID << "] With Runtime Error: AccepterID=" << aID << ", SessionID=" << sID << ", rsLen=" << msg.GetStreamLen() << ", Error Message=\"" << e.what() << "\"");
	}
	catch (...)
	{
		LOGE("Leave OnSessionMessage[" << pID << "] With Unknown Runtime Error: AccepterID=" << aID << ", SessionID=" << sID);
	}
}


inline bool CMessageDispatcher::DispatchOrgConnectorMessage(ConnectorID cID, const char * blockBegin, FrameStreamTraits::Integer blockSize)
{
	if (m_funOrgConnectorDispatch == nullptr)
	{
		return true;
	}

	try
	{
		LOGT("Entry DispatchOrgConnectorMessage ConnectorID=" << cID << ", blockSize=" << blockSize);
		if (!m_funOrgConnectorDispatch(cID, blockBegin, blockSize))
		{
			return false;
		}
	}
	catch (std::runtime_error e)
	{
		LOGE("Leave OnOrgConnectorMessage With Runtime Error: ConnectorID=" << cID << ", Error Message=\"" << e.what() << "\"");
		return false;
	}
	catch (...)
	{
		LOGE("Leave OnOrgConnectorMessage With Unknown Runtime Error: ConnectorID=" << cID);
		return false;
	}
	return true;
}

inline void CMessageDispatcher::DispatchConnectorMessage(ConnectorID cID, ProtocolID pID, ReadStreamPack & msg)
{
	MapConnectorDispatch::iterator iter = m_mapConnectorDispatch.find(pID);
	if (iter == m_mapConnectorDispatch.end() && m_funDefaultConnectorDispatch == nullptr)
	{
		LOGE("Entry ConnectorMessage[" << pID << "] Failed: UNKNOWN ProtocolID. ConnectorID=" << cID << ", ProtocolID=" << pID);
		//error
		return;
	}
	try
	{
		LOGT("Entry OnConnectorMessage[" << pID << "] ConnectorID=" << cID);
		if (iter != m_mapConnectorDispatch.end())
		{
			(iter->second)(cID, pID, msg);
		}
		else
		{
			m_funDefaultConnectorDispatch(cID, pID, msg);
		}
		LOGT("Leave OnConnectorMessage[" << pID << "] ConnectorID=" << cID);
	}
	catch (std::runtime_error e)
	{
		LOGE("Leave OnConnectorMessage[" << pID << "] With Runtime Error: ConnectorID=" << cID << ", rsLen=" << msg.GetStreamLen() << ", Error Message=\"" << e.what() << "\"");
	}
	catch (...)
	{
		LOGE("Leave OnConnectorMessage[" << pID << "] With Unknown Runtime Error: ConnectorID=" << cID);
	}
}

inline void CMessageDispatcher::DispatchOnSessionEstablished(AccepterID aID, SessionID sID)
{
	if (m_vctOnSessionEstablished.empty())
	{
		return;
	}
	for (auto &fun : m_vctOnSessionEstablished)
	{
		try
		{
			LOGT("Entry OnSessionEstablished SessionID=" << sID);
			fun(aID, sID);
			LOGT("Leave OnSessionEstablished SessionID=" << sID);
		}

		catch (std::runtime_error e)
		{
			LOGE("Leave OnSessionEstablished Runtime Error: SessionID=" << sID << ", Error Message=\"" << e.what() << "\"");
		}
		catch (...)
		{
			LOGE("Leave OnSessionEstablished Unknown Runtime Error: SessionID=" << sID);
		}
	}


}
inline void CMessageDispatcher::DispatchOnSessionDisconnect(AccepterID aID, SessionID sID)
{
	if (m_vctOnSessionDisconnect.empty())
	{
		return;
	}
	for (auto & fun : m_vctOnSessionDisconnect)
	{
		try
		{
			LOGT("Entry OnSessionDisconnect SessionID=" << sID);
			fun(aID, sID);
			LOGT("Leave OnSessionDisconnect SessionID=" << sID);
		}
		catch (std::runtime_error e)
		{
			LOGE("Leave OnSessionDisconnect Runtime Error: SessionID=" << sID << ", Error Message=\"" << e.what() << "\"");
		}
		catch (...)
		{
			LOGE("Leave OnSessionDisconnect Unknown Runtime Error: SessionID=" << sID);
		}
	}
}
inline void CMessageDispatcher::DispatchOnConnectorEstablished(ConnectorID cID)
{
	if (m_vctOnConnectorEstablished.empty())
	{
		return;
	}
	for (auto & fun : m_vctOnConnectorEstablished)
	{
		try
		{
			LOGT("Entry OnConnectorEstablished ConnectorID=" << cID);
			fun(cID);
			LOGT("Leave OnConnectorEstablished ConnectorID=" << cID);
		}
		catch (std::runtime_error e)
		{
			LOGE("Leave OnConnectorEstablished Runtime Error: ConnectorID=" << cID << ", Error Message=\"" << e.what() << "\"");
		}
		catch (...)
		{
			LOGE("Leave OnConnectorEstablished With Unknown Runtime Error: ConnectorID=" << cID);
		}
	}
}

inline void CMessageDispatcher::DispatchOnConnectorDisconnect(ConnectorID cID)
{
	if (m_vctOnConnectorDisconnect.empty())
	{
		return;
	}
	for (auto & fun : m_vctOnConnectorDisconnect)
	{
		try
		{
			LOGT("Entry OnConnectorDisconnect ConnectorID=" << cID);
			fun(cID);
			LOGT("Leave OnConnectorDisconnect Success: ConnectorID=" << cID);
		}
		catch (std::runtime_error e)
		{
			LOGE("Leave OnConnectorDisconnect Runtime Error: ConnectorID=" << cID << ", Error Message=\"" << e.what() << "\"");
		}
		catch (...)
		{
			LOGE("Leave OnConnectorDisconnect Unknown Runtime Error: ConnectorID=" << cID);
		}
	}
}

inline void CMessageDispatcher::DispatchOnSessionHeartbeat(AccepterID aID, SessionID sID)
{
	if (m_vctOnSessionHeartbeat.empty())
	{
		return;
	}
	for (auto & fun : m_vctOnSessionHeartbeat)
	{
		try
		{
			LOGT("Entry OnMySessionHeartbeatTimer SessionID=" << sID);
			fun(aID, sID);
			LOGT("Leave OnMySessionHeartbeatTimer SessionID=" << sID);
		}
		catch (std::runtime_error e)
		{
			LOGE("Leave OnMySessionHeartbeatTimer Runtime Error: SessionID=" << sID << ", Error Message=\"" << e.what() << "\"");
		}
		catch (...)
		{
			LOGE("Leave OnMySessionHeartbeatTimer Unknown Runtime Error: SessionID=" << sID);
		}
	}
}

inline void CMessageDispatcher::DispatchOnConnectorHeartbeat(ConnectorID cID)
{
	if (m_vctOnConnectorHeartbeat.empty())
	{
		return;
	}
	for (auto & fun : m_vctOnConnectorHeartbeat)
	{
		try
		{
			LOGT("Entry OnMyConnectorHeartbeatTimer ConnectorID=" << cID);
			fun(cID);
			LOGT("Leave OnMyConnectorHeartbeatTimer Success: ConnectorID=" << cID);
		}
		catch (std::runtime_error e)
		{
			LOGE("Leave OnMyConnectorHeartbeatTimer Runtime Error: ConnectorID=" << cID << ", Error Message=\"" << e.what() << "\"");
		}
		catch (...)
		{
			LOGE("Leave OnMyConnectorHeartbeatTimer Unknown Runtime Error: ConnectorID=" << cID);
		}
	}

}


inline bool  CMessageDispatcher::DispatchSessionHTTPMessage(AccepterID aID, SessionID sID, const zsummer::proto4z::HTTPHeadMap &head, const std::string & body)
{
	try
	{
		LOGT("Entry OnSessionHTTPMessage AccepterID=" << aID << ", SessionID=" << sID << ", head count=" << head.size() << ", bodySize=" << body.length());
		bool ret = m_funSessionHTTPMessage(aID, sID, head, body);
		LOGT("Leave OnSessionHTTPMessage AccepterID=" << aID << ", SessionID=" << sID);
		return ret;
	}
	catch (std::runtime_error e)
	{
		LOGE("Leave OnSessionHTTPMessage With Runtime Error: AccepterID=" << aID << ", SessionID=" << sID << ", Error Message=\"" << e.what() << "\"");
	}
	catch (...)
	{
		LOGE("Leave OnSessionHTTPMessage With Unknown Runtime Error: AccepterID=" << aID << ", SessionID=" << sID);
	}
	return false;
}
inline bool  CMessageDispatcher::DispatchConnectorHTTPMessage(ConnectorID cID, const zsummer::proto4z::HTTPHeadMap &head, const std::string & body)
{
	try
	{
		LOGT("Entry OnConnectorHTTPMessage ConnectorID=" << cID << ", head count=" << head.size() << ", bodySize=" << body.length());
		bool ret = m_funConnectorHTTPMessage(cID, head, body);
		LOGT("Leave OnConnectorHTTPMessage ConnectorID=" << cID);
		return ret;
	}
	catch (std::runtime_error e)
	{
		LOGE("Leave OnConnectorHTTPMessage With Runtime Error: ConnectorID=" << cID << ", Error Message=\"" << e.what() << "\"");
	}
	catch (...)
	{
		LOGE("Leave OnConnectorHTTPMessage With Unknown Runtime Error: ConnectorID=" << cID);
	}
	return false;
}


#endif



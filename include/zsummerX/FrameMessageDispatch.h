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
	typedef std::unordered_map<ProtocolID, std::vector<OnSessionMessageFunction> > MapSessionDispatch;
	typedef std::unordered_map<ProtocolID, std::vector<OnConnectorMessageFunction> > MapConnectorDispatch;
public:
	static CMessageDispatcher & getRef();
	static CMessageDispatcher * getPtr(){ return &getRef(); }
	~CMessageDispatcher(){};

	//message
	inline void RegisterSessionOrgMessage(const OnSessionOrgMessageFunction & msgfun){ m_vctOrgSessionDispatch.push_back(msgfun); }
	inline void RegisterSessionMessage(ProtocolID protocolID, const OnSessionMessageFunction & msgfun){ m_mapSessionDispatch[protocolID].push_back(msgfun); }
	inline void RegisterSessionDefaultMessage(const OnSessionMessageFunction & msgfun){ m_vctDefaultSessionDispatch.push_back(msgfun); }

	inline void RegisterConnectorOrgMessage(const OnConnectorOrgMessageFunction & msgfun){ m_vctOrgConnectorDispatch.push_back(msgfun); }
	inline void RegisterConnectorMessage(ProtocolID protocolID, const OnConnectorMessageFunction & msgfun){ m_mapConnectorDispatch[protocolID].push_back(msgfun); }
	inline void RegisterConnectorDefaultMessage(const OnConnectorMessageFunction & msgfun){ m_vctDefaultConnectorDispatch.push_back(msgfun); }

	//event
	inline void RegisterOnSessionEstablished(const OnSessionEstablished & fun){m_vctOnSessionEstablished.push_back(fun); }
	inline void RegisterOnSessionDisconnect(const OnSessionDisconnect & fun){ m_vctOnSessionDisconnect.push_back(fun); }
	inline void RegisterOnConnectorEstablished(const OnConnectorEstablished & fun){ m_vctOnConnectorEstablished.push_back(fun); }
	inline void RegisterOnConnectorDisconnect(const OnConnectorDisconnect & fun) { m_vctOnConnectorDisconnect.push_back(fun); }

	//heartbeat
	inline void RegisterOnSessionPulse(const OnSessionPulseTimer & fun) { m_vctOnSessionPulse.push_back(fun); }
	inline void RegisterOnConnectorPulse(const OnConnectorPulseTimer &fun){ m_vctOnConnectorPulse.push_back(fun); }

	//http
	inline void RegisterOnSessionHTTPMessage(const OnSessionHTTPMessageFunction & fun) { m_vctSessionHTTPMessage.push_back(fun); }
	inline void RegisterOnConnectorHTTPMessage(const OnConnectorHTTPMessageFunction & fun) { m_vctConnectorHTTPMessage.push_back(fun); }
public:
	inline bool DispatchOrgSessionMessage(AccepterID aID, SessionID sID, const char * blockBegin, FrameStreamTraits::Integer blockSize);
	inline void DispatchSessionMessage(AccepterID aID, SessionID sID, ProtocolID pID, ReadStreamPack & msg);
	inline bool DispatchOrgConnectorMessage(ConnectorID cID, const char * blockBegin, FrameStreamTraits::Integer blockSize);
	inline void DispatchConnectorMessage(ConnectorID cID, ProtocolID pID, ReadStreamPack & msg);
	inline void DispatchOnSessionEstablished(AccepterID aID, SessionID sID);
	inline void DispatchOnSessionDisconnect(AccepterID aID, SessionID sID);
	inline void DispatchOnConnectorEstablished(ConnectorID cID);
	inline void DispatchOnConnectorDisconnect(ConnectorID cID);
	inline void DispatchOnSessionPulse(AccepterID aID, SessionID sID, unsigned int pulseInterval);
	inline void DispatchOnConnectorPulse(ConnectorID cID, unsigned int pulseInterval);
	inline bool DispatchSessionHTTPMessage(AccepterID aID, SessionID sID, const zsummer::proto4z::HTTPHeadMap &head, const std::string & body);
	inline bool DispatchConnectorHTTPMessage(ConnectorID cID, const zsummer::proto4z::HTTPHeadMap &head, const std::string & body);

	private:
		//!message
		MapSessionDispatch m_mapSessionDispatch;
		std::vector<OnSessionOrgMessageFunction> m_vctOrgSessionDispatch;
		std::vector<OnSessionMessageFunction> m_vctDefaultSessionDispatch;

		MapConnectorDispatch m_mapConnectorDispatch;
		std::vector<OnConnectorOrgMessageFunction> m_vctOrgConnectorDispatch;
		std::vector<OnConnectorMessageFunction> m_vctDefaultConnectorDispatch;

		//http
		std::vector<OnSessionHTTPMessageFunction> m_vctSessionHTTPMessage;
		std::vector<OnConnectorHTTPMessageFunction> m_vctConnectorHTTPMessage;

		//event
		std::vector<OnSessionEstablished> m_vctOnSessionEstablished;
		std::vector<OnSessionDisconnect> m_vctOnSessionDisconnect;
		std::vector<OnConnectorEstablished> m_vctOnConnectorEstablished;
		std::vector<OnConnectorDisconnect> m_vctOnConnectorDisconnect;

		std::vector<OnSessionPulseTimer> m_vctOnSessionPulse;
		std::vector<OnConnectorPulseTimer> m_vctOnConnectorPulse;
};




inline bool CMessageDispatcher::DispatchOrgSessionMessage(AccepterID aID, SessionID sID, const char * blockBegin, FrameStreamTraits::Integer blockSize)
{
	if (m_vctOrgSessionDispatch.empty())
	{
		return true;
	}

	try
	{
		for (auto & fun : m_vctOrgSessionDispatch)
		{
			LOGT("Entry DispatchOrgSessionMessage AccepterID=" << aID << ", SessionID=" << sID << ", blockSize=" << blockSize);
			if (!fun(aID, sID, blockBegin, blockSize))
			{
				return false;
			}
		}
	}
	catch (std::runtime_error e)
	{
		LOGE("Leave DispatchOrgSessionMessage With Runtime Error: AccepterID=" << aID << ", SessionID=" << sID << ", Error Message=\"" << e.what() << "\"");
		return false;
	}
	catch (...)
	{
		LOGE("Leave DispatchOrgSessionMessage With Unknown Runtime Error: AccepterID=" << aID << ", SessionID=" << sID);
		return false;
	}
	return true;
}

inline void CMessageDispatcher::DispatchSessionMessage(AccepterID aID, SessionID sID, ProtocolID pID, ReadStreamPack & msg)
{
	MapSessionDispatch::iterator iter = m_mapSessionDispatch.find(pID);
	if ((iter == m_mapSessionDispatch.end() || iter->second.empty()) && m_vctDefaultSessionDispatch.empty())
	{
		LOGE("Entry DispatchSessionMessage[" << pID << "] Failed: UNKNOWN ProtocolID. AccepterID=" << aID << ", SessionID=" << sID << ", ProtocolID=" << pID);
		return;
	}
	try
	{
		if (iter != m_mapSessionDispatch.end() && !iter->second.empty())
		{
			for (auto & fun : iter->second)
			{
				LOGT("Entry DispatchSessionMessage[" << pID << "] AccepterID=" << aID << ", SessionID=" << sID);
				msg.ResetMoveCursor();
				msg >> pID;
				fun(aID, sID, pID, msg);
			}	
		}
		else
		{
			for (auto & fun : m_vctDefaultSessionDispatch)
			{
				msg.ResetMoveCursor();
				msg >> pID;
				fun(aID, sID, pID, msg);
			}
		}
		LOGT("Leave DispatchSessionMessage[" << pID << "] AccepterID=" << aID << ", SessionID=" << sID);
	}
	catch (std::runtime_error e)
	{
		LOGE("Leave DispatchSessionMessage[" << pID << "] With Runtime Error: AccepterID=" << aID << ", SessionID=" << sID << ", rsLen=" << msg.GetStreamLen() << ", Error Message=\"" << e.what() << "\"");
	}
	catch (...)
	{
		LOGE("Leave DispatchSessionMessage[" << pID << "] With Unknown Runtime Error: AccepterID=" << aID << ", SessionID=" << sID);
	}
}


inline bool CMessageDispatcher::DispatchOrgConnectorMessage(ConnectorID cID, const char * blockBegin, FrameStreamTraits::Integer blockSize)
{
	if (m_vctOrgConnectorDispatch.empty())
	{
		return true;
	}
	try
	{
		for (auto & fun : m_vctOrgConnectorDispatch)
		{
			LOGT("Entry DispatchOrgConnectorMessage ConnectorID=" << cID << ", blockSize=" << blockSize);
			if (!fun(cID, blockBegin, blockSize))
			{
				return false;
			}
		}
	}
	catch (std::runtime_error e)
	{
		LOGE("Leave DispatchOrgConnectorMessage With Runtime Error: ConnectorID=" << cID << ", Error Message=\"" << e.what() << "\"");
		return false;
	}
	catch (...)
	{
		LOGE("Leave DispatchOrgConnectorMessage With Unknown Runtime Error: ConnectorID=" << cID);
		return false;
	}
	return true;
}

inline void CMessageDispatcher::DispatchConnectorMessage(ConnectorID cID, ProtocolID pID, ReadStreamPack & msg)
{
	MapConnectorDispatch::iterator iter = m_mapConnectorDispatch.find(pID);
	if ((iter == m_mapConnectorDispatch.end() || iter->second.empty()) && m_vctDefaultConnectorDispatch.empty())
	{
		LOGE("Entry DispatchConnectorMessage[" << pID << "] Failed: UNKNOWN ProtocolID. ConnectorID=" << cID << ", ProtocolID=" << pID);
		//error
		return;
	}
	try
	{
		
		if (iter != m_mapConnectorDispatch.end() && !iter->second.empty())
		{
			for (auto & fun : iter->second)
			{
				LOGT("Entry DispatchConnectorMessage[" << pID << "] ConnectorID=" << cID);
				msg.ResetMoveCursor();
				msg >> pID;
				fun(cID, pID, msg);
			}
			
		}
		else
		{
			for (auto & fun : m_vctDefaultConnectorDispatch)
			{
				msg.ResetMoveCursor();
				msg >> pID;
				fun(cID, pID, msg);
			}
		}
		LOGT("Leave DispatchConnectorMessage[" << pID << "] ConnectorID=" << cID);
	}
	catch (std::runtime_error e)
	{
		LOGE("Leave DispatchConnectorMessage[" << pID << "] With Runtime Error: ConnectorID=" << cID << ", rsLen=" << msg.GetStreamLen() << ", Error Message=\"" << e.what() << "\"");
	}
	catch (...)
	{
		LOGE("Leave DispatchConnectorMessage[" << pID << "] With Unknown Runtime Error: ConnectorID=" << cID);
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
			LOGT("Entry DispatchOnSessionEstablished SessionID=" << sID);
			fun(aID, sID);
			LOGT("Leave DispatchOnSessionEstablished SessionID=" << sID);
		}

		catch (std::runtime_error e)
		{
			LOGE("Leave DispatchOnSessionEstablished Runtime Error: SessionID=" << sID << ", Error Message=\"" << e.what() << "\"");
		}
		catch (...)
		{
			LOGE("Leave DispatchOnSessionEstablished Unknown Runtime Error: SessionID=" << sID);
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
			LOGT("Entry DispatchOnSessionDisconnect SessionID=" << sID);
			fun(aID, sID);
			LOGT("Leave DispatchOnSessionDisconnect SessionID=" << sID);
		}
		catch (std::runtime_error e)
		{
			LOGE("Leave DispatchOnSessionDisconnect Runtime Error: SessionID=" << sID << ", Error Message=\"" << e.what() << "\"");
		}
		catch (...)
		{
			LOGE("Leave DispatchOnSessionDisconnect Unknown Runtime Error: SessionID=" << sID);
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
			LOGT("Entry DispatchOnConnectorEstablished ConnectorID=" << cID);
			fun(cID);
			LOGT("Leave DispatchOnConnectorEstablished ConnectorID=" << cID);
		}
		catch (std::runtime_error e)
		{
			LOGE("Leave DispatchOnConnectorEstablished Runtime Error: ConnectorID=" << cID << ", Error Message=\"" << e.what() << "\"");
		}
		catch (...)
		{
			LOGE("Leave DispatchOnConnectorEstablished With Unknown Runtime Error: ConnectorID=" << cID);
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
			LOGT("Entry DispatchOnConnectorDisconnect ConnectorID=" << cID);
			fun(cID);
			LOGT("Leave DispatchOnConnectorDisconnect Success: ConnectorID=" << cID);
		}
		catch (std::runtime_error e)
		{
			LOGE("Leave DispatchOnConnectorDisconnect Runtime Error: ConnectorID=" << cID << ", Error Message=\"" << e.what() << "\"");
		}
		catch (...)
		{
			LOGE("Leave DispatchOnConnectorDisconnect Unknown Runtime Error: ConnectorID=" << cID);
		}
	}
}

inline void CMessageDispatcher::DispatchOnSessionPulse(AccepterID aID, SessionID sID, unsigned int pulseInterval)
{
	if (m_vctOnSessionPulse.empty())
	{
		return;
	}
	for (auto & fun : m_vctOnSessionPulse)
	{
		try
		{
			LOGT("Entry DispatchOnSessionPulse SessionID=" << sID);
			fun(aID, sID, pulseInterval);
			LOGT("Leave DispatchOnSessionPulse SessionID=" << sID);
		}
		catch (std::runtime_error e)
		{
			LOGE("Leave DispatchOnSessionPulse Runtime Error: SessionID=" << sID << ", Error Message=\"" << e.what() << "\"");
		}
		catch (...)
		{
			LOGE("Leave DispatchOnSessionPulse Unknown Runtime Error: SessionID=" << sID);
		}
	}
}

inline void CMessageDispatcher::DispatchOnConnectorPulse(ConnectorID cID, unsigned int pulseInterval)
{
	if (m_vctOnConnectorPulse.empty())
	{
		return;
	}
	for (auto & fun : m_vctOnConnectorPulse)
	{
		try
		{
			LOGT("Entry DispatchOnConnectorPulse ConnectorID=" << cID);
			fun(cID, pulseInterval);
			LOGT("Leave DispatchOnConnectorPulse Success: ConnectorID=" << cID);
		}
		catch (std::runtime_error e)
		{
			LOGE("Leave DispatchOnConnectorPulse Runtime Error: ConnectorID=" << cID << ", Error Message=\"" << e.what() << "\"");
		}
		catch (...)
		{
			LOGE("Leave DispatchOnConnectorPulse Unknown Runtime Error: ConnectorID=" << cID);
		}
	}

}


inline bool  CMessageDispatcher::DispatchSessionHTTPMessage(AccepterID aID, SessionID sID, const zsummer::proto4z::HTTPHeadMap &head, const std::string & body)
{
	try
	{
		for (auto & fun : m_vctSessionHTTPMessage)
		{
			LOGT("Entry DispatchSessionHTTPMessage AccepterID=" << aID << ", SessionID=" << sID << ", head count=" << head.size() << ", bodySize=" << body.length());
			if (!fun(aID, sID, head, body))
			{
				return false;
			}
			LOGT("Leave DispatchSessionHTTPMessage AccepterID=" << aID << ", SessionID=" << sID);
		}
		return true;
	}
	catch (std::runtime_error e)
	{
		LOGE("Leave DispatchSessionHTTPMessage With Runtime Error: AccepterID=" << aID << ", SessionID=" << sID << ", Error Message=\"" << e.what() << "\"");
	}
	catch (...)
	{
		LOGE("Leave DispatchSessionHTTPMessage With Unknown Runtime Error: AccepterID=" << aID << ", SessionID=" << sID);
	}
	return false;
}
inline bool  CMessageDispatcher::DispatchConnectorHTTPMessage(ConnectorID cID, const zsummer::proto4z::HTTPHeadMap &head, const std::string & body)
{
	try
	{
		for (auto & fun : m_vctConnectorHTTPMessage)
		{
			LOGT("Entry DispatchConnectorHTTPMessage ConnectorID=" << cID << ", head count=" << head.size() << ", bodySize=" << body.length());
			if (!fun(cID, head, body))
			{
				return false;
			}
			LOGT("Leave DispatchConnectorHTTPMessage ConnectorID=" << cID);
		}
		return true;
	}
	catch (std::runtime_error e)
	{
		LOGE("Leave DispatchConnectorHTTPMessage With Runtime Error: ConnectorID=" << cID << ", Error Message=\"" << e.what() << "\"");
	}
	catch (...)
	{
		LOGE("Leave DispatchConnectorHTTPMessage With Unknown Runtime Error: ConnectorID=" << cID);
	}
	return false;
}


#endif



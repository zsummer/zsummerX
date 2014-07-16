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
	typedef std::map<ProtocolID, OnSessionMessageFunction> MapSessionDispatch;
	typedef std::map<ProtocolID, OnConnectorMessageFunction> MapConnectorDispatch;
public:
	static CMessageDispatcher & getRef();
	static CMessageDispatcher * getPtr(){ return &getRef(); }
	~CMessageDispatcher(){};

	inline void RegisterSessionMessage(ProtocolID protocolID, const OnSessionMessageFunction & msgfun){ m_mapSessionDispatch[protocolID] = msgfun; }
	inline void RegisterSessionMessage(const OnSessionMessageFunction & msgfun){ m_funDefaultSessionDispatch = msgfun; }
	inline void RegisterConnectorMessage(ProtocolID protocolID, const OnConnectorMessageFunction & msgfun){ m_mapConnectorDispatch[protocolID] = msgfun; }
	inline void RegisterConnectorMessage(const OnConnectorMessageFunction & msgfun){ m_funDefaultConnectorDispatch = msgfun; }

	inline void RegisterOnSessionEstablished(const OnSessionEstablished & fun){m_vctOnSessionEstablished.push_back(fun); }
	inline void RegisterOnSessionDisconnect(const OnSessionDisconnect & fun){ m_vctOnSessionDisconnect.push_back(fun); }
	inline void RegisterOnConnectorEstablished(const OnConnectorEstablished & fun){ m_vctOnConnectorEstablished.push_back(fun); }
	inline void RegisterOnConnectorDisconnect(const OnConnectorDisconnect & fun) { m_vctOnConnectorDisconnect.push_back(fun); }

	inline void RegisterOnMySessionHeartbeatTimer(const OnMySessionHeartbeatTimer & fun) { m_vctOnSessionHeartbeat.push_back(fun); }
	inline void RegisterOnMyConnectorHeartbeatTimer(const OnMyConnectorHeartbeatTimer &fun){ m_vctOnConnectorHeartbeat.push_back(fun); }
public:
	inline void DispatchSessionMessage(AccepterID aID, SessionID sID, ProtocolID pID, ReadStreamPack & msg)
	{
		MapSessionDispatch::iterator iter = m_mapSessionDispatch.find(pID);
		if (iter == m_mapSessionDispatch.end() && m_funDefaultSessionDispatch == nullptr)
		{
			LOGE("Entry OnSessionMessage[" <<pID <<"] Failed: UNKNOWN ProtocolID. AccepterID=" << aID << ", SessionID=" << sID << ", ProtocolID=" << pID);
			//error
			return;
		}
		try
		{
			LOGD("Entry OnSessionMessage[" << pID << "] AccepterID=" << aID << ", SessionID=" << sID);
			if (iter != m_mapSessionDispatch.end())
			{
				(iter->second)(aID, sID, pID, msg);
			}
			else
			{
				m_funDefaultSessionDispatch(aID, sID, pID, msg);
			}
			LOGD("Leave OnSessionMessage[" << pID << "] AccepterID=" << aID << ", SessionID=" << sID);
		}
		catch (std::runtime_error e)
		{
			LOGE("Leave OnSessionMessage[" << pID << "] With Runtime Error: AccepterID=" << aID << ", SessionID=" << sID << ", Error Message=\"" << e.what() << "\"");
		}
		catch (...)
		{
			LOGE("Leave OnSessionMessage[" << pID << "] With Unknown Runtime Error: AccepterID=" << aID << ", SessionID=" << sID);
		}
	}
	inline void DispatchConnectorMessage(ConnectorID cID, ProtocolID pID, ReadStreamPack & msg)
	{
		MapConnectorDispatch::iterator iter = m_mapConnectorDispatch.find(pID);
		if (iter == m_mapConnectorDispatch.end() && m_funDefaultConnectorDispatch == nullptr)
		{
			LOGE("Entry ConnectorMessage[" <<pID <<"] Failed: UNKNOWN ProtocolID. ConnectorID=" << cID << ", ProtocolID=" << pID);
			//error
			return;
		}
		try
		{
			LOGD("Entry OnConnectorMessage[" << pID << "] ConnectorID=" << cID);
			if (iter != m_mapConnectorDispatch.end())
			{
				(iter->second)(cID, pID, msg);
			}
			else
			{
				m_funDefaultConnectorDispatch(cID, pID, msg);
			}
			LOGD("Leave OnConnectorMessage[" <<pID <<"] ConnectorID=" << cID);
		}
		catch (std::runtime_error e)
		{
			LOGE("Leave OnConnectorMessage[" <<pID <<"] With Runtime Error: ConnectorID=" << cID << ", Error Message=\"" << e.what() << "\"");
		}
		catch (...)
		{
			LOGE("Leave OnConnectorMessage[" <<pID <<"] With Unknown Runtime Error: ConnectorID=" << cID );
		}
	}

	inline void DispatchOnSessionEstablished(AccepterID aID, SessionID sID)
	{
		if (m_vctOnSessionEstablished.empty())
		{
			return;
		}
		for (auto &fun : m_vctOnSessionEstablished)
		{
			try
			{
				LOGD("Entry OnSessionEstablished SessionID=" << sID);
				fun(aID, sID);
				LOGD("Leave OnSessionEstablished SessionID=" << sID);
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
	inline void DispatchOnSessionDisconnect(AccepterID aID, SessionID sID)
	{
		if (m_vctOnSessionDisconnect.empty())
		{
			return;
		}
		for (auto & fun : m_vctOnSessionDisconnect)
		{
			try
			{
				LOGD("Entry OnSessionDisconnect SessionID=" << sID);
				fun(aID, sID);
				LOGD("Leave OnSessionDisconnect SessionID=" << sID);
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
	inline void DispatchOnConnectorEstablished(ConnectorID cID)
	{
		if (m_vctOnConnectorEstablished.empty())
		{
			return;
		}
		for (auto & fun : m_vctOnConnectorEstablished)
		{
			try
			{
				LOGD("Entry OnConnectorEstablished ConnectorID=" << cID);
				fun(cID);
				LOGD("Leave OnConnectorEstablished ConnectorID=" << cID);
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

	inline void DispatchOnConnectorDisconnect(ConnectorID cID)
	{
		if (m_vctOnConnectorDisconnect.empty())
		{
			return;
		}
		for (auto & fun : m_vctOnConnectorDisconnect)
		{
			try
			{
				LOGD("Entry OnConnectorDisconnect ConnectorID=" << cID);
				fun(cID);
				LOGD("Leave OnConnectorDisconnect Success: ConnectorID=" << cID);
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

	inline void DispatchOnSessionHeartbeat(AccepterID aID, SessionID sID)
	{
		if (m_vctOnSessionHeartbeat.empty())
		{
			return;
		}
		for (auto & fun: m_vctOnSessionHeartbeat)
		{
			try
			{
				LOGD("Entry OnMySessionHeartbeatTimer SessionID=" << sID);
				fun(aID, sID);
				LOGD("Leave OnMySessionHeartbeatTimer SessionID=" << sID);
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

	inline void DispatchOnConnectorHeartbeat(ConnectorID cID)
	{
		if (m_vctOnConnectorHeartbeat.empty())
		{
			return;
		}
		for (auto & fun : m_vctOnConnectorHeartbeat)
		{
			try
			{
				LOGD("Entry OnMyConnectorHeartbeatTimer ConnectorID=" << cID);
				fun(cID);
				LOGD("Leave OnMyConnectorHeartbeatTimer Success: ConnectorID=" << cID);
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

	private:
		MapSessionDispatch m_mapSessionDispatch;
		OnSessionMessageFunction m_funDefaultSessionDispatch;
		MapConnectorDispatch m_mapConnectorDispatch;
		OnConnectorMessageFunction m_funDefaultConnectorDispatch;

		std::vector<OnSessionEstablished> m_vctOnSessionEstablished;
		std::vector<OnSessionDisconnect> m_vctOnSessionDisconnect;
		std::vector<OnConnectorEstablished> m_vctOnConnectorEstablished;
		std::vector<OnConnectorDisconnect> m_vctOnConnectorDisconnect;

		std::vector<OnMySessionHeartbeatTimer> m_vctOnSessionHeartbeat;
		std::vector<OnMyConnectorHeartbeatTimer> m_vctOnConnectorHeartbeat;
};

#endif



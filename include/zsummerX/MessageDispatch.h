/*
 * ZSUMMER License
 * -----------
 * 
 * ZSUMMER is licensed under the terms of the MIT license reproduced below.
 * This means that ZSUMMER is free software and can be used for both academic
 * and commercial purposes at absolutely no cost.
 * 
 * 
 * ===============================================================================
 * 
 * Copyright (C) 2010-2013 YaweiZhang <yawei_zhang@foxmail.com>.
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
	typedef std::map<ProtocolID, std::vector<OnMessageFunction> > MapDispatch;
public:
	static CMessageDispatcher & getRef();
	~CMessageDispatcher(){};

	inline void RegisterSessionMessage(ProtocolID protocolID, const OnMessageFunction & msgfun){ m_mapSessionDispatch[protocolID].push_back(msgfun); }
	inline void RegisterConnectorMessage(ProtocolID protocolID, const OnMessageFunction & msgfun){ m_mapConnectorDispatch[protocolID].push_back(msgfun); }

	inline void RegisterOnSessionEstablished(const OnSessionEstablished & fun){ m_vctOnSessionEstablished.push_back(fun); }
	inline void RegisterOnSessionDisconnect(const OnSessionDisconnect & fun){ m_vctOnSessionDisconnect.push_back(fun); }
	inline void RegisterOnConnectorEstablished(const OnConnectorEstablished & fun){ m_vctOnConnectorEstablished.push_back(fun); }
	inline void REgisterOnConnectorDisconnect(const OnConnectorDisconnect & fun) { m_vctOnConnectorDisconnect.push_back(fun); }
public:
	inline void DispatchSessionMessage(SessionID sID, ProtocolID pID, ReadStreamPack & msg)
	{
		MapDispatch::iterator iter = m_mapSessionDispatch.find(pID);
		if (iter == m_mapSessionDispatch.end() || iter->second.empty())
		{
			LOGE("Entry SessionMessage Process Failed: UNKNOWN ProtocolID. SessionID=" << sID << ", ProtocolID=" << pID);
			//error
			return;
		}
		for (auto &dis : iter->second)
		{
			try
			{
				LOGD("Entry SessionMessage Process: SessionID=" << sID << ", ProtocolID=" << pID);
				(dis)(sID, pID, msg);
				LOGD("Leave SessionMessage Process With Success: SessionID=" << sID << ", ProtocolID=" << pID);
			}
			catch (std::runtime_error e)
			{
				LOGE("Leave SessionMessage Process With Runtime Error: SessionID=" << sID << ", ProtocolID=" << pID << ", Error Message=\"" << e.what() << "\"");
			}
			catch (...)
			{
				LOGE("Leave SessionMessage Process With Unknown Runtime Error: SessionID=" << sID << ", ProtocolID=" << pID);
			}
		}
	}
	inline void DispatchConnectorMessage(SessionID sID, ProtocolID pID, ReadStreamPack & msg)
	{
		MapDispatch::iterator iter = m_mapConnectorDispatch.find(pID);
		if (iter == m_mapConnectorDispatch.end() || iter->second.empty())
		{
			LOGE("Entry ConnectorMessage Process Failed: UNKNOWN ProtocolID. SessionID=" << sID << ", ProtocolID=" << pID);
			//error
			return;
		}
		for (auto &dis :iter->second)
		{
			try
			{
				LOGD("Entry ConnectorMessage Process: SessionID=" << sID << ", ProtocolID=" << pID);
				(dis)(sID, pID, msg);
				LOGD("Leave ConnectorMessage Process With Success: SessionID=" << sID << ", ProtocolID=" << pID);
			}
			catch (std::runtime_error e)
			{
				LOGE("Leave ConnectorMessage Process With Runtime Error: SessionID=" << sID << ", ProtocolID=" << pID << ", Error Message=\"" << e.what() << "\"");
			}
			catch (...)
			{
				LOGE("Leave ConnectorMessage Process With Unknown Runtime Error: SessionID=" << sID << ", ProtocolID=" << pID);
			}
		}
	}

	inline void DispatchOnSessionEstablished(SessionID sID)
	{ 
		for (auto & fun : m_vctOnSessionEstablished)
		{
			try
			{
				LOGD("Entry OnSessionEstablished Process: SessionID=" << sID);
				fun(sID);
				LOGD("Leave OnSessionEstablished Process With Success: SessionID=" << sID );
			}
			catch (std::runtime_error e)
			{
				LOGE("Leave OnSessionEstablished Process With Runtime Error: SessionID=" << sID  << ", Error Message=\"" << e.what() << "\"");
			}
			catch (...)
			{
				LOGE("Leave OnSessionEstablished Process With Unknown Runtime Error: SessionID=" << sID );
			}
		}
	}
	inline void DispatchOnSessionDisconnect(SessionID sID)
	{
		for (auto & fun : m_vctOnSessionDisconnect)
		{
			try
			{
				LOGD("Entry OnSessionDisconnect Process: SessionID=" << sID);
				fun(sID);
				LOGD("Leave OnSessionDisconnect Process With Success: SessionID=" << sID);
			}
			catch (std::runtime_error e)
			{
				LOGE("Leave OnSessionDisconnect Process With Runtime Error: SessionID=" << sID << ", Error Message=\"" << e.what() << "\"");
			}
			catch (...)
			{
				LOGE("Leave OnSessionDisconnect Process With Unknown Runtime Error: SessionID=" << sID);
			}
		}
	}
	inline void DispatchOnConnectorEstablished(SessionID sID)
	{
		for (auto & fun : m_vctOnConnectorEstablished)
		{
			try
			{
				LOGD("Entry OnConnector Process: SessionID=" << sID);
				fun(sID);
				LOGD("Leave OnConnector Process With Success: SessionID=" << sID);
			}
			catch (std::runtime_error e)
			{
				LOGE("Leave OnConnector Process With Runtime Error: SessionID=" << sID << ", Error Message=\"" << e.what() << "\"");
			}
			catch (...)
			{
				LOGE("Leave OnConnector Process With Unknown Runtime Error: SessionID=" << sID);
			}
		}
	}
	inline void DispatchOnConnectorDisconnect(SessionID sID)
	{
		for (auto & fun : m_vctOnConnectorDisconnect)
		{
			try
			{
				LOGD("Entry OnConnectorDisconnect Process: SessionID=" << sID);
				fun(sID);
				LOGD("Leave OnConnectorDisconnect Process With Success: SessionID=" << sID);
			}
			catch (std::runtime_error e)
			{
				LOGE("Leave OnConnectorDisconnect Process With Runtime Error: SessionID=" << sID << ", Error Message=\"" << e.what() << "\"");
			}
			catch (...)
			{
				LOGE("Leave OnConnectorDisconnect Process With Unknown Runtime Error: SessionID=" << sID);
			}
		}
	}


	private:
		MapDispatch m_mapSessionDispatch;
		MapDispatch m_mapConnectorDispatch;

		std::vector<OnSessionEstablished> m_vctOnSessionEstablished;
		std::vector<OnSessionDisconnect> m_vctOnSessionDisconnect;
		std::vector<OnConnectorEstablished> m_vctOnConnectorEstablished;
		std::vector<OnConnectorDisconnect> m_vctOnConnectorDisconnect;

};

#endif



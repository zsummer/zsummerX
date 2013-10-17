/*
* ZSUMMER_11X License
* -----------
* 
* ZSUMMER_11X is licensed under the terms of the MIT license reproduced below.
* This means that ZSUMMER_11X is free software and can be used for both academic
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



#ifndef _ZSUMMER_11X_IOCP_IMPL_H_
#define _ZSUMMER_11X_IOCP_IMPL_H_


#include "../common/common.h"


namespace zsummer
{
	namespace network
	{
		//! 消息泵, message loop.
		class CZSummerImpl
		{
		public:

			CZSummerImpl()
			{
				m_io = NULL;
				m_queSeq = 0;
			}
			~CZSummerImpl()
			{
			}
			inline bool Initialize()
			{
				if (m_io != NULL)
				{
					LCF("iocp is craeted !");
					return false;
				}
				m_io = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);
				if (!m_io)
				{
					LCF("CreateIoCompletionPort False!");
					return false;
				}
				m_nextExpire = (unsigned long long)-1;
				return true;
			}
			void RunOnce();
			template <typename handle>
			inline void Post(const handle &h)
			{
				PostMsg(PCK_USER_DATA, h);
			}

			inline void PostMsg(POST_COM_KEY pck, const _OnPostHandler &handle)
			{
				_OnPostHandler *ptr = new _OnPostHandler(handle);
				PostQueuedCompletionStatus(m_io, 0, pck,(LPOVERLAPPED)(ptr));
			}

			inline unsigned long long CreateTimer(unsigned int delayms, const _OnTimerHandler &handle)
			{
				_OnTimerHandler *pfunc= new _OnTimerHandler(handle);
				unsigned long long now = std::chrono::system_clock::now().time_since_epoch()/std::chrono::milliseconds(1);
				unsigned long long expire = now+delayms;
				assert(!(expire&0xfffff00000000000));
				expire <<= 20;
				{
					std::lock_guard<std::mutex> lg(m_lockTimer);
					m_queSeq++;
					expire |= (m_queSeq&0xfffff);

					m_queTimer.insert(std::make_pair(expire, pfunc));
					if (m_nextExpire > now+delayms || m_nextExpire < now)
					{
						m_nextExpire = now+delayms;
					}
				}
				//LCI("create timerID=" << expire << " expire time= " << delayms );
				return expire;
			}
			inline bool CancelTimer(unsigned long long timerID)
			{
				bool bRet = false;
				{
					std::lock_guard<std::mutex> l(m_lockTimer);
					std::map<unsigned long long, _OnTimerHandler* >::iterator iter = m_queTimer.find(timerID);
					if (iter != m_queTimer.end())
					{
						delete iter->second;
						m_queTimer.erase(iter);
						bRet = true;
					}
				}
				return bRet;
			}
		public:

			inline void CheckTimer()
			{
				if (!m_queTimer.empty())
				{
					unsigned long long nowMs = std::chrono::system_clock::now().time_since_epoch()/std::chrono::milliseconds(1);
					unsigned long long expire = m_nextExpire;
					if (expire <= nowMs)
					{
						std::vector<std::pair<unsigned long long, _OnTimerHandler*> > allexpire;
						{
							std::lock_guard<std::mutex> lg(m_lockTimer);
							while(1)
							{
								if (m_queTimer.empty())
								{
									m_nextExpire = (unsigned long long)-1;
									//LCI("m_queTimer.empty()");
									break;
								}
								auto iter = m_queTimer.begin();
								unsigned long long nextexpire = (iter->first)>>20;
								if (nowMs < nextexpire)
								{
									m_nextExpire = nextexpire;
									//LCI("next timerID=" << iter->first << ", next expire=" << nextexpire-cur);
									break;
								}
								//LCI("timer expire, timeID=" << iter->first);
								allexpire.push_back(*iter);
								m_queTimer.erase(iter);
							}
						}
						//LCI("allexpire size=" << allexpire.size());
						for (auto iter = allexpire.begin(); iter != allexpire.end(); ++iter)
						{
							(*(iter->second))(iter->first);
							delete iter->second;
						}
					}

				}
			}

		public:
			//! IOCP句柄
			HANDLE m_io;
			//! 定时器
			std::map<unsigned long long, _OnTimerHandler* > m_queTimer;
			unsigned int m_queSeq; //! 用于生成定时器ID
			volatile unsigned long long m_nextExpire; //! 最快触发时间
			std::mutex m_lockTimer; //! 锁


		};
	}

}





















#endif












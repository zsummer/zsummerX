/*
 * Protocol4z License
 * -----------
 * 
 * Protocol4z is licensed under the terms of the MIT license reproduced below.
 * This means that Protocol4z is free software and can be used for both academic
 * and commercial purposes at absolutely no cost.
 * 
 * 
 * ===============================================================================
 * 
 * Copyright (C) 2013-2013 YaweiZhang <yawei_zhang@foxmail.com>.
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


/*
 * AUTHORS:  YaweiZhang <yawei_zhang@foxmail.com>
 * VERSION:  0.1
 * PURPOSE:  A lightweight library for process protocol .
 * CREATION: 2013.07.04
 * LCHANGE:  2013.07.04
 * LICENSE:  Expat/MIT License, See Copyright Notice at the begin of this file.
 */

/*
 * Web Site: www.zsummer.net
 * mail: yawei_zhang@foxmail.com
 */

/* 
 * UPDATES LOG
 * 
 * VERSION 0.1.0 <DATE: 2010.10.4>
 * 	create the first project.  
 * 	support big-endian or little-endian
 * 
 */
#pragma once
#ifndef _PROTOCOL4Z_H_
#define _PROTOCOL4Z_H_

#include <string.h>
#include <string>
#ifndef WIN32
#include <stdexcept>
#else
#include <exception>
#endif
#ifndef _ZSUMMER_BEGIN
#define _ZSUMMER_BEGIN namespace zsummer {
#endif  
#ifndef _ZSUMMER_PROTOCOL4Z_BEGIN
#define _ZSUMMER_PROTOCOL4Z_BEGIN namespace protocol4z {
#endif
_ZSUMMER_BEGIN
_ZSUMMER_PROTOCOL4Z_BEGIN


//! Write Stream class
//! 
//! 包头:包体
//! 包头用双字存储, 内容为整个包的长度(包括包头).
//! [unsigned short] : [variable content]
//! 
class WriteStream
{
public:
	WriteStream(char *buf, unsigned short maxlen, bool bigEndType = false)
	{
		m_buf = buf;
		m_bufLen = maxlen;
		m_curLen = 2;
		m_reversalEndianType = false;
		//! check reversal endian.
		{
			short t = 1;
			const char *p = (const char *)&t;
			if (*p == 1)
			{
				if (bigEndType)
				{
					m_reversalEndianType = true;
				}
			}
			else
			{
				if (!bigEndType)
				{
					m_reversalEndianType = true;
				}
			}
		}
	}

public:
	template <class T>
	inline WriteStream & WriteSimpleData(T const t, unsigned short len = sizeof(T))
	{
		if (len != 1 && len != 2 && len != 4 && len != 8) //! support byte only 1,2,4,8
		{
			throw std::runtime_error("bad type id");
		}
		if (m_curLen + len > m_bufLen)
		{
			throw std::runtime_error("out of range");
		}

		do 
		{
			if (!m_reversalEndianType)
			{
				const char * p = (const char *)&t;
				m_buf[m_curLen] = *p++;
				if (len == 1) break;
				m_buf[m_curLen+1] = *p++;
				if (len == 2) break;
				m_buf[m_curLen+2] = *p++;
				m_buf[m_curLen+3] = *p++;
				if (len == 4) break;
				m_buf[m_curLen+4] = *p++;
				m_buf[m_curLen+5] = *p++;
				m_buf[m_curLen+6] = *p++;
				m_buf[m_curLen+7] = *p++;
				break;
			}
			else
			{
				const char * p = (const char *)&t;
				p+= len-1;
				m_buf[m_curLen] = *p--;
				if (len == 1) break;
				m_buf[m_curLen+1] = *p--;
				if (len == 2) break;
				m_buf[m_curLen+2] = *p--;
				m_buf[m_curLen+3] = *p--;
				if (len == 4) break;
				m_buf[m_curLen+4] = *p--;
				m_buf[m_curLen+5] = *p--;
				m_buf[m_curLen+6] = *p--;
				m_buf[m_curLen+7] = *p--;
				break;
			}
		} while (0);
		m_curLen += len;
		if (!m_reversalEndianType)
		{
			m_buf[0] = *((const char*) &m_curLen);
			m_buf[1] = *(((const char*) &m_curLen) +1);
		}
		else
		{
			m_buf[0] = *(((const char*) &m_curLen) +1);
			m_buf[1] = *((const char*) &m_curLen);
		}
		return * this;
	}
	inline WriteStream & WriteContentData(const void * data, unsigned short len)
	{
		if (m_curLen + len > m_bufLen)
		{
			throw std::runtime_error("out of range");
		}
		if (len == 0)
		{
			return *this;
		}
		memcpy(&m_buf[m_curLen], data, len);
		m_curLen += len;
		if (!m_reversalEndianType)
		{
			m_buf[0] = *((const char*) &m_curLen);
			m_buf[1] = *(((const char*) &m_curLen) +1);
		}
		else
		{
			m_buf[0] = *(((const char*) &m_curLen) +1);
			m_buf[1] = *((const char*) &m_curLen);
		}
		return *this;
	}

	inline unsigned short GetWriteLen(){return m_curLen;}
	inline WriteStream & operator << (const bool data) { return WriteSimpleData(data);}
	inline WriteStream & operator << (const char data) { return WriteSimpleData(data);}
	inline WriteStream & operator << (const unsigned char data) { return WriteSimpleData(data);}
	inline WriteStream & operator << (const short data) { return WriteSimpleData(data);}
	inline WriteStream & operator << (const unsigned short data) { return WriteSimpleData(data);}
	inline WriteStream & operator << (const int data) { return WriteSimpleData(data);}
	inline WriteStream & operator << (const unsigned int data) { return WriteSimpleData(data);}
// 	inline WriteStream & operator << (const long data) { return WriteSimpleData(data);}
// 	inline WriteStream & operator << (const unsigned long data) { return WriteSimpleData(data);}
	inline WriteStream & operator << (const long long data) { return WriteSimpleData(data);}
	inline WriteStream & operator << (const unsigned long long data) { return WriteSimpleData(data);}
	inline WriteStream & operator << (const float data) { return WriteSimpleData(data);}
	inline WriteStream & operator << (const double data) { return WriteSimpleData(data);}
	inline WriteStream & operator << (const char * data) 
	{ 
		unsigned short len = (unsigned short) strlen(data);
		WriteSimpleData(len);
		return WriteContentData(data, len);
	}
	inline WriteStream & operator << (const std::string & data) { return *this << data.c_str();}

private:
	WriteStream(){}
	char * m_buf;
	unsigned short m_bufLen;
	unsigned short m_curLen;
	bool m_reversalEndianType;
};

//! return: -1:error,  0:ready, >0: need buff length to recv.
inline int CheckBuffIntegrity(const char * buff, unsigned short curBuffLen, unsigned short maxBuffLen, bool bigEndType = false)
{
	//! 检查包头是否完整
	if (curBuffLen == 0)
	{
		return 2;
	}
	else if (curBuffLen == 1)
	{
		return 1;
	}
	//! 检查大小端字节序
	bool reversalEndianType = false;
	{
		short t = 1;
		const char *p = (const char *)&t;
		if (*p == 1)
		{
			if (bigEndType)
			{
				reversalEndianType = true;
			}
		}
		else
		{
			if (!bigEndType)
			{
				reversalEndianType = true;
			}
		}
	}


	//! 获取包长度
	unsigned short packLen = 0;
	if (!reversalEndianType)
	{
		char * p = (char *)&packLen;
		*p++ = buff[0];
		*p = buff[1];
	}
	else
	{
		char * p = (char *)&packLen;
		*p++ = buff[1];
		*p = buff[0];
	}

	//! check
	if (packLen > maxBuffLen)
	{
		return -1;
	}
	if (packLen == curBuffLen)
	{
		return 0;
	}
	if (packLen < curBuffLen)
	{
		return -1;
	}
	return packLen - curBuffLen;
}

//! Read Stream class
//! 
//! 包头   : 包体 
//! [unsigned short] : [variable content]
//!
//! header = length(header section)+ length(body section); 
//! 
class ReadStream
{
public:
	ReadStream(const char *buf, unsigned short len, bool bigEndType = false)
	{
		m_buf = buf;
		m_bufLen = len;
		m_curLen = 2;
		m_reversalEndianType = false;

		{
			short t = 1;
			const char *p = (const char *)&t;
			if (*p == 1)
			{
				if (bigEndType)
				{
					m_reversalEndianType = true;
				}
			}
			else
			{
				if (!bigEndType)
				{
					m_reversalEndianType = true;
				}
			}
		}
	}

public:
	template <class T>
	inline ReadStream & ReadSimpleData(T & t, unsigned short len = sizeof(T))
	{
		if (len != 1 && len != 2 && len != 4 && len != 8) //! support byte only 1,2,4,8
		{
			throw std::runtime_error("bad type id");
		}
		if (m_curLen + len > m_bufLen)
		{
			throw std::runtime_error("out of range");
		}

		do 
		{
			if (!m_reversalEndianType)
			{
				char * p = (char *)&t;
				*p++ = m_buf[m_curLen];
				if (len == 1) break;
				*p++ = m_buf[m_curLen+1];
				if (len == 2) break;
				*p++ = m_buf[m_curLen+2];
				*p++ = m_buf[m_curLen+3];
				if (len == 4) break;
				*p++ = m_buf[m_curLen+4];
				*p++ = m_buf[m_curLen+5];
				*p++ = m_buf[m_curLen+6];
				*p++ = m_buf[m_curLen+7];
				break;
			}
			else
			{
				char * p = (char *)&t;
				p+= len-1;
				*p-- = m_buf[m_curLen];
				if (len == 1) break;
				*p-- = m_buf[m_curLen+1];
				if (len == 2) break;
				*p-- = m_buf[m_curLen+2];
				*p-- = m_buf[m_curLen+3];
				if (len == 4) break;
				*p-- = m_buf[m_curLen+4];
				*p-- = m_buf[m_curLen+5];
				*p-- = m_buf[m_curLen+6];
				*p-- = m_buf[m_curLen+7];
				break;
			}
		} while (0);
		m_curLen += len;
		return * this;
	}
	inline const char * PeekContentData(unsigned short len)
	{
		if (m_curLen + len > m_bufLen)
		{
			throw std::runtime_error("out of range");
		}
		return &m_buf[m_curLen];
	}
	inline void SkipContentData(unsigned short len)
	{
		if (m_curLen + len > m_bufLen)
		{
			throw std::runtime_error("out of range");
		}
		m_curLen += len;
	}
	inline ReadStream & ReadContentData(char * data, unsigned short len)
	{
		memcpy(data, PeekContentData(len), len);
		SkipContentData(len);
		return *this;
	}
public:
	inline unsigned short GetReadLen(){return m_curLen;}
	inline ReadStream & operator >> (bool & data) { return ReadSimpleData(data);}
	inline ReadStream & operator >> (char & data) { return ReadSimpleData(data);}
	inline ReadStream & operator >> (unsigned char & data) { return ReadSimpleData(data);}
	inline ReadStream & operator >> (short & data) { return ReadSimpleData(data);}
	inline ReadStream & operator >> (unsigned short & data) { return ReadSimpleData(data);}
	inline ReadStream & operator >> (int & data) { return ReadSimpleData(data);}
	inline ReadStream & operator >> (unsigned int & data) { return ReadSimpleData(data);}
// 	inline ReadStream & operator >> (long & data) { return ReadSimpleData(data);}
// 	inline ReadStream & operator >> (unsigned long & data) { return ReadSimpleData(data);}
	inline ReadStream & operator >> (long long & data) { return ReadSimpleData(data);}
	inline ReadStream & operator >> (unsigned long long & data) { return ReadSimpleData(data);}
	inline ReadStream & operator >> (float & data) { return ReadSimpleData(data);}
	inline ReadStream & operator >> (double & data) { return ReadSimpleData(data);}
	inline ReadStream & operator >> (std::string & data) 
	{
		unsigned short len = 0;
		ReadSimpleData(len);
		const char * p = PeekContentData(len);
		data.assign(p, len);
		SkipContentData(len);
		return *this;
	}
private:
	ReadStream(){}
	const char * m_buf;
	unsigned short m_bufLen;
	unsigned short m_curLen;
	bool m_reversalEndianType;
};


#ifndef _ZSUMMER_END
#define _ZSUMMER_END }
#endif  
#ifndef _ZSUMMER_PROTOCOL4Z_END
#define _ZSUMMER_PROTOCOL4Z_END }
#endif

_ZSUMMER_PROTOCOL4Z_END
_ZSUMMER_END

#endif

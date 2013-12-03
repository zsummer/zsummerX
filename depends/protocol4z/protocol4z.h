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
 * VERSION:  0.2
 * PURPOSE:  A lightweight library for process protocol .
 * CREATION: 2013.07.04
 * LCHANGE:  2013.12.03
 * LICENSE:  Expat/MIT License, See Copyright Notice at the begin of this file.
 */

/*
 * Web Site: www.zsummer.net
 * mail: yawei_zhang@foxmail.com
 */

/* 
 * UPDATES LOG
 * 
 * VERSION 0.1.0 <DATE: 2013.07.4>
 * 	create the first project.  
 * 	support big-endian or little-endian
 * 
 */
#pragma once
#ifndef _PROTOCOL4Z_H_
#define _PROTOCOL4Z_H_

#include <string.h>
#include <string>
#include <assert.h>
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

//!check runtime system's endian type. it's return true if big endian type.
inline bool CheckBigEndianType()
{
	short t = 1;
	const char *p = (const char *)&t;
	if (*p == 1)
	{
		return false;
	}
	return true;
}

//!read stream header from buff
inline unsigned short ReadStreamHeader(const char *buf, bool bigEndType)
{
	unsigned short packLen = 0;
	if (CheckBigEndianType() == bigEndType)
	{
		char * p = (char *)&packLen;
		*p++ = buf[0];
		*p = buf[1];
	}
	else
	{
		char * p = (char *)&packLen;
		*p++ = buf[1];
		*p = buf[0];
	}
	return packLen;
}

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

	//! 获取包长度
	unsigned short packLen = ReadStreamHeader(buff, bigEndType);

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

//! Write Stream class
//! 
//! 包头:包体
//! 包头用双字存储, 内容为整个包的长度(包括包头).
//! [unsigned short] : [variable content]
//! 
class Stream
{
public:
	//! attach memory & check EndianType
	Stream(char *buf, unsigned short maxlen, bool bigEndType)
	{
		assert(buf);
		m_buff = buf;
		m_bufLen = maxlen;
		m_curLen = 2;
		m_reversalEndianType = (bigEndType != CheckBigEndianType());
	}
	~Stream(){}

	//! 
	inline void CheckMoveCursor(unsigned short unit)
	{
		unsigned short tmp = m_curLen+unit;
		if (tmp < m_curLen || tmp < unit)
		{
			throw std::runtime_error("out of 64K memory.");
		}
		if (tmp > m_bufLen)
		{
			throw std::runtime_error("out of memory");
		}
	}
	inline void MoveCursor(unsigned short unit)
	{
		m_curLen += unit;
	}

	//!
	inline void MemoryCopy(char *dst, char *src, unsigned short len)
	{
		if (!m_reversalEndianType)
		{
			memcpy(dst, src, len);
		}
		else
		{
			while (len >0)
			{
				*dst = src[len-1];
				dst++;
				len --;
			}
		}
	}
	//!
	inline void FillHeader()
	{
		if (m_reversalEndianType)
		{
			m_buff[0] = *(((const char*) &m_curLen) +1);
			m_buff[1] = *((const char*) &m_curLen);
		}
		else
		{
			m_buff[0] = *((const char*) &m_curLen);
			m_buff[1] = *(((const char*) &m_curLen) +1);
		}
	}

protected:
	char * m_buff;
	unsigned short m_bufLen;
	unsigned short m_curLen;
	bool m_reversalEndianType;
private:
	Stream(){}
};
class WriteStream :public Stream
{
public:
	WriteStream(char *buf, unsigned short maxlen, bool bigEndType = false):Stream(buf, maxlen, bigEndType){}
	~WriteStream(){}
public:
	template <class T>
	inline WriteStream & WriteSimpleData(T t, unsigned short len = sizeof(T))
	{
		CheckMoveCursor(len);
		MemoryCopy(m_buff+m_curLen, (char *)&t, len);
		MoveCursor(len);
		FillHeader();
		return * this;
	}
	inline WriteStream & WriteContentData(const void * data, unsigned short len)
	{
		CheckMoveCursor(len);
		memcpy(m_buff+m_curLen, data, len);
		MoveCursor(len);
		FillHeader();
		return *this;
	}

	inline unsigned short GetWriteLen(){return m_curLen;}
	inline WriteStream & operator << (bool data) { return WriteSimpleData(data);}
	inline WriteStream & operator << (char data) { return WriteSimpleData(data);}
	inline WriteStream & operator << (unsigned char data) { return WriteSimpleData(data);}
	inline WriteStream & operator << (short data) { return WriteSimpleData(data);}
	inline WriteStream & operator << (unsigned short data) { return WriteSimpleData(data);}
	inline WriteStream & operator << (int data) { return WriteSimpleData(data);}
	inline WriteStream & operator << (unsigned int data) { return WriteSimpleData(data);}
 	inline WriteStream & operator << (long data) { return WriteSimpleData((long long)data);}
 	inline WriteStream & operator << (unsigned long data) { return WriteSimpleData((unsigned long long)data);}
	inline WriteStream & operator << (long long data) { return WriteSimpleData(data);}
	inline WriteStream & operator << (unsigned long long data) { return WriteSimpleData(data);}
	inline WriteStream & operator << (float data) { return WriteSimpleData(data);}
	inline WriteStream & operator << (double data) { return WriteSimpleData(data);}
	inline WriteStream & operator << (const char *const data) 
	{
		size_t len = strlen(data);
		if (len > 65536-4)
		{
			throw std::runtime_error("string length too long");
		}
		WriteSimpleData((unsigned short)len);
		WriteContentData(data, (unsigned short)len);
		return *this;
	}
	inline WriteStream & operator << (const std::string & data) { return *this << data.c_str();}
};



//! Read Stream class
//! 
//! 包头   : 包体 
//! [unsigned short] : [variable content]
//!
//! header = length(header section)+ length(body section); 
//! 
class ReadStream :public Stream
{
public:
	ReadStream(const char *buf, unsigned short len, bool bigEndType = false):Stream((char*)buf, len, bigEndType){}
	~ReadStream(){}
public:
	template <class T>
	inline ReadStream & ReadSimpleData(T & t, unsigned short len = sizeof(T))
	{
		CheckMoveCursor(len);
		MemoryCopy((char*)&t, m_buff+m_curLen, len);
		MoveCursor(len);
		return * this;
	}
	inline const char * PeekContentData(unsigned short len)
	{
		CheckMoveCursor(len);
		return m_buff+m_curLen;
	}
	inline void SkipContentData(unsigned short len)
	{
		CheckMoveCursor(len);
		MoveCursor(len);
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
 	inline ReadStream & operator >> (long & data)
	{ 
		long long tmp = 0;
		ReadStream & ret = ReadSimpleData(tmp);
		data =(long) tmp;
		return ret;
	}
 	inline ReadStream & operator >> (unsigned long & data)
	{ 
		unsigned long long tmp = 0;
		ReadStream & ret = ReadSimpleData(tmp);
		data = (unsigned long)tmp;
		return ret;
	}
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

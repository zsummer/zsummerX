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
 * VERSION:  0.3
 * PURPOSE:  A lightweight library for process protocol .
 * CREATION: 2013.07.04
 * LCHANGE:  2014.03.13
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
 * VERSION 0.3.0 <DATE: 2014.03.13>
 *  support user-defined header
 *  WriteStream auto alloc memory
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

enum ZSummer_EndianType
{
	BigEndian,
	LittleEndian,
};

//!get runtime local endian type. 
static const unsigned short gc_localEndianType = 1;
inline ZSummer_EndianType LocalEndianType()
{
	if (*(const unsigned char *)&gc_localEndianType == 1)
	{
		return LittleEndian;
	}
	return BigEndian;
}

struct DefaultStreamHeadTrait
{
	typedef unsigned short Integer; 
	const static Integer PreOffset = 0; //前置偏移字节数
	const static Integer PackLenSize = sizeof(Integer); //存储包长的内存字节数
	const static Integer PostOffset = 0; //后置偏移字节数
	const static Integer HeadLen = PreOffset + PackLenSize + PostOffset; //头部总长
	const static bool PackLenIsContainHead = true; // 包长是否包括头部(不包含表示其长度为包体总长)
	const static ZSummer_EndianType EndianType = LittleEndian; //序列化时对整形的字节序
};

struct TestBigStreamHeadTrait
{
	typedef unsigned int Integer;
	const static Integer PreOffset = 2; //前置偏移字节数
	const static Integer PackLenSize = sizeof(Integer); //存储包长的内存字节数
	const static Integer PostOffset = 4; //后置偏移字节数
	const static Integer HeadLen = PreOffset + PackLenSize + PostOffset; //头部总长
	const static bool PackLenIsContainHead = false; // 包长是否包括头部(不包含表示其长度为包体总长)
	const static ZSummer_EndianType EndianType = BigEndian; //序列化时对整形的字节序
};

template<class Integer, class StreamHeadTrait>
Integer StreamToInteger(const char stream[sizeof(Integer)])
{
	unsigned short integerLen = sizeof(Integer);
	Integer integer = 0 ;
	if (integerLen == 1)
	{
		integer = (Integer)stream[0];
	}
	else
	{
		if (StreamHeadTrait::EndianType != LocalEndianType())
		{
			unsigned char *dst = (unsigned char*)&integer;
			unsigned char *src = (unsigned char*)stream + integerLen;
			while (integerLen > 0)
			{
				*dst++ = *--src;
				integerLen --;
			}
		}
		else
		{
			memcpy(&integer, stream, integerLen);
		}
	}
	return integer;
}
template<class Integer, class StreamHeadTrait>
void IntegerToStream(Integer integer, char *stream)
{
	unsigned short integerLen = sizeof(Integer);
	if (integerLen == 1)
	{
		stream[0] = (char)integer;
	}
	else
	{
		if (StreamHeadTrait::EndianType != LocalEndianType())
		{
			unsigned char *src = (unsigned char*)&integer + integerLen;
			unsigned char *dst = (unsigned char*)stream;
			while (integerLen > 0)
			{
				*dst++ = *--src;
				integerLen --;
			}
		}
		else
		{
			memcpy(stream, &integer, integerLen);
		}
	}
}


//! return: -1:error,  0:ready, >0: need buff length to recv.
template<class StreamHeadTrait>
inline std::pair<bool, typename StreamHeadTrait::Integer> CheckBuffIntegrity(const char * buff, typename StreamHeadTrait::Integer curBuffLen, typename StreamHeadTrait::Integer maxBuffLen)
{
	//! 检查包头是否完整
	if (curBuffLen < StreamHeadTrait::HeadLen)
	{
		return std::make_pair(true, StreamHeadTrait::HeadLen - curBuffLen);
	}

	//! 获取包长度
	typename StreamHeadTrait::Integer packLen = StreamToInteger<typename StreamHeadTrait::Integer, StreamHeadTrait>(buff+StreamHeadTrait::PreOffset);
	if (!StreamHeadTrait::PackLenIsContainHead)
	{
		typename StreamHeadTrait::Integer oldInteger = packLen;
		packLen += StreamHeadTrait::HeadLen;
		if (packLen < oldInteger) //over range
		{
			return std::make_pair(false, curBuffLen);
		}
	}

	//! check
	if (packLen > maxBuffLen)
	{
		return std::make_pair(false, curBuffLen);
	}
	if (packLen == curBuffLen)
	{
		return std::make_pair(true, (typename StreamHeadTrait::Integer)0);
	}
	if (packLen < curBuffLen)
	{
		return std::make_pair(false, curBuffLen);;
	}
	return std::make_pair(true, packLen - curBuffLen);
}




template<class StreamHeadTrait=DefaultStreamHeadTrait, class AllocType = std::allocator<char> >
class WriteStream
{
public:
	WriteStream(typename StreamHeadTrait::Integer nBufReserve = StreamHeadTrait::HeadLen)
	{
		m_data.reserve((size_t)nBufReserve);
		m_data.resize((size_t)StreamHeadTrait::HeadLen, '\0');
		m_cursor = StreamHeadTrait::HeadLen;
		m_dataLen = (typename StreamHeadTrait::Integer) m_data.length();
	}
	~WriteStream(){}
public:
	//!
	inline void FixPackLen()
	{
		typename StreamHeadTrait::Integer packLen =m_cursor;
		if (!StreamHeadTrait::PackLenIsContainHead)
		{
			packLen -= StreamHeadTrait::HeadLen;
		}
		IntegerToStream<typename StreamHeadTrait::Integer, StreamHeadTrait>(packLen, &m_data[StreamHeadTrait::PreOffset]);
	}
	inline void GetPackHead(char *& packHead)
	{
		memcpy(packHead,&m_data[0], StreamHeadTrait::HeadLen);
	}
	template <class T>
	inline WriteStream<StreamHeadTrait> & WriteIntegerData(T t, unsigned short len = sizeof(T))
	{
		m_data.append((const char*)&t, len);
		if (StreamHeadTrait::EndianType != LocalEndianType())
		{
			IntegerToStream<T,StreamHeadTrait>(t, &m_data[m_cursor]);
		}
		m_cursor += len;
		FixPackLen();
		return * this;
	}

	template <class T>
	inline WriteStream<StreamHeadTrait> & WriteSimpleData(T t, unsigned short len = sizeof(T))
	{
		m_data.append((const char*)&t, len);
		m_cursor += len;
		FixPackLen();
		return * this;
	}
	inline WriteStream<StreamHeadTrait> & WriteContentData(const void * data, typename StreamHeadTrait::Integer len)
	{
		m_data.append((const char*)data, len);
		m_cursor += len;
		FixPackLen();
		return *this;
	}

	inline char* GetWriteStream(){return &m_data[0];}
	inline typename StreamHeadTrait::Integer GetWriteLen(){return m_cursor;}

	inline WriteStream<StreamHeadTrait> & operator << (bool data) { return WriteSimpleData(data); }
	inline WriteStream<StreamHeadTrait> & operator << (char data) { return WriteSimpleData(data); }
	inline WriteStream<StreamHeadTrait> & operator << (unsigned char data) { return WriteSimpleData(data); }
	inline WriteStream<StreamHeadTrait> & operator << (short data) { return WriteIntegerData(data); }
	inline WriteStream<StreamHeadTrait> & operator << (unsigned short data) { return WriteIntegerData(data); }
	inline WriteStream<StreamHeadTrait> & operator << (int data) { return WriteIntegerData(data); }
	inline WriteStream<StreamHeadTrait> & operator << (unsigned int data) { return WriteIntegerData(data); }
	inline WriteStream<StreamHeadTrait> & operator << (long data) { return WriteIntegerData((long long)data); }
	inline WriteStream<StreamHeadTrait> & operator << (unsigned long data) { return WriteIntegerData((unsigned long long)data); }
	inline WriteStream<StreamHeadTrait> & operator << (long long data) { return WriteIntegerData(data); }
	inline WriteStream<StreamHeadTrait> & operator << (unsigned long long data) { return WriteIntegerData(data); }
	inline WriteStream<StreamHeadTrait> & operator << (float data) { return WriteSimpleData(data); }
	inline WriteStream<StreamHeadTrait> & operator << (double data) { return WriteSimpleData(data); }
	inline WriteStream<StreamHeadTrait> & operator << (const char *const data)
	{
		typename StreamHeadTrait::Integer len = (typename StreamHeadTrait::Integer)strlen(data);
		WriteIntegerData(len);
		WriteContentData(data, len);
		return *this;
	}
	inline WriteStream<StreamHeadTrait> & operator << (const std::string & data) { return *this << data.c_str(); }
private:
	std::basic_string<char, std::char_traits<char>, AllocType > m_data;
	typename StreamHeadTrait::Integer m_dataLen;
	typename StreamHeadTrait::Integer m_cursor;
};



template<class StreamHeadTrait=DefaultStreamHeadTrait>
class ReadStream
{
public:
	ReadStream(const char *buf, typename StreamHeadTrait::Integer len)
	{
		m_data = buf;
		m_dataLen = len;
		m_cursor = StreamHeadTrait::HeadLen;
	}
	~ReadStream(){}
public:
	inline void CheckMoveCursor(typename StreamHeadTrait::Integer unit)
	{
		if (m_cursor>= m_dataLen)
		{
			throw std::runtime_error("buff is full of");
		}
		if (unit > m_dataLen)
		{
			throw std::runtime_error("unit is too large");
		}
		if (m_dataLen - m_cursor < unit)
		{
			throw std::runtime_error("out of buff");
		}
	}
	inline void MoveCursor(typename StreamHeadTrait::Integer unit)
	{
		m_cursor += unit;
	}
	inline typename StreamHeadTrait::Integer GetCursor()
	{
		return m_cursor;
	}
	template <class T>
	inline ReadStream<StreamHeadTrait> & ReadIntegerData(T & t, unsigned short len = sizeof(T))
	{
		CheckMoveCursor(len);
		t = StreamToInteger<T, StreamHeadTrait>(&m_data[m_cursor]);
		MoveCursor(len);
		return * this;
	}
	template <class T>
	inline ReadStream<StreamHeadTrait> & ReadSimpleData(T & t, unsigned short len = sizeof(T))
	{
		CheckMoveCursor(len);
		memcpy(&t,&m_data[m_cursor],len);
		MoveCursor(len);
		return * this;
	}
	inline const char * PeekContentData(typename StreamHeadTrait::Integer len)
	{
		CheckMoveCursor(len);
		return &m_data[m_cursor];
	}
	inline void SkipContentData(typename StreamHeadTrait::Integer len)
	{
		CheckMoveCursor(len);
		MoveCursor(len);
	}
	inline ReadStream<StreamHeadTrait> & ReadContentData(char * data, typename StreamHeadTrait::Integer len)
	{
		memcpy(data, PeekContentData(len), len);
		SkipContentData(len);
		return *this;
	}
public:
	inline typename StreamHeadTrait::Integer GetReadLen(){return m_cursor;}
	inline ReadStream<StreamHeadTrait> & operator >> (bool & data) { return ReadSimpleData(data); }
	inline ReadStream<StreamHeadTrait> & operator >> (char & data) { return ReadSimpleData(data); }
	inline ReadStream<StreamHeadTrait> & operator >> (unsigned char & data) { return ReadSimpleData(data); }
	inline ReadStream<StreamHeadTrait> & operator >> (short & data) { return ReadIntegerData(data); }
	inline ReadStream<StreamHeadTrait> & operator >> (unsigned short & data) { return ReadIntegerData(data); }
	inline ReadStream<StreamHeadTrait> & operator >> (int & data) { return ReadIntegerData(data); }
	inline ReadStream<StreamHeadTrait> & operator >> (unsigned int & data) { return ReadIntegerData(data); }
	inline ReadStream<StreamHeadTrait> & operator >> (long & data)
	{ 
		long long tmp = 0;
		ReadStream & ret = ReadIntegerData(tmp);
		data =(long) tmp;
		return ret;
	}
	inline ReadStream<StreamHeadTrait> & operator >> (unsigned long & data)
	{ 
		unsigned long long tmp = 0;
		ReadStream & ret = ReadIntegerData(tmp);
		data = (unsigned long)tmp;
		return ret;
	}
	inline ReadStream<StreamHeadTrait> & operator >> (long long & data) { return ReadIntegerData(data); }
	inline ReadStream<StreamHeadTrait> & operator >> (unsigned long long & data) { return ReadIntegerData(data); }
	inline ReadStream<StreamHeadTrait> & operator >> (float & data) { return ReadSimpleData(data); }
	inline ReadStream<StreamHeadTrait> & operator >> (double & data) { return ReadSimpleData(data); }
	inline ReadStream<StreamHeadTrait> & operator >> (std::string & data)
	{
		typename StreamHeadTrait::Integer len = 0;
		ReadIntegerData(len);
		const char * p = PeekContentData(len);
		data.assign(p, len);
		SkipContentData(len);
		return *this;
	}
private:
	const char * m_data;
	typename StreamHeadTrait::Integer m_dataLen;
	typename StreamHeadTrait::Integer m_cursor;
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

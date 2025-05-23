/*
 * proto4z License
 * -----------
 * 
 * proto4z is licensed under the terms of the MIT license reproduced below.
 * This means that proto4z is free software and can be used for both academic
 * and commercial purposes at absolutely no cost.
 * 
 * 
 * ===============================================================================
 * 
 * Copyright (C) 2013-2017 YaweiZhang <yawei.zhang@foxmail.com>.
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
 * AUTHORS:  YaweiZhang <yawei.zhang@foxmail.com>
 * VERSION:  1.0
 * PURPOSE:  A lightweight library for process protocol .
 * CREATION: 2013.07.04
 * LCHANGE:  2017.09.18
 * LICENSE:  Expat/MIT License, See Copyright Notice at the begin of this file.
 */

/*
 * mail: yawei.zhang@foxmail.com
 */

#pragma once
#ifndef _PROTO4Z_H_
#define _PROTO4Z_H_

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <list>
#include <queue>
#include <deque>
#include <assert.h>
#include <sstream>
#include <algorithm>
#include <type_traits>
#include <functional>

#ifndef WIN32
#include <stdexcept>
#include <unistd.h>
#include <execinfo.h>
#else
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#define WIN32_LEAN_AND_MEAN
#include <exception>
#include <windows.h>
#pragma warning(push)
#pragma warning(disable : 4091)
#include <DbgHelp.h>
#pragma warning(pop)
#pragma comment(lib, "Dbghelp")
#endif
#ifndef _ZSUMMER_BEGIN
#define _ZSUMMER_BEGIN namespace zsummer {
#endif  
#ifndef _ZSUMMER_PROTO4Z_BEGIN
#define _ZSUMMER_PROTO4Z_BEGIN namespace proto4z {
#endif
_ZSUMMER_BEGIN
_ZSUMMER_PROTO4Z_BEGIN

#ifdef WIN32
#pragma warning(disable:4996)
#endif




inline std::string proto4z_traceback();

#define PROTO4Z_THROW(log)\
do{\
    std::stringstream ss; \
    ss << log << " throw by " << __FILE__ << ":" << __LINE__  << " ==> " << proto4z_traceback() << "\r\n"; \
    throw std::runtime_error(ss.str()); \
} while (0)



//////////////////////////////////////////////////////////////////////////
//! protocol traits instruction
//////////////////////////////////////////////////////////////////////////

// Memory layout
//|----------  header  ---------|-------  body  --------|
//|--packlen-|-reserve-protoID--|-------  body  --------|

//header
typedef unsigned int    LenInteger;
typedef unsigned short  ReserveInteger;
typedef unsigned short  ProtoInteger;
//header end.


const static LenInteger kMaxPacketLen = (LenInteger)(-1) > 1024 * 1024 ? 1024 * 1024 : (LenInteger)-1;

//stream translate to LenInteger with endian type.


template<class BaseType>
typename std::enable_if<true, BaseType>::type ReadPodData(const char stream[sizeof(BaseType)]);

template<class T>
void WritePodData(char *stream, T v);



//////////////////////////////////////////////////////////////////////////
//! get the residue length of packet  on the information received.
//////////////////////////////////////////////////////////////////////////


enum IntegrityType
{
    kIntegrityIntact = 0,
    kIntegrityShortage = 1,
    kIntegrityCorrupted = 2,
};
//! return value:
//! first: kIntegrityIntact data integrity. second: current integrity data lenght.
//! first: kIntegrityShortage data not integrity. second: shortage lenght.
//! first: kIntegrityCorrupted data corruption. second: data lenght
//! buff 缓冲区内容起始位置 
//! curBuffLen 当前缓冲区内容大小 
//! boundLen 当前缓冲区的边界大小, 如果对boundLen有疑惑 请填写和maxBuffLen一样的值 
//! maxBuffLen 当前缓冲区实际最大大小 
inline std::pair<IntegrityType, LenInteger>
HasRawPacket(const char * buff, LenInteger curBuffLen, LenInteger boundLen, LenInteger maxBuffLen);

template<int kBufCnt, int kBufSize, int kNoCache>
class TLSQueue
{
public:
    TLSQueue(std::function<void* (unsigned long long)> alloc = nullptr, std::function<unsigned long long(void*)> dealloc = nullptr)
    {
        if (alloc)
        {
            alloc_ = alloc;
        }
        else
        {
            alloc_ = [](unsigned long long size) {return malloc((size_t)size); };
        }
        if (dealloc)
        {
            dealloc_ = dealloc;
        }
        else
        {
            dealloc_ = [](void* addr) {free(addr); return 0; };
        }
        free_cnt_ = 0;
        used_cnt_ = 0;
        error_cnt_ = 0;
    }


    ~TLSQueue()
    {
        if (used_cnt_ > 0)
        {
            error_cnt_ += used_cnt_;
        }

        while (free_cnt_ > 0)
        {
            void* addr = tls_que_[free_cnt_ - 1];
            free_cnt_--;
            dealloc_(addr);
        }
    }

    inline char * pop()
    {
        char* ptr = nullptr;
        if (used_cnt_ >= kBufCnt)
        {
            return ptr;
        }
        
        if (free_cnt_ > 0)
        {
            ptr = tls_que_[free_cnt_ - 1];
            free_cnt_--;
            used_cnt_++;
            return ptr;
        }
        used_cnt_++;
        ptr = (char*)alloc_(kBufSize);
        return ptr;
    }

    inline void push(char * ptr)
    {
        if (ptr == nullptr)
        {
            return;
        }
        if (free_cnt_ >= kBufCnt)
        {
            error_cnt_++;
            return;
        }
        if (used_cnt_ <= 0)
        {
            error_cnt_++;
            return;
        }
        if (kNoCache)
        {
            dealloc_(ptr);
        }
        else
        {
            tls_que_[free_cnt_++] = ptr;
        }
        
        used_cnt_--;
    }
    int free_cnt()const { return free_cnt_; }
    int used_cnt()const { return used_cnt_; }
    int error_cnt()const { return error_cnt_; }

    int has_buf()const { return used_cnt_ < kBufCnt; }

   
private:
    std::function<void* (unsigned long long)> alloc_;
    std::function<unsigned long long(void *)> dealloc_;

    using BufType = char*;
    BufType tls_que_[kBufCnt];
    int free_cnt_;
    int used_cnt_;
    int error_cnt_;
};

//////////////////////////////////////////////////////////////////////////
//! class WriteStreamImpl: serializes the specified data to byte stream.
//////////////////////////////////////////////////////////////////////////
//StreamHeadTrait: User-Defined like DefaultStreamHeadTrait

template<int kBufCnt, int kBufSize, int kHasHeader, int kNoCache>
class WriteStreamImpl
{
private:
    thread_local static TLSQueue<kBufCnt, kBufSize, kNoCache> tlsque_;
public:
    //! testStream : if true then WriteStreamImpl will not do any write operation.
    //! attach : the existing memory.
    WriteStreamImpl(ProtoInteger proto_id);
    ~WriteStreamImpl();
public:
    //get total stream buff, the pointer must be used immediately.
    inline char* GetStream();
    //get total stream length.
    inline LenInteger GetStreamLen(){return cursor_;}

    //get body stream buff, the pointer used by reflecting immediately.
    inline char* GetStreamBody();
    //get body stream length.
    inline LenInteger GetStreamBodyLen(){ return cursor_ - head_len_; }

    
    template<class U>
    inline WriteStreamImpl& AppendRawData(U unit);
    inline WriteStreamImpl& AppendRawData(const void* data, LenInteger len);
    template<class U>
    inline WriteStreamImpl & ResetRawData(LenInteger offset, U unit);
    inline WriteStreamImpl & ResetRawData(LenInteger offset, const void * data, LenInteger len);


    inline WriteStreamImpl & SetReserve(ReserveInteger n);

    template<class U>
    inline typename std::enable_if<std::is_arithmetic<U>::value, WriteStreamImpl>::type & operator << (U data)
    {
        CheckCursor(sizeof(U));
        WritePodData(attach_ + cursor_, data);
        cursor_ += sizeof(data);
        if (kHasHeader)
        {
            WritePodData(attach_, cursor_);
        }
        return *this;
    }


protected:
    //! check move cursor is valid. if invalid then throw exception.
    inline void CheckCursor(LenInteger unit = (LenInteger)0);


private:

    char* attach_; //! If not attach any memory, class WriteStreamImpl will used this to managing memory.
    LenInteger cursor_; //! current move cursor.
    ReserveInteger reserve_;
    ProtoInteger proto_id_; //! proto ID
    LenInteger head_len_;
};

//http://zh.cppreference.com/w/cpp/language/storage_duration  

template<int kBufCnt, int kBufSize, int kHasHeader, int kNoCache>
thread_local  TLSQueue<kBufCnt, kBufSize, kNoCache> WriteStreamImpl<kBufCnt, kBufSize, kHasHeader, kNoCache>::tlsque_;

//////////////////////////////////////////////////////////////////////////
//class ReadStreamImpl: De-serialization the specified data from byte stream.
//////////////////////////////////////////////////////////////////////////

template<int kHasHeader>
class ReadStreamImpl
{
public:
    inline ReadStreamImpl(const char *attach, LenInteger attach_len);
    ~ReadStreamImpl(){}
public:
    //reset cursor
    inline void ResetCursor();
    //get protocol id
    inline ProtoInteger GetProtoID(){ return proto_id_; }
    //get reserve id
    inline ReserveInteger GetReserve(){ return reserve_; }
    //get attach data buff
    inline const char* GetStream();
    //get pack length in stream
    inline LenInteger GetStreamLen();

    //get body stream buff, the pointer used by reflecting immediately.
    inline const char* GetStreamBody();
    //get body stream length.
    inline LenInteger GetStreamBodyLen();

    //get current unread stream buff, the pointer used by reflecting immediately.
    inline const char* GetStreamUnread();
    //get current unread stream buff length
    inline LenInteger GetStreamUnreadLen();


    inline const char * PeekRawData(LenInteger unit);
    inline void SkipRawData(LenInteger unit);

    template <class T>
    inline typename std::enable_if<std::is_arithmetic<T>::value, ReadStreamImpl>::type & 
        operator >> (T & data)
    {
        CheckCursor(sizeof(T));
        memcpy(&data, &attach_[cursor_], sizeof(T));
        cursor_ += sizeof(T);
        return *this;
    }
   
protected:
    inline void CheckCursor(LenInteger unit);


private:
    const char * attach_;
    LenInteger attach_len_;
    LenInteger cursor_;
    ReserveInteger reserve_;
    ProtoInteger proto_id_; //! proto ID
};


//////////////////////////////////////////////////////////////////////////
//! stl container
//////////////////////////////////////////////////////////////////////////

//write c-style string
template<int kBufCnt, int kBufSize, int kHasHeader, int kNoCache>
inline WriteStreamImpl<kBufCnt, kBufSize, kHasHeader, kNoCache> & operator << (WriteStreamImpl<kBufCnt, kBufSize, kHasHeader, kNoCache> & ws, const char *const data)
{
    LenInteger len = (LenInteger)strlen(data);
    ws << len;
    ws.AppendRawData(data, len);
    return ws;
}

//write std::string
template<int kBufCnt, int kBufSize, int kHasHeader, int kNoCache, class _Traits, class _Alloc>
inline WriteStreamImpl<kBufCnt, kBufSize, kHasHeader, kNoCache> & operator << (WriteStreamImpl<kBufCnt, kBufSize, kHasHeader, kNoCache> & ws, const std::basic_string<char, _Traits, _Alloc> & data)
{
    LenInteger len = (LenInteger)data.length();
    ws << len;
    ws.AppendRawData(data.c_str(), len);
    return ws;
}
//read std::string
template<int kHasHeader, class _Traits, class _Alloc>
inline ReadStreamImpl<kHasHeader> & operator >> (ReadStreamImpl<kHasHeader> & rs, std::basic_string<char, _Traits, _Alloc> & data)
{
    LenInteger len = 0;
    rs >> len;
    data.assign(rs.PeekRawData(len), len);
    rs.SkipRawData(len);
    return rs;
}


//std::vector
template<int kBufCnt, int kBufSize, int kHasHeader, int kNoCache, class U, class _Alloc>
inline WriteStreamImpl<kBufCnt, kBufSize, kHasHeader, kNoCache> & operator << (WriteStreamImpl<kBufCnt, kBufSize, kHasHeader, kNoCache> & ws, const std::vector<U, _Alloc> & vct)
{
    ws << (LenInteger)vct.size();
    for (typename std::vector<U, _Alloc>::const_iterator iter = vct.begin(); iter != vct.end(); ++iter)
    {
        ws << *iter;
    }
    return ws;
}

template<int kHasHeader, typename T, class _Alloc>
inline ReadStreamImpl<kHasHeader> & operator >> (ReadStreamImpl<kHasHeader> & rs, std::vector<T, _Alloc> & vct)
{
    LenInteger totalCount = 0;
    rs >> totalCount;
    if (totalCount > 0)
    {
        T t;
        vct.clear();
        vct.reserve(totalCount > 100 ? 100 : totalCount);
        for (LenInteger i = 0; i < totalCount; ++i)
        {
            rs >> t;
            vct.push_back(t);
        }
    }
    return rs;
}

//std::set
template<int kBufCnt, int kBufSize, int kHasHeader, int kNoCache, class Key, class _Pr, class _Alloc>
inline WriteStreamImpl<kBufCnt, kBufSize, kHasHeader, kNoCache> & operator << (WriteStreamImpl<kBufCnt, kBufSize, kHasHeader, kNoCache> & ws, const std::set<Key, _Pr, _Alloc> & k)
{
    ws << (LenInteger)k.size();
    for (typename std::set<Key, _Pr, _Alloc>::const_iterator iter = k.begin(); iter != k.end(); ++iter)
    {
        ws << *iter;
    }
    return ws;
}

template<int kHasHeader, class Key, class _Pr, class _Alloc>
inline ReadStreamImpl<kHasHeader> & operator >> (ReadStreamImpl<kHasHeader> & rs, std::set<Key, _Pr, _Alloc> & k)
{
    LenInteger totalCount = 0;
    rs >> totalCount;
    Key t;
    k.clear();
    for (LenInteger i = 0; i < totalCount; ++i)
    {
        rs >> t;
        k.insert(t);
    }
    return rs;
}

//std::multiset
template<int kBufCnt, int kBufSize, int kHasHeader, int kNoCache, class Key, class _Pr, class _Alloc>
inline WriteStreamImpl<kBufCnt, kBufSize, kHasHeader, kNoCache> & operator << (WriteStreamImpl<kBufCnt, kBufSize, kHasHeader, kNoCache> & ws, const std::multiset<Key, _Pr, _Alloc> & k)
{
    ws << (LenInteger)k.size();
    for (typename std::multiset<Key, _Pr, _Alloc>::const_iterator iter = k.begin(); iter != k.end(); ++iter)
    {
        ws << *iter;
    }
    return ws;
}

template<int kHasHeader, class Key, class _Pr, class _Alloc>
inline ReadStreamImpl<kHasHeader> & operator >> (ReadStreamImpl<kHasHeader> & rs, std::multiset<Key, _Pr, _Alloc> & k)
{
    LenInteger totalCount = 0;
    rs >> totalCount;
    Key t;
    k.clear();
    for (LenInteger i = 0; i < totalCount; ++i)
    {
        rs >> t;
        k.insert(t);
    }
    return rs;
}

//std::map
template<int kBufCnt, int kBufSize, int kHasHeader, int kNoCache, class Key, class Value, class _Pr, class _Alloc>
inline WriteStreamImpl<kBufCnt, kBufSize, kHasHeader, kNoCache> & operator << (WriteStreamImpl<kBufCnt, kBufSize, kHasHeader, kNoCache> & ws, const std::map<Key, Value, _Pr, _Alloc> & kv)
{
    ws << (LenInteger)kv.size();
    for (typename std::map<Key, Value, _Pr, _Alloc>::const_iterator iter = kv.begin(); iter != kv.end(); ++iter)
    {
        ws << iter->first;
        ws << iter->second;
    }
    return ws;
}

template<int kHasHeader, class Key, class Value, class _Pr, class _Alloc>
inline ReadStreamImpl<kHasHeader> & operator >> (ReadStreamImpl<kHasHeader> & rs, std::map<Key, Value, _Pr, _Alloc> & kv)
{
    LenInteger totalCount = 0;
    rs >> totalCount;
    std::pair<Key, Value> pr;
    kv.clear();
    for (LenInteger i = 0; i < totalCount; ++i)
    {
        rs >> pr.first;
        rs >> pr.second;
        kv.insert(pr);
    }
    return rs;
}

//std::multimap
template<int kBufCnt, int kBufSize, int kHasHeader, int kNoCache, class Key, class Value, class _Pr, class _Alloc>
inline WriteStreamImpl<kBufCnt, kBufSize, kHasHeader, kNoCache> & operator << (WriteStreamImpl<kBufCnt, kBufSize, kHasHeader, kNoCache> & ws, const std::multimap<Key, Value, _Pr, _Alloc> & kv)
{
    ws << (LenInteger)kv.size();
    for (typename std::multimap<Key, Value, _Pr, _Alloc>::const_iterator iter = kv.begin(); iter != kv.end(); ++iter)
    {
        ws << iter->first;
        ws << iter->second;
    }
    return ws;
}

template<int kHasHeader, class Key, class Value, class _Pr, class _Alloc>
inline ReadStreamImpl<kHasHeader> & operator >> (ReadStreamImpl<kHasHeader> & rs, std::multimap<Key, Value, _Pr, _Alloc> & kv)
{
    LenInteger totalCount = 0;
    rs >> totalCount;
    std::pair<Key, Value> pr;
    kv.clear();
    for (LenInteger i = 0; i < totalCount; ++i)
    {
        rs >> pr.first;
        rs >> pr.second;
        kv.insert(pr);
    }
    return rs;
}


//std::list
template<int kBufCnt, int kBufSize, int kHasHeader, int kNoCache, class Value, class _Alloc>
inline WriteStreamImpl<kBufCnt, kBufSize, kHasHeader, kNoCache> & operator << (WriteStreamImpl<kBufCnt, kBufSize, kHasHeader, kNoCache> & ws, const std::list<Value, _Alloc> & l)
{
    ws << (LenInteger)l.size();
    for (typename std::list<Value,_Alloc>::const_iterator iter = l.begin(); iter != l.end(); ++iter)
    {
        ws << *iter;
    }
    return ws;
}

template<int kHasHeader, class Value, class _Alloc>
inline ReadStreamImpl<kHasHeader> & operator >> (ReadStreamImpl<kHasHeader> & rs, std::list<Value, _Alloc> & l)
{
    LenInteger totalCount = 0;
    rs >> totalCount;
    Value t;
    l.clear();
    for (LenInteger i = 0; i < totalCount; ++i)
    {
        rs >> t;
        l.push_back(t);
    }
    return rs;
}
//std::deque
template<int kBufCnt, int kBufSize, int kHasHeader, int kNoCache, class Value, class _Alloc>
inline WriteStreamImpl<kBufCnt, kBufSize, kHasHeader, kNoCache> & operator << (WriteStreamImpl<kBufCnt, kBufSize, kHasHeader, kNoCache> & ws, const std::deque<Value, _Alloc> & l)
{
    ws << (LenInteger)l.size();
    for (typename std::deque<Value,_Alloc>::const_iterator iter = l.begin(); iter != l.end(); ++iter)
    {
        ws << *iter;
    }
    return ws;
}

template<int kHasHeader, class Value, class _Alloc>
inline ReadStreamImpl<kHasHeader> & operator >> (ReadStreamImpl<kHasHeader> & rs, std::deque<Value, _Alloc> & l)
{
    LenInteger totalCount = 0;
    rs >> totalCount;
    Value t;
    l.clear();
    for (LenInteger i = 0; i < totalCount; ++i)
    {
        rs >> t;
        l.push_back(t);
    }
    return rs;
}



//////////////////////////////////////////////////////////////////////////
//! implement 
//////////////////////////////////////////////////////////////////////////




template<class BaseType>
typename std::enable_if<true, BaseType>::type ReadPodData(const char stream[sizeof(BaseType)])
{
    BaseType v = 0;
    memcpy(&v, stream, sizeof(BaseType));
    return v;
}

template<class T>
void WritePodData(char *stream, T v)
{
    memcpy(stream, &v, sizeof(T));
}


//////////////////////////////////////////////////////////////////////////
//! implement 
//////////////////////////////////////////////////////////////////////////



inline std::pair<IntegrityType, LenInteger> HasRawPacket(const char * buff, LenInteger curBuffLen, LenInteger boundLen, LenInteger maxBuffLen)
{
    if (boundLen < curBuffLen || maxBuffLen < boundLen)
    {
        return std::make_pair(kIntegrityCorrupted, curBuffLen);
    }

    unsigned short headLen = (unsigned short)sizeof(LenInteger) + (unsigned short)sizeof(ProtoInteger);
    if (curBuffLen < headLen)
    {
        return std::make_pair(kIntegrityShortage, headLen - curBuffLen);
    }

    LenInteger packLen = ReadPodData<LenInteger>(buff);
    if (packLen < headLen)
    {
        return std::make_pair(kIntegrityCorrupted, curBuffLen);
    }
    if (packLen > boundLen)
    {
        if (packLen > maxBuffLen)
        {
            return std::make_pair(kIntegrityCorrupted, curBuffLen);
        }
        else
        {
            return std::make_pair(kIntegrityShortage, packLen - curBuffLen);
        }
    }
    
    //! check
    if (packLen > maxBuffLen)
    {
        return std::make_pair(kIntegrityCorrupted, curBuffLen);
    }
    if (packLen == curBuffLen)
    {
        return std::make_pair(kIntegrityIntact, packLen);
    }
    if (packLen < curBuffLen)
    {
        return std::make_pair(kIntegrityIntact, packLen);
    }
    return std::make_pair(kIntegrityShortage, packLen - curBuffLen);
}



//////////////////////////////////////////////////////////////////////////
//! implement 
//////////////////////////////////////////////////////////////////////////

template<int kBufCnt, int kBufSize, int kHasHeader, int kNoCache>
WriteStreamImpl<kBufCnt, kBufSize, kHasHeader, kNoCache>::WriteStreamImpl(ProtoInteger proto_id)
{
    attach_ = nullptr;
    cursor_ = 0;
    reserve_ = 0;
    proto_id_ = proto_id;
    head_len_ = 0;
    attach_ = tlsque_.pop();
    if (attach_ == nullptr)
    {
        PROTO4Z_THROW("tlsque_ no buff to attach . kBufSize=" << kBufSize );
    }

    if (kHasHeader)
    {
        LenInteger headLen = sizeof(LenInteger) + sizeof(ReserveInteger) + sizeof(ProtoInteger);
        head_len_ = headLen;
        cursor_ = head_len_;
        WritePodData(attach_, head_len_);
        WritePodData(attach_ + sizeof(LenInteger), reserve_);
        WritePodData(attach_ + sizeof(LenInteger) + sizeof(ReserveInteger), proto_id);
    }
}

template<int kBufCnt, int kBufSize, int kHasHeader, int kNoCache>
WriteStreamImpl<kBufCnt, kBufSize, kHasHeader, kNoCache>::~WriteStreamImpl()
{
    if (attach_ != nullptr)
    {
        tlsque_.push(attach_);
        attach_ = nullptr;
    }
}


template<int kBufCnt, int kBufSize, int kHasHeader, int kNoCache>
inline void WriteStreamImpl<kBufCnt, kBufSize, kHasHeader, kNoCache>::CheckCursor(LenInteger unit)
{
    if (cursor_ > kBufSize)
    {
        PROTO4Z_THROW("bound over. cursor in end-of-data. kBufSize=" << kBufSize << ", cursor_=" << cursor_);
    }
    if (unit > kBufSize)
    {
        PROTO4Z_THROW("bound over. new unit be discarded. kBufSize=" << kBufSize << ", cursor_=" << cursor_ << ", unit=" << unit);
    }
    if (kBufSize - cursor_ < unit)
    {
        PROTO4Z_THROW("bound over. new unit be discarded. kBufSize=" << kBufSize << ", cursor_=" << cursor_ << ", unit=" << unit);
    }
}


template<int kBufCnt, int kBufSize, int kHasHeader, int kNoCache>
inline char* WriteStreamImpl<kBufCnt, kBufSize, kHasHeader, kNoCache>::GetStream()
{
    return attach_;
}

template<int kBufCnt, int kBufSize, int kHasHeader, int kNoCache>
inline char* WriteStreamImpl<kBufCnt, kBufSize, kHasHeader, kNoCache>::GetStreamBody()
{
    return attach_ + head_len_;
}

template<int kBufCnt, int kBufSize, int kHasHeader, int kNoCache>
template<class U>
inline WriteStreamImpl<kBufCnt, kBufSize, kHasHeader, kNoCache>& WriteStreamImpl<kBufCnt, kBufSize, kHasHeader, kNoCache>::AppendRawData(U unit)
{
    CheckCursor(sizeof(unit));
    WritePodData(attach_ + cursor_, unit);
    cursor_ += sizeof(unit);

    if (kHasHeader)
    {
        WritePodData(attach_, cursor_);
    }
    return *this;
}

template<int kBufCnt, int kBufSize, int kHasHeader, int kNoCache>
inline WriteStreamImpl<kBufCnt, kBufSize, kHasHeader, kNoCache> & WriteStreamImpl<kBufCnt, kBufSize, kHasHeader, kNoCache>::AppendRawData(const void * data, LenInteger len)
{
    CheckCursor(len);
    memcpy(attach_ + cursor_, data, len);
    cursor_ += len;
    if (kHasHeader)
    {
        WritePodData(attach_, cursor_);
    }
    return *this;
}

template<int kBufCnt, int kBufSize, int kHasHeader, int kNoCache>
template<class U>
inline WriteStreamImpl<kBufCnt, kBufSize, kHasHeader, kNoCache> & WriteStreamImpl<kBufCnt, kBufSize, kHasHeader, kNoCache>::ResetRawData(LenInteger offset, U unit)
{
    if (offset + sizeof(unit) > cursor_)
    {
        PROTO4Z_THROW("ResetRawData over stream.  cursor_=" << cursor_ << ", unit=" << unit << ", offset=" << offset);
    }

    WritePodData(attach_ + offset, unit);
    return *this;
}

template<int kBufCnt, int kBufSize, int kHasHeader, int kNoCache>
inline WriteStreamImpl<kBufCnt, kBufSize, kHasHeader, kNoCache> & WriteStreamImpl<kBufCnt, kBufSize, kHasHeader, kNoCache>::ResetRawData(LenInteger offset, const void * data, LenInteger len)
{
    if (offset + len > cursor_)
    {
        PROTO4Z_THROW("ResetRawData over stream.  cursor_=" << cursor_ << ", data len=" << len << ", offset=" << offset);
    }
    memcpy(attach_ + offset, data, len);
    return *this;
}

template<int kBufCnt, int kBufSize, int kHasHeader, int kNoCache>
inline WriteStreamImpl<kBufCnt, kBufSize, kHasHeader, kNoCache> & WriteStreamImpl<kBufCnt, kBufSize, kHasHeader, kNoCache>::SetReserve(ReserveInteger n)
{
    return ResetRawData(sizeof(LenInteger), n);
}








//////////////////////////////////////////////////////////////////////////
//! implement 
//////////////////////////////////////////////////////////////////////////
template<int kHasHeader>
inline ReadStreamImpl<kHasHeader>::ReadStreamImpl(const char *attach, LenInteger attach_len)
{
    attach_ = attach;
    attach_len_ = attach_len;
    reserve_ = 0;
    cursor_ = 0;
    proto_id_ = 0;

    if (kHasHeader)
    {
        if (attach_len_ > kMaxPacketLen)
        {
            attach_len_ = kMaxPacketLen;
        }

        if (attach_len_ < sizeof(LenInteger) + sizeof(ReserveInteger) + sizeof(ProtoInteger))
        {
            PROTO4Z_THROW("ReadStreamImpl attach buff less then head len. attach_len_=" << attach_len_ << ", cursor_=" << cursor_ << ", kHasHeader=" << kHasHeader);
        }

        cursor_ = sizeof(LenInteger) + sizeof(ReserveInteger) + sizeof(ProtoInteger);
        LenInteger len = ReadPodData<LenInteger>(attach_);
        reserve_ = ReadPodData<ReserveInteger>(attach_ + sizeof(LenInteger));
        proto_id_ = ReadPodData<ProtoInteger>(attach_ + sizeof(LenInteger) + sizeof(ReserveInteger));
        if (len < attach_len_) // if stream invalid, ReadStreamImpl try read data as much as possible.
        {
            attach_len_ = len;
        }
    }

}

template<int kHasHeader>
inline void ReadStreamImpl<kHasHeader>::ResetCursor()
{
    cursor_ = 0;
    if (kHasHeader)
    {
        cursor_ = sizeof(LenInteger) + sizeof(ReserveInteger) + sizeof(ProtoInteger);
    }
}

template<int kHasHeader>
inline void ReadStreamImpl<kHasHeader>::CheckCursor(LenInteger unit)
{
    if (cursor_ > attach_len_)
    {
        PROTO4Z_THROW("bound over. cursor in end-of-data. attach_len_=" << attach_len_ << ", cursor_=" << cursor_ << ", kHasHeader=" << kHasHeader);
    }
    if (unit > attach_len_)
    {
        PROTO4Z_THROW("bound over. new unit be discarded. attach_len_=" << attach_len_ << ", cursor_=" << cursor_ << ", kHasHeader=" << kHasHeader);
    }
    if (attach_len_ - cursor_ < unit)
    {
        PROTO4Z_THROW("bound over. new unit be discarded. attach_len_=" << attach_len_ << ", cursor_=" << cursor_ << ", kHasHeader=" << kHasHeader);
    }
}


template<int kHasHeader>
inline const char* ReadStreamImpl<kHasHeader>::GetStream()
{
    return attach_;
}

template<int kHasHeader>
inline LenInteger ReadStreamImpl<kHasHeader>::GetStreamLen()
{
    return attach_len_;
}

template<int kHasHeader>
inline const char* ReadStreamImpl<kHasHeader>::GetStreamBody()
{
    if (kHasHeader)
    {
        return attach_ + sizeof(LenInteger) + sizeof(ReserveInteger) + sizeof(ProtoInteger);
    }
    return attach_;
}

template<int kHasHeader>
inline LenInteger ReadStreamImpl<kHasHeader>::GetStreamBodyLen()
{
    if (kHasHeader)
    {
        return GetStreamLen() - sizeof(LenInteger) - sizeof(ReserveInteger) - sizeof(ProtoInteger);
    }
    return GetStreamLen();
}

template<int kHasHeader>
inline const char* ReadStreamImpl<kHasHeader>::GetStreamUnread()
{
    return &attach_[cursor_];
}

template<int kHasHeader>
inline LenInteger ReadStreamImpl<kHasHeader>::GetStreamUnreadLen()
{
    return GetStreamLen()  - cursor_;
}


template<int kHasHeader>
inline const char * ReadStreamImpl<kHasHeader>::PeekRawData(LenInteger unit)
{
    CheckCursor(unit);
    return &attach_[cursor_];
}

template<int kHasHeader>
inline void ReadStreamImpl<kHasHeader>::SkipRawData(LenInteger unit)
{
    CheckCursor(unit);
    cursor_ += unit;
}








/////////////////////////
//http proto 
/////////////////////////
const char *const CRLF = "\r\n";
const char CR = '\r'; //CRLF
const char LF = '\n';
const char SEGM = ':';
const char BLANK = ' ';


inline std::pair<IntegrityType, unsigned int> HasWebRawPacket(const char * buff, unsigned int curBuffLen, unsigned int maxBuffLen,
                            bool & isChunked, std::string & method, std::string & line, std::map<std::string,std::string> & head, std::string & body);

std::string urlEncode(const std::string& orgString);
//"Content-Type" value is "application/x-www-form-urlencoded" means that your POST body will need to be URL encoded just like a GET parameter string.  
std::string urlDecode(const std::string& orgString);
class WriteWebStream
{
public:
    const char * GetStream(){ return buff_.c_str();}
    unsigned int GetStreamLen() { return (unsigned int)buff_.length();}
    void addHead(std::string key, std::string val)
    {
        head_.insert(std::make_pair(key, val));
    }
    void post(std::string uri, std::string content)
    {
        char buf[100];
        sprintf(buf, "%u", (unsigned int)content.length());
        head_.insert(std::make_pair("Content-Length", buf));
        buff_.append("POST " + uri + " HTTP/1.1" + CRLF);
        if (head_.find("Connection") == head_.end())
        {
            head_.insert(std::make_pair("Connection", " keep-alive"));
        }
        if (head_.find("User-Agent") == head_.end())
        {
            head_.insert(std::make_pair("User-Agent", " Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) proto4z"));
        }
        writeGeneralHead();
        buff_.append(CRLF);
        buff_.append(content);
    }
    void get(std::string uri)
    {
        head_.insert(std::make_pair("Content-Length", "0"));
        buff_.append("GET " + uri + " HTTP/1.1" + CRLF);
        if (head_.find("Connection") == head_.end())
        {
            head_.insert(std::make_pair("Connection", " keep-alive"));
        }
        if (head_.find("User-Agent") == head_.end())
        {
            head_.insert(std::make_pair("User-Agent", " Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) proto4z"));
        }
        writeGeneralHead();
        buff_.append(CRLF);
    }
    void response(std::string statusCode, std::string content)
    {
        char buf[100];
        sprintf(buf, "%u", (unsigned int)content.length());
        head_.insert(std::make_pair("Content-Length", buf));
        buff_.append("HTTP/1.1 " + statusCode + " ^o^" + CRLF);
        writeGeneralHead();
        buff_.append(CRLF);
        buff_.append(content);
    }
protected:
    void writeGeneralHead()
    {

        if (head_.find("Accept") == head_.end())
        {
            head_.insert(std::make_pair("Accept", " */*"));
        }

        if (head_.find("Accept-Languaget") == head_.end())
        {
            head_.insert(std::make_pair("Accept-Languaget", " zh-CN,zh;q=0.8"));
        }
        if (head_.find("Content-Type") == head_.end())
        {
            head_.insert(std::make_pair("Content-Type", " text/html; charset=utf-8"));
        }     

        for (auto iter = head_.begin(); iter != head_.end(); ++iter)
        {
            buff_.append(iter->first + ":" + iter->second + CRLF);
        }

    }
private:
    std::map<std::string, std::string> head_;
    std::string buff_;
};


inline std::string urlEncode(const std::string & orgString)
{
    std::string ret;
    for (int i = 0; i < (int)orgString.length(); ++i)
    {
        char ch = orgString[i];
        if (ch == '\0')
        {
            break;
        }
        if ((ch >= 'A' && ch <= 'Z')
            || (ch >= 'a' && ch <= 'z')
            || (ch >= '0' && ch <= '9')
            || ch == '-' || ch == '_' || ch == '.' || ch == '~')
        {
            ret += ch;
        }
        else if (ch == ' ')
        {
            ret += '+';
        }
        else
        {
            ret += '%';
            unsigned char tmp = ch / 16;
            ret += tmp > 9 ? tmp + 55 : tmp + 48;
            tmp = ch % 16;
            ret += tmp > 9 ? tmp + 55 : tmp + 48;
        }
    }
    return ret;
}
inline std::string urlDecode(const std::string & orgString)
{
    std::string ret;
    unsigned int count = (unsigned int)orgString.length();
    unsigned int cursor = 0;
    while (cursor < count)
    {
        char ch = orgString[cursor];
        if (ch == '\0')
        {
            break;
        }
        if (ch == '+')
        {
            ret += ' ';
            cursor++;
            continue;
        }
        if (ch == '%')
        {
            if (count - cursor < 2)
            {
                break; //error
            }
            unsigned char och;
            unsigned char x = orgString[cursor + 1];
            if (x >= 'A' && x <= 'Z') x = x - 'A' + 10;
            else if (x >= 'a' && x <= 'z') x = x - 'a' + 10;
            else if (x >= '0' && x <= '9') x = x - '0';
            och = x * 16;
            x = orgString[cursor + 2];
            if (x >= 'A' && x <= 'Z') x = x - 'A' + 10;
            else if (x >= 'a' && x <= 'z') x = x - 'a' + 10;
            else if (x >= '0' && x <= '9') x = x - '0';
            och += x;
            ret += och;
            cursor += 3;
            continue;
        }
        ret += ch;
        cursor++;
    }
    return ret;
}


inline unsigned int InnerReadLine(const char * buff, unsigned int curBuffLen, unsigned int maxBuffLen,  bool isKV, bool isCommondLine,
    std::string & outStringKey, std::string &outStringValue)
{
    unsigned int cursor = 0;
    short readStatus = 0;
    IntegrityType isIntegrityData = kIntegrityShortage;

    outStringKey.clear();
    outStringValue.clear();

    while (cursor < curBuffLen)
    {
        if (cursor >= maxBuffLen)
        {
            isIntegrityData = kIntegrityCorrupted;
            break;
        }
        if (buff[cursor] == CR)
        {
            cursor++;
            continue;
        }
        if (buff[cursor] == LF)
        {
            cursor++;
            isIntegrityData = kIntegrityIntact;
            break;
        }
        if (!isKV)
        {
            outStringKey.push_back(buff[cursor]);
            cursor++;
            continue;
        }
        else
        {
            if (isCommondLine)
            {
                if (buff[cursor] == BLANK)
                {
                    readStatus ++;
                    cursor++;
                    continue;
                }
            }
            else // ! isCommondLine
            {
                if (buff[cursor] == SEGM)
                {
                    readStatus ++;
                    cursor++;
                    continue;
                }
                if (buff[cursor] == BLANK && readStatus == 0)
                {
                    cursor++;
                    continue;
                }
                if (buff[cursor] == BLANK && readStatus == 1 && outStringValue.empty())
                {
                    cursor++;
                    continue;
                }
            }//end. !isCommondLine
            if (readStatus == 0)
            {
                outStringKey.push_back(buff[cursor]);
            }
            else if (readStatus == 1)
            {
                outStringValue.push_back(buff[cursor]);
            }
            cursor++;
            continue;
        }//end. isKV
    }//extract character loop

    if (isIntegrityData != kIntegrityIntact)
    {
        throw isIntegrityData;
    }
    return cursor;
}

inline std::pair<IntegrityType, unsigned int> HasWebRawPacket(const char * buff, unsigned int curBuffLen, unsigned int maxBuffLen,
    bool & isChunked, std::string & method, std::string & line, std::map<std::string, std::string> & head, std::string & body)
{
    if (head.empty())
    {
        isChunked = false;
    }
    int bodyLenght = -1;
    unsigned int usedCount = 0;


    //extract head
    if (!isChunked)
    {

        //extract common line
        try
        {
            usedCount += InnerReadLine(buff + usedCount, curBuffLen - usedCount, maxBuffLen - usedCount, true, true, method, line);
        }
        catch (IntegrityType t)
        {
            return std::make_pair(t, usedCount);
        }

        //extract head line
        head.clear();
        try
        {
            std::string tmpMethod;
            std::string tmpLine;
            do
            {
                tmpMethod.clear();
                tmpLine.clear();
                usedCount += InnerReadLine(buff + usedCount, curBuffLen - usedCount, maxBuffLen - usedCount, true, false, tmpMethod, tmpLine);
                if (tmpMethod.empty() && tmpLine.empty())
                {
                    break;
                }
                if (tmpMethod == "Content-Length")
                {
                    bodyLenght = atoi(tmpLine.c_str());
                }
                else if (tmpMethod == "Transfer-Encoding")
                {
                    isChunked = true;
                }
                head.insert(std::make_pair(std::move(tmpMethod), std::move(tmpLine)));
            } while (true);
        }
        catch (IntegrityType t)
        {
            return std::make_pair(t, 0);
        }
    }
    


    //read chunked header
    if (isChunked)
    {
        try
        {
            std::string tmpMethod;
            std::string tmpLine;
            usedCount += InnerReadLine(buff + usedCount, curBuffLen - usedCount, maxBuffLen - usedCount, false, false, tmpMethod, tmpLine);
            //chunked end. need closed.
            if (tmpMethod.empty())
            {
                return std::make_pair(kIntegrityCorrupted, 0);
            }
            sscanf(tmpMethod.c_str(), "%x", &bodyLenght);
            if (bodyLenght == 0)
            {
                //http socket end.
                return std::make_pair(kIntegrityCorrupted, 0);
            }
            
        }
        catch (IntegrityType t)
        {
            return std::make_pair(t, 0);
        }
    }
    else if (head.empty())
    {
        return std::make_pair(kIntegrityShortage, 0);
    }
    else if (method == "GET")
    {
        return std::make_pair(kIntegrityIntact, usedCount);
    }
    
    if (bodyLenght == -1 || usedCount + bodyLenght > maxBuffLen)
    {
        return std::make_pair(kIntegrityCorrupted, 0);
    }
    if (bodyLenght + usedCount > curBuffLen)
    {
        return std::make_pair(kIntegrityShortage, 0);
    }
    body.assign(buff + usedCount, bodyLenght);
    usedCount += bodyLenght;

    if (isChunked)//clean chunked end CRLF
    {
        try
        {
            std::string tmpMethod;
            std::string tmpLine;
            usedCount += InnerReadLine(buff + usedCount, curBuffLen - usedCount, maxBuffLen - usedCount, false, false, tmpMethod, tmpLine);
            //chunked end. need closed.
            if (!tmpMethod.empty())
            {
                return std::make_pair(kIntegrityCorrupted, 0);
            }
        }
        catch (IntegrityType t)
        {
            return std::make_pair(t, 0);
        }
    }
    

    return std::make_pair(kIntegrityIntact, usedCount);
}




inline std::string proto4z_traceback()
{
    std::stringstream ss;
#ifdef WIN32
    ss << "\r\n";
    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
	SymInitialize(GetCurrentProcess(), NULL, TRUE);

    //     typedef USHORT(WINAPI *CaptureStackBackTraceType)(__in ULONG, __in ULONG, __out PVOID*, __out_opt PULONG);
    //     CaptureStackBackTraceType capture = (CaptureStackBackTraceType)(GetProcAddress(LoadLibraryA("kernel32.dll"), "RtlCaptureStackBackTrace"));
    //     if (capture == NULL) return;
    const int stackMax = 128;
    void* trace[stackMax];
    //    int count = (capture)(0, stackMax, trace, NULL);
    int count = (CaptureStackBackTrace)(0, stackMax, trace, NULL);
    for (int i = 1; i < count; i++)
    {
        ULONG64 buffer[(sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR) + sizeof(ULONG64) - 1) / sizeof(ULONG64)];
        PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;
        pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        pSymbol->MaxNameLen = MAX_SYM_NAME;
        DWORD64 dwDisplacement = 0;
        if (SymFromAddr(GetCurrentProcess(), (DWORD64)trace[i], &dwDisplacement, pSymbol))
        {
            ss << "bt[" << i - 1 << "]       --[ " << pSymbol->Name << " ]--              from     ";
        }
        else
        {
            ss << "bt[" << i - 1 << "]   " << "error[" << GetLastError() << "]              from     ";
        }

        IMAGEHLP_LINE64 lineInfo = { sizeof(IMAGEHLP_LINE64) };
        DWORD dwLineDisplacement;
        if (SymGetLineFromAddr64(GetCurrentProcess(), (DWORD64)trace[i], &dwLineDisplacement, &lineInfo))
        {
            std::string pathfile = lineInfo.FileName;
            if (pathfile.empty())
            {
                ss << "\r\n";
                continue;
            }
            std::for_each(pathfile.begin(), pathfile.end(), [](char &ch) { if (ch == '/') ch = '\\'; });
            auto pos = pathfile.find_last_of('\\');
            if (pos != std::string::npos) pathfile[pos] = '/';
            pos = pathfile.find_last_of('\\');
            if (pos != std::string::npos) pathfile[pos] = '/'; else pos = -1;
            ss << pathfile.substr(pos + 1) << ":" << lineInfo.LineNumber;
        }
        else
        {
            ss << "------:0";
        }
        ss << "\r\n";
        if (strcmp(pSymbol->Name, "main") == 0) break;
    }
#else
    void *stack[200];
    size_t size = backtrace(stack, 200);
    char **stackSymbol = backtrace_symbols(stack, size);
    ss << "backtrace: ";
    for (size_t i = 1; i < size; i++)
    {
        ss << stack[i] << "  ";
    }
    ss << "\r\n";
    for (size_t i = 1; i < size && stackSymbol ; i++)
    {
        if (stackSymbol[i] && strlen(stackSymbol[i]) > 0)
        {
            ss << "bt[" << i - 1 << "] " << stackSymbol[i] << "\r\n";
        }
    }
    free(stackSymbol);
#endif
    return ss.str();
}


using WriteStream = WriteStreamImpl<20, 1024 * 1024, 1, 0>;
using ReadStream = ReadStreamImpl<1>;




#ifndef _ZSUMMER_END
#define _ZSUMMER_END }
#endif  
#ifndef _ZSUMMER_PROTO4Z_END
#define _ZSUMMER_PROTO4Z_END }
#endif

_ZSUMMER_PROTO4Z_END
_ZSUMMER_END

#endif

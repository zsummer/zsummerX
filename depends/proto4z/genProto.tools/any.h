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
 * Copyright (C) 2014-2015 YaweiZhang <yawei.zhang@foxmail.com>.
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



#pragma once
#ifndef _ANY_H_
#define _ANY_H_

#include <map>
#include <vector>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include "utility.h"
#include "log4z.h"
#include "tinyxml2.h"
#include "md5.h"
#ifndef WIN32
#include <sys/stat.h>
#else
#include <direct.h>
#endif



//store type enum
enum AnyType : short
{
    GT_DataComment,
    GT_DataEnum,
    GT_DataArray,
    GT_DataMap,
    GT_DataConstValue,
    GT_DataStruct,
    GT_DataProto,
};

//tag
enum MemberTag : short
{
    MT_NORMAL,
    MT_DB_KEY,
    MT_DB_IGNORE,
    MT_DELETE,
    //MT_OPTION,
};

//comment
struct DataComment
{
    std::string _desc;
};



//array type
struct DataArray
{
    std::string _type;
    std::string _arrayName;
    std::string _desc;
};

//dict type
struct DataMap
{
    std::string _typeKey;
    std::string _typeValue;
    std::string _mapName;
    std::string _desc;
};

//const type
struct DataConstValue
{
    std::string _type;
    std::string _name;
    std::string _value;
    std::string _desc;
};

//include file name, without suffix
struct DataEnum
{
    std::string _type;
    std::string _name;
    std::string _desc;
    std::vector<DataConstValue> _members;
};

//struct type
struct DataStruct
{
    std::string _name;
    std::string _desc;
    unsigned long long _tag;
    bool _isStore = false;
    struct DataMember
    {
        std::string _type;
        std::string _name;
        std::string _desc;
        MemberTag _tag;
    };
    std::vector<DataMember> _members;
};

//proto type
struct DataProto
{
    DataConstValue _const;
    DataStruct _struct;
};



//general store type
struct AnyData
{
    AnyType _type;
    DataComment _comment;
    DataConstValue _const;
    DataEnum _enum;
    DataArray _array;
    DataMap _map;
    DataProto _proto;
};




#endif

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
#ifndef _COMMON_H_
#define _COMMON_H_
#ifndef _CRT_SECURE_NO_WARNINGS
#define  _CRT_SECURE_NO_WARNINGS
#endif
#include <map>
#include <vector>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <list>
#include <time.h>
#include <algorithm>
#include "utility.h"
#include "log4z.h"
#include "tinyxml2.h"
#include "md5.h"
#ifndef WIN32
#include <sys/stat.h>
#else
#include <direct.h>
#endif
#include "log4z.h"

//proto id type
const std::string ProtoIDType = "ui16";

//lfcr
const std::string LFCR = " \r\n";

//parse
enum ParseCode
{
    PC_SUCCESS,
    PC_NEEDSKIP,
    PC_ERROR,
};

enum SupportLanguageType
{
    SL_NORMAL,
    SL_CPP,
    SL_LUA,
    SL_CSHARP,
    SL_SQL,
    SL_XML,
    SL_END,
};

const char * const SupportLanguageString[] =
{
    "",
    "cppmd5",
    "luamd5",
    "csharpmd5",
    "sqlmd5",
    "xmlmd5",
    ""
};

const char * const SupportLanguageFileSuffix[] =
{
    "",
    ".h",
    ".lua",
    ".cs",
    "_SQL.h",
    ".xml",
    ""
};


const char * const SupportLanguageFilePath[] =
{
    "",
    "C++",
    "lua",
    "CSharp",
    "C++",
    "",
    ""
};



#define E(log)\
do{\
    char logBuf[LOG4Z_LOG_BUF_SIZE]; \
    zsummer::log4z::Log4zStream ss(logBuf, LOG4Z_LOG_BUF_SIZE); \
    ss << log << " " << __FILE__ << ":" << __LINE__; \
    throw std::runtime_error(logBuf); \
}while (0)  
  


#endif




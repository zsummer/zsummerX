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

#ifndef _CRT_SECURE_NO_WARNINGS
#define  _CRT_SECURE_NO_WARNINGS
#endif
#include "genBase.h"
#include <time.h>
#include <algorithm>




GenBase::GenBase()
{

}
GenBase::~GenBase()
{

}

void GenBase::init(std::string fileName, SupportLanguageType t)
{
    _filename = fileName;
    _type = t;
    std::string path = SupportLanguageFilePath[t];
    if (!zsummer::utility::IsDirectory(path) && !zsummer::utility::CreateDir(path))
    {
        E("CreateDir path [" << path << "] Error. ");
    }
}

std::string GenBase::getRealType(const std::string & xmltype)
{
    return xmltype;
}

std::string GenBase::getTypeDefault(const std::string & xmltype)
{
    if (xmltype == "i8") return "0";
    else if (xmltype == "ui8") return "0";
    else if (xmltype == "i16") return "0";
    else if (xmltype == "ui16") return "0";
    else if (xmltype == "i32") return "0";
    else if (xmltype == "ui32") return "0";
    else if (xmltype == "i64") return "0";
    else if (xmltype == "ui64") return "0";
    else if (xmltype == "float") return "0.0";
    else if (xmltype == "double") return "0.0";
    return "";
}

std::string GenBase::genRealContent(const std::list<AnyData> & stores)
{
    return "";
}



void GenBase::write(const std::string & content)
{
    std::string filename = SupportLanguageFilePath[_type];
    filename += "/";
    filename += _filename;
    filename += SupportLanguageFileSuffix[_type];
    std::ofstream os;
    os.open(filename, std::ios::binary);
    if (!os.is_open())
    {
        E("genCppFile open file Error. : " << filename);
    }
    os.write(content.c_str(), content.length());
    os.close();
}


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


#include "genLUA.h"

std::string GenLUA::genRealContent(const std::list<AnyData> & stores)
{
    std::string text;
    for (auto &info : stores)
    {
        if (info._type == GT_DataConstValue)
        {
            text += LFCR;
            text += genDataConst(info._const);
        }
        else if (info._type == GT_DataEnum)
        {
            text += LFCR;
            text += genDataEnum(info._enum);
        }
        else if (info._type == GT_DataArray)
        {
            text += LFCR;
            text += genDataArray(info._array);
        }
        else if (info._type == GT_DataMap)
        {
            text += LFCR;
            text += genDataMap(info._map);
        }
        else if (info._type == GT_DataStruct || info._type == GT_DataProto)
        {
            text += LFCR;
            text += genDataProto(info._proto, info._type == GT_DataProto);
        }
    }

    return std::move(text);
}



std::string GenLUA::genDataConst(const DataConstValue & dc)
{
    std::string text;
    text += "Proto4z." + dc._name + " = " + dc._value;
    if (!dc._desc.empty())
    {
        text += "--" + dc._desc;
    }
    return text;
}

std::string GenLUA::genDataEnum(const DataEnum & de)
{
    std::string text;
    for (const auto & m : de._members)
    {
        text += "Proto4z." + m._name + " = " + m._value;
        if (!m._desc.empty())
        {
            text += "--" + m._desc;
        }
        text += LFCR;
    }
    return text;
}
std::string GenLUA::genDataArray(const DataArray & da)
{
    std::string text;
    text += "Proto4z." + da._arrayName + " = {} ";
    if (!da._desc.empty())
    {
        text += "--" + da._desc;
    }
    text += LFCR;

    text += "Proto4z." + da._arrayName + ".__getName = \"" + da._arrayName + "\"" + LFCR;
    text += "Proto4z." + da._arrayName + ".__getDesc = \"array\"" + LFCR;
    text += "Proto4z." + da._arrayName + ".__getTypeV = \"" + da._type + "\"" + LFCR;
    return text;
}
std::string GenLUA::genDataMap(const DataMap & dm)
{
    std::string text;
    text += "Proto4z." + dm._mapName + " = {} ";
    if (!dm._desc.empty())
    {
        text += "--" + dm._desc;
    }
    text += LFCR;

    text += "Proto4z." + dm._mapName + ".__getName = \"" + dm._mapName + "\"" + LFCR;
    text += "Proto4z." + dm._mapName + ".__getDesc = \"map\"" + LFCR;
    text += "Proto4z." + dm._mapName + ".__getTypeK = \"" + dm._typeKey + "\"" + LFCR;
    text += "Proto4z." + dm._mapName + ".__getTypeV = \"" + dm._typeValue + "\"" + LFCR;
    return text;
}
std::string GenLUA::genDataProto(const DataProto & dp, bool isProto)
{
    std::string text;
    if (isProto)
    {
        text += "Proto4z.register(" + dp._const._value + ",\"" + dp._struct._name + "\")" + LFCR;
    }

    text += "Proto4z." + dp._struct._name + " = {} ";
    if (!dp._struct._desc.empty())
    {
        text += "--" + dp._struct._desc;
    }
    text += LFCR;

    if (isProto)
    {
        text += "Proto4z." + dp._struct._name + ".__getID = " + dp._const._value + "" + LFCR;
    }
    text += "Proto4z." + dp._struct._name + ".__getName = \"" + dp._struct._name + "\"" + LFCR;

    for (size_t i = 0; i < dp._struct._members.size(); ++i)
    {
        text += "Proto4z." + dp._struct._name + "[" + boost::lexical_cast<std::string>(i + 1)
            + "] = {name=\"" + dp._struct._members[i]._name + "\", type=\"" + dp._struct._members[i]._type + "\"";
        if (dp._struct._members[i]._tag == MT_DELETE)
        {
            text += ", del = true";
        }
        text += " } ";
        if (!dp._struct._members[i]._desc.empty())
        {
            text += "--" + dp._struct._members[i]._desc;
        }
        text += LFCR;
    }
    return text;
}



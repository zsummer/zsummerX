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


#include "genCPP.h"



std::string GenCPP::getRealType(const std::string & xmltype)
{
    if ( xmltype == "i8") return "char";
    else if ( xmltype == "ui8") return "unsigned char";
    else if ( xmltype == "i16") return "short";
    else if ( xmltype == "ui16") return "unsigned short";
    else if ( xmltype == "i32") return "int";
    else if ( xmltype == "ui32") return "unsigned int";
    else if ( xmltype == "i64") return "long long";
    else if ( xmltype == "ui64") return "unsigned long long";
    else if ( xmltype == "float") return "float";
    else if ( xmltype == "double") return "double";
    else if ( xmltype == "string") return "std::string";
    return xmltype;
}


std::string GenCPP::genRealContent(const std::list<AnyData> & stores)
{
    std::string macroFileName = std::string("_") + _filename  + "_H_";
    std::transform(macroFileName.begin(), macroFileName.end(), macroFileName.begin(), [](char ch){ return std::toupper(ch); });


    std::string text = LFCR + "#ifndef " + macroFileName + LFCR;
    text += "#define " + macroFileName + LFCR + LFCR;

    for (auto &info : stores)
    {
        if (info._type == GT_DataComment)
        {
            text += LFCR;
            text += "/*" + info._comment._desc + "*/" + LFCR;
        }
        else if (info._type == GT_DataConstValue)
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
    text += LFCR + "#endif" + LFCR;

    return std::move(text);
}


std::string GenCPP::genDataConst(const DataConstValue & dc)
{
    std::string text;
    text += "const " + getRealType(dc._type) + " " + dc._name + " = " + dc._value + "; ";
    if (!dc._desc.empty())
    {
        text += "//" + dc._desc + " ";
    }
    text += LFCR;
    return text;
}
std::string GenCPP::genDataEnum(const DataEnum & de)
{
    std::string text;
    text += "enum " + de._name + " : " + getRealType(de._type) + LFCR;
    text += "{" + LFCR;
    for (auto m : de._members)
    {
        text += "    " + m._name + " = " + m._value + ", ";
        if (!m._desc.empty())
        {
            text += "//" + m._desc + " ";
        }
        text += LFCR;
    }
    text += "};" + LFCR;
    return text;
}
std::string GenCPP::genDataArray(const DataArray & da)
{
    std::string text;
    text += LFCR + "typedef std::vector<" + getRealType(da._type) + "> " + da._arrayName + "; ";
    if (!da._desc.empty())
    {
        text += "//" + da._desc + " ";
    }
    text += LFCR;
    return text;
}
std::string GenCPP::genDataMap(const DataMap & dm)
{
    std::string text;
    text += LFCR + "typedef std::map<"
        + getRealType(dm._typeKey) + ", " + getRealType(dm._typeValue)
        + "> " + dm._mapName + "; ";
    if (!dm._desc.empty())
    {
        text += "//" + dm._desc + " ";
    }
    text += LFCR;
    return text;
}
std::string GenCPP::genDataProto(const DataProto & dp, bool isProto)
{
    std::string text;

    //write struct
    text += "struct " + dp._struct._name;
    if (!dp._struct._desc.empty())
    {
        text += " //" + dp._struct._desc + " ";
    }
    text += LFCR;
    text += "{" + LFCR;

    if (isProto)
    {
        text += std::string("    static const ") + getRealType(ProtoIDType) + " GetProtoID() { return " + dp._const._value + ";}" + LFCR;
        text += std::string("    static const ") + getRealType("string") + " GetProtoName() { return \""
            + dp._const._name + "\";}" + LFCR;
    }
    
    for (const auto & m : dp._struct._members)
    {
        text += "    " + getRealType(m._type) + " " + m._name + "; ";
        if (m._tag == MT_DELETE)
        {
            text += "//[already deleted] ";
        }
        if (!m._desc.empty())
        {
            text += "//" + m._desc + " ";
        }
        text += LFCR;
    }

    if (!dp._struct._members.empty())
    {    //struct init
        text += "    " + dp._struct._name + "()" + LFCR;
        text += "    {" + LFCR;
        for (const auto &m : dp._struct._members)
        {
            std::string def = getTypeDefault(m._type);
            if (!def.empty())
            {
                text += "        " + m._name + " = " + def + ";" + LFCR;
            }
        }
        text += "    }" + LFCR;
    }

    if (!dp._struct._members.empty())
    {    //struct init
        text += "    " + dp._struct._name + "(";
        for (size_t i = 0; i < dp._struct._members.size(); i++)
        {
            const auto & m = dp._struct._members[i];
            if (i != 0) text += ", ";
            text += "const " + getRealType(m._type) + " & " + m._name;
        }
        text += ")" + LFCR;
        text += "    {" + LFCR;
        for (const auto &m : dp._struct._members)
        {
            text += "        this->" + m._name + " = " + m._name + ";" + LFCR;
        }
        text += "    }" + LFCR;
    }
    text += "};" + LFCR;



    //input stream operator
    text += "inline zsummer::proto4z::WriteStream & operator << (zsummer::proto4z::WriteStream & ws, const " + dp._struct._name + " & data)" + LFCR;
    text += "{" + LFCR;
    for (const auto &m : dp._struct._members)
    {
        if (m._tag == MT_DELETE)
        {
            text += "//    ws << data." + m._name + "; //[already deleted] " + LFCR;
        }
        else
        {
            text += "    ws << data." + m._name + "; " + LFCR;
        }
    }

    text += "    return ws;" + LFCR;
    text += "}" + LFCR;


    //output stream operator
    text += "inline zsummer::proto4z::ReadStream & operator >> (zsummer::proto4z::ReadStream & rs, " + dp._struct._name + " & data)" + LFCR;
    text += "{" + LFCR;

    for (const auto &m : dp._struct._members)
    {
        if (m._tag == MT_DELETE)
            text += "//        rs >> data." + m._name + "; " + LFCR;
        else
            text += "        rs >> data." + m._name + "; " + LFCR;
    }

    text += "    return rs;" + LFCR;
    text += "}" + LFCR;

    //input log4z operator
    if (false)
    {
        text += "inline zsummer::log4z::Log4zStream & operator << (zsummer::log4z::Log4zStream & stm, const " + dp._struct._name + " & info)" + LFCR;
        text += "{" + LFCR;
        bool bFirst = true;
        for (const auto &m : dp._struct._members)
        {
            if (bFirst)
            {
                bFirst = false;
                text += "    stm << \"" + m._name + "=\"" + " << info." + m._name;
            }
            else
            {
                text += " << \", " + m._name + "=\"" + " << info." + m._name;
            }
        }
        text += ";" + LFCR;
        text += "    return stm;" + LFCR;
        text += "}" + LFCR;
    }
    return text;
}




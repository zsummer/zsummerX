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
#include "genCSharp.h"
#include <time.h>
#include <algorithm>


CSharpType GenCSharp::getCSharpType(const std::string & xmltype)
{
    if (xmltype == "i8") return{ true, "char", "Proto4z.BaseProtoObject.encodeI8", "Proto4z.BaseProtoObject.decodeI8" };
    if (xmltype == "ui8") return{ true, "byte", "Proto4z.BaseProtoObject.encodeUI8", "Proto4z.BaseProtoObject.decodeUI8" };
    if (xmltype == "i16") return{ true, "short", "Proto4z.BaseProtoObject.encodeI16", "Proto4z.BaseProtoObject.decodeI16" };
    if (xmltype == "ui16") return{ true, "ushort", "Proto4z.BaseProtoObject.encodeUI16", "Proto4z.BaseProtoObject.decodeUI16" };
    if (xmltype == "i32") return{ true, "int", "Proto4z.BaseProtoObject.encodeI32", "Proto4z.BaseProtoObject.decodeI32" };
    if (xmltype == "ui32") return{ true, "uint", "Proto4z.BaseProtoObject.encodeUI32", "Proto4z.BaseProtoObject.decodeUI32" };
    if (xmltype == "i64") return{ true, "long", "Proto4z.BaseProtoObject.encodeI64", "Proto4z.BaseProtoObject.decodeI64" };
    if (xmltype == "ui64") return{ true, "ulong", "Proto4z.BaseProtoObject.encodeUI64", "Proto4z.BaseProtoObject.decodeUI64" };
    if (xmltype == "float") return{ true, "float", "Proto4z.BaseProtoObject.encodeSingle", "Proto4z.BaseProtoObject.decodeSingle" };
    if (xmltype == "double") return{ true, "double", "Proto4z.BaseProtoObject.encodeDouble", "Proto4z.BaseProtoObject.decodeDouble" };
    if (xmltype == "string") return{ true, "string", "Proto4z.BaseProtoObject.encodeString", "Proto4z.BaseProtoObject.decodeString" };

    return{ false, xmltype, "", "" };
}

std::string  GenCSharp::getTypeDefault(const std::string & xmltype)
{
    if (xmltype == "i8") return "'\\0'";
    else if (xmltype == "float") return "0.0f";
    else if (xmltype == "string") return "\"\"";
    return GenBase::getTypeDefault(xmltype);
}

std::string GenCSharp::genRealContent(const std::list<AnyData> & stores)
{
    std::string text = LFCR;

    {
        char buf[100];
        time_t now = time(NULL);
        tm *ptm = localtime(&now);
        sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d", ptm->tm_year + 1990, ptm->tm_mon + 1, ptm->tm_yday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
    }

    text += "namespace Proto4z " + LFCR + "{" + LFCR;

    for (auto &info : stores)
    {
        if (info._type == GT_DataConstValue)
        {
            text += LFCR;
            text += genDataConst(info._const);
        }
        if (info._type == GT_DataEnum)
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
    text += LFCR;
    text += "}" + LFCR;
    text += LFCR + LFCR;

    return text;
}

std::string GenCSharp::genDataConst(const DataConstValue & dc)
{
    std::string text;
    text += "    public class " + dc._name + " ";
    if (!dc._desc.empty())
    {
        text += "//" + dc._desc + " ";
    }
    text += LFCR;
    text += "    {" + LFCR;
    text += "        public const " + getCSharpType(dc._type).realType + " value = " + dc._value + "; " + LFCR;
    text += "    }" + LFCR;
    return text;
}

std::string GenCSharp::genDataEnum(const DataEnum & de)
{
    std::string text;
    text += "    public enum " + de._name + " : " + getCSharpType(de._type).realType + LFCR;
    text += "    {" + LFCR;
    for (auto m : de._members)
    {
        text += "        " + m._name + " = " + m._value + ", ";
        if (!m._desc.empty())
        {
            text += "//" + m._desc + " ";
        }
        text += LFCR;
    }
    text += "    };" + LFCR;
    return text;
}

std::string GenCSharp::genDataArray(const DataArray & da)
{
    std::string text;
    text += LFCR + "    public class " + da._arrayName + " : System.Collections.Generic.List<" + getCSharpType(da._type).realType + ">, Proto4z.IProtoObject ";
    if (!da._desc.empty())
    {
        text += "//" + da._desc + " ";
    }
    text += LFCR;
    text += "    {" + LFCR;
    text += "        "   "public System.Collections.Generic.List<byte> __encode()" + LFCR;
    text += "        {" + LFCR;
    text += "            "   "var ret = new System.Collections.Generic.List<byte>();" + LFCR;
    text += "            "   "int len = (int)this.Count;" + LFCR;
    text += "            "   "ret.AddRange(Proto4z.BaseProtoObject.encodeI32(len));" + LFCR;
    text += "            "   "for (int i = 0; i < this.Count; i++ )" + LFCR;
    text += "            "   "{" + LFCR;
    if (getCSharpType(da._type).isBase)
    {
        text += "                "  "ret.AddRange(" + getCSharpType(da._type).baseEncode + "(this[i])); " + LFCR;
    }
    else
    {
        text += "                "  "ret.AddRange(this[i].__encode());" + LFCR;
    }
    text += "            "   "}" + LFCR;
    text += "            "   "return ret;" + LFCR;
    text += "        }" + LFCR;
    text += LFCR;

    text += "        "   "public int __decode(byte[] binData, ref int pos)" + LFCR;
    text += "        {" + LFCR;
    text += "            "   "int len = Proto4z.BaseProtoObject.decodeI32(binData, ref pos);" + LFCR;
    text += "            "  "if(len > 0)" + LFCR;
    text += "            "   "{" + LFCR;
    text += "                "  "for (int i=0; i<len; i++)" + LFCR;
    text += "                "   "{" + LFCR;
    if (getCSharpType(da._type).isBase)
    {
        text += "                    "  "this.Add(" + getCSharpType(da._type).baseDecode + "(binData, ref pos));" + LFCR;
    }
    else
    {
        text += "                    "  "var data = new " + getCSharpType(da._type).realType + "();" + LFCR;
        text += "                    "  "data.__decode(binData, ref pos);" + LFCR;
        text += "                    "  "this.Add(data);" + LFCR;
    }
    text += "                "   "}" + LFCR;
    text += "            "   "}" + LFCR;
    text += "            "   "return pos;" + LFCR;
    text += "        }" + LFCR;
    text += "    }" + LFCR;
    return text;
}


std::string GenCSharp::genDataMap(const DataMap & dm)
{
    std::string text;
    text += LFCR + "    public class " + dm._mapName + " : System.Collections.Generic.Dictionary<" + getCSharpType(dm._typeKey).realType + ", " + getCSharpType(dm._typeValue).realType + ">, Proto4z.IProtoObject ";
    if (!dm._desc.empty())
    {
        text += "//" + dm._desc + " ";
    }
    text += LFCR;
    text += "    {" + LFCR;
    text += "        "   "public System.Collections.Generic.List<byte> __encode()" + LFCR;
    text += "        {" + LFCR;
    text += "            "   "var ret = new System.Collections.Generic.List<byte>();" + LFCR;
    text += "            "   "int len = (int)this.Count;" + LFCR;
    text += "            "   "ret.AddRange(Proto4z.BaseProtoObject.encodeI32(len));" + LFCR;
    text += "            "   "foreach(var kv in this)" + LFCR;
    text += "            "   "{" + LFCR;
    if (getCSharpType(dm._typeKey).isBase)
    {
        text += "                "   "ret.AddRange(" + getCSharpType(dm._typeKey).baseEncode + "(kv.Key));" + LFCR;
    }
    else
    {
        text += "                "   "ret.AddRange(kv.Key.__encode());" + LFCR;
    }
    if (getCSharpType(dm._typeValue).isBase)
    {
        text += "                "   "ret.AddRange(" + getCSharpType(dm._typeValue).baseEncode + "(kv.Value));" + LFCR;
    }
    else
    {
        text += "                "   "ret.AddRange(kv.Value.__encode());" + LFCR;
    }
    text += "            "   "}" + LFCR;
    text += "            "   "return ret;" + LFCR;
    text += "        }" + LFCR;
    text += LFCR;

    text += "        "   "public int __decode(byte[] binData, ref int pos)" + LFCR;
    text += "        {" + LFCR;
    text += "            "   "int len = Proto4z.BaseProtoObject.decodeI32(binData, ref pos);" + LFCR;
    text += "            "   "if(len > 0)" + LFCR;
    text += "            "   "{" + LFCR;
    text += "                "   "for (int i=0; i<len; i++)" + LFCR;
    text += "                "   "{" + LFCR;
    if (getCSharpType(dm._typeKey).isBase)
    {
        text += "                    "   "var key = " + getCSharpType(dm._typeKey).baseDecode + "(binData, ref pos);" + LFCR;
    }
    else
    {
        text += "                    "   "var key = new " + getCSharpType(dm._typeKey).realType + "();" + LFCR;
        text += "                    "    "key.__decode(binData, ref pos);" + LFCR;
    }

    if (getCSharpType(dm._typeValue).isBase)
    {
        text += "                    "   "var val = " + getCSharpType(dm._typeValue).baseDecode + "(binData, ref pos);" + LFCR;
    }
    else
    {
        text += "                    "   "var val = new " + getCSharpType(dm._typeValue).realType + "();" + LFCR;
        text += "                    "    "val.__decode(binData, ref pos);" + LFCR;
    }
    text += "                    "   "this.Add(key, val);" + LFCR;
    text += "                "   "}" + LFCR;
    text += "            "   "}" + LFCR;
    text += "            "   "return pos;" + LFCR;
    text += "        }" + LFCR;
    text += "    }" + LFCR;
    return text;
}


std::string GenCSharp::genDataProto(const DataProto & dp, bool isProto)
{
    std::string text;
    text += "    public class " + dp._struct._name + ": Proto4z.IProtoObject";
    if (!dp._struct._desc.empty())
    {
        text += " //" + dp._struct._desc + " ";
    }
    text += LFCR;
    text += "    {    " + LFCR;

    //write ProtoID
    if (isProto)
    {
        text += "        " "//proto id  " + LFCR;
        text += "        " "public const ushort protoID = " + dp._const._value + "; " + LFCR;
        text += "        " "static public ushort getProtoID() { return " + dp._const._value + "; }" + LFCR;
        text += "        " "static public string getProtoName() { return \"" + dp._struct._name + "\"; }" + LFCR;
    }

    //members
    text += "        " "//members  " + LFCR;
    for (const auto & m : dp._struct._members)
    {
        text += "        public " + getCSharpType(m._type).realType + " " + m._name + "; ";
        if (!m._desc.empty())
        {
            text += "//" + m._desc + " ";
        }
        text += LFCR;
    }
    //default struct ()
    text += "        public " + dp._struct._name + "() " + LFCR;
    text += "        {" + LFCR;
    for (const auto & m : dp._struct._members)
    {
        text += "            " + m._name + " = ";
        if (getCSharpType(m._type).isBase)
        {
            text += getTypeDefault(m._type);
        }
        else
        {
            text += "new " + getCSharpType(m._type).realType + "()";
        }
        text += "; " + LFCR;
    }
    text += "        }" + LFCR;

    //default struct (....) 
    if (!dp._struct._members.empty())
    {
        text += "        public " + dp._struct._name + "(";
        for (size_t i = 0; i < dp._struct._members.size(); i ++)
        {
            const auto &m = dp._struct._members[i];
            if (i > 0) text += ", ";
            text += "" + getCSharpType(m._type).realType + " " + m._name;
        }
        text += ")" + LFCR;
        text += "        {" + LFCR;
        for (const auto & m : dp._struct._members)
        {
            text += "            this." + m._name + " = " + m._name + ";" + LFCR;
        }
        text += "        }" + LFCR;
    }


    //encode
    text += "        public System.Collections.Generic.List<byte> __encode()" + LFCR;
    text += "        {" + LFCR;
    text += "            "   "var data = new System.Collections.Generic.List<byte>();" + LFCR;
    for (const auto &m : dp._struct._members)
    {
        if (m._tag == MT_DELETE)
        {
            continue;
        }

        //null
        if (!getCSharpType(m._type).isBase)
        {
            text += "            if (" + m._name + " == null) " + m._name + " = new " + m._type + "();" + LFCR;
        }


        //encode
        if (getCSharpType(m._type).isBase)
        {
            text += "            "  "data.AddRange(" + getCSharpType(m._type).baseEncode + "(" + m._name + "));" + LFCR;
        }
        else
        {
            text += "            "  "data.AddRange(" + m._name + ".__encode());" + LFCR;
        }
    }
    text += "            "  "return data;" + LFCR;
    text += "        }" + LFCR;

    //decode
    text += "        public int __decode(byte[] binData, ref int pos)" + LFCR;
    text += "        {" + LFCR;
    for (const auto &m : dp._struct._members)
    {
        if (m._tag == MT_DELETE)
        {
            continue;
        }
        if (getCSharpType(m._type).isBase)
        {
            text += "            this." + m._name + " = " + getCSharpType(m._type).baseDecode + "(binData, ref pos);" + LFCR;
        }
        else
        {
            text += "            this." + m._name + " = new " + getCSharpType(m._type).realType + "();" + LFCR;
            text += "            this." + m._name + ".__decode(binData, ref pos);" + LFCR;
        }
    }
    text += "            return pos;" + LFCR;
    text += "        }" + LFCR;
    text += "    }" + LFCR;
    return text;
}












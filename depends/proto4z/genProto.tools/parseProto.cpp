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
#include "parseProto.h"
#include <time.h>
#include <algorithm>
#include "genCPP.h"
#include "genCSharp.h"
#include "genSQL.h"
#include "genLUA.h"


void processTag(std::list<AnyData> & data)
{
    for (auto &info : data)
    {
        if (info._type != GT_DataStruct && info._type != GT_DataProto)
        {
            continue;
        }

        info._proto._struct._tag = 0;

    }
}


std::list<AnyData> parseProto(std::string fileName, ParseCache & cache)
{
    unsigned short minProtoID = 0;
    unsigned short maxProtoID = 0;
    std::list<AnyData> anydata;


    std::string filename = fileName + ".xml";

    if (!zsummer::utility::GetFileStatus(filename, 6))
    {
        E(filename << " not found.");
    }


    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(filename.c_str()) != tinyxml2::XML_SUCCESS)
    {
        doc.PrintError();
        E(filename << " Load Error. ");
    }

    do
    {
        XMLElement * ele = doc.FirstChildElement("ProtoTraits");
        if (ele == NULL)
        {
            E("doc.FirstChildElement(\"ProtoTraits\") Error.");
        }
        auto minNo = ele->FirstChildElement("MinNo");
        auto maxNo = ele->FirstChildElement("MaxNo");
        if (!minNo || !minNo->GetText() || !maxNo || !maxNo->GetText())
        {
            E("FirstChildElement(\"MinNo\") || FirstChildElement(\"MaxNo\")  Error");
        }
        minProtoID = atoi(minNo->GetText());
        maxProtoID = atoi(maxNo->GetText());
        if (cache.getCurrentProtoID() < minProtoID)
        {
            cache.setCurrentProtoID(minProtoID);
        }
        if (cache.getCurrentProtoID() > maxProtoID)
        {
            E("Current cache Proto No Error. NextNumber=" << cache.getCurrentProtoID() << ", minNo=" << minProtoID << ", maxNo=" << maxProtoID);
        }
    } while (0);
    LOGI("parseConfig [" << filename << "] NextNumber=" << cache.getCurrentProtoID() << ", minProtoNo=" << minProtoID << ", maxProtoNo=" << maxProtoID);


    {
        XMLElement * ele = doc.FirstChildElement("Proto");
        if (ele == NULL)
        {
            E("doc.FirstChildElement(\"Proto\") Error.");
        }
        ele = ele->FirstChildElement();
        do
        {
            if (ele == NULL)
            {
                break;
            }
            std::string stype = ele->Name();
            if (stype == "const")
            {
                DataConstValue dc;
                if (!ele->Attribute("type") || !ele->Attribute("name") || !ele->Attribute("value"))
                {
                    E("Attribute Error. ");
                }
                dc._type = ele->Attribute("type");
                dc._name = ele->Attribute("name");
                dc._value = ele->Attribute("value");
                
                if (ele->Attribute("desc"))
                {
                    dc._desc = ele->Attribute("desc");
                }
                AnyData info;
                info._const = dc;
                info._type = GT_DataConstValue;
                anydata.push_back(info);
            }
            else if (stype == "enum")
            {
                AnyData enumData;
                enumData._type = GT_DataEnum;
                int lastID = 0;
                if (!ele->Attribute("type"))
                {
                    E("Attribute Error. ");
                }
                enumData._enum._type = ele->Attribute("type");
                if (ele->Attribute("name"))
                {
                    enumData._enum._name = ele->Attribute("name");
                }
                if (ele->Attribute("desc"))
                {
                    enumData._enum._desc = ele->Attribute("desc");
                }
                
                XMLElement * member = ele->FirstChildElement("member");
                do
                {
                    DataConstValue dc;
                    if (member == NULL)
                    {
                        break;
                    }
                    if (!member->Attribute("name"))
                    {
                        E("Attribute Error. ");
                    }
                    dc._type = enumData._enum._type;
                    dc._name = member->Attribute("name");
                    if (member->Attribute("value"))
                    {
                        int v = atoi(member->Attribute("value"));
                        if (v < lastID)
                        {
                            E("enum value is invalid.");
                        }
                        lastID = v;
                    }
                    dc._value = boost::lexical_cast<std::string>(lastID);
                    lastID++;
                    if (member->Attribute("desc"))
                    {
                        dc._desc = member->Attribute("desc");
                    }
                    enumData._enum._members.push_back(dc);
                    member = member->NextSiblingElement("member");
                } while (true);
                anydata.push_back(enumData);
            }
            //数组类型
            else if (stype == "array")
            {
                DataArray ar;
                if (!ele->Attribute("type") || !ele->Attribute("name"))
                {
                    E("Attribute Error. ");
                }
                ar._type = ele->Attribute("type");
                ar._arrayName = ele->Attribute("name");
                if (ele->Attribute("desc"))
                {
                    ar._desc = ele->Attribute("desc");
                }
                AnyData info;
                info._type = GT_DataArray;
                info._array = ar;
                anydata.push_back(info);
            }
            //K-V类型
            if (stype == "map")
            {
                DataMap dm;
                if (!ele->Attribute("key") || !ele->Attribute("value") || !ele->Attribute("name"))
                {
                    E("Attribute Error. ");
                }
                dm._typeKey = ele->Attribute("key");
                dm._typeValue = ele->Attribute("value");
                dm._mapName = ele->Attribute("name");
                if (ele->Attribute("desc"))
                {
                    dm._desc = ele->Attribute("desc");
                }
                AnyData info;
                info._type = GT_DataMap;
                info._map = dm;
                anydata.push_back(info);
            }
            //结构体类型
            else if (stype == "struct" || stype == "proto")
            {
                DataProto dp;
                if (!ele->Attribute("name"))
                {
                    E("Attribute Error. ");
                }
                dp._struct._name = ele->Attribute("name");
                if (ele->Attribute("desc"))
                {
                    dp._struct._desc = ele->Attribute("desc");
                }
                if (ele->Attribute("store"))
                {
                    dp._struct._isStore = strcmp("true", ele->Attribute("store")) == 0;
                }
                if (stype == "proto")
                {

                    dp._const._type = ProtoIDType;
                    dp._const._name = "ID_"  + dp._struct._name;
                    dp._const._desc = dp._struct._desc;
                    dp._const._value = boost::lexical_cast<std::string>(cache.genProtoID(dp._const._name, minProtoID, maxProtoID));
                }

                XMLElement * member = ele->FirstChildElement("member");
                do
                {
                    if (member == NULL)
                    {
                        break;
                    }
                    DataStruct::DataMember dm;
                    if (!member->Attribute("type") || !member->Attribute("name"))
                    {
                        E("Attribute Error. ");
                    }
                    dm._type = member->Attribute("type");
                    dm._name = member->Attribute("name");
                    dm._tag = MT_NORMAL;
                    if (member->Attribute("tag"))
                    {
                        if (strcmp(member->Attribute("tag"), "del") == 0)
                        {
                            dm._tag = MT_DELETE;
                        }
                        else if (strcmp(member->Attribute("tag"), "key") == 0)
                        {
                            dm._tag = MT_DB_KEY;
                        }
                        else if (strcmp(member->Attribute("tag"), "ignore") == 0)
                        {
                            dm._tag = MT_DB_IGNORE;
                        }

                    }

                    if (member->Attribute("desc"))
                    {
                        dm._desc = member->Attribute("desc");
                    }
                    dp._struct._members.push_back(dm);
                    member = member->NextSiblingElement("member");

                } while (true);

                AnyData info;
                info._type = GT_DataStruct;
                info._proto = dp;
                if (stype == "proto")
                {
                    info._type = GT_DataProto;
                }
                anydata.push_back(info);
            }
            
            ele = ele->NextSiblingElement();

        } while (true);
    }

    for (auto &info : anydata)
    {
        if (info._type != GT_DataStruct && info._type != GT_DataProto)
        {
            continue;
        }
        int curTagIndex = 0;
        info._proto._struct._tag = 0;
        for (const auto & m : info._proto._struct._members)
        {
            if (m._tag != MT_DELETE)
            {
                info._proto._struct._tag |= (1ULL << curTagIndex);
            }
            curTagIndex++;
        }
    }

    return std::move(anydata);
}

GenBase * createGenerate(SupportLanguageType t)
{
    if (t == SL_CPP)
    {
        return new GenCPP();
    }
    else if (t == SL_CSHARP)
    {
        return new GenCSharp();
    }
    else if (t == SL_SQL)
    {
        return new GenSQL();
    }
    else if (t == SL_LUA)
    {
        return new GenLUA();
    }
    return nullptr;
}

void destroyGenerate(GenBase * gen)
{
    if (gen)
    {
        delete gen;
    }
}
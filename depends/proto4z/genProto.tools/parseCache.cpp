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
#include "parseCache.h"
#include <time.h>
#include <algorithm>



void ParseCache::parse(std::string filename)
{
    _fileName = filename;
    std::string configFile = filename + ".xml.cache";
    if (!zsummer::utility::GetFileStatus(configFile, 6))
    {
        LOGW("ParseCache::parse [" << configFile << " not found.");
        return;
    }
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(configFile.c_str()) != tinyxml2::XML_SUCCESS)
    {
        E(" ParseCache::parse Error. configFile=" << configFile << ", err1=" << doc.GetErrorStr1() << ", err2" << doc.GetErrorStr2());
    }

    for (int i = SL_NORMAL+1; i < SL_END; i++)
    {
        XMLElement * md5 = doc.FirstChildElement(SupportLanguageString[i]);
        if (md5 && md5->GetText())
        {
            _md5Cache[i] = md5->GetText();
        }
    }
    
    XMLElement * cacheEles = doc.FirstChildElement("cacheNumber");
    if (cacheEles == NULL)
    {
        LOGW("ParseCache::parse can not found cacheNumber. configFile=" << configFile);
        return ;
    }

    XMLElement * next = cacheEles->FirstChildElement("cache");
    do
    {
        if (next == NULL)
        {
            break;
        }
        const char * key = next->Attribute("key");
        const char * number = next->Attribute("Number");
        if (key == NULL || number == NULL)
        {
            E("ParseCache::parse cache number is invalid. configFile=" << configFile);
        }


        auto founder = _cacheNumber.find(key);
        if (founder != _cacheNumber.end())
        {
            E("ParseCache::parse dumplicate key on " << key << " from " << configFile);
        }
        _cacheNumber[key] = atoi(number);

        if (_currentProtoID <= atoi(number))
        {
            _currentProtoID = atoi(number) + 1;
        }

        next = next->NextSiblingElement("cache");
    } while (true);
}
bool   ParseCache::write()
{
    std::string filename = _fileName + ".xml.cache";
    LOGI("writeCache [" << filename );
    std::ofstream os;
    os.open(filename, std::ios::binary);
    if (!os.is_open())
    {
        E(filename << " can not open!.");
    }
    std::string text = "<?xml version=\"1.0\" encoding=\"UTF - 8\"?>\n\n";
    for (int i = SL_NORMAL + 1; i < SL_END; i++)
    {
        std::string md5 = genFileMD5(std::string("./") + SupportLanguageFilePath[i] + "/" + _fileName + SupportLanguageFileSuffix[i]);
        text += std::string() + "<" + SupportLanguageString[i] + ">";
        text += md5;
        text += std::string() + "</" + SupportLanguageString[i] + ">" + LFCR;
    }
    
    text += "<cacheNumber>\n";

    for (auto &pr : _cacheNumber)
    {
        text += "    <cache key = \"" + pr.first +"\" Number = \"" + boost::lexical_cast<std::string>(pr.second) + "\" /> \n";
    }
    text += "</cacheNumber>\n";
    os.write(text.c_str(), text.length());
    os.close();
    return true;
}

bool ParseCache::isNeedUpdate()
{
    for (int i = SL_NORMAL+1; i < SL_END; i++)
    {
        if (_md5Cache[i].empty())
        {
            return true;
        }
        std::string md5 = genFileMD5(std::string("./") + SupportLanguageFilePath[i] + _fileName + SupportLanguageFileSuffix[i]);
        if (md5 != _md5Cache[i])
        {
            return true;
        }
    }
    return false;
}

unsigned short ParseCache::getCacheNumber(std::string key)
{
    auto founder = _cacheNumber.find(key);
    if (founder == _cacheNumber.end())
    {
        return -1;
    }
    return founder->second;
}
bool ParseCache::setCacheNumber(std::string key, unsigned short number)
{
    auto founder = _cacheNumber.find(key);
    if (founder != _cacheNumber.end())
    {
        return false;
    }
    _cacheNumber[key] = number;
    return true;
}

unsigned short ParseCache::genProtoID(std::string key, unsigned short minProtoID, unsigned short maxProtoID)
{
    unsigned short ret = getCacheNumber(key);
    if (ret == (unsigned short)-1)
    {
        ret = _currentProtoID++;
        setCacheNumber(key, ret);
        if (ret < minProtoID || ret >= maxProtoID)
        {
            E("proto number override. key=" << key << ", next number=" << ret);
        }
    }
    return ret;
}


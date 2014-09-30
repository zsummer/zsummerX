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
 * Copyright (C) 2012 YaweiZhang <yawei_zhang@foxmail.com>.
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
#ifndef _GEN_PROTO_H_
#define _GEN_PROTO_H_

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
#include "genProto.h"
using namespace tinyxml2;

//支持的的数据类型
static  std::map<std::string, std::string> xmlTypeToCppType = {
		{ "i8", "char" },
		{ "ui8", "unsigned char" },
		{ "i16", "short" },
		{ "ui16", "unsigned short" },
		{ "i32", "unsigned int" },
		{ "ui32", "unsigned int" },
		{ "i64", "long long" },
		{ "ui64", "unsigned long long" },
		{ "float", "float" },
		{ "double", "double" },
		{ "string", "std::string" },
};

//数据类型对应的初始化数值
static  std::map<std::string, std::string> xmlTypeToCppDefaultValue = {
		{ "i8", "'\0'" },
		{ "ui8", "0" },
		{ "i16", "0" },
		{ "ui16", "0" },
		{ "i32", "0" },
		{ "ui32", "0" },
		{ "i64", "0" },
		{ "ui64", "0" },
		{ "float", "0.0" },
		{ "double", "0.0" },
};


//支持数组类型
struct DataArray
{
	std::string _type;
	std::string _arrayName;
	std::string _desc;
};

//支持字典类型
struct DataMap
{
	std::string _typeKey;
	std::string _typeValue;
	std::string _mapName;
	std::string _desc;
};

//支持常量类型
struct DataConstValue
{
	std::string _type;
	std::string _name;
	std::string _value;
	std::string _desc;
};

//支持结构体类型
struct DataStruct
{
	std::string _name;
	std::string _desc;
	struct DataMember
	{
		std::string _type;
		std::string _name;
		std::string _desc;
	};
	std::vector<DataMember> _members;
};

//支持协议封包类型
struct DataProto
{
	DataConstValue _const;
	DataStruct _struct;
};


//协议ID类型
const std::string ProtoIDType = "ui16";

//生成的代码换行
const std::string LFCR = " \r\n ";


//语法分析用的通用类型.
enum StoreType
{
	GT_DataArray,
	GT_DataMap,
	GT_DataConstValue,
	GT_DataStruct,
	GT_DataProto,
};

//语法分析用的通用数据结构
struct StoreInfo
{
	StoreType _type;
	DataArray _array;
	DataMap _map;
	DataConstValue _const;
	DataProto _proto;
};


//生成代码
bool genCppFile(std::string path, std::string filename, std::string attr, std::vector<StoreInfo> & stores);
bool genLuaFile(std::string path, std::string filename, std::string attr, std::vector<StoreInfo> & stores);
bool genCSharpFile(std::string path, std::string filename, std::string attr, std::vector<StoreInfo> & stores);
bool genJsFile(std::string path, std::string filename, std::string attr, std::vector<StoreInfo> & stores);




//语法分析结果
enum ParseCode
{
	PC_SUCCESS,
	PC_NEEDSKIP,
	PC_ERROR,
};


//语法分析和代码生成
class genProto
{
	//初始化信息
	std::string m_fileName;
	std::string m_fileConfigAttr = ".xml";
	std::string m_fileCacheAttr = ".xml.cache";

	//cache分析数据
	unsigned short m_curNo = 0;
	std::string m_md5;
	struct DataCache
	{
		std::string protoName;
		unsigned int protoValue;
	};
	std::map<std::string, DataCache> m_mapCacheNo;

	//xml分析数据
	unsigned short m_minNo = 0;
	unsigned short m_maxNo = 0;
	std::vector<StoreInfo> m_vctStoreInfo;

public:
	//filename不包含文件后缀
	genProto(std::string filename){	m_fileName = filename;}

	//分析cache
	ParseCode ParseCache();
	//分析协议配置
	ParseCode ParseConfig();
	//生成对应语言的代码
	ParseCode GenCode();
	//写入cache
	ParseCode WriteCache();
};


#endif

/*
 * genProto License
 * -----------
 * 
 * genProto is licensed under the terms of the MIT license reproduced below.
 * This means that genProto is free software and can be used for both academic
 * and commercial purposes at absolutely no cost.
 * 
 * 
 * ===============================================================================
 * 
 * Copyright (C) 2014 YaweiZhang <yawei_zhang@foxmail.com>.
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

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include "log4z.h"
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>


#ifndef WIN32
#include <sys/stat.h>
#else
#include <direct.h>
#endif

struct DataArray
{
	std::string type;
	std::string arrayName;
	std::string desc;
};

struct DataMap
{
	std::string typeKey;
	std::string typeValue;
	std::string mapName;
	std::string desc;
};

struct DataConstValue 
{
	std::string type;
	std::string name;
	std::string value;
	std::string desc;
};

struct DataMember 
{
	std::string type;
	std::string name;
	std::string desc;
};
struct ProtoStruct
{
	std::string from;
	std::string to;
	std::string name;
	unsigned short protoID;
	std::string desc;
	std::vector<DataMember> members;

};

struct GeneralStruct 
{
	std::string name;
	std::string desc;
	std::vector<DataMember> members;
};

const std::string LFCR = "\r\n";

std::string genProtoIDName(const ProtoStruct & ps){return "ID_" + ps.from + "2" + ps.to + "_" + ps.name;}
std::string genProtoStructName(const ProtoStruct & ps){ return "Proto"  + ps.name; }



std::string WriteCppConstValue(const DataConstValue & v)
{
	std::string text;
	std::string type = v.type;
	if (type == "string")
	{
		type = "std::string";
	}
	text += "const " + type + " " + v.name + " = " + v.value + "; ";
	if (!v.desc.empty())
	{
		text += "//" + v.desc;
	}
	text += LFCR;
	
	return text;
}

std::string WriteCppArray(const DataArray & a)
{
	std::string text;
	std::string type = a.type;
	if (type == "string")
	{
		type = "std::string";
	}
	text += "typedef std::vector<" + type + "> " + a.arrayName + "; "; 
	if (!a.desc.empty())
	{
		text += "//" + a.desc;
	}
	text += LFCR;
	return text;
}

std::string WriteCppMap(const DataMap & dm)
{
	std::string text;
	std::string typeKey = dm.typeKey;
	std::string typeValue = dm.typeValue;
	if (typeKey == "string")
	{
		typeKey = "std::string";
	}
	if (typeValue == "string")
	{
		typeValue = "std::string";
	}
	text += "typedef std::map<" + typeKey + ", " + typeValue + "> " + dm.mapName + "; ";
	if (!dm.desc.empty())
	{
		text += "//" + dm.desc;
	}
	text += LFCR;
	return text;
}

std::string WriteCppDataMember(const std::vector<DataMember> & ms)
{
	std::string text;
	for (const auto & m:ms)
	{
		std::string type = m.type;
		if (type == "string")
		{
			type = "std::string";
		}
		text += "\t" + type + " " + m.name + "; ";
		if (!m.desc.empty())
		{
			text += "//" + m.desc;
		}
		text += LFCR;
	}
	return text;
}

std::string WriteCppDataStructStream(const std::string &name, const std::vector<DataMember> & ms)
{
	//输入流
	std::string text;
	text += LFCR;
	text = "template<class T>" + LFCR;
	text += "T & operator << (T & t, const " + name + " & data)" + LFCR;
	text += "{" + LFCR;

	for (const auto &m : ms)
	{
		text += "\tt << data." + m.name + ";" + LFCR;
	}
	text += "\treturn t;" + LFCR;
	text += "}" + LFCR;

	//输出流
	text += "template<class T>" + LFCR;
	text += "T & operator >> (T & t, " + name + " & data)" + LFCR;
	text += "{" + LFCR;
	for (const auto &m : ms)
	{
		text += "\tt >> data." + m.name + ";" + LFCR;
	}
	text += "\treturn t;" + LFCR;
	text += "}" + LFCR;
	return text;
}



std::string WriteCppStruct(const GeneralStruct & gs)
{
	std::string text;
	text += "struct " + gs.name;
	if (!gs.desc.empty())
	{
		text += " //" + gs.desc;
	}
	text += LFCR;

	text += "{" + LFCR;
	text += WriteCppDataMember(gs.members);
	text += "};" + LFCR;
	return text;
}



std::string WriteCppProto(const ProtoStruct & ps)
{
	std::string text;
	DataConstValue proto;
	proto.type = "ui16";
	proto.name = genProtoIDName(ps);
	proto.value = boost::lexical_cast<std::string>(ps.protoID);
	proto.desc = ps.desc;
	text += WriteCppConstValue(proto);

	text += "struct " + genProtoStructName(ps);
	if (!ps.desc.empty())
	{
		text += " //" + ps.desc;
	}
	text += LFCR;

	text += "{" + LFCR;
	text += WriteCppDataMember(ps.members);
	text += "};" + LFCR;
	return text;
}


class genProto
{
public:
	std::map<std::string, unsigned short> m_mapCacheNo;
	bool LoadCache(std::string filename)
	{
		try
		{
			boost::property_tree::ptree pt;
			boost::property_tree::read_xml(filename+".cache", pt);
			auto keys = pt.get_child("CacheNo");
			for (auto iter = keys.begin(); iter != keys.end(); ++iter)
			{

				std::string key = iter->second.get<std::string>("<xmlattr>.key");
				unsigned short value = iter->second.get<unsigned short>("<xmlattr>.No");
				m_mapCacheNo.insert(std::make_pair(key, value));
			}
		}
		catch (std::string err)
		{
			LOGW("LoadCache catch exception: error=" << err);
			return false;
		}
		catch (std::exception e)
		{
			LOGW("LoadCache catch exception: error=" << e.what());
			return false;
		}
		catch (...)
		{
			LOGW("LoadCache catch exception: unknow exception.");
			return false;
		}
		return true;
	}

	unsigned short m_minNo = 0;
	unsigned short m_maxNo = 0;
	unsigned short m_curNo = 0;
	bool m_bUseLog4z = false;

	bool gen(std::string filename)
	{
		try
		{
			boost::property_tree::ptree pt;
			boost::property_tree::read_xml(filename, pt);
			try
			{
				auto traits = pt.get_child("ProtoTraits");
				m_minNo = boost::lexical_cast<unsigned short>(traits.get_child("MinNo").data());
				m_maxNo = boost::lexical_cast<unsigned short>(traits.get_child("MaxNo").data());
			}
			catch (...)
			{

			}

			if (m_curNo < m_minNo)
			{
				m_curNo = m_minNo;
			}

			for (auto & pr : m_mapCacheNo)
			{
				if (pr.second < m_minNo || pr.second >= m_maxNo)
				{
					throw std::runtime_error("No.xml have error. please remove No.xml file and re-genProto.");
				}
				if (pr.second >= m_curNo)
				{
					m_curNo = pr.second + 1;
				}
				
			}


			

			auto protos = pt.get_child("Proto", pt);
			std::string cppFileName = filename;
			cppFileName = cppFileName.substr(0, cppFileName.length() - 4);
			std::string macroFileName = "_";
			macroFileName += cppFileName;
			macroFileName += "_H_";
			cppFileName += ".h";
#ifndef WIN32
			mkdir("C++");
#else
			_mkdir("C++");
#endif
			
			std::ofstream os;
			os.open(std::string("C++/") + cppFileName, std::ios::binary);
			if (!os.is_open())
			{
				throw std::runtime_error("open cppFileName Error");
			}
			std::string filehead = "#ifndef " + macroFileName + LFCR;
			filehead += "#define " + macroFileName + LFCR;

			os.write(filehead.c_str(), filehead.length());

			for (auto iter = protos.begin(); iter != protos.end(); ++iter)
			{
				std::string stype = iter->first;
				if (stype == "const")
				{
					DataConstValue dc;
					dc.type = iter->second.get<std::string>("<xmlattr>.type");
					dc.name = iter->second.get<std::string>("<xmlattr>.name");
					dc.value = iter->second.get<std::string>("<xmlattr>.value");
					try
					{
						dc.desc = iter->second.get<std::string>("<xmlattr>.desc");
					}
					catch (...)
					{
					}
					std::string text = LFCR;
					text += WriteCppConstValue(dc);
					os.write(text.c_str(), text.length());
					os.flush();
				}
				//结构体类型
				if (stype  == "struct" || stype == "proto")
				{
					ProtoStruct  proto;
					GeneralStruct stt;

					

					if (stype == "proto")
					{
						proto.name = iter->second.get<std::string>("<xmlattr>.name");
						proto.from = iter->second.get<std::string>("<xmlattr>.from");
						proto.to = iter->second.get<std::string>("<xmlattr>.to");
						try
						{
							proto.desc = iter->second.get<std::string>("<xmlattr>.desc");
						}
						catch (...)
						{
						}
						
						std::string idName = genProtoIDName(proto);

						unsigned short No = m_curNo;
						auto iterNo = m_mapCacheNo.find(idName);
						if (iterNo == m_mapCacheNo.end())
						{
							m_mapCacheNo[idName] = No;
							m_curNo++;
						}
						else
						{
							No = iterNo->second;
						}
						if (No >= m_maxNo)
						{
							LOGE("proto No. overflow. curNo=" << m_curNo << ", maxNo=" << m_maxNo);
							return false;
						}
						proto.protoID = No;
					}
					else
					{
						stt.name = iter->second.get<std::string>("<xmlattr>.name");
						try
						{
							stt.desc = iter->second.get<std::string>("<xmlattr>.desc");
						}
						catch (...)
						{
						}
					}
					

					for (auto member = iter->second.begin(); member != iter->second.end(); ++member)
					{
						if (member->first != "member") continue;
						DataMember dm;
						dm.type = member->second.get<std::string>("<xmlattr>.type");
						dm.name = member->second.get<std::string>("<xmlattr>.name");
						try
						{
							dm.desc = member->second.get<std::string>("<xmlattr>.desc");
						}
						catch (...)
						{
						}
						if (stype == "proto")
						{
							proto.members.push_back(dm);
						}
						else
						{
							stt.members.push_back(dm);
						}
					}
					
					std::string text = LFCR;
					if (stype == "proto")
					{
						text += WriteCppProto(proto);
						text += WriteCppDataStructStream(genProtoStructName(proto), proto.members);
					}
					else
					{
						text += WriteCppStruct(stt);
						text += WriteCppDataStructStream(stt.name, stt.members);
					}
					os.write(text.c_str(), text.length());
					os.flush();
				}
				//数组类型
				if (stype == "array")
				{
					DataArray ar;
					ar.type = iter->second.get<std::string>("<xmlattr>.type");
					ar.arrayName = iter->second.get<std::string>("<xmlattr>.name");
					try
					{
						ar.desc = iter->second.get<std::string>("<xmlattr>.desc");
					}
					catch (...)
					{
					}
					std::string text = LFCR;
					text += WriteCppArray(ar);
					os.write(text.c_str(), text.length());
					os.flush();
				}
				//K-V类型
				if (stype == "map")
				{
					DataMap dm;

					dm.typeKey = iter->second.get<std::string>("<xmlattr>.key");
					dm.typeValue = iter->second.get<std::string>("<xmlattr>.value");
					dm.mapName = iter->second.get<std::string>("<xmlattr>.name");
					try
					{
						dm.desc = iter->second.get<std::string>("<xmlattr>.desc");
					}
					catch (...)
					{
					}
					std::string text = LFCR;
					text += WriteCppMap(dm);
					os.write(text.c_str(), text.length());
					os.flush();
				}
			}
			std::string fileEnd = LFCR;
			fileEnd += "#endif" + LFCR;
			os.write(fileEnd.c_str(), fileEnd.length());
			os.close();


		}
		catch (std::string err)
		{
			LOGE("ServerConfig catch exception: error=" << err);
			return false;
		}
		catch (std::exception e)
		{
			LOGE("ServerConfig catch exception: error=" << e.what());
			return false;
		}
		catch (...)
		{
			LOGE("ServerConfig catch exception: unknow exception.");
			return false;
		}
		return true;
	}
	bool WriteNoCache(std::string filename)
	{
		std::ofstream os;
		os.open(filename + ".cache", std::ios::binary);
		if (!os.is_open())
		{
			LOGE(filename + ".cache" << " can not open!.");
			return false;
		}
		std::string text = "<?xml version=\"1.0\" encoding=\"UTF - 8\"?>\n<CacheNo>\n";
		for (auto &pr : m_mapCacheNo)
		{
			text += "\t<cache key = \"" + pr.first + "\" No = \"" + boost::lexical_cast<std::string>(pr.second) + "\" /> \n";
		}
		text += "</CacheNo>\n"; 
		os.write(text.c_str(), text.length());
		os.close();
		return true;
	}

};











int main(int argc, char *argv[])
{
	zsummer::log4z::ILog4zManager::GetInstance()->Start();
	boost::filesystem::path full_path("./", boost::filesystem::native);
	if (boost::filesystem::exists(full_path))
	{
		boost::filesystem::directory_iterator  iter(full_path);
		boost::filesystem::directory_iterator  iter_end;
		for (; iter != iter_end; iter++)
		{
			if (boost::filesystem::is_regular_file(*iter))
			{
				std::stringstream os;
				os <<*iter;
				std::string xmlFile;
				os >> xmlFile;
				
				if (xmlFile.length() <= 8)
				{
					continue;
				}
				xmlFile = xmlFile.substr(3, xmlFile.length() - 3);
				xmlFile = xmlFile.substr(0, xmlFile.length() - 1);
				
				std::string xmlattr = xmlFile.substr(xmlFile.length() - 4, 4);
				if (xmlattr != ".xml")
				{
					continue;
				}
				LOGI(xmlFile);
				
				genProto gen;
				gen.LoadCache(xmlFile);
				gen.gen(xmlFile);
				gen.WriteNoCache(xmlFile);
			}
		}
	}


	return 0;
}







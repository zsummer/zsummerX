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
#include <log4z/log4z.h>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>


#ifndef WIN32
#include <sys/stat.h>
#else
#include <direct.h>
#endif



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
			auto traits = pt.get_child("ProtoTraits");
			m_minNo = boost::lexical_cast<unsigned short>(traits.get_child("MinNo").data());
			m_maxNo = boost::lexical_cast<unsigned short>(traits.get_child("MaxNo").data());
			try
			{
				m_bUseLog4z = boost::lexical_cast<bool>(traits.get_child("UseLog4z").data());
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
			std::string filehead = "#ifndef " + macroFileName + "\n";
			filehead += "#define " + macroFileName + "\n";

			os.write(filehead.c_str(), filehead.length());

			for (auto iter = protos.begin(); iter != protos.end(); ++iter)
			{
				std::string stype = iter->first;
				//枚举类型
				if (stype == "enum")
				{
					std::string enumText = std::string("\n\nenum ") + iter->second.get<std::string>("<xmlattr>.name") + " //" + iter->second.get<std::string>("<xmlattr>.desc") + "\n";
					enumText += "{\n";
					for (auto member = iter->second.begin(); member != iter->second.end(); ++member)
					{
						if(member->first != "member") continue;
						std::string dtype = member->second.get<std::string>("<xmlattr>.name");
						if (dtype == "string")
						{
							dtype = "std::string";
						}
						enumText += "\t";
						enumText += dtype + " = " + member->second.get<std::string>("<xmlattr>.value")  + ", //" + iter->second.get<std::string>("<xmlattr>.desc") + ",\n";
					}
					enumText += "};\n";
					os.write(enumText.c_str(), enumText.length());
					os.flush();
				}
				//结构体类型
				if (stype  == "struct" || stype == "proto")
				{
					std::string structName = iter->second.get<std::string>("<xmlattr>.name");

					if (stype == "proto")
					{
						std::string from = iter->second.get<std::string>("<xmlattr>.from");
						std::string to = iter->second.get<std::string>("<xmlattr>.to");

						unsigned short No = m_curNo;
						auto iterNo = m_mapCacheNo.find(structName);
						if (iterNo == m_mapCacheNo.end())
						{
							m_mapCacheNo[structName] = No;
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
						
						std::string NoText = std::string("\n\nconst unsigned short ID_") + from + "2" + to + "_" + structName + " = " + boost::lexical_cast<std::string>(m_curNo)+";";
						structName = std::string("Proto") + structName;
						os.write(NoText.c_str(), NoText.length());
						os.flush();
					}
					


					std::vector<std::pair<std::string, std::string> > memberVct;

					//struct
					std::string structText = std::string("\n\nstruct ") + structName + " //" + iter->second.get<std::string>("<xmlattr>.desc") + "\n";
					structText += "{\n";
					for (auto member = iter->second.begin(); member != iter->second.end(); ++member)
					{
						if (member->first != "member") continue;
						std::string dtype = member->second.get<std::string>("<xmlattr>.type");
						std::string dname = member->second.get<std::string>("<xmlattr>.name");
						memberVct.push_back(std::make_pair(dtype, dname));
						if (dtype == "string")
						{
							dtype = "std::string";
						}
						structText += "\t";
						structText += dtype + " " + dname + "; //" + iter->second.get<std::string>("<xmlattr>.desc") + ",\n";
					}
					structText += "};\n";
					os.write(structText.c_str(), structText.length());
					

					//输入流
					structText = "\ntemplate<class T> \n";
					structText += "T & operator << (T & t, const " + structName + " & data)\n{\n";
					for (auto &pr : memberVct)
					{
						structText += "\t";
						structText += "t << data." + pr.second + ";\n";
					}
					structText += "\t";
					structText += "return t;\n}\n";
					os.write(structText.c_str(), structText.length());

					//输出流
					structText = "\ntemplate<class T> \n";
					structText += "T & operator >> (T & t, " + structName + " & data)\n{\n";
					for (auto &pr : memberVct)
					{
						structText += "\t";
						structText += "t >> data." + pr.second + ";\n";
					}
					structText += "\t";
					structText += "return t;\n}\n";
					
					os.write(structText.c_str(), structText.length());

					//支持log4z打印
					if (m_bUseLog4z)
					{
						structText = "\ntemplate<> \n";
						structText += "zsummer::log4z::CStringStream & operator << (zsummer::log4z::CStringStream & t, " + structName + " & data)\n{\n";
						structText += "\tt << " + structName + " <<\"{ \"\n";
						for (auto &pr : memberVct)
						{
							structText += "\t << \" " + pr.second + "=\" << " + pr.second + " \n";
						}
						structText += "\t << \"}\";\n";
						structText += "\t return t;\n}\n";
						os.write(structText.c_str(), structText.length());
					}
					os.flush();
				}
				//数组类型
				if (stype == "array")
				{
					std::string dtype = iter->second.get<std::string>("<xmlattr>.type");
					if (dtype == "string")
					{
						dtype = "std::string";
					}
					std::string arrayText = std::string("\n\ntypdef std::vector<") + dtype + "> "
						+ iter->second.get<std::string>("<xmlattr>.name")
						+ "; //" + iter->second.get<std::string>("<xmlattr>.desc") + "\n";
					os.write(arrayText.c_str(), arrayText.length());
					os.flush();
				}
				//K-V类型
				if (stype == "map")
				{
					std::string ktype = iter->second.get<std::string>("<xmlattr>.key");
					if (ktype == "string")
					{
						ktype = "std::string";
					}
					std::string vtype = iter->second.get<std::string>("<xmlattr>.value");
					if (vtype == "string")
					{
						vtype = "std::string";
					}
					std::string mapText = std::string("\n\ntypdef std::map<") + ktype + ", "+ vtype + "> "
						+ iter->second.get<std::string>("<xmlattr>.name") +  "; //" + iter->second.get<std::string>("<xmlattr>.desc") + "\n";
					os.write(mapText.c_str(), mapText.length());
					os.flush();
				}



			}
			os.write("\n#endif\n", 8);
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







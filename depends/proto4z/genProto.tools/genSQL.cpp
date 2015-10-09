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

#include "genSQL.h"
std::string GenSQL::getRealType(const std::string & xmltype)
{
    if (xmltype == "i8") return "char";
    else if (xmltype == "ui8") return "unsigned char";
    else if (xmltype == "i16") return "short";
    else if (xmltype == "ui16") return "unsigned short";
    else if (xmltype == "i32") return "int";
    else if (xmltype == "ui32") return "unsigned int";
    else if (xmltype == "i64") return "long long";
    else if (xmltype == "ui64") return "unsigned long long";
    else if (xmltype == "float") return "float";
    else if (xmltype == "double") return "double";
    else if (xmltype == "string") return "std::string";
    return xmltype;
}


std::string GenSQL::genRealContent(const std::list<AnyData> & stores)
{
    std::string macroFileName = std::string("_") + _filename + "SQL_H_";
    std::transform(macroFileName.begin(), macroFileName.end(), macroFileName.begin(), [](char ch){ return std::toupper(ch); });


    std::string text = LFCR + "#ifndef " + macroFileName + LFCR;
    text += "#define " + macroFileName + LFCR + LFCR;

    for (auto &info : stores)
    {
        if ((info._type == GT_DataStruct || info._type == GT_DataProto) && info._proto._struct._isStore)
        {

            //build
            text += LFCR;
            text += "inline std::vector<std::string> " + info._proto._struct._name + "_BUILD()" + LFCR;
            text += "{" + LFCR;
            text += "    std::vector<std::string> ret;" + LFCR;
            text += "    ret.push_back(\"desc `tb_" + info._proto._struct._name + "`\");" + LFCR;
            text += "    ret.push_back(\"CREATE TABLE `tb_" + info._proto._struct._name + "` (" ; 
            for (auto& m : info._proto._struct._members)
            {
                if (m._tag != MT_DB_KEY)
                {
                    continue;
                }
                text +="        `" + m._name + "`";
                if (m._type == "string")
                {
                    text += " varchar(255) NOT NULL DEFAULT '' , ";
                }
                else if (m._type == "i8" || m._type == "i16" || m._type == "i32" || m._type == "i64")
                {
                    text += " bigint(20) NOT NULL DEFAULT '0' ,  ";
                }
                else if (m._type == "ui8" || m._type == "ui16" || m._type == "ui32" || m._type == "ui64")
                {
                    text += " bigint(20) unsigned NOT NULL DEFAULT '0' , ";
                }
            }
            text += "        PRIMARY KEY("; 
            for (auto& m : info._proto._struct._members)
            {
                if (m._tag != MT_DB_KEY)
                {
                    continue;
                }
                text += "`" + m._name + "`,";
            }
            text.pop_back();
            text +=") ) ENGINE = MyISAM DEFAULT CHARSET = utf8\");" + LFCR;



            for (auto& m : info._proto._struct._members)
            {
                if (m._tag == MT_DB_KEY || m._tag == MT_DB_IGNORE || m._tag == MT_DELETE)
                {
                    continue;
                }


                text += "    ret.push_back(\"alter table `tb_" + info._proto._struct._name + "` add `" + m._name + "` ";
                if (m._type == "string")
                {
                    text += " varchar(255) NOT NULL DEFAULT '' ";
                }
                else if (m._type == "i8" || m._type == "i16" || m._type == "i32" || m._type == "i64")
                {
                    text += " bigint(20) NOT NULL DEFAULT '0' ";
                }
                else if (m._type == "ui8" || m._type == "ui16" || m._type == "ui32" || m._type == "ui64")
                {
                    text += " bigint(20) unsigned NOT NULL DEFAULT '0' ";
                }
                else
                {
                    text += " blob ";
                }
                text += "\");" + LFCR;
            }
        
            text += "    return std::move(ret);" + LFCR;
            text += "}" + LFCR;


            //load
            text += LFCR;
            text += "inline std::string " + info._proto._struct._name + "_LOAD( unsigned long long curLoaded)" + LFCR;
            text += "{" + LFCR;
            text += "    zsummer::mysql::DBQuery q(\" select ";
            for (auto& m: info._proto._struct._members)
            {
                if (m._tag == MT_DB_IGNORE || m._tag == MT_DELETE)
                {
                    continue;
                }
                text += "`" + m._name + "`";
                text += ",";
            }
            text.pop_back();

            text += "from `tb_" + info._proto._struct._name + "` order by ";
            for (auto& m : info._proto._struct._members)
            {
                if (m._tag != MT_DB_KEY)
                {
                    continue;
                }
                text += "`" + m._name + "`,";
            }
            text.pop_back();
            text += " desc limit ?, 1000 \");" + LFCR;
            text += "    q << curLoaded;" + LFCR;
            text += "    return q.popSQL();" + LFCR;
            text += "}" + LFCR;

            //select
            text += LFCR;
            text += "inline std::string " + info._proto._struct._name + "_SELECT(";
            for (auto& m : info._proto._struct._members)
            {
                if (m._tag != MT_DB_KEY)
                {
                    continue;
                }
                text += getRealType(m._type) + " " + m._name + ", ";
            }
            text.pop_back();
            text.pop_back();
            text += ")" + LFCR;
            text += "{" + LFCR;
            text += "    zsummer::mysql::DBQuery q(\" select ";
            for (auto& m : info._proto._struct._members)
            {
                if (m._tag == MT_DB_IGNORE || m._tag == MT_DELETE)
                {
                    continue;
                }
                text += "`" + m._name + "`";
                text += ",";
            }
            text.pop_back();

            text += "from `tb_" + info._proto._struct._name + "` where ";
            for (auto& m : info._proto._struct._members)
            {
                if (m._tag != MT_DB_KEY)
                {
                    continue;
                }
                text += "`" + m._name + "` = ? and ";
            }
            text.pop_back();
            text.pop_back();
            text.pop_back();
            text.pop_back();
            text += "\");" + LFCR;
            for (auto& m : info._proto._struct._members)
            {
                if (m._tag != MT_DB_KEY)
                {
                    continue;
                }
                text += "    q << " + m._name + ";" + LFCR;
            }
            
            text += "    return q.popSQL();" + LFCR;
            text += "}" + LFCR;

            //fetch
            text += LFCR;
            text += "inline ";
            std::string fetchType;
            for (auto& m : info._proto._struct._members)
            {
                if (m._tag != MT_DB_KEY)
                {
                    continue;
                }
                fetchType += "std::map<" + getRealType(m._type) + ", ";
            }
            fetchType += info._proto._struct._name;
            for (auto& m : info._proto._struct._members)
            {
                if (m._tag != MT_DB_KEY)
                {
                    continue;
                }
                fetchType += "> ";
            }
            text += fetchType + " ";
            text += info._proto._struct._name + "_FETCH(zsummer::mysql::DBResultPtr ptr)" + LFCR;
            text += "{" + LFCR;
            text += "    " + fetchType + " ret;" + LFCR;
            text += "    if (ptr->getErrorCode() != zsummer::mysql::QEC_SUCCESS)" + LFCR;
            text += "    {" + LFCR;
            text += "        "  "LOGE(\"fetch info from db found error. ErrorCode=\"  <<  ptr->getErrorCode() << \", Error=\" << ptr->getLastError());" + LFCR;
            text += "        "  "return ret;" + LFCR;
            text += "    }" + LFCR;
            text += "    try" + LFCR;
            text += "    {" + LFCR;

            text += "        "  "while (ptr->haveRow())" + LFCR;
            text += "        {" + LFCR;
            text += "            " + info._proto._struct._name + " " + "info;" + LFCR;
            for (auto& m : info._proto._struct._members)
            {
                if (m._tag == MT_DB_IGNORE || m._tag == MT_DELETE)
                {
                    continue;
                }
                if (m._type == "ui8" || m._type == "ui16" || m._type == "ui32" || m._type == "ui64"
                    || m._type == "i8" || m._type == "i16" || m._type == "i32" || m._type == "i64"
                    || m._type == "double" || m._type == "float" || m._type == "string")
                {
                    text += "            *ptr >> info." + m._name + ";" + LFCR;
                }
                else
                {
                    text += "            try" + LFCR;
                    text += "            {" + LFCR;

                    text += "                "  "std::string blob;" + LFCR;
                    text += "                "  "*ptr >> blob;" + LFCR;
                    text += "                "  "if(!blob.empty())" + LFCR;
                    text += "                "  "{" + LFCR;
                    text += "                    "  "zsummer::proto4z::ReadStream rs(blob.c_str(), (zsummer::proto4z::Integer)blob.length(), false);" + LFCR;
                    text += "                    "  "rs >> info." + m._name + ";" + LFCR;
                    text += "                "  "}" + LFCR;

                    text += "            }" + LFCR;
                    text += "            catch(std::runtime_error e)" + LFCR;
                    text += "            {" + LFCR;
                    text += "                "  "LOGW(\"fetch blob catch one runtime warning. what=\" << e.what());" + LFCR;
                    text += "            }" + LFCR;

                }
            }

            text += "            ret";
            for (auto& m : info._proto._struct._members)
            {
                if (m._tag != MT_DB_KEY)
                {
                    continue;
                }
                text += "[info." + m._name + "]";
            }
            text += " = info; " + LFCR;
            text += "        }" + LFCR;

            text += "    }" + LFCR;
            text += "    catch(std::runtime_error e)" + LFCR;
            text += "    {" + LFCR;
            text += "        "  "LOGE(\"fetch info catch one runtime error. what=\" << e.what());" + LFCR;
            text += "        "  "return ret;" + LFCR;
            text += "    }" + LFCR;

            text += "    return std::move(ret);" + LFCR;
            text += "}" + LFCR;
            text += LFCR;

            //insert
            text += LFCR;
            text += "inline std::string " + info._proto._struct._name + "_INSERT( const " + info._proto._struct._name + " & info) " + LFCR;
            text += "{" + LFCR;
            text += "    zsummer::mysql::DBQuery q(\" insert into tb_" + info._proto._struct._name + "( ";
            for (auto& m : info._proto._struct._members)
            {
                if (m._tag == MT_DB_IGNORE || m._tag == MT_DELETE)
                {
                    continue;
                }
                text += "`" + m._name + "`,";
            }
            text.pop_back();

            text += ") values(";
            for (auto& m : info._proto._struct._members)
            {
                if (m._tag == MT_DB_IGNORE || m._tag == MT_DELETE)
                {
                    continue;
                }
                text += "?,";
            }
            text.pop_back();
            text += ")\");" + LFCR;
            for (auto& m : info._proto._struct._members)
            {
                if (m._tag == MT_DB_IGNORE || m._tag == MT_DELETE)
                {
                    continue;
                }
                if (m._type == "ui8" || m._type == "ui16" || m._type == "ui32" || m._type == "ui64"
                    || m._type == "i8" || m._type == "i16" || m._type == "i32" || m._type == "i64"
                    || m._type == "double" || m._type == "float" || m._type == "string")
                {
                    text += "    q << info." + m._name + ";" + LFCR;
                }
                else
                {
                    text += "    "  "try" + LFCR;
                    text += "    "  "{" + LFCR;
                    text += "        "  "zsummer::proto4z::WriteStream ws(0);" + LFCR;
                    text += "        "  "ws << info." + m._name + ";" + LFCR;
                    text += "        "  "q << std::string(ws.getStreamBody(), ws.getStreamBodyLen());" + LFCR;
                    text += "    "  "}" + LFCR;
                    text += "    "  "catch(std::runtime_error e)" + LFCR;
                    text += "    "  "{" + LFCR;
                    text += "        "  "LOGW(\"write blob catch one runtime warning. what=\" << e.what());" + LFCR;
                    text += "        "  "return \"\";" + LFCR;
                    text += "    "  "}" + LFCR;
                }

            }
            text += "    return q.popSQL();" + LFCR;
            text += "}" + LFCR;
            text += LFCR;


            //update
            text += LFCR;
            text += "inline std::string " + info._proto._struct._name + "_UPDATE( const " + info._proto._struct._name + " & info) " + LFCR;
            text += "{" + LFCR;
            text += "    zsummer::mysql::DBQuery q(\" insert into tb_" + info._proto._struct._name + "(";
            for (auto& m : info._proto._struct._members)
            {
                if (m._tag != MT_DB_KEY)
                {
                    continue;
                }
                text += m._name;
                text += ",";
            }
            text.pop_back();
            text += ") values(";
            for (auto& m : info._proto._struct._members)
            {
                if (m._tag != MT_DB_KEY)
                {
                    continue;
                }
                text += "?,";
            }
            text.pop_back();
            text += " ) on duplicate key update ";
            for (auto& m : info._proto._struct._members)
            {
                if (m._tag == MT_DB_KEY)
                {
                    continue;
                }
                if (m._tag == MT_DB_IGNORE || m._tag == MT_DELETE)
                {
                    continue;
                }
                text += "`" + m._name + "` = ?,";
            }
            text.pop_back();

            text += " \");" + LFCR;
            for (auto& m : info._proto._struct._members)
            {
                if (m._tag != MT_DB_KEY)
                {
                    continue;
                }
                text += "    q << info." + m._name + ";" + LFCR;
            }
            

            for (auto& m : info._proto._struct._members)
            {
                if (m._tag == MT_DB_KEY)
                {
                    continue;
                }
                if (m._tag == MT_DB_IGNORE || m._tag == MT_DELETE)
                {
                    continue;
                }
                if (m._type == "ui8" || m._type == "ui16" || m._type == "ui32" || m._type == "ui64"
                    || m._type == "i8" || m._type == "i16" || m._type == "i32" || m._type == "i64"
                    || m._type == "double" || m._type == "float" || m._type == "string")
                {
                    text += "    q << info." + m._name + ";" + LFCR;
                }
                else
                {
                    text += "    "  "try" + LFCR;
                    text += "    "  "{" + LFCR;
                    text += "        "  "zsummer::proto4z::WriteStream ws(0);" + LFCR;
                    text += "        "  "ws << info." + m._name + ";" + LFCR;
                    text += "        "  "q << std::string(ws.getStreamBody(), ws.getStreamBodyLen());" + LFCR;
                    text += "    "  "}" + LFCR;
                    text += "    "  "catch(std::runtime_error e)" + LFCR;
                    text += "    "  "{" + LFCR;
                    text += "        "  "LOGW(\"write blob catch one runtime warning. what=\" << e.what());" + LFCR;
                    text += "        "  "return \"\";" + LFCR;
                    text += "    "  "}" + LFCR;
                }
                
            }
            text += "    return q.popSQL();" + LFCR;
            text += "}" + LFCR;
            text += LFCR;
        }

    }

    text += LFCR + "#endif" + LFCR;

    return std::move(text);
}




 
#ifndef _TESTPROTO_H_ 
#define _TESTPROTO_H_ 
 
const unsigned short EC_SUCCESS = 0; //success 
const unsigned short EC_ERROR = 1; //error 
const unsigned short EC_INVALID_PARAM = 1000; //invalid param 
const unsigned short EC_UNKNOWN_ID = 1001; //unknwon id 
 
struct TestIntegerData //测试 
{ 
    char _char;  
    unsigned char _uchar;  
    short _short;  
    unsigned short _ushort;  
    int _int;  
    unsigned int _uint;  
    long long _i64;  
    unsigned long long _ui128; //[already deleted] 
    unsigned long long _ui64;  
    TestIntegerData() 
    { 
        _char = 0; 
        _uchar = 0; 
        _short = 0; 
        _ushort = 0; 
        _int = 0; 
        _uint = 0; 
        _i64 = 0; 
        _ui128 = 0; 
        _ui64 = 0; 
    } 
}; 
inline zsummer::proto4z::WriteStream & operator << (zsummer::proto4z::WriteStream & ws, const TestIntegerData & data) 
{ 
    ws << data._char; 
    ws << data._uchar; 
    ws << data._short; 
    ws << data._ushort; 
    ws << data._int; 
    ws << data._uint; 
    ws << data._i64; 
//    ws << data._ui128; //[already deleted] 
    ws << data._ui64; 
    return ws; 
} 
inline zsummer::proto4z::ReadStream & operator >> (zsummer::proto4z::ReadStream & rs, TestIntegerData & data) 
{ 
        rs >> data._char;  
        rs >> data._uchar;  
        rs >> data._short;  
        rs >> data._ushort;  
        rs >> data._int;  
        rs >> data._uint;  
        rs >> data._i64;  
//        rs >> data._ui128;  
        rs >> data._ui64;  
    return rs; 
} 
 
struct TestFloatData //测试 
{ 
    float _float;  
    double _double;  
    TestFloatData() 
    { 
        _float = 0.0; 
        _double = 0.0; 
    } 
}; 
inline zsummer::proto4z::WriteStream & operator << (zsummer::proto4z::WriteStream & ws, const TestFloatData & data) 
{ 
    ws << data._float; 
    ws << data._double; 
    return ws; 
} 
inline zsummer::proto4z::ReadStream & operator >> (zsummer::proto4z::ReadStream & rs, TestFloatData & data) 
{ 
        rs >> data._float;  
        rs >> data._double;  
    return rs; 
} 
 
struct TestStringData //测试 
{ 
    std::string _string;  
}; 
inline zsummer::proto4z::WriteStream & operator << (zsummer::proto4z::WriteStream & ws, const TestStringData & data) 
{ 
    ws << data._string; 
    return ws; 
} 
inline zsummer::proto4z::ReadStream & operator >> (zsummer::proto4z::ReadStream & rs, TestStringData & data) 
{ 
        rs >> data._string;  
    return rs; 
} 
 
typedef std::vector<unsigned int> IntArray;  
 
typedef std::vector<TestIntegerData> TestIntegerDataArray;  
 
typedef std::vector<TestFloatData> TestFloatDataArray;  
 
typedef std::vector<TestStringData> TestStringDataArray;  
 
typedef std::map<std::string, TestIntegerData> TestIntegerDataMap;  
 
typedef std::map<std::string, TestFloatData> TestFloatDataMap;  
 
typedef std::map<std::string, TestStringData> TestStringDataMap;  
 
const unsigned short ID_EchoPack = 30000;  
struct EchoPack 
{ 
    TestIntegerDataArray _iarray;  
    TestFloatDataArray _farray;  
    TestStringDataArray _sarray;  
    TestIntegerDataMap _imap;  
    TestFloatDataMap _fmap;  
    TestStringDataMap _smap;  
    static const unsigned short GetProtoID() { return 30000;} 
    static const std::string GetProtoName() { return "ID_EchoPack";} 
}; 
inline zsummer::proto4z::WriteStream & operator << (zsummer::proto4z::WriteStream & ws, const EchoPack & data) 
{ 
    ws << data._iarray; 
    ws << data._farray; 
    ws << data._sarray; 
    ws << data._imap; 
    ws << data._fmap; 
    ws << data._smap; 
    return ws; 
} 
inline zsummer::proto4z::ReadStream & operator >> (zsummer::proto4z::ReadStream & rs, EchoPack & data) 
{ 
        rs >> data._iarray;  
        rs >> data._farray;  
        rs >> data._sarray;  
        rs >> data._imap;  
        rs >> data._fmap;  
        rs >> data._smap;  
    return rs; 
} 
 
const unsigned short ID_Pulse = 30001;  
struct Pulse 
{ 
    static const unsigned short GetProtoID() { return 30001;} 
    static const std::string GetProtoName() { return "ID_Pulse";} 
}; 
inline zsummer::proto4z::WriteStream & operator << (zsummer::proto4z::WriteStream & ws, const Pulse & data) 
{ 
    return ws; 
} 
inline zsummer::proto4z::ReadStream & operator >> (zsummer::proto4z::ReadStream & rs, Pulse & data) 
{ 
    return rs; 
} 
 
#endif 

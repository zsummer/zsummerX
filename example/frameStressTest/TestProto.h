 
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
    unsigned long long tag = 383ULL; 
    if (zsummer::proto4z::__localEndianType() != zsummer::proto4z::LittleEndian) tag = zsummer::proto4z::byteRevese(tag); 
    ws << (zsummer::proto4z::Integer)0; 
    zsummer::proto4z::Integer offset = ws.getStreamLen(); 
    ws << tag; 
    ws << data._char; 
    ws << data._uchar; 
    ws << data._short; 
    ws << data._ushort; 
    ws << data._int; 
    ws << data._uint; 
    ws << data._i64; 
//    ws << data._ui128; //[already deleted] 
    ws << data._ui64; 
    ws.fixOriginalData(offset - 4, ws.getStreamLen() - offset); 
    return ws; 
} 
inline zsummer::proto4z::ReadStream & operator >> (zsummer::proto4z::ReadStream & rs, TestIntegerData & data) 
{ 
    zsummer::proto4z::Integer sttLen = 0; 
    rs >> sttLen; 
    zsummer::proto4z::Integer cursor = rs.getStreamUnreadLen(); 
    unsigned long long tag = 0; 
    rs >> tag; 
    if (zsummer::proto4z::__localEndianType() != zsummer::proto4z::LittleEndian) tag = zsummer::proto4z::byteRevese(tag); 
    if ( (1ULL << 0) & tag) 
    { 
        rs >> data._char;  
    } 
    if ( (1ULL << 1) & tag) 
    { 
        rs >> data._uchar;  
    } 
    if ( (1ULL << 2) & tag) 
    { 
        rs >> data._short;  
    } 
    if ( (1ULL << 3) & tag) 
    { 
        rs >> data._ushort;  
    } 
    if ( (1ULL << 4) & tag) 
    { 
        rs >> data._int;  
    } 
    if ( (1ULL << 5) & tag) 
    { 
        rs >> data._uint;  
    } 
    if ( (1ULL << 6) & tag) 
    { 
        rs >> data._i64;  
    } 
    if ( (1ULL << 7) & tag) 
    { 
        rs >> data._ui128;  
    } 
    if ( (1ULL << 8) & tag) 
    { 
        rs >> data._ui64;  
    } 
    cursor = cursor - rs.getStreamUnreadLen(); 
    rs.skipOriginalData(sttLen - cursor); 
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
    unsigned long long tag = 3ULL; 
    if (zsummer::proto4z::__localEndianType() != zsummer::proto4z::LittleEndian) tag = zsummer::proto4z::byteRevese(tag); 
    ws << (zsummer::proto4z::Integer)0; 
    zsummer::proto4z::Integer offset = ws.getStreamLen(); 
    ws << tag; 
    ws << data._float; 
    ws << data._double; 
    ws.fixOriginalData(offset - 4, ws.getStreamLen() - offset); 
    return ws; 
} 
inline zsummer::proto4z::ReadStream & operator >> (zsummer::proto4z::ReadStream & rs, TestFloatData & data) 
{ 
    zsummer::proto4z::Integer sttLen = 0; 
    rs >> sttLen; 
    zsummer::proto4z::Integer cursor = rs.getStreamUnreadLen(); 
    unsigned long long tag = 0; 
    rs >> tag; 
    if (zsummer::proto4z::__localEndianType() != zsummer::proto4z::LittleEndian) tag = zsummer::proto4z::byteRevese(tag); 
    if ( (1ULL << 0) & tag) 
    { 
        rs >> data._float;  
    } 
    if ( (1ULL << 1) & tag) 
    { 
        rs >> data._double;  
    } 
    cursor = cursor - rs.getStreamUnreadLen(); 
    rs.skipOriginalData(sttLen - cursor); 
    return rs; 
} 
 
struct TestStringData //测试 
{ 
    std::string _string;  
}; 
inline zsummer::proto4z::WriteStream & operator << (zsummer::proto4z::WriteStream & ws, const TestStringData & data) 
{ 
    unsigned long long tag = 1ULL; 
    if (zsummer::proto4z::__localEndianType() != zsummer::proto4z::LittleEndian) tag = zsummer::proto4z::byteRevese(tag); 
    ws << (zsummer::proto4z::Integer)0; 
    zsummer::proto4z::Integer offset = ws.getStreamLen(); 
    ws << tag; 
    ws << data._string; 
    ws.fixOriginalData(offset - 4, ws.getStreamLen() - offset); 
    return ws; 
} 
inline zsummer::proto4z::ReadStream & operator >> (zsummer::proto4z::ReadStream & rs, TestStringData & data) 
{ 
    zsummer::proto4z::Integer sttLen = 0; 
    rs >> sttLen; 
    zsummer::proto4z::Integer cursor = rs.getStreamUnreadLen(); 
    unsigned long long tag = 0; 
    rs >> tag; 
    if (zsummer::proto4z::__localEndianType() != zsummer::proto4z::LittleEndian) tag = zsummer::proto4z::byteRevese(tag); 
    if ( (1ULL << 0) & tag) 
    { 
        rs >> data._string;  
    } 
    cursor = cursor - rs.getStreamUnreadLen(); 
    rs.skipOriginalData(sttLen - cursor); 
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
    unsigned long long tag = 63ULL; 
    if (zsummer::proto4z::__localEndianType() != zsummer::proto4z::LittleEndian) tag = zsummer::proto4z::byteRevese(tag); 
    ws << (zsummer::proto4z::Integer)0; 
    zsummer::proto4z::Integer offset = ws.getStreamLen(); 
    ws << tag; 
    ws << data._iarray; 
    ws << data._farray; 
    ws << data._sarray; 
    ws << data._imap; 
    ws << data._fmap; 
    ws << data._smap; 
    ws.fixOriginalData(offset - 4, ws.getStreamLen() - offset); 
    return ws; 
} 
inline zsummer::proto4z::ReadStream & operator >> (zsummer::proto4z::ReadStream & rs, EchoPack & data) 
{ 
    zsummer::proto4z::Integer sttLen = 0; 
    rs >> sttLen; 
    zsummer::proto4z::Integer cursor = rs.getStreamUnreadLen(); 
    unsigned long long tag = 0; 
    rs >> tag; 
    if (zsummer::proto4z::__localEndianType() != zsummer::proto4z::LittleEndian) tag = zsummer::proto4z::byteRevese(tag); 
    if ( (1ULL << 0) & tag) 
    { 
        rs >> data._iarray;  
    } 
    if ( (1ULL << 1) & tag) 
    { 
        rs >> data._farray;  
    } 
    if ( (1ULL << 2) & tag) 
    { 
        rs >> data._sarray;  
    } 
    if ( (1ULL << 3) & tag) 
    { 
        rs >> data._imap;  
    } 
    if ( (1ULL << 4) & tag) 
    { 
        rs >> data._fmap;  
    } 
    if ( (1ULL << 5) & tag) 
    { 
        rs >> data._smap;  
    } 
    cursor = cursor - rs.getStreamUnreadLen(); 
    rs.skipOriginalData(sttLen - cursor); 
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
    unsigned long long tag = 0ULL; 
    if (zsummer::proto4z::__localEndianType() != zsummer::proto4z::LittleEndian) tag = zsummer::proto4z::byteRevese(tag); 
    ws << (zsummer::proto4z::Integer)0; 
    zsummer::proto4z::Integer offset = ws.getStreamLen(); 
    ws << tag; 
    ws.fixOriginalData(offset - 4, ws.getStreamLen() - offset); 
    return ws; 
} 
inline zsummer::proto4z::ReadStream & operator >> (zsummer::proto4z::ReadStream & rs, Pulse & data) 
{ 
    zsummer::proto4z::Integer sttLen = 0; 
    rs >> sttLen; 
    zsummer::proto4z::Integer cursor = rs.getStreamUnreadLen(); 
    unsigned long long tag = 0; 
    rs >> tag; 
    if (zsummer::proto4z::__localEndianType() != zsummer::proto4z::LittleEndian) tag = zsummer::proto4z::byteRevese(tag); 
    cursor = cursor - rs.getStreamUnreadLen(); 
    rs.skipOriginalData(sttLen - cursor); 
    return rs; 
} 
 
#endif 

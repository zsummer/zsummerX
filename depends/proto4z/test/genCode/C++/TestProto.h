 
#ifndef _TESTPROTO_H_ 
#define _TESTPROTO_H_ 
 
 
const unsigned short MAX_SESSIONS = 5000; //max session count  
 
enum GAME_TYPE : unsigned short 
{ 
    GTYPE_INVALID = 0, //无效  
    GTYPE_WAITING = 1, //等待  
    GTYPE_INGAME = 10, //游戏中  
    GTYPE_LEAVE = 11, //离开  
}; 
 
struct TestIntegerData //测试  
{ 
    static const unsigned short getProtoID() { return 30000;} 
    static const std::string getProtoName() { return "TestIntegerData";} 
    char _char;  
    unsigned char _uchar;  
    short _short;  
    unsigned short _ushort;  
    int _int;  
    unsigned int _uint;  
    long long _i64;  
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
        _ui64 = 0; 
    } 
    TestIntegerData(const char & _char, const unsigned char & _uchar, const short & _short, const unsigned short & _ushort, const int & _int, const unsigned int & _uint, const long long & _i64, const unsigned long long & _ui64) 
    { 
        this->_char = _char; 
        this->_uchar = _uchar; 
        this->_short = _short; 
        this->_ushort = _ushort; 
        this->_int = _int; 
        this->_uint = _uint; 
        this->_i64 = _i64; 
        this->_ui64 = _ui64; 
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
    rs >> data._ui64;  
    return rs; 
} 
 
struct TestFloatData //测试  
{ 
    static const unsigned short getProtoID() { return 30001;} 
    static const std::string getProtoName() { return "TestFloatData";} 
    float _float;  
    double _double;  
    TestFloatData() 
    { 
        _float = 0.0; 
        _double = 0.0; 
    } 
    TestFloatData(const float & _float, const double & _double) 
    { 
        this->_float = _float; 
        this->_double = _double; 
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
    static const unsigned short getProtoID() { return 30002;} 
    static const std::string getProtoName() { return "TestStringData";} 
    std::string _string;  
    TestStringData() 
    { 
    } 
    TestStringData(const std::string & _string) 
    { 
        this->_string = _string; 
    } 
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
 
struct EchoPack 
{ 
    static const unsigned short getProtoID() { return 30003;} 
    static const std::string getProtoName() { return "EchoPack";} 
    TestIntegerDataArray _iarray;  
    TestFloatDataArray _farray;  
    TestStringDataArray _sarray;  
    TestIntegerDataMap _imap;  
    TestFloatDataMap _fmap;  
    TestStringDataMap _smap;  
    EchoPack() 
    { 
    } 
    EchoPack(const TestIntegerDataArray & _iarray, const TestFloatDataArray & _farray, const TestStringDataArray & _sarray, const TestIntegerDataMap & _imap, const TestFloatDataMap & _fmap, const TestStringDataMap & _smap) 
    { 
        this->_iarray = _iarray; 
        this->_farray = _farray; 
        this->_sarray = _sarray; 
        this->_imap = _imap; 
        this->_fmap = _fmap; 
        this->_smap = _smap; 
    } 
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
 
struct Pulse 
{ 
    static const unsigned short getProtoID() { return 30004;} 
    static const std::string getProtoName() { return "Pulse";} 
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

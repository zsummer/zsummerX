//! yawei_zhang@foxmail.com

#include <proto4z.h>

#include <iostream>
#include <time.h>
#include <stdio.h>

#ifdef WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif // WIN32

#ifdef __APPLE__
#include<mach/mach_time.h>
#endif // __APPLE__

using namespace std;
using namespace zsummer::proto4z;

#include "C++/TestProto.h"
#include "TestHTTP.h"

void  fillOnePack(EchoPack &pack)
{
    TestIntegerData idata;
    idata._char = 'a';
    idata._uchar = 100;
    idata._short = 200;
    idata._ushort = 300;
    idata._int = 400;
    idata._uint = 500;
    idata._i64 = 600;
    idata._ui64 = 700;

    TestFloatData fdata;
    fdata._float = (float)123123.111111;
    fdata._double = 12312312.88888;

    TestStringData sdata;
    sdata._string = "abcdefg";

    pack._iarray.push_back(idata);
    pack._iarray.push_back(idata);
    pack._farray.push_back(fdata);
    pack._farray.push_back(fdata);
    pack._sarray.push_back(sdata);
    pack._sarray.push_back(sdata);

    pack._imap.insert(std::make_pair("123", idata));
    pack._imap.insert(std::make_pair("223", idata));
    pack._fmap.insert(std::make_pair("323", fdata));
    pack._fmap.insert(std::make_pair("423", fdata));
    pack._smap.insert(std::make_pair("523", sdata));
    pack._smap.insert(std::make_pair("623", sdata));
}


unsigned int getSteadyTime();



int main()
{
    cout << "check http proto ..." << endl;
    TestHTTP th;
    WriteHTTP whGet;
    whGet.addHead("Content-Type", "application/x-www-form-urlencoded");
    whGet.addHead("Host", "www.google.com");
    whGet.get("/");
    th.Test(whGet);

    WriteHTTP whPost;
    whPost.addHead("Content-Type", "application/x-www-form-urlencoded");
    whPost.addHead("Host", "www.google.com");
    whPost.post("/", "index.php?aaa=333");
    th.Test(whPost);

    WriteHTTP whResult;
    whResult.addHead("test", "test");
    whResult.addHead("Content-Type", "application/x-www-form-urlencoded");
    whResult.addHead("Host", "www.google.com");
    whResult.response("200", "");
    th.Test(whResult);

    cout << "check binary proto" << endl;
    try
    {
        WriteStream ws(100);
        EchoPack echo;
        fillOnePack(echo);
        ws << echo;
        ReadStream rs(ws.getStream(), ws.getStreamLen());
        rs >> echo;
        cout << "success" << endl;
    }
    catch (std::runtime_error & e)
    {
        cout << "error:" << e.what() << endl;
    }

    

#define StressCount 1*10000
    unsigned int now = getSteadyTime();
    for (int i = 0; i < StressCount; i++)
    {
        WriteStream ws(100);
        EchoPack echo;
        fillOnePack(echo);
        ws << echo;
    }
    std::cout << "writeStream used time: " << getSteadyTime() - now << std::endl;
    
    now = getSteadyTime();
    for (int i = 0; i < StressCount; i++)
    {
        WriteStream ws(100);
        EchoPack echo;
        fillOnePack(echo);
        ws << echo;
        ReadStream rs(ws.getStream(), ws.getStreamLen());
        rs >> echo;
    }
    std::cout << "write and read stream used time: " << getSteadyTime() - now << std::endl;

  
//     cout << "press any key to continue." << endl;
//     getchar();
    return 0;
}


unsigned int getSteadyTime()
{
#ifdef WIN32
    return ::GetTickCount();
#elif defined(__APPLE__)
    const int64_t kOneMillion = 1000 * 1000;
    mach_timebase_info_data_t timebase_info;
    mach_timebase_info(&timebase_info);
    return (unsigned int)((mach_absolute_time() * timebase_info.numer) / (kOneMillion * timebase_info.denom));
#else
    timespec ts;
    if(clock_gettime(CLOCK_MONOTONIC_RAW, &ts) != 0)
        return 0;
    return ts.tv_sec * 1000 + (ts.tv_nsec / 1000/1000);
#endif
}


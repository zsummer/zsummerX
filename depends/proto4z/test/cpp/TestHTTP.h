#pragma once
#ifndef _TEST_HTTP_H_
#define _TEST_HTTP_H_
#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <list>
#include <deque>
#include <queue>
#include <typeinfo>
#include <proto4z.h>
using namespace  std;
using namespace zsummer::proto4z;

const int MAX_HTTP_LEN = 1000;
class TestHTTP
{
public:
    bool Test(WriteHTTP &wh)
    {
        bool isChunked = false;
        std::string method;
        std::string methodLine;
        std::map<std::string,std::string> head;
        std::string body;
        if (checkHTTPBuffIntegrity(wh.getStream(), wh.getStreamLen(), 1024, isChunked, method, methodLine, head, body).first == IRT_SUCCESS)
        {
            if (head.find("Host") != head.end()
                && (method == "GET" || method == "POST" || methodLine == "200"))
            {
                cout << "Check checkHTTPBuffIntegrity Success" << endl;
            }
            else
            {
                cout << "Check checkHTTPBuffIntegrity Data error" << endl;
                return false;
            }
        }
        else
        {
            cout << "Check checkHTTPBuffIntegrity unpack error. ret =" << (IRT_SHORTAGE ? "IRT_SHORTAGE":"IRT_CORRUPTION") << endl;
            return false;
        }
        if (checkHTTPBuffIntegrity(wh.getStream(), wh.getStreamLen() - 1, 1024, isChunked, method, methodLine, head, body).first != IRT_SHORTAGE)
        {
            cout << "Check checkHTTPBuffIntegrity IRT_SHORTAGE error" << endl;
            return false;
        }
        if (checkHTTPBuffIntegrity(wh.getStream(), wh.getStreamLen(), wh.getStreamLen() - 1,  isChunked, method, methodLine, head, body).first != IRT_CORRUPTION)
        {
            cout << "Check checkHTTPBuffIntegrity IRT_CORRUPTION error" << endl;
            return false;
        }    
        return true;
    }
protected:
private:
};




#endif



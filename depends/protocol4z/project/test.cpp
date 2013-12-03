//! yawei_zhang@foxmail.com

#include "../protocol4z.h"
#include <iostream>
using namespace std;
using namespace zsummer::protocol4z;


struct tagData
{
	bool bl;
	char ch;
	unsigned char uch;
	short sh;
	unsigned short ush;
	int n;
	unsigned int u;
	long l;
	unsigned long ul;
	long long ll;
	unsigned long long ull;
	float f;
	double lf;
	std::string str;
};
void Clear(tagData&tag1)
{
	tag1.bl = false;
	tag1.ch = 0; 
	tag1.uch = 0; 
	tag1.ush = 0; 
	tag1.n = 0; 
	tag1.u = 0; 
	tag1.l = 0; 
	tag1.ul = 0;  
	tag1.ll = 0; 
	tag1.ull = 0; 
	tag1.f = 0; 
	tag1.lf = 0;
	tag1.str.clear();
}
bool operator==(const tagData & tag1, const tagData &tag2)
{
	return tag1.bl == tag2.bl 
		&& tag1.ch == tag2.ch
		&& tag1.uch == tag2.uch
		&& tag1.ush == tag2.ush
		&& tag1.n == tag2.n
		&& tag1.u == tag2.u
		&& tag1.l == tag2.l
		&& tag1.ul == tag2.ul
		&& tag1.ll == tag2.ll
		&& tag1.ull == tag2.ull
		&& tag1.f == tag2.f
		&& tag1.lf == tag2.lf
		&& tag1.str == tag2.str;
}

ReadStream & operator >>(ReadStream & rs, tagData & data)
{
	rs >> data.bl;
	rs >> data.ch;
	rs >> data.uch;
	rs >> data.sh;
	rs >> data.ush;
	rs >> data.n;
	rs >> data.u;
	rs >> data.l;
	rs >> data.ul;
	rs >> data.ll;
	rs >> data.ull;
	rs >> data.f;
	rs >> data.lf;
	rs >> data.str;
	return rs;
}
WriteStream & operator <<(WriteStream & ws, const tagData & data)
{
	ws << data.bl;
	ws << data.ch;
	ws << data.uch;
	ws << data.sh;
	ws << data.ush;
	ws << data.n;
	ws << data.u;
	ws << data.l;
	ws << data.ul;
	ws << data.ll;
	ws << data.ull;
	ws << data.f;
	ws << data.lf;
	ws << data.str;
	return ws;
}
int main()
{
	char buf[100];
	
	tagData test1;
	tagData test2;
	Clear(test1);
	Clear(test2);
	test1.bl = true;
	test1.ch = 'a';
	test1.uch = 200;
	test1.sh = -1;
	test1.ush = 65000;
	test1.n = -2;
	test1.u = 3333;
	test1.l = -3;
	test1.ul = 111;
	test1.ll = -4;
	test1.ull = 250;
	test1.f = (float)123.231;
	test1.lf = -120.333333333;
	test1.str = "1234567";


	int randCount = 2;
	bool bigEndianType = true;

	
	while (randCount >0)
	{
		randCount--;
		bigEndianType = !bigEndianType;
		memset(buf, 0, sizeof(buf));
		Clear(test2);
		//2head, 59 pod, 2 str head, 7 string char count.
		const int protocolLen = 2+59+2+7;
		WriteStream ws(buf, protocolLen, bigEndianType);
		try
		{
			ws << test1;
		}
		catch(std::runtime_error e)
		{
			cout << e.what() << endl;
		}

		unsigned short headerlen = ReadStreamHeader(buf, bigEndianType);
		
		if (headerlen != protocolLen)
		{
			cout << "read header len error" << endl;
		}

		ReadStream rs(buf, headerlen, bigEndianType);
		try
		{
			rs >> test2;
		}
		catch(std::runtime_error e)
		{
			cout << e.what() << endl;
		}
		if (test1 == test2)
		{
			cout <<"check  protocol success" << endl;
		}
		else
		{
			cout <<"check lprotocol failed" << endl;
		}

		try
		{
			ws << char(1);
			cout <<"Bounds check  WriteStream failed" << endl;
		}
		catch (std::runtime_error e)
		{
			cout <<"Bounds check  WriteStream success. "<< endl;
		}
		try
		{
			char ch = 'a';
			rs >> ch;
			cout <<"Bounds check  ReadStream failed" << endl;
		}
		catch (std::runtime_error e)
		{
			cout <<"Bounds check  ReadStream success."  << endl;
		}
	}
	


	cout << "all check done . " << endl;
	getchar();
	return 0;
}
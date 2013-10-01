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
};
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
	return ws;
}
int main()
{
	char buf[100];
	
	tagData test1;
	tagData test2;
	memset(&test1,0,sizeof(test1));
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
	{
		memset(buf, 0, sizeof(buf));
		memset(&test2,0, sizeof(test2));
		WriteStream ws(buf, (53-8)+sizeof(long)*2);
		try
		{
			ws << test1;
		}
		catch(std::runtime_error e)
		{
			cout << e.what() << endl;
		}

		memset(&test2, 0, sizeof(test2));
		unsigned short headerlen = 1;
		{
			char *p = (char*)&headerlen;
			if (*p == 1)
			{
				memcpy(&headerlen, buf, 2);
			}
			else
			{
				*((char*)&headerlen) = buf[1];
				*(((char*)&headerlen)+1) = buf[0];
			}
		}

		ReadStream rs(buf, headerlen);
		try
		{
			rs >> test2;
		}
		catch(std::runtime_error e)
		{
			cout << e.what() << endl;
		}
		if (memcmp(&test1, &test2, sizeof(test1)) == 0)
		{
			cout <<"check little endian protocol success" << endl;
		}
		else
		{
			cout <<"check little endian protocol failed" << endl;
		}

		try
		{
			ws << char(1);
			cout <<"Bounds check  litter-endian WriteStream failed" << endl;
		}
		catch (std::runtime_error e)
		{
			cout <<"Bounds check  litter-endian WriteStream success. exception.what()=" << e.what() << endl;
		}
		try
		{
			char ch = 'a';
			rs >> ch;
			cout <<"Bounds check  litter-endian ReadStream failed" << endl;
		}
		catch (std::runtime_error e)
		{
			cout <<"Bounds check  litter-endian ReadStream success. exception.what()=" << e.what() << endl;
		}
	}
	



	{
		memset(buf, 0, sizeof(buf));
		memset(&test2,0, sizeof(test2));
		WriteStream ws(buf, (53-8)+sizeof(long)*2, true);
		try
		{
			ws << test1;
		}
		catch(std::exception e)
		{
			cout << e.what() << endl;
		}

		memset(&test2, 0, sizeof(test2));
		unsigned short headerlen = 1;
		{
			char *p = (char*)&headerlen;
			if (*p == 1)
			{
				*((char*)&headerlen) = buf[1];
				*(((char*)&headerlen)+1) = buf[0];
			}
			else
			{
				memcpy(&headerlen, buf, 2);
			}
		}

		ReadStream rs(buf, headerlen, true);
		try
		{
			rs >> test2;
		}
		catch(std::runtime_error e)
		{
			cout << e.what() << endl;
		}
		if (memcmp(&test1, &test2, sizeof(test1)) == 0)
		{
			cout <<"check big-endian protocol success" << endl;
		}
		else
		{
			cout <<"check big-endian protocol failed" << endl;
		}

		try
		{
			ws << char(1);
			cout <<"Bounds check  big-endian WriteStream failed" << endl;
		}
		catch (std::runtime_error e)
		{
			cout <<"Bounds check  big-endian WriteStream success. exception.what()=" << e.what() << endl;
		}
		try
		{
			char ch = 'a';
			rs >> ch;
			cout <<"Bounds check  big-endian ReadStream failed" << endl;
		}
		catch (std::runtime_error e)
		{
			cout <<"Bounds check  big-endian ReadStream success. exception.what()=" << e.what() << endl;
		}
	}


	return 0;
}
//! yawei_zhang@foxmail.com

#include "../protocol4z.h"
#include <iostream>
#include <stdio.h>
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
template<class StreamHeadTrait>
ReadStream<StreamHeadTrait> & operator >>(ReadStream<StreamHeadTrait> & rs, tagData & data)
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
template<class StreamHeadTrait>
WriteStream<StreamHeadTrait> & operator <<(WriteStream<StreamHeadTrait> & ws, const tagData & data)
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


	tagData test1 = {true, 'a', 200, -1, 65000, -2, 333,-3,111,-4,250, (float)123.2,123.4, "1234567"};


	
	{
		tagData test2 = { 0 };
		//2head, 59 pod, 2 str head, 7 string char count.
		DefaultStreamHeadTrait::Integer packLen = 59 + 7 + DefaultStreamHeadTrait::PackLenSize * 2 + DefaultStreamHeadTrait::PreOffset + DefaultStreamHeadTrait::PostOffset;
		WriteStream<DefaultStreamHeadTrait> ws;
		try
		{
			ws << test1;
			cout << "write all type success" << endl;
		}
		catch (std::runtime_error e)
		{
			cout << "write all type failed. error=" << e.what() << endl;
		}

		DefaultStreamHeadTrait::Integer packLen2 = StreamToInteger<DefaultStreamHeadTrait::Integer, DefaultStreamHeadTrait>(ws.GetWriteStream() + DefaultStreamHeadTrait::PreOffset);

		if (packLen2 != packLen || packLen2 != ws.GetWriteLen())
		{
			cout << "check write header len error" << endl;
		}
		else
		{
			cout << "check write header len success" << endl;
		}
		
		std::pair<bool,DefaultStreamHeadTrait::Integer> ret = CheckBuffIntegrity<DefaultStreamHeadTrait>(ws.GetWriteStream(), 1, 100);
		if (ret.first && ret.second == DefaultStreamHeadTrait::HeadLen-1)
		{
			cout << "CheckBuffIntegrity check write header len success" << endl;
		}
		else
		{
			cout << "CheckBuffIntegrity check write header len failed" << endl;
		}
		ret = CheckBuffIntegrity<DefaultStreamHeadTrait>(ws.GetWriteStream(), 50, 100);
		if (ret.first && ret.second == ws.GetWriteLen() - 50)
		{
			cout << "CheckBuffIntegrity check write header len success" << endl;
		}
		else
		{
			cout << "CheckBuffIntegrity check write header len failed" << endl;
		}
		ret = CheckBuffIntegrity<DefaultStreamHeadTrait>(ws.GetWriteStream(), ws.GetWriteLen(), 200);
		if (ret.first && ret.second == 0)
		{
			cout << "CheckBuffIntegrity check write header len success" << endl;
		}
		else
		{
			cout << "CheckBuffIntegrity check write header len failed" << endl;
		}
		ReadStream<DefaultStreamHeadTrait> rs(ws.GetWriteStream(), ws.GetWriteLen());
		try
		{
			rs >> test2;
		}
		catch (std::runtime_error e)
		{
			cout << e.what() << endl;
		}
		if (test1 == test2)
		{
			cout << "check protocol success" << endl;
		}
		else
		{
			cout << "check protocol failed" << endl;
		}

		try
		{
			char ch = 'a';
			rs >> ch;
			cout << "Bounds check  ReadStream failed" << endl;
		}
		catch (std::runtime_error e)
		{
			cout << "Bounds check  ReadStream success." << endl;
		}
	}

	cout << "check big stream" << endl;
	//--------------------------
	{
		tagData test2 = { 0 };
		//2head, 59 pod, 2 str head, 7 string char count.
		TestBigStreamHeadTrait::Integer packLen = 59 + 7 + TestBigStreamHeadTrait::PackLenSize * 2 + TestBigStreamHeadTrait::PreOffset + TestBigStreamHeadTrait::PostOffset;
		WriteStream<TestBigStreamHeadTrait> ws;
		try
		{
			ws << test1;
			cout << "write all type success" << endl;
		}
		catch (std::runtime_error e)
		{
			cout << "write all type failed. error=" << e.what() << endl;
		}

		TestBigStreamHeadTrait::Integer packLen2 = StreamToInteger<TestBigStreamHeadTrait::Integer, TestBigStreamHeadTrait>(ws.GetWriteStream() + TestBigStreamHeadTrait::PreOffset);
		packLen2 += TestBigStreamHeadTrait::HeadLen;
		if (packLen2 != packLen || packLen2 != ws.GetWriteLen())
		{
			cout << "check write packLen2 len error" << endl;
		}
		else
		{
			cout << "check write packLen2 len success" << endl;
		}

		std::pair<bool, TestBigStreamHeadTrait::Integer> ret = CheckBuffIntegrity<TestBigStreamHeadTrait>(ws.GetWriteStream(), 1, 100);
		if (ret.first && ret.second == TestBigStreamHeadTrait::HeadLen - 1)
		{
			cout << "CheckBuffIntegrity check write header len success" << endl;
		}
		else
		{
			cout << "CheckBuffIntegrity check write header len failed" << endl;
		}
		ret = CheckBuffIntegrity<TestBigStreamHeadTrait>(ws.GetWriteStream(), 50, 100);
		if (ret.first && ret.second == ws.GetWriteLen() - 50)
		{
			cout << "CheckBuffIntegrity check write header len success" << endl;
		}
		else
		{
			cout << "CheckBuffIntegrity check write header len failed" << endl;
		}
		ret = CheckBuffIntegrity<TestBigStreamHeadTrait>(ws.GetWriteStream(), ws.GetWriteLen(), 200);
		if (ret.first && ret.second == 0)
		{
			cout << "CheckBuffIntegrity check write header len success" << endl;
		}
		else
		{
			cout << "CheckBuffIntegrity check write header len failed" << endl;
		}
		ReadStream<TestBigStreamHeadTrait> rs(ws.GetWriteStream(), ws.GetWriteLen());
		try
		{
			rs >> test2;
		}
		catch (std::runtime_error e)
		{
			cout << e.what() << endl;
		}
		if (test1 == test2)
		{
			cout << "check protocol success" << endl;
		}
		else
		{
			cout << "check protocol failed" << endl;
		}

		try
		{
			char ch = 'a';
			rs >> ch;
			cout << "Bounds check  ReadStream failed" << endl;
		}
		catch (std::runtime_error e)
		{
			cout << "Bounds check  ReadStream success." << endl;
		}
	}

	cout << "all check done . " << endl;
	getchar();
	return 0;
}
//! yawei_zhang@foxmail.com

#include "../protocol4z.h"
#include <iostream>
#include <stdio.h>
using namespace std;
using namespace zsummer::protocol4z;

#include "TestBase.h"

int main()
{
	bool bAllCheckOK = false;
	do 
	{
		TestBase<DefaultStreamHeadTraits> test1;
		if (!test1.CheckLenght())
		{
			break;
		}
		
		cout << endl;
		if (!test1.CheckAttachProtocol())
		{
			break;
		}
		cout << endl;
		if (!test1.CheckNoAttachProtocol())
		{
			break;
		}
		cout << endl;
		if (!test1.CheckRouteProtocol())
		{
			break;
		}
		cout << endl;
		cout << endl;
		TestBase<TestBigStreamHeadTraits> test2;
		if (!test2.CheckLenght())
		{
			break;
		}

		cout << endl;
		if (!test2.CheckAttachProtocol())
		{
			break;
		}
		cout << endl;
		if (!test2.CheckNoAttachProtocol())
		{
			break;
		}
		cout << endl;
		if (!test2.CheckRouteProtocol())
		{
			break;
		}
		bAllCheckOK = true;

	} while (0);
	cout << endl;
	if (bAllCheckOK)
	{
		cout << "all check OK . " << endl;
	}
	else
	{
		cout << "check failed . " << endl;
	}
	cout << "press any key to continue." << endl;
	getchar();
	return 0;
}
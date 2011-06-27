// trycatch.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <string>
using namespace std;

class CMyException
{
public:
	CMyException(char*err)
	{
		strcpy_s(m_err,err);
	}
	void Print()
	{
		printf("%s\n",m_err);
	}
protected:
private:
	char m_err[128];
};

void test(int a,int b)
{
	if (a>b)
	{
		throw CMyException("123");
	}

	return;
}

int _tmain(int argc, _TCHAR* argv[])
{
	try
	{
		test(2,1);
	}
	catch (CMyException e)
	{
		e.Print();
	}

	system("pause");

	return 0;
}


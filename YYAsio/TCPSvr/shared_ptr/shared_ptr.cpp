// shared_ptr.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <boost/ptr_container/ptr_container.hpp>
#define _crtdbg_map_alloc
#include "stdlib.h"
#include "crtdbg.h"

class CMan
{
public:
	CMan()
	{
		printf("construct\n");
	}
	~CMan()
	{
		printf("deconstruct\n");
	}
protected:
private:
};

int _tmain(int argc, _TCHAR* argv[])
{
	{
		boost::ptr_vector<CMan> listMan;

		CMan *p1 = new CMan();
		listMan.push_back(p1);

		//char * sz = new char[32];
	}

	_CrtDumpMemoryLeaks();

	//system("pause");

	return 0;
}


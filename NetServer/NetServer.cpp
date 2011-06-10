// NetServer.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

#include "../NetCore/NetCoreDLL/IOCPSvr.h"
#pragma comment(lib,"../bin/debug/netcoredll.lib")

using namespace YYNetSDK::IOCPServer;

static int count = 0;

struct tagMyMsg
{
	char msg[32];
};
typedef YYNetSDK::CMsgT<tagMyMsg> MyMsg;

class CServer : public CIOCPSvr
{
public:
	virtual void OnHandleMsg(LPVOID pAddr,BYTE *data,int dataLen)
	{

		MyMsg *msg = (MyMsg*)data;

		printf("handle msg len=%d,msg=%s,count=%d\n",dataLen,msg->msg.msg,count);

		count++;
	}
	virtual void OnClientClose(LPVOID pAddr)
	{
		printf("OnClientClose\n");
	}
	virtual void OnClientConnect(LPVOID pAddr)
	{
		printf("OnClientConnect\n");
	}
protected:
private:
};

int _tmain(int argc, _TCHAR* argv[])
{
	CServer * svr = new CServer();

	svr->Start(6001,100);

	char buf[32];
	gets(buf);
	while (buf[0]!='q')
	{
		gets(buf);
	}

	return 0;
}


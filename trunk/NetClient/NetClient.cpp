// NetClient.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "../NetCore/NetCoreDLL/ClientSock.h"
#pragma comment(lib,"../bin/debug/netcoredll.lib")

#include "../NetCore/NetCoreDLL/MsgQueue.h"

using namespace YYNetSDK;

struct tagMyMsg 
{
	char msg[32];
};
typedef CMsgT<tagMyMsg> MyMsg;

char *szstr[] = {"a","ab","abc","1","12","123"};

int _tmain(int argc, _TCHAR* argv[])
{
	ClientSock::CClientSock * sock = new ClientSock::CClientSock();

	bool ret = sock->Connect("127.0.0.1",6001);

	if (!ret)
	{
		printf("connect err\n");
	}

	char buf[32];

jj:
	for (int i=0;i<100;i++)
	{
		sprintf(buf,szstr[i%5]);
		buf[strlen(buf)+1]='\0';

		MyMsg msg;
		msg.head.id=1;
		msg.head.len= strlen(buf)+1;
		msg.head.extra = 0;
		memcpy(&msg.msg,buf,strlen(buf));
		sock->SendMsg(msg.GetMsg());
	}

	memset(buf,0,sizeof(buf));
	gets(buf);
	while (buf[0]!='q')
	{
		//MyMsg msg;
		//msg.head.id=1;
		//msg.head.len= strlen(buf);
		//msg.head.extra = 0;

		//memcpy(&msg.msg,buf,strlen(buf));

		//sock->SendMsg(msg.GetMsg());

		//memset(buf,0,sizeof(buf));
		//gets(buf);

		if (buf[0]=='c')
		{
			goto jj;
		}
	}

	return 0;
}


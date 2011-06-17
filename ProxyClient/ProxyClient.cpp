// ProxyClient.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "mysocket.h"
#include "../YYSocketProxy/myMsg.h"

int _tmain(int argc, _TCHAR* argv[])
{
	CMySocket * sock = new CMySocket;

	if(!sock->Connect("127.0.0.1",6002))
	{
		printf("connect err\n");
		goto M;
	}

	MsgConnect msg;
	msg.head.id = 10001;
	msg.head.channel = 0;
	msg.head.extra = 0;
	msg.head.len = sizeof(tagConnect);

	sprintf_s(msg.msg.ip,"127.0.0.1");
	sprintf_s(msg.msg.port,"6001");

	sock->SendMsg((BYTE*)&msg,msg.head.len + sizeof(CMsgHead));

	printf("send len=%d\n",msg.head.len + sizeof(CMsgHead));

	//////////////////////////////////////////////////////////////////////////
	Sleep(3000);
	MsgTest _msg;
	_msg.head.id = 10003;
	_msg.head.channel = 0;
	_msg.head.extra = 0;
	_msg.head.len = sizeof(tagTest);
	sprintf_s(_msg.msg.data,"hello");
	sock->SendMsg((BYTE*)&_msg,_msg.head.len + sizeof(CMsgHead));
	printf("send len=%d\n",_msg.head.len + sizeof(CMsgHead));
M:
	char buf[32];
	gets(buf);
	while (buf[0]!='q')
	{
		gets(buf);
	}

	return 0;
}


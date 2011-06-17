#pragma once

#include "msg.h"

const int MSG_ID_PROXY		= 10000;
const int MSG_ID_CONNECT	= MSG_ID_PROXY + 1;
const int MSG_ID_DISCONNECT = MSG_ID_PROXY + 2;

#pragma pack(1)

struct tagConnect
{
	char ip[17];
	char port[6];
};
typedef CMsgT<tagConnect> MsgConnect;


struct tagTest 
{
	char data[32];
};
typedef CMsgT<tagTest> MsgTest;

#pragma pack()
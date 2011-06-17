#pragma once

#ifdef _DLL_EXPORT
#define _DLL _declspec(dllexport)
#else
#define _DLL _declspec(dllimport)
#endif

#define MSG_MAX_LEN	1024

#define CONNECT_MAX_NUM	3		//one client 最多可连的svr端 3个

#define closesocketex(s)     { \
	LINGER lingerStruct;\
	lingerStruct.l_onoff = 1;\
	lingerStruct.l_linger = 0;\
	setsockopt(s, SOL_SOCKET, SO_LINGER,(char *)&lingerStruct, sizeof(lingerStruct) );\
	CancelIo((HANDLE)s);::shutdown(s,SD_BOTH);::closesocket(s);s = INVALID_SOCKET; }


enum IOType
{
	itInit,
	itReadZero,
	itReadZeroComplete,
	itRead,
	itReadComplete,
	itWrite,
	itWriteComplete
};

enum IODirect
{
	idClient,
	idServer,
	idHeart,
};

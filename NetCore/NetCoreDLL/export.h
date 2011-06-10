#pragma once

#ifndef _DLL_EXPORT
#define _DLL _declspec(dllexport)
#else
#define _DLL _declspec(dllimport)
#endif

#define closesocketex(s)     { \
	LINGER lingerStruct;\
	lingerStruct.l_onoff = 1;\
	lingerStruct.l_linger = 0;\
	setsockopt(s, SOL_SOCKET, SO_LINGER,(char *)&lingerStruct, sizeof(lingerStruct) );\
	CancelIo((HANDLE)s);::shutdown(s,SD_BOTH);::closesocket(s);s = INVALID_SOCKET; }


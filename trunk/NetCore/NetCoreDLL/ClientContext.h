#pragma once
#include "IOBuffer.h"
#include "IOArray.h"
#include <map>
using namespace std;
#include "lock.h"

typedef map<SOCKET,CIOBuffer*> IOBufferMap;
typedef IOBufferMap::iterator IOBufferMapIt;

class CClientContext
{
public:
	CClientContext(void);
	~CClientContext(void);
public:
	SOCKET			m_sock;
	CIOArray<BYTE>	m_IOArray;		//clientÊý¾ÝÇø
	CLock			m_lock;			//lock

	//order
	bool		m_bSendOrder;	
	UINT		m_iSendSequence;
	UINT		m_iSendSequenceCurrent;
	IOBufferMap m_sendBuf;

	bool		m_bRedvOrder;
	UINT		m_iRecvSequence;
	UINT		m_iRecvSequenceCurrent;
	IOBufferMap m_recvBuf;
};

typedef list<CClientContext*> ClientContextList;
typedef ClientContextList::iterator ClientContextListIt;

typedef map<SOCKET,CClientContext*>ClientContextMap;
typedef ClientContextMap::iterator ClientContextMapIt;

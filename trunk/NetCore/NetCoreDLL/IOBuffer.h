#pragma once
#include "MsgQueue.h"
#include "iotype.h"
#include "export.h"

class _DLL CIOBuffer
{
public:
	CIOBuffer(void);
	~CIOBuffer(void);
public:
	OVERLAPPED m_overlapped;
	ULONG		m_ref;
	BYTE		m_data[MAX_MSG_LEN];
	IOType		m_ioType;
	UINT		m_nUsed;			// π”√
	UINT		m_iSequenceNumber;	//À≥–Ú
	WSABUF		m_wsaBuf;
public:
	void	SetupRead();
	void	SetupReadZero();
	void	SetupWrite();
public:
	bool	Flush(UINT nLen);
};

typedef list<CIOBuffer*> IOBufferList;
typedef IOBufferList::iterator IOBufferListIt;
#pragma once
#include "winsock2.h"
#include "export.h"

class CIOBuffer
{
public:
	CIOBuffer()
	{
		memset(this,0,sizeof(CIOBuffer));
	}
	~CIOBuffer()
	{

	}
protected:
private:
public:
	OVERLAPPED	m_overlapped;
	IOType		m_ioType;
	UINT		m_ref;
	BYTE		m_buf[MSG_MAX_LEN];
	UINT		m_used;
	WSABUF		m_wsaBuf;
};
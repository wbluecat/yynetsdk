#pragma once
#include "winsock2.h"
#include "IOArray.h"
#include "lock.h"

class CClientContext
{
public:
	CClientContext()
	{
		memset(this,0,sizeof(CClientContext));
	}
	~CClientContext()
	{
	}
	CClientContext* GetClientPair(BYTE channel)
	{
		if (channel>CONNECT_MAX_NUM)
		{
			return NULL;
		}

		if (pClientPair[channel]->m_sock == INVALID_SOCKET)
		{
			return NULL;
		}

		return pClientPair[channel];
	}
protected:
private:
public:
	SOCKET			m_sock;
	CIOArray<BYTE>	m_ioarray;
	CLock			m_lock;
	IODirect		m_ioDirect;
	BYTE			m_byConnected;	//已经连接数
	char			m_ip[17];
	char			m_port[6];
public:
	typedef CClientContext* LPClientContext;
	LPClientContext pClientPair[CONNECT_MAX_NUM];
};
#include "StdAfx.h"
#include ".\iobuffer.h"

CIOBuffer::CIOBuffer(void)
{
}

CIOBuffer::~CIOBuffer(void)
{
}

void CIOBuffer::SetupReadZero()
{
	m_wsaBuf.buf = (char*)m_data;
	m_wsaBuf.len = 0;
}

void CIOBuffer::SetupRead()
{
	if (m_nUsed==0)
	{
		m_wsaBuf.buf = (char*)m_data;
		m_wsaBuf.len = MAX_MSG_LEN;
	}
	else
	{
		m_wsaBuf.buf = (char*)m_data + m_nUsed;
		m_wsaBuf.len = MAX_MSG_LEN - m_nUsed;
	}
}

void CIOBuffer::SetupWrite()
{
	m_wsaBuf.buf = (char*)m_data;
	m_wsaBuf.len = m_nUsed;
}

bool	CIOBuffer::Flush(UINT nLen)
{
	if (nLen > MAX_MSG_LEN || nLen > m_nUsed)
	{
		return false;
	}

	m_nUsed -= nLen;

	memmove(m_data,m_data+nLen,m_nUsed);

	return true;
}

#include "StdAfx.h"
#include ".\msgqueue.h"
#include <limits>

namespace YYNetSDK
{
	CMsgQueue::CMsgQueue(void)
	{
		m_sema = CreateSemaphore(NULL, 0, (std::numeric_limits<long>::max)(), NULL);
	}

	CMsgQueue::~CMsgQueue(void)
	{
		if (m_sema)
		{
			WaitForSingleObject(m_sema,10);
			CloseHandle(m_sema);
			m_sema = NULL;
		}
	}

	CMsg CMsgQueue::PopMsg()
	{
		WaitForSingleObject(m_sema,-1);

		CMsg msg = m_msgList.front();
		m_msgList.pop_front();
		return msg;
	}

	void CMsgQueue::PushMsg(CMsg msg)
	{
		m_msgList.push_back(msg);

		ReleaseSemaphore(m_sema,1,NULL);
	}

	void CMsgQueue::Destory()
	{
		CMsg msg;

		m_msgList.push_back(msg);

		ReleaseSemaphore(m_sema,1,NULL);
	}
}


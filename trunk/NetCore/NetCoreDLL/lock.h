#pragma once
#include "export.h"

template<class T>
class AutoLock{
public:
	AutoLock(T* pLock):m_pLock(pLock){
		Lock();
	}

	~AutoLock(){
		Unlock();
	}

	void Lock(){
		if(m_pLock != NULL)
			m_pLock->Lock();
	}

	void Unlock(){
		if(m_pLock != NULL)
			m_pLock->Unlock();
	}

private:
	typedef T*  LPLOCK;

	LPLOCK	m_pLock;
};

class _DLL CLock
{
private:
	CRITICAL_SECTION m_CritSect;
public:
	CLock() { InitializeCriticalSection(&m_CritSect); }
	~CLock() { DeleteCriticalSection(&m_CritSect); }

	void Lock() { EnterCriticalSection(&m_CritSect); }
	void Unlock() { LeaveCriticalSection(&m_CritSect); }
	LONG GetLockedThreadCount(){ return ( (- m_CritSect.LockCount -1 ) >> 2 ); }
};

typedef AutoLock<CLock> YYAutoLock;
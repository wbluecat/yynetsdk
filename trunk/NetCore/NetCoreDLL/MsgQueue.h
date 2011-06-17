#pragma once
#include <list>
using namespace std;
#include "export.h"
#include "winsock2.h"


#define MAX_MSG_LEN	1024

#pragma pack(1)

namespace YYNetSDK
{
	class CMsgHead
	{
	public:
		int id;
		int len;		//only body len
		BYTE channel;	//connect channel
		int	extra;
	protected:
	private:
	};

	class CMsg
	{
	public:
		BYTE data[MAX_MSG_LEN];
	public:
		CMsg()
		{
			memset(this,0,sizeof(CMsg));
		}
		CMsgHead GetMsgHead()
		{
			CMsgHead head;
			memcpy(&head,data,sizeof(CMsgHead));
			return head;
		}
	protected:
	private:
	};


	template<typename T>
	class CMsgT
	{
	public:
		CMsgHead head;
		T msg;
	public:
		CMsg GetMsg()
		{
			CMsg tmp;
			memcpy(tmp.data,&head,sizeof(CMsgHead));
			memcpy(tmp.data+sizeof(CMsgHead),&msg,sizeof(T));
			return tmp;
		}
	protected:
	private:
	};

	struct null_type
	{

	};

	template<>
	class CMsgT<null_type>
	{
	public:
		CMsgHead head;
	public:
		CMsg GetMsg()
		{
			CMsg tmp;
			memcpy(&tmp,&head,sizeof(CMsgHead));
			return tmp;
		}
	};

	typedef list<CMsg> MsgList;

	class _DLL CMsgQueue
	{
	public:
		CMsgQueue(void);
		virtual~CMsgQueue(void);
	public:
		CMsg PopMsg();
		void PushMsg(CMsg msg);
		void Destory();
	private:
		MsgList m_msgList;
		HANDLE m_sema;
	};

}

#pragma pack()
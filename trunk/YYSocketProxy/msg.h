#pragma once
#include "export.h"

#pragma pack(1)
class CMsgHead
{
public:
	int		id;
	int		len;		//only body len
	BYTE	channel;	//connect channel
	int		extra;
protected:
private:
};

class CMsg
{
public:
	BYTE data[MSG_MAX_LEN];
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
	void SetHead(int id,int len,int ex)
	{
		head.id = id;
		head.len = len;
		head.extra = ex;
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
#pragma pack()
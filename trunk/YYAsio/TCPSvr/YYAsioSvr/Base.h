#pragma once

#define MSG_MAX_LEN	1024

#pragma pack(1)

class CMsgHead
{
public:
	int id;
	int len;	//body len
	int ex;
protected:
private:
};

class CMsg
{
public:
	BYTE data[MSG_MAX_LEN];

	CMsgHead* GetMsgHead()
	{
		return (CMsgHead*)(data);
	}
protected:
private:
};

template<typename T>
class CMsgT
{
public:
	CMsgHead	head;
	T			msg;
protected:
private:
public:
	void GetMsg(BYTE*data,int &len)
	{
		memcpy(data,&head,sizeof(CMsgHead));
		memcpy(data+sizeof(CMsgHead),&msg,sizeof(T));
		len = sizeof(CMsgHead) + head.len;
	}

	CMsg GetMsg()
	{
		CMsg tmp;
		memcpy(tmp,&head,sizeof(CMsgHead));
		memcpy(tmp+sizeof(CMsgHead),&msg,sizeof(T));
		return tmp;
	}
};

//////////////////////////////////////////////////////////////////////////
//Client Msg

struct tagClient 
{
	char msg[32];
};
typedef CMsgT<tagClient>MsgClient;

//////////////////////////////////////////////////////////////////////////
//Server Msg

#pragma pack()

#pragma once
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <string>
#include <iostream>
#include "Pool.h"
#include "IOArray.h"

using namespace std;
using boost::asio::ip::tcp;
using namespace boost::asio;

class CSession
{
public:
	CSession();
	CSession(boost::asio::io_service& ios);
	CSession* GetThis()
	{
		return this;
	}
	tcp::socket& GetSocket();
	void SetSocket(boost::asio::io_service& ios);
	void OnReadZero();
	void OnRead(const boost::system::error_code& err);
	void OnReadComplete(const boost::system::error_code& err);

	void OnWrite(const boost::system::error_code& err);

	bool SendMsg(LPVOID pClient,BYTE*data,int len);

	//virtual third
	virtual void OnRecvMsg(LPVOID pClient,BYTE*data,int len);

protected:
private:
	tcp::socket *m_socket;
	//boost::asio::streambuf m_recvbuf;
	//boost::asio::streambuf m_sendbuf;
	CMsg			m_msgRead;
	CMsg			m_msgSend;
	////object_pool 分配的静态内存区域，不适合动态内存
	//CIOArray<CMsg>	m_ioArray;
};

typedef CSession* LPSession;

class CServer
{
public:
	CServer();
	~CServer(void);
	CServer(UINT port,UINT maxOLNums);
public:
	bool Startup(UINT port);
	void Stop();
public:

	void Accept();
	void HandleAccept(LPSession pSession,const boost::system::error_code& error);

	//
	bool BroadCastMsg(LPVOID pClient,BYTE*data,int len);

	//LPSession	AllocalSession();
	//void		ReleaseSession(LPSession pSession);

public:
private:
	boost::asio::io_service m_ios;
	tcp::acceptor			*m_Acceptor;
private:
	//CPool<CSession>	m_SessionPool;
	CPoolEx<CSession>	m_SessionPool;
	UINT			m_uMaxOLNums;
};
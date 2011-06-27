#include "StdAfx.h"
#include "Server.h"
//#include <algorithm>
//////////////////////////////////////////////////////////////////////////
//CSession

CSession::CSession()
{

}

CSession::CSession(boost::asio::io_service& ios)
:m_socket(new tcp::socket(ios))
{
}

tcp::socket& CSession::GetSocket()
{
	return *m_socket;
}

void CSession::SetSocket(boost::asio::io_service& ios)
{
	m_socket = new tcp::socket(ios);
}

void CSession::OnReadZero()
{
	//async_read(
	//	m_socket, 
	//	boost::asio::buffer(m_msgRead.data,sizeof(CMsgHead)),
	//	boost::bind(&CSession::OnRead, this,boost::asio::placeholders::error));

	m_socket->async_read_some(
		boost::asio::buffer(m_msgRead.data,sizeof(CMsgHead)),
		boost::bind(&CSession::OnRead, this,boost::asio::placeholders::error));
}

void CSession::OnRead(const boost::system::error_code& err)
{
	if(!err)
	{
		m_socket->async_read_some(
			boost::asio::buffer(m_msgRead.data,sizeof(CMsgHead)),
			boost::bind(&CSession::OnRead, this,boost::asio::placeholders::error));

	}
	else
	{
		cout << "read end, close session" << endl;
		delete this;
	}
}

void CSession::OnReadComplete(const boost::system::error_code& err)
{
	if(!err)
	{

		printf_s("msg read complete:%s\n",m_msgRead.data);

		//outline
		OnRecvMsg(this,(BYTE*)&m_msgRead,sizeof(CMsgHead)+m_msgRead.GetMsgHead()->len);

		m_socket->async_read_some(
			boost::asio::buffer(m_msgRead.data,sizeof(CMsgHead)),
			boost::bind(&CSession::OnRead, this,boost::asio::placeholders::error));

	}
	else
	{
		cout << "read end, close session" << endl;
		delete this;
	}
}

void CSession::OnWrite(const boost::system::error_code& err)
{
	//if(!err)
	//{
	//	boost::asio::async_write(
	//		m_socket, 
	//		boost::asio::buffer(m_msgSend.data,m_msgSend.GetMsgHead()->len+sizeof(CMsgHead)),
	//		boost::bind(&CSession::OnWrite, this,boost::asio::placeholders::error));
	//}
	//else
	//{
	//	delete this;
	//}
}


void CSession::OnRecvMsg(LPVOID pClient,BYTE*data,int len)
{

}

bool CSession::SendMsg(LPVOID pClient,BYTE*data,int len)
{
	return true;
}

//////////////////////////////////////////////////////////////////////////
//CServer
CServer::CServer()
{

}

CServer::~CServer(void)
{
	delete m_Acceptor;
	m_Acceptor = NULL;
}

CServer::CServer(UINT port,UINT maxOLNums)
:m_uMaxOLNums(maxOLNums)
{
	m_Acceptor = new tcp::acceptor(m_ios,tcp::endpoint(tcp::v4(),port))	;
}

bool CServer::Startup(UINT port)
{
	Accept();

	m_ios.run();
	
	return true;
}

void CServer::Stop()
{
	m_ios.stop();
}


void CServer::Accept()
{
	LPSession p = m_SessionPool.Allocal();

	if (!p)
	{
		return;
	}

	p->SetSocket(m_ios);

	m_Acceptor->async_accept(p->GetSocket(),
		boost::bind(&CServer::HandleAccept, this, p,boost::asio::placeholders::error));
}

void CServer::HandleAccept(LPSession pSession,const boost::system::error_code& error)
{
	if(!error)
	{
		cout << "accept connection ..." << endl;
		pSession->OnReadZero();
		Accept();
	}
	else
	{
		m_SessionPool.Release(pSession);
	}
}


bool CServer::BroadCastMsg(LPVOID pClient,BYTE*data,int len)
{
	return true;
}
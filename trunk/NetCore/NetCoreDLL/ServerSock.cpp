#include "StdAfx.h"
#include ".\serversock.h"

namespace YYNetSDK
{
	namespace ServerSock
	{
		CServerSock::CServerSock(void)
		{
		}

		CServerSock::~CServerSock(void)
		{
		}

		bool CServerSock::Start(int port)
		{
			WSAData wsaData;
			if(WSAStartup(MAKEWORD(2,2),&wsaData)!=0)
			{
				return false;
			}

			m_port = port;

			sockaddr_in addr;
			addr.sin_family = AF_INET;
			addr.sin_port = htons(port);
			addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

			m_sockSvr = socket(AF_INET,SOCK_STREAM,0);

			int sz = sizeof(addr);

			if (bind(m_sockSvr,(sockaddr*)&addr,sz) == SOCKET_ERROR)
			{
				return false;
			}

			m_running = true;

			DWORD dwThreadID;
			HANDLE h = CreateThread(NULL,NULL,AcceptFunc,this,NULL,&dwThreadID);
			h = CreateThread(NULL,NULL,SendFunc,this,NULL,&dwThreadID);
			h = CreateThread(NULL,NULL,RecvFunc,this,NULL,&dwThreadID);

			return true;
		}

		void CServerSock::Close()
		{
			m_running = false;

			m_recvList.Destory();
			m_sendList.Destory();

			closesocket(m_sockSvr);
		}

		DWORD CServerSock::AcceptFunc(LPVOID param)
		{
			CServerSock *pThis = (CServerSock*)param;

			sockaddr_in addr;
			addr.sin_family = AF_INET;
			int sz = sizeof(addr);

			while (pThis->m_running)
			{
				SOCKET sock = accept(pThis->m_sockSvr,(sockaddr*)&addr,&sz);
				if (sock == SOCKET_ERROR)
				{
					break;
				}
				pThis->m_clientList.push_back(sock);
			}

			return 0;
		}

		void CServerSock::SendMsg(CMsg msg)
		{
			m_sendList.PushMsg(msg);
		}

		DWORD CServerSock::SendFunc(LPVOID param)
		{
			//broadcast

			CServerSock *pThis = (CServerSock*)param;
			while (pThis->m_running)
			{
				CMsg tmp = pThis->m_sendList.PopMsg();
				for (ClientSocketListIt iter = pThis->m_clientList.begin();iter!=pThis->m_clientList.end();++iter)
				{
					int len = send(*iter,(char*)&tmp,tmp.GetMsgHead().len + sizeof(CMsgHead),0);
					if (len != tmp.GetMsgHead().len + sizeof(CMsgHead))
					{
						break;
					}
				}
			}
			return 0;
		}
		
		DWORD CServerSock::RecvFunc(LPVOID param)
		{
			CServerSock *pThis = (CServerSock*)param;
			while (pThis->m_running)
			{
				//可采用select、iocp等模型接收数据,此阻塞模型不适合
				//详见iocpSvr
			}
			return 0;
		}
	}
}

#include "StdAfx.h"
#include ".\clientsock.h"

namespace YYNetSDK
{
	namespace ClientSock
	{
		CClientSock::CClientSock(void)
		{

		}

		CClientSock::~CClientSock(void)
		{
		}

		bool CClientSock::Connect(char*ip,int port)
		{
			WSAData wsaData;
			if (WSAStartup(MAKEWORD(2,2),&wsaData) != 0)
			{
				return false;
			}

			sockaddr_in addr;
			addr.sin_family = AF_INET;
			addr.sin_addr.S_un.S_addr = inet_addr(ip);
			addr.sin_port = htons(port);

			m_sock = socket(AF_INET,SOCK_STREAM,0);

			int sz = sizeof(addr);

			if (connect(m_sock,(sockaddr*)&addr,sz) == SOCKET_ERROR)
			{
				return false;
			}

			DWORD dwThreadID;
			HANDLE h = CreateThread(NULL,NULL,SendFunc,this,NULL,&dwThreadID);
			if (!h)
			{
				return false;
			}
			h = CreateThread(NULL,NULL,RecvFunc,this,NULL,&dwThreadID);
			if (!h)
			{
				return false;
			}
			return true;
		}

		void CClientSock::Close()
		{
			m_running = false;

			m_recvList.Destory();
			m_sendList.Destory();

			closesocket(m_sock);
		}

		DWORD CClientSock::SendFunc(LPVOID param)
		{
			CClientSock *pThis = (CClientSock*)param;

			while (pThis->m_running)
			{
				CMsg tmp = pThis->m_sendList.PopMsg();

				int len = send(pThis->m_sock,(char*)&tmp,tmp.GetMsgHead().len + sizeof(CMsgHead),0);

				if (len != tmp.GetMsgHead().len + sizeof(CMsgHead))
				{
					break;
				}

				continue;
			}

			return 0;
		}

		DWORD CClientSock::RecvFunc(LPVOID param)
		{
			CClientSock *pThis = (CClientSock*)param;

			while (pThis->m_running)
			{
				CMsg tmp;

				int len = recv(pThis->m_sock,(char*)&tmp,sizeof(CMsgHead),0);
				if (len != sizeof(CMsgHead))
				{
					break;
				}

				int dstlen = tmp.GetMsgHead().len;

				if (dstlen == 0)
				{
					pThis->m_recvList.PushMsg(tmp);
					continue;
				}

				len = recv(pThis->m_sock,(char*)(&tmp+sizeof(CMsgHead)),dstlen,0);

				if (len != dstlen)
				{
					break;
				}

				pThis->m_recvList.PushMsg(tmp);

				continue;
			}
			return 0;
		}

		CMsg CClientSock::GetMsg()
		{
			return m_recvList.PopMsg();
		}

		void CClientSock::SendMsg(CMsg msg)
		{
			m_sendList.PushMsg(msg);
		}	
	}
}


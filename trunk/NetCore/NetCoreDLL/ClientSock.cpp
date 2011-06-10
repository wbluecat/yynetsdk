#include "StdAfx.h"
#include ".\clientsock.h"

namespace YYNetSDK
{
	namespace ClientSock
	{
		bool DisableNagle(SOCKET pSock)
		{
			// 禁用Nagle算法
			char bNagleValue = 1;
			if(SOCKET_ERROR == setsockopt(pSock,IPPROTO_TCP,TCP_NODELAY,(char*)&bNagleValue,sizeof(bNagleValue)))
			{
				return false;
			}
			// 设置缓冲区
			int nBufferSize = 0;//NET_BUFFER_SIZE;
			if(SOCKET_ERROR == setsockopt(pSock,SOL_SOCKET,SO_SNDBUF,(char*)&nBufferSize,sizeof(nBufferSize)))
			{
				return false;
			}
			nBufferSize = 0;//NET_BUFFER_SIZE;
			if(SOCKET_ERROR == setsockopt(pSock,SOL_SOCKET,SO_RCVBUF,(char*)&nBufferSize,sizeof(nBufferSize)))
			{
				return false;
			}

			nBufferSize = 0;
			if(SOCKET_ERROR == setsockopt(pSock,SOL_SOCKET,SO_RCVTIMEO,(char*)&nBufferSize,sizeof(nBufferSize)))
			{
				return false;
			}

			if(SOCKET_ERROR == setsockopt(pSock,SOL_SOCKET,SO_SNDTIMEO,(char*)&nBufferSize,sizeof(nBufferSize)))
			{
				return false;
			}

			return true;
		}

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

			DisableNagle(m_sock);

			int sz = sizeof(addr);

			if (connect(m_sock,(sockaddr*)&addr,sz) == SOCKET_ERROR)
			{
				return false;
			}

			LINGER lingerStruct;
			lingerStruct.l_onoff = 1;
			lingerStruct.l_linger = 0;
			setsockopt(m_sock,SOL_SOCKET,SO_DONTLINGER,(const char*)(&lingerStruct),sizeof(lingerStruct));

			m_running = true;

			DWORD dwThreadID;
			HANDLE h = CreateThread(NULL,NULL,SendFunc,this,NULL,&dwThreadID);
			if (!h)
			{
				m_running = false;
				return false;
			}
			h = CreateThread(NULL,NULL,RecvFunc,this,NULL,&dwThreadID);
			if (!h)
			{
				m_running = false;
				return false;
			}
			printf("clientSock is running...\n");
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

				if (len < 0)
				{
					printf("remote server close sock\n");
					pThis->Close();
					break;
				}
				else if (len != tmp.GetMsgHead().len + sizeof(CMsgHead))
				{
					printf("send err\n");
					//break;
				}
				else
				{
					printf("send ok\n");
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

				if (len < 0)
				{
					printf("remote server drive to close sock\n");
					break;
				}
				else if (len != sizeof(CMsgHead))
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

				if (len < 0)
				{
					printf("remote server drive to close sock\n");
					break;
				}
				else if (len != dstlen)
				{
					printf("recv err\n");
					continue;
					//break;
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


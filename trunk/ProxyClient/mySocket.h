#pragma once
#include "winsock2.h"
#pragma comment(lib,"ws2_32.lib")

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


class CMySocket
{
public:
	bool Connect(char*ip,int port)
	{
		WSADATA wsadata;
		if (WSAStartup(MAKEWORD(2,2),&wsadata) == SOCKET_ERROR)
		{
			return false;
		}

		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = inet_addr(ip);
		
		int len = sizeof(addr);

		m_sock = socket(AF_INET,SOCK_STREAM,0);

		if (m_sock == INVALID_SOCKET)
		{
			return false;
		}

		DisableNagle(m_sock);

		if (connect(m_sock,(sockaddr*)&addr,len) == SOCKET_ERROR)
		{
			return false;
		}

		DWORD id = 0;
		HANDLE h = CreateThread(NULL,0,SendThread,this,0,&id);
		if (!h)
		{
			return false;
		}

		return true;

	}
	void SendMsg(BYTE*data,int len)
	{
		memcpy(m_buf,data,len);
		m_bufLen = len;
	}
	static DWORD WINAPI SendThread(LPVOID param)
	{
		CMySocket *pThis = (CMySocket*)param;
		while (true)
		{
			if (pThis->m_bufLen>0)
			{
				int len = send(pThis->m_sock,(char*)pThis->m_buf,pThis->m_bufLen,0);
				if(len == pThis->m_bufLen)
				{
					pThis->m_bufLen = 0;
				}
				else
				{
					printf("send err,or svr cut\n");
					break;
				}
			}
			else
			{
				Sleep(100);
			}
		}
		return 0;
	}
protected:
private:
	SOCKET m_sock;
	BYTE m_buf[1024];
	int m_bufLen;
};
#include "StdAfx.h"
#include ".\proxycore.h"

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

	LINGER lingerStruct;
	lingerStruct.l_onoff = 1;
	lingerStruct.l_linger = 0;
	if(SOCKET_ERROR == setsockopt(pSock,SOL_SOCKET,SO_DONTLINGER,(const char*)(&lingerStruct),sizeof(lingerStruct)))
	{
		return false;
	}

	return true;
}



CProxyCore::CProxyCore(void)
{
}

CProxyCore::~CProxyCore(void)
{
}

bool CProxyCore::Start(UINT port,UINT maxOL)
{
	WSAData wsadata;
	if (WSAStartup(MAKEWORD(2,2),&wsadata) != 0)
	{
		return false;
	}

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.S_un.S_addr = /*inet_addr*/(INADDR_ANY);
	//addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);
	int len = sizeof(addr);

	m_listenSock = WSASocket(AF_INET,SOCK_STREAM,0,NULL,0,WSA_FLAG_OVERLAPPED);//socket(AF_INET,SOCK_STREAM,0);

	if (m_listenSock == INVALID_SOCKET)
	{
		return false;
	}

	if (bind(m_listenSock,(sockaddr*)&addr,len)==SOCKET_ERROR)
	{
		printf("%d\n",WSAGetLastError());
		return false;
	}

	if (listen(m_listenSock,5) != 0)
	{
		return false;
	}

	SOCKET sock = socket(AF_INET,SOCK_STREAM,0);

	m_hIOCP = CreateIoCompletionPort((HANDLE)sock,NULL,NULL,NULL);

	if (!m_hIOCP)
	{
		return false;
	}

	closesocket(sock);

	//thread
	HANDLE h = NULL;
	DWORD id = 0;

	SYSTEM_INFO si;
	GetSystemInfo(&si);
	for (int i=0;i<si.dwNumberOfProcessors;i++)
	{
		h = CreateThread(NULL,0,Worker,this,0,&id);
		if (h == NULL)
		{
			return false;
		}
	}

	h = CreateThread(NULL,0,Listen,this,0,&id);
	if (h == NULL)
	{
		return false;
	}

	//init memory pool
	m_clientListFree.clear();
	m_clientListBusy.clear();

	m_bufferListBusy.clear();
	m_bufferListFree.clear();

	//for (int i=0;i<10;i++)
	//{
	//	CClientContext*pClient = new CClientContext();
	//	m_clientListFree.push_back(pClient);

	//	CIOBuffer*pBuffer = new CIOBuffer();
	//	m_bufferListFree.push_back(pBuffer);
	//}


	m_port = port;
	m_maxOnlineUser = maxOL;

	m_svrStopEvent = CreateEvent(NULL,TRUE,FALSE,"");

	return true;

}

bool CProxyCore::Stop()
{
	SetEvent(m_svrStopEvent);

	return true;
}

DWORD WINAPI CProxyCore::Listen(LPVOID param)
{

	CProxyCore *pThis = (CProxyCore*)param;
	
	//退出无信号
	while (true)//WaitForSingleObject(pThis->m_svrStopEvent,0) == WAIT_TIMEOUT)
	{

		SOCKET sock = WSAAccept(pThis->m_listenSock,NULL,NULL,NULL,NULL);

		if (sock == INVALID_SOCKET)
		{
			continue;
		}

		DisableNagle(sock);

		//allocate
		CClientContext*pClient = pThis->AllocateClient(sock);
		if(!pClient)
		{
			printf("AllocateClient err\n");
			continue;
		}
		CIOBuffer *pBuffer = pThis->AllocateBuffer(itInit);
		if (!pBuffer)
		{
			printf("AllocateBuffer err\n");
			continue;
		}

		//bind iocp
		HANDLE h = CreateIoCompletionPort((HANDLE)sock,pThis->m_hIOCP,(DWORD)pClient,NULL);

		if (h != pThis->m_hIOCP)
		{
			continue;
		}

		pClient->m_ioDirect = idServer;
		pClient->m_byConnected = 0;

		if (!PostQueuedCompletionStatus(pThis->m_hIOCP,0,(DWORD)pClient,&pBuffer->m_overlapped))
		{
			if (WSAGetLastError() != WSA_IO_PENDING)
			{
				pThis->ReleaseBuffer(pBuffer);
				pThis->ReleaseClient(pClient);
				continue;
			}
		}		
	}

	return 0;
}

DWORD WINAPI CProxyCore::Worker(LPVOID param)
{
	CProxyCore *pThis = (CProxyCore*)param;

	LPOVERLAPPED lpOverlapped = NULL;
	CClientContext *pClient = NULL;
	DWORD dwIOSize = 0;
	CIOBuffer*pBuffer = NULL;
	HANDLE hIOCP = pThis->m_hIOCP;

	//退出无信号
	while (true)//WaitForSingleObject(pThis->m_svrStopEvent,0) == WAIT_TIMEOUT)
	{
		pBuffer = NULL;
		pClient = NULL;

		BOOL bRet = GetQueuedCompletionStatus(hIOCP,&dwIOSize,(LPDWORD)&pClient,&lpOverlapped,INFINITE);
		if (!bRet)
		{
			if(lpOverlapped)
				pBuffer = CONTAINING_RECORD(lpOverlapped,CIOBuffer,m_overlapped);
			pThis->ReleaseBuffer(pBuffer);
			pThis->ReleaseClient(pClient);
			continue;			
		}

		if (pClient && lpOverlapped && pClient->m_sock != INVALID_SOCKET)
		{
			pBuffer = CONTAINING_RECORD(lpOverlapped,CIOBuffer,m_overlapped);

			if (pBuffer)
			{
				pThis->ProcessIO(pClient,pBuffer,dwIOSize);
			}
		}

	}

	return 0;
}

CClientContext* CProxyCore::AllocateClient(SOCKET sock)
{
	YYAutoLock _lock(&m_clientLock); 

	CClientContext *pClient = NULL;

	if (m_clientListFree.empty())
	{
		pClient = new CClientContext();
	}
	else
	{
		pClient = m_clientListFree.front();

		m_clientListFree.pop_front();
	}

	if (pClient)
	{
		pClient->m_byConnected = 0;
		pClient->m_ioarray.Clear();
		pClient->m_sock = sock;
		pClient->m_ioDirect = idHeart;

		if (m_maxOnlineUser+1 <  m_clientListBusy.size())
		{
			//超限人数
			printf("超限人数%d\n",m_clientListBusy.size());
			ReleaseClient(pClient);
			return NULL;
		}

		m_clientListBusy.push_back(pClient);
	}

	return pClient;
}

CIOBuffer* CProxyCore::AllocateBuffer(IOType type)
{

	CIOBuffer *pBuffer = NULL;

	if (m_bufferListFree.empty())
	{
		pBuffer = new CIOBuffer;
	}
	else
	{
		pBuffer = m_bufferListFree.front();

		m_bufferListFree.pop_front();
	}

	if (pBuffer)
	{
		ZeroMemory(pBuffer,sizeof(CIOBuffer));
		pBuffer->m_ioType = type;
	}

	return pBuffer;
}

void CProxyCore::ReleaseClient(CClientContext*pClient)
{
	if (!pClient || pClient->m_sock == INVALID_SOCKET)
	{
		return;
	}

	//lock
	YYAutoLock _lock(&m_clientLock);

	switch (pClient->m_ioDirect)
	{
	case idServer:
		{
			for (int i=0;i<CONNECT_MAX_NUM;i++)
			{
				if (pClient->pClientPair[i])
				{
					closesocket(pClient->pClientPair[i]->m_sock);
				}
			}
		}
		break;
	case idClient:
		{
			if (pClient->pClientPair[0])
			{
				closesocket(pClient->pClientPair[0]->m_sock);
			}
		}
		break;
	}

	ClientContextListIt iter = find(m_clientListBusy.begin(),m_clientListBusy.end(),pClient);
	if (iter!=m_clientListBusy.end())
	{
		m_clientListFree.push_back(pClient);
		m_clientListBusy.erase(iter);
	}

	closesocketex(pClient->m_sock);

	m_maxOnlineUser--;

}

void CProxyCore::ReleaseBuffer(CIOBuffer*pBuffer)
{
	if (!pBuffer)
	{
		return;
	}

	//lock
	YYAutoLock _lock(&m_bufferLock);

	IOBufferListIt iter = find(m_bufferListBusy.begin(),m_bufferListBusy.end(),pBuffer);
	if (iter != m_bufferListBusy.end())
	{
		m_bufferListFree.push_back(pBuffer);
		m_bufferListBusy.erase(iter);
	}

}

void CProxyCore::ProcessIO(CClientContext*pClient,CIOBuffer*pBuffer,DWORD dwSize)
{

	switch (pBuffer->m_ioType)
	{
	case itInit:
		{
			ZeroMemory(&pBuffer->m_overlapped,sizeof(pBuffer->m_overlapped));

			pBuffer->m_ioType = itReadZero;

			if(!PostQueuedCompletionStatus(m_hIOCP,0,(DWORD)pClient,&pBuffer->m_overlapped))
			{
				if (WSAGetLastError() != WSA_IO_PENDING)
				{
					ReleaseBuffer(pBuffer);
					ReleaseClient(pClient);
				}
			}
		}
		break;
	case itReadZero:
		{
			pBuffer->m_ioType = itReadZeroComplete;

			//ZeroMemory(&pBuffer->m_overlapped,sizeof(OVERLAPPED));
			memset(&pBuffer->m_overlapped,0,sizeof(OVERLAPPED));

			//pBuffer->m_used = 0;
			pBuffer->m_wsaBuf.buf = (char*)pBuffer->m_buf;
			pBuffer->m_wsaBuf.len = 0;

			DWORD _dwSize=0,_dwFlag=0;

			BOOL bRet = WSARecv(pClient->m_sock,&pBuffer->m_wsaBuf,1,&_dwSize,&_dwFlag,&pBuffer->m_overlapped,NULL);

			int errCode = WSAGetLastError() ;
			if (bRet == SOCKET_ERROR && errCode != WSA_IO_PENDING)
			{
				printf("%d\n",errCode);
				ReleaseBuffer(pBuffer);
				ReleaseClient(pClient);
			}
		}
		break;
	case itReadZeroComplete:
		{
			ZeroMemory(&pBuffer->m_overlapped,sizeof(pBuffer->m_overlapped));

			pBuffer->m_ioType = itRead;

			pBuffer->m_wsaBuf.buf = (char*)pBuffer->m_buf + pBuffer->m_used;
			pBuffer->m_wsaBuf.len = MSG_MAX_LEN - pBuffer->m_used;

			if (!PostQueuedCompletionStatus(m_hIOCP,0,(DWORD)pClient,&pBuffer->m_overlapped))
			{
				if (WSAGetLastError() != WSA_IO_PENDING)
				{
					ReleaseBuffer(pBuffer);
					ReleaseClient(pClient);
				}
			}
		}
		break;
	case itRead:
		{
			ZeroMemory(&pBuffer->m_overlapped,sizeof(OVERLAPPED));

			pBuffer->m_ioType = itReadComplete;

			pBuffer->m_wsaBuf.buf = (char*)pBuffer->m_buf + pBuffer->m_used;
			pBuffer->m_wsaBuf.len = MSG_MAX_LEN - pBuffer->m_used;

			DWORD _dwSize=0,_dwFlag=0;

			BOOL bRet = WSARecv(pClient->m_sock,&pBuffer->m_wsaBuf,1,&_dwSize,&_dwFlag,&pBuffer->m_overlapped,NULL);

			if (bRet == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
			{
				ReleaseBuffer(pBuffer);
				ReleaseClient(pClient);
			}
		}
		break;
	case itReadComplete:
		{
			pBuffer->m_used += dwSize;

			//add to ioarray
			pClient->m_ioarray.PushBack(pBuffer->m_buf,pBuffer->m_used);
			CMsg *_msg = (CMsg*)pClient->m_ioarray.GetFirst();
			while (_msg)//pClient->m_ioarray.Size() >= sizeof(CMsgHead) && pClient->m_ioarray.Size() <= MSG_MAX_LEN)
			{
				OnHandleMsg(pClient,(BYTE*)_msg,_msg->GetMsgHead().len + sizeof(CMsgHead));

				pClient->m_ioarray.PopFront((BYTE*)_msg,_msg->GetMsgHead().len + sizeof(CMsgHead));

				_msg = (CMsg*)pClient->m_ioarray.GetFirst();
			}

			pBuffer = NULL;

			pBuffer = AllocateBuffer(itReadZero);

			if(!PostQueuedCompletionStatus(m_hIOCP,0,(DWORD)pClient,&pBuffer->m_overlapped))
			{
				if (WSAGetLastError() != WSA_IO_PENDING)
				{
					ReleaseBuffer(pBuffer);
					ReleaseClient(pClient);
				}
			}
		}
		break;
	case itWrite:
		{
			ZeroMemory(&pBuffer->m_overlapped,sizeof(OVERLAPPED));
			pBuffer->m_wsaBuf.buf = (char*)pBuffer->m_buf;
			pBuffer->m_wsaBuf.len = pBuffer->m_used;
			pBuffer->m_ioType = itWriteComplete;
			DWORD dwIOSize = 0,dwFlag = 0;

			if (SOCKET_ERROR == WSASend(pClient->m_sock,&pBuffer->m_wsaBuf,1,&dwIOSize,dwFlag,&pBuffer->m_overlapped,NULL))
			{
				if (WSAGetLastError() != WSA_IO_PENDING)
				{
					ReleaseClient(pClient);
					ReleaseBuffer(pBuffer);
				}
			}
		}
		break;
	case itWriteComplete:
		{
			if(pBuffer->m_used = dwSize)
			{
				ReleaseBuffer(pBuffer);
			}
			else if (dwSize > 0 && dwSize < pBuffer->m_used)
			{
				ZeroMemory(&pBuffer->m_overlapped,sizeof(OVERLAPPED));

				pBuffer->m_ioType = itWrite;

				pBuffer->m_used-=dwSize;
				memmove(pBuffer->m_buf,pBuffer->m_buf+dwSize,pBuffer->m_used);

				if (!PostQueuedCompletionStatus(m_hIOCP,pBuffer->m_used,(DWORD)pClient,&pBuffer->m_overlapped))
				{
					if (WSAGetLastError() != WSA_IO_PENDING)
					{
						ReleaseClient(pClient);
						ReleaseBuffer(pBuffer);
					}
				}
			}
		}
		break;
	default:
		ReleaseBuffer(pBuffer);
		break;
	}
}

void CProxyCore::OnHandleMsg(CClientContext* pClient,BYTE *data,int dataLen)
{
	CMsg * msg = (CMsg*)data;

	if (msg == NULL)
	{
		return;
	}

	switch (msg->GetMsgHead().id)
	{
	case MSG_ID_DISCONNECT:
		{
			return;
		}
		break;
	case MSG_ID_CONNECT:
		{
			MsgConnect *_msg = (MsgConnect*)data;

			if(_msg && CreateSocket(pClient,_msg->msg.ip,_msg->msg.port))
			{
				//notice connect ok
				printf("create pair ok\n");
			}
			else
			{
				//notice connect err
				printf("create pair err\n");
			}
			return;
		}
		break;
	default:
		{
			OnProxyMsg(pClient,msg);

			switch (pClient->m_ioDirect)
			{
			case idServer:
				SendMsg(pClient->GetClientPair(msg->GetMsgHead().channel),msg);
				break;
			case idClient:
				SendMsg(pClient->GetClientPair(0),msg);
				break;
			case idHeart:
				break;
			}
		}
		break;
	}
}

void	CProxyCore::SendMsg(CClientContext*pClient,CMsg*msg)
{
	if (!pClient || pClient->m_sock == INVALID_SOCKET)
	{
		return;
	}

	SendMsg(pClient,(BYTE*)msg,msg->GetMsgHead().len+sizeof(CMsgHead));
}

void	CProxyCore::SendMsg(CClientContext*pClient,BYTE *data,int dataLen)
{
	if (!pClient || pClient->m_sock == INVALID_SOCKET)
	{
		return;
	}

	CIOBuffer *pBuffer = AllocateBuffer(itWrite);

	memcpy(pBuffer->m_buf,data,dataLen);
	pBuffer->m_wsaBuf.buf = (char*)pBuffer->m_buf;
	pBuffer->m_used = dataLen;

	if (!PostQueuedCompletionStatus(m_hIOCP,dataLen,(DWORD)pClient,&pBuffer->m_overlapped))
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			ReleaseClient(pClient);
			ReleaseBuffer(pBuffer);
		}
	}
}

bool	CProxyCore::CreateSocket(CClientContext*pClient,char*ip,char*port)
{
	if (pClient->m_byConnected > CONNECT_MAX_NUM)
	{
		return false;
	}

	int iPort = atoi(port);

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(iPort);
	addr.sin_addr.S_un.S_addr = inet_addr(ip);
	int len = sizeof(addr);

	SOCKET sock = socket(AF_INET,SOCK_STREAM,0);

	if (sock == INVALID_SOCKET)
	{
		return false;
	}

	DisableNagle(sock);

	if (connect(sock,(sockaddr*)&addr,len) == SOCKET_ERROR)
	{
		return false;
	}

	CClientContext * _client = AllocateClient(sock);

	int tmp = pClient->m_byConnected;

	_client->m_ioDirect = idClient;
	_client->pClientPair[0] = pClient;
	_client->m_byConnected = pClient->m_byConnected;
	memcpy(_client->m_ip,ip,strlen(ip));
	memcpy(_client->m_port,ip,strlen(port));

	pClient->pClientPair[tmp] = _client;

	pClient->m_byConnected++;			//update connected nums

	HANDLE h = CreateIoCompletionPort((HANDLE)sock,m_hIOCP,(DWORD)_client,NULL);

	if (h != m_hIOCP)
	{
		return false;
	}

	CIOBuffer * pBuffer = AllocateBuffer(itInit);

	if (!PostQueuedCompletionStatus(m_hIOCP,0,(DWORD)_client,&pBuffer->m_overlapped))
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			ReleaseBuffer(pBuffer);
			ReleaseClient(_client);
			return false;
		}
	}

	return true;
}

#include "StdAfx.h"
#include ".\iocpsvr.h"
#include <algorithm>

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

namespace YYNetSDK
{
	namespace IOCPServer
	{
		CIOCPSvr::CIOCPSvr(void)
		{
		}

		CIOCPSvr::~CIOCPSvr(void)
		{
		}

		bool CIOCPSvr::Stop()
		{
			m_bSvrRunning = false;

			printf("iocpsvr will stop...\n");
			return true;
		}

		bool CIOCPSvr::Start(int port,int maxOLClient,bool sendOrder/*=false*/,bool readOrder/*=false*/)
		{
			//init net
			WSAData wsaData;
			if (WSAStartup(MAKEWORD(2,2),&wsaData)!=0)
			{
				printf("%d\n",WSAGetLastError());
				return false;
			}

			//createiocp
			SOCKET sock = socket(AF_INET,SOCK_STREAM,0);
			if (sock == INVALID_SOCKET)
			{
				printf("%d\n",WSAGetLastError());
				return false;
			}

			m_hIOCP = CreateIoCompletionPort((HANDLE)sock,NULL,0,0);
			if (m_hIOCP == NULL)
			{
				printf("%d\n",WSAGetLastError());
				closesocket(sock);
				return false;
			}
			closesocket(sock);

			//create worker
			SYSTEM_INFO si;
			GetSystemInfo(&si);
			int processNum = si.dwNumberOfProcessors;
			if (processNum==1)
			{
				processNum=2;
			}
			HANDLE hThread;
			DWORD dwThreadID;
			for (int i=0;i<processNum;i++)
			{
				hThread = CreateThread(NULL,0,Worker,this,0,&dwThreadID);
				if (!hThread)
				{
					printf("%d\n",WSAGetLastError());
					return false;
				}
			}
			//create listen
			m_SockListen = WSASocket(AF_INET,SOCK_STREAM,0,NULL,0,WSA_FLAG_OVERLAPPED);
			if (m_SockListen == INVALID_SOCKET)
			{
				printf("%d\n",WSAGetLastError());
				return false;
			}
			sockaddr_in addr;
			addr.sin_family = AF_INET;
			addr.sin_port = htons(port);
			addr.sin_addr.s_addr = htonl(INADDR_ANY);

			if (bind(m_SockListen,(sockaddr*)&addr,sizeof(addr))==SOCKET_ERROR)
			{
				printf("%d\n",WSAGetLastError());
				return false;
			}

			if (listen(m_SockListen,5)==SOCKET_ERROR)
			{
				printf("%d\n",WSAGetLastError());
				return false;
			}

			hThread = CreateThread(NULL,0,Listen,this,0,&dwThreadID);
			if (!hThread)
			{
				printf("%d\n",WSAGetLastError());
				return false;
			}

			m_bSvrRunning = true;

			m_bReadOrder = readOrder;
			m_bSendOrder = sendOrder;
			m_iMaxOLUser = maxOLClient;

			printf("IOCPSvr will running...\n");
			return true;

		}

		HANDLE CIOCPSvr::GetIOCPHandle()
		{
			return m_hIOCP;
		}
		
		bool CIOCPSvr::IsSvrRunning()
		{
			return m_bSvrRunning;
		}

		void CIOCPSvr::ReleaseContext(CClientContext*pContext)
		{
			if (!pContext)
			{
				return;
			}

			//out inter
			OnClientClose(pContext);

			if (1)
			{
				YYAutoLock _lock(&m_ContextMapLock);
				ClientContextMapIt it = m_UsedContextMap.find(pContext->m_sock);
				if (it!=m_UsedContextMap.end())
				{
					m_UsedContextMap.erase(it);
					m_iNowOLUser--;
				}

				if (pContext->m_sock != INVALID_SOCKET)
				{
					closesocketex(pContext->m_sock);
				}
			}

			if (1)
			{
				YYAutoLock _lock(&m_ContextLock);
				pContext->m_sock = INVALID_SOCKET;
				pContext->m_IOArray.Clear();
				pContext->m_sendBuf.clear();
				pContext->m_recvBuf.clear();
				pContext->m_iSendSequence = 0;
				pContext->m_iRecvSequence = 0;
				pContext->m_iRecvSequenceCurrent = 0;
				pContext->m_iSendSequenceCurrent = 0;
				m_FreeContextList.push_front(pContext);
			}

		}

		void CIOCPSvr::ReleaseIOBuffer(CIOBuffer*pBuffer)
		{
			if (!pBuffer)
			{
				return;
			}

			if (!pBuffer->ReleaseRef())
			{
				printf("pBuffer reference err =%d\n",pBuffer->GetRef());
				return;
			}

			if (1)
			{
				YYAutoLock _lock(&m_UsedIOBufferLock);

				IOBufferListIt it = find(m_UsedIOBuffer.begin(),m_UsedIOBuffer.end(),pBuffer);
				if (it!=m_UsedIOBuffer.end())
				{
					m_UsedIOBuffer.erase(it);
				}
			}

			if (1)
			{
				YYAutoLock _lock(&m_FreeIOBufferLock);

				m_FreeIOBuffer.push_front(pBuffer);
			}

			//printf("pBuffer释放ok\n");
		}

		CClientContext*	CIOCPSvr::AllocateClient(SOCKET sock)
		{
			CClientContext * pContext = NULL;

			if (1)
			{
				YYAutoLock _lock(&m_ContextLock);		

				if (m_FreeContextList.empty())
				{
					pContext = new CClientContext();
				}
				else
				{
					pContext = m_FreeContextList.front();
					m_FreeContextList.pop_front();
				}

			}

			if (pContext)
			{
				YYAutoLock _lock(&pContext->m_lock);

				pContext->m_sock = sock;
				pContext->m_iSendSequence = 0;
				pContext->m_iRecvSequence = 0;
				pContext->m_iSendSequenceCurrent = 0;
				pContext->m_iRecvSequenceCurrent = 0;
				pContext->m_IOArray.Clear();
				pContext->m_sendBuf.clear();
				pContext->m_recvBuf.clear();
			}

			if (pContext)
			{
				YYAutoLock _lock(&m_ContextMapLock);

				ClientContextMapIt it = m_UsedContextMap.find(sock);

				if (it != m_UsedContextMap.end())
				{
					ReleaseContext(pContext);
					return NULL;
				}

				m_UsedContextMap.insert(ClientContextMap::value_type(sock,pContext));
			}

			return pContext;
		}
		CIOBuffer * CIOCPSvr::AllocateBuffer(IOType type)
		{
			CIOBuffer*pBuffer = NULL;

			if (1)
			{
				YYAutoLock _lock(&m_FreeIOBufferLock);

				if (m_FreeIOBuffer.empty())
				{
					pBuffer = new CIOBuffer();
				}
				else
				{
					pBuffer = m_FreeIOBuffer.front();
					m_FreeIOBuffer.pop_front();
				}
			}

			if (pBuffer)
			{
				memset(pBuffer,0,sizeof(CIOBuffer));
				pBuffer->m_ioType = type;

				pBuffer->AddRef();

				YYAutoLock _lock(&m_UsedIOBufferLock);
				m_UsedIOBuffer.push_front(pBuffer);
			}

			return pBuffer;

		}


		DWORD WINAPI CIOCPSvr::Worker(LPVOID param)
		{
			CIOCPSvr *pThis = (CIOCPSvr*)param;

			HANDLE hIOCP = pThis->GetIOCPHandle();

			DWORD dwIOSize;
			CClientContext*pContext;
			CIOBuffer*pBuffer;
			LPOVERLAPPED lpOverlapped;

			bool bError = false;

			while (!bError)//pThis->IsSvrRunning())
			{
				pBuffer = NULL;
				pContext = NULL;
				BOOL bRet = GetQueuedCompletionStatus(
					hIOCP,
					&dwIOSize,
					(LPDWORD)&pContext,
					&lpOverlapped,
					INFINITE);

				if (!bRet)
				{
					DWORD dwIOError = WSAGetLastError();
					if (dwIOError != WAIT_TIMEOUT)
					{
						printf("Worker errcode=%d\n",dwIOError);
						if (pContext)
						{
							pThis->ReleaseContext(pContext);
						}
						pBuffer = NULL;

						if (lpOverlapped)
						{
							pBuffer = CONTAINING_RECORD(lpOverlapped,CIOBuffer,m_overlapped);
						}
						if (pBuffer)
						{
							pThis->ReleaseIOBuffer(pBuffer);
						}
						continue;
					}

					bError = true;

					if (lpOverlapped)
					{
						pBuffer = CONTAINING_RECORD(lpOverlapped,CIOBuffer,m_overlapped);
					}
					if (pBuffer)
					{
						pThis->ReleaseIOBuffer(pBuffer);
					}

					continue;
				}
				else if(lpOverlapped && pContext)
				{
					pBuffer = CONTAINING_RECORD(lpOverlapped,CIOBuffer,m_overlapped);
					if (pBuffer)
					{
						pThis->ProcessIOMessage(pContext,dwIOSize,pBuffer);
					}
				}

				if (!pContext && !pBuffer && !pThis->IsSvrRunning())
				{
					bError = true;
					break;
				}

				//handle
			}

			return 0;
		}

		DWORD WINAPI CIOCPSvr::Listen(LPVOID param)
		{

			CIOCPSvr *pThis = (CIOCPSvr*)param;

			HANDLE hIOCP = pThis->GetIOCPHandle();

			while (pThis->IsSvrRunning())
			{
				//printf("new coming...\n");

				SOCKET sock = WSAAccept(pThis->m_SockListen,NULL,NULL,NULL,0);

				if (sock == INVALID_SOCKET)
				{
					printf("%d\n",WSAGetLastError());
					continue;
				}

				DisableNagle(sock);

				//bind ClientContext
				CClientContext * pClient = pThis->AllocateClient(sock);

				if (!pClient)
				{
					continue;
				}

				if (NULL == CreateIoCompletionPort((HANDLE)sock,pThis->m_hIOCP,(DWORD)pClient,NULL))
				{
					printf("%d\n",WSAGetLastError());
					continue;
				}

				//bind iobuffer
				CIOBuffer * pBuffer = pThis->AllocateBuffer(itInit);

				if (!pBuffer)
				{
					pThis->ReleaseContext(pClient);
					continue;
				}

				//pBuffer->AddRef();

				if (!PostQueuedCompletionStatus(pThis->m_hIOCP,0,(DWORD)pClient,&pBuffer->m_overlapped))
				{
					if(WSAGetLastError() != ERROR_IO_PENDING)
					{
						pThis->ReleaseContext(pClient);
						pThis->ReleaseIOBuffer(pBuffer);
						continue;
					}
				}
			}

			return 0;
		}

		void CIOCPSvr::ProcessIOMessage(CClientContext*pContext,DWORD dwSize,CIOBuffer*pBuffer)
		{
			//handle net msg

			if (pContext->m_sock == INVALID_SOCKET)
			{
				ReleaseContext(pContext);
				ReleaseIOBuffer(pBuffer);
				return;
			}

			if(!pBuffer)
				return;
			
			switch (pBuffer->m_ioType)
			{
			case itInit:
				OnInitialize(pContext,dwSize,pBuffer);
				break;
			case itReadZero:
				OnReadZero(pContext,dwSize,pBuffer);
				break;
			case itReadZeroComplete:
				OnReadZeroComplete(pContext,dwSize,pBuffer);
				break;
			case itRead:
				OnRead(pContext,dwSize,pBuffer);
				break;
			case itReadComplete:
				OnReadComplete(pContext,dwSize,pBuffer);
				break;
			case itWrite:
				OnWrite(pContext,dwSize,pBuffer);
				break;
			case itWriteComplete:
				OnWriteComplete(pContext,dwSize,pBuffer);
				break;
			default:
				ReleaseIOBuffer(pBuffer);
				break;
			}
		}

		void CIOCPSvr::OnInitialize(CClientContext*pContext,DWORD dwSize,CIOBuffer*pBuffer)
		{
			if (!IsSvrRunning())
			{
				return;
			}

			OnClientConnect(pContext);

			if (!pBuffer)
			{
				pBuffer = AllocateBuffer(itReadZero);
				if (!pBuffer)
				{
					ReleaseContext(pContext);
					return;
				}
				//pBuffer->AddRef();
			}

			pBuffer->m_ioType = itReadZero;
			memset(&pBuffer->m_overlapped,0,sizeof(OVERLAPPED));

			if (!PostQueuedCompletionStatus(m_hIOCP,0,(DWORD)pContext,&pBuffer->m_overlapped))
			{
				if (WSAGetLastError() != ERROR_IO_PENDING)
				{
					ReleaseIOBuffer(pBuffer);
					ReleaseContext(pContext);
					return;
				}
			}
		}

		void CIOCPSvr::OnReadZero(CClientContext*pContext,DWORD dwSize,CIOBuffer*pBuffer)
		{
			DWORD dwIOSize = 0;
			DWORD dwFlags = 0;

			if (!pBuffer)
			{
				pBuffer = AllocateBuffer(itReadZeroComplete);
				if (!pBuffer)
				{
					ReleaseContext(pContext);
					return;
				}
				//pBuffer->AddRef();
			}

			pBuffer->m_ioType = itReadZeroComplete;
			memset(&pBuffer->m_overlapped,0,sizeof(OVERLAPPED));
			pBuffer->SetupReadZero();

			BOOL ret = WSARecv(pContext->m_sock,&pBuffer->m_wsaBuf,1,&dwIOSize,
				&dwFlags,&pBuffer->m_overlapped,NULL);

			int errCode = WSAGetLastError();

			if (SOCKET_ERROR ==  ret && errCode != WSA_IO_PENDING)
			{
				printf("errid=%d\n",errCode);

				ReleaseIOBuffer(pBuffer);
				ReleaseContext(pContext);
				return;
			}

		}

		void CIOCPSvr::OnReadZeroComplete(CClientContext*pContext,DWORD dwSize,CIOBuffer*pBuffer)
		{
			if (!pBuffer)
			{
				pBuffer = AllocateBuffer(itRead);
				if (!pBuffer)
				{
					ReleaseContext(pContext);
					return;
				}
				//pBuffer->AddRef();
			}

			pBuffer->m_ioType = itRead;
			memset(&pBuffer->m_overlapped,0,sizeof(OVERLAPPED));

			pBuffer->SetupRead();

			if (!PostQueuedCompletionStatus(m_hIOCP,0,(DWORD)pContext,&pBuffer->m_overlapped))
			{
				if (WSAGetLastError() != ERROR_IO_PENDING)
				{
					ReleaseContext(pContext);
					ReleaseIOBuffer(pBuffer);
					return;
				}
			}
		}

		void CIOCPSvr::OnRead(CClientContext*pContext,DWORD dwSize,CIOBuffer*pBuffer)
		{
			if (!pBuffer)
			{
				pBuffer = AllocateBuffer(itReadComplete);
				if (!pBuffer)
				{
					ReleaseIOBuffer(pBuffer);
					ReleaseContext(pContext);
					return;
				}
				//pBuffer->AddRef();
			}

			pBuffer->m_ioType = itReadComplete;
			memset(&pBuffer->m_overlapped,0,sizeof(OVERLAPPED));
			pBuffer->SetupRead();

			if (m_bReadOrder)
			{
				//...
			}
			else
			{
				DWORD dwIOSize=0;
				DWORD dwFlags=0;
				if (SOCKET_ERROR == WSARecv(pContext->m_sock,&pBuffer->m_wsaBuf,1,&dwIOSize,
					&dwFlags,&pBuffer->m_overlapped,NULL))
				{
					ReleaseContext(pContext);
					ReleaseIOBuffer(pBuffer);
					return;
				}
			}
		}

		void CIOCPSvr::OnReadComplete(CClientContext*pContext,DWORD dwSize,CIOBuffer*pBuffer)
		{
			if (dwSize == 0 || pBuffer == NULL)
			{
				ReleaseIOBuffer(pBuffer);
				ReleaseContext(pContext);
				return;
			}

			if (m_bReadOrder)
			{
				//...
			}

			bool msgErr = false;

			while (pBuffer)
			{
				pBuffer->m_nUsed += dwSize;

				if (!msgErr)
				{
					//handle msg

					pContext->m_IOArray.PushBack(pBuffer->m_data,pBuffer->m_nUsed);

					CMsg *tMsg = (CMsg*)pContext->m_IOArray.GetFirst();

					while (1)
					{
						if (pContext->m_IOArray.Size() < sizeof(CMsgHead))
						{
							break;
						}
						else if (tMsg->GetMsgHead().len > MAX_MSG_LEN)
						{
							break;
						}
						else if (tMsg->GetMsgHead().len + sizeof(CMsgHead) > pContext->m_IOArray.Size())
						{
							msgErr = true;
							break;
						}
						//post msg
						OnHandleMsg(pContext,(BYTE*)tMsg,tMsg->GetMsgHead().len + sizeof(CMsgHead));
						//erase
						pContext->m_IOArray.PopFront((BYTE*)tMsg,tMsg->GetMsgHead().len + sizeof(CMsgHead));
						//next msg
						tMsg = (CMsg*)pContext->m_IOArray.GetFirst();

					}
				}

				pContext->m_iRecvSequenceCurrent = (pContext->m_iRecvSequenceCurrent+1)%5001;

				ReleaseIOBuffer(pBuffer);
				pBuffer = NULL;

				if (m_bReadOrder)
				{
					//...
				}
			}

			//post zero read

			if (!pBuffer)
			{
				pBuffer = AllocateBuffer(itReadZero);
				if (!pBuffer)
				{
					ReleaseContext(pContext);
					ReleaseIOBuffer(pBuffer);
					return;
				}
				//pBuffer->AddRef();
			}

			pBuffer->m_ioType = itReadZero;
			memset(&pBuffer->m_overlapped,0,sizeof(OVERLAPPED));

			if (!PostQueuedCompletionStatus(m_hIOCP,0,(DWORD)pContext,&pBuffer->m_overlapped))
			{
				int errCode = WSAGetLastError();
				if ( errCode!= ERROR_IO_PENDING)
				{
					printf("getlasterr code=%d\n",errCode);
					ReleaseContext(pContext);
					ReleaseIOBuffer(pBuffer);
					return;
				}
			}
		}

		void CIOCPSvr::OnWrite(CClientContext*pContext,DWORD dwSize,CIOBuffer*pBuffer)
		{
			if (!pBuffer)
			{
				ReleaseIOBuffer(pBuffer);
				ReleaseContext(pContext);
				return;
			}

			while (pBuffer)
			{
				pBuffer->m_ioType = itWriteComplete;
				memset(&pBuffer->m_overlapped,0,sizeof(OVERLAPPED));
				pBuffer->SetupWrite();

				DWORD dwFlags = 0;
				DWORD dwIOSize = 0;
				if (SOCKET_ERROR == WSASend(pContext->m_sock,&pBuffer->m_wsaBuf,1,&dwIOSize,dwFlags,
					&pBuffer->m_overlapped,
					NULL))
				{
					if (WSAGetLastError() != WSA_IO_PENDING)
					{
						ReleaseIOBuffer(pBuffer);
						pBuffer = NULL;
						pContext->m_iSendSequenceCurrent = (pContext->m_iSendSequenceCurrent+1)%5001;

						if (m_bSendOrder && IsSvrRunning())
						{
							//...
							continue;
						}

						ReleaseContext(pContext);
					}
				}
				else
				{
					pContext->m_iSendSequenceCurrent = (pContext->m_iSendSequenceCurrent+1)%5001;
					ReleaseIOBuffer(pBuffer);
					pBuffer = NULL;
					if (m_bSendOrder && IsSvrRunning())
					{
						//...
					}
				}

			}
			//ReleaseIOBuffer(pBuffer);
		}

		void CIOCPSvr::OnWriteComplete(CClientContext*pContext,DWORD dwSize,CIOBuffer*pBuffer)
		{
			if (!pBuffer || dwSize == 0)
			{
				return;
			}

			if (pBuffer->m_nUsed != dwSize)
			{
				if (dwSize < pBuffer->m_nUsed && dwSize > 0)
				{
					//not send finish

					if (pBuffer->Flush(dwSize))
					{
						pBuffer->m_ioType = itWrite;
						memset(&pBuffer->m_overlapped,0,sizeof(OVERLAPPED));
						
						//PostSend(pContext,pBuffer);
						if (!PostQueuedCompletionStatus(m_hIOCP,pBuffer->m_nUsed,(DWORD)pContext,&pBuffer->m_overlapped))
						{
							if (WSAGetLastError() != WSA_IO_PENDING)
							{
								ReleaseContext(pContext);
								ReleaseIOBuffer(pBuffer);
								return;
							}
						}
					}
				}
			}

			//send finish
			ReleaseIOBuffer(pBuffer);
		}

		void CIOCPSvr::PostRead(CClientContext*pClient,CIOBuffer*pBuffer)
		{
			if (!pClient || pClient->m_sock == INVALID_SOCKET)
			{
				return;
			}

			if (!pBuffer)
			{
				pBuffer = AllocateBuffer(itRead);
				if (!pBuffer)
				{
					ReleaseContext(pClient);
					return;
				}
			}

			pBuffer->m_ioType = itRead;
			memset(&pBuffer->m_overlapped,0,sizeof(OVERLAPPED));

			if (!PostQueuedCompletionStatus(m_hIOCP,0,(DWORD)pClient,&pBuffer->m_overlapped))
			{
				if (WSAGetLastError() != WSA_IO_PENDING)
				{
					ReleaseContext(pClient);
					ReleaseIOBuffer(pBuffer);
					return;
				}
			}
		}

		void CIOCPSvr::PostSend(CClientContext*pClient,CIOBuffer*pBuffer)
		{
			if (!pClient || !pBuffer || !IsSvrRunning() || pClient->m_sock == INVALID_SOCKET)
			{
				ReleaseIOBuffer(pBuffer);
				ReleaseContext(pClient);
				return;
			}

			if (m_bSendOrder)
			{
				//...
			}

			if (!PostQueuedCompletionStatus(m_hIOCP,pBuffer->m_nUsed,(DWORD)pClient,&pBuffer->m_overlapped))
			{
				if (WSAGetLastError() != WSA_IO_PENDING)
				{
					ReleaseIOBuffer(pBuffer);
					ReleaseContext(pClient);
					return;
				}
			}

		}

		//////////////////////////////////////////////////////////////////////////
		//virtual Function

		void CIOCPSvr::OnClientClose(LPVOID pAddr)
		{

		}

		void CIOCPSvr::OnClientConnect(LPVOID pAddr)
		{

		}

		void CIOCPSvr::OnHandleMsg(LPVOID pAddr,BYTE *data,int dataLen)
		{

		}
	}
}



//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
///////////////////////////MicroPopÍøÂç³öÆ·///////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
/*
							v1.0.0.1
							2011-06-07
							MicroPop@126.com
*/

#pragma once
#include "MsgQueue.h"
#include "ClientContext.h"

namespace YYNetSDK
{
	namespace IOCPServer
	{
		class _DLL CIOCPSvr
		{
		public:
			CIOCPSvr(void);
			~CIOCPSvr(void);
		public:
			bool Start(int port,int maxOLClient,bool sendOrder=false,bool readOrder=false);
			bool Stop();
		public:
			static DWORD WINAPI Listen(LPVOID param);
			static DWORD WINAPI Worker(LPVOID param);
		public:
			HANDLE GetIOCPHandle();
			bool	IsSvrRunning();

			CClientContext*	AllocateClient(SOCKET sock);
			CIOBuffer * AllocateBuffer(IOType type);

			void ReleaseContext(CClientContext*pContext);
			void ReleaseIOBuffer(CIOBuffer*pBuffer);

			void ProcessIOMessage(CClientContext*pContext,DWORD dwSize,CIOBuffer*pBuffer);
			void OnInitialize(CClientContext*pContext,DWORD dwSize,CIOBuffer*pBuffer);
			void OnReadZero(CClientContext*pContext,DWORD dwSize,CIOBuffer*pBuffer);
			void OnReadZeroComplete(CClientContext*pContext,DWORD dwSize,CIOBuffer*pBuffer);
			void OnRead(CClientContext*pContext,DWORD dwSize,CIOBuffer*pBuffer);
			void OnReadComplete(CClientContext*pContext,DWORD dwSize,CIOBuffer*pBuffer);
			void OnWrite(CClientContext*pContext,DWORD dwSize,CIOBuffer*pBuffer);
			void OnWriteComplete(CClientContext*pContext,DWORD dwSize,CIOBuffer*pBuffer);

		public:
			//////////////////////////////////////////////////////////////////////////
			//virtual Function
			virtual void OnHandleMsg(LPVOID pAddr,BYTE *data,int dataLen);
			virtual void OnClientClose(LPVOID pAddr);
			virtual void OnClientConnect(LPVOID pAddr);
		public:
			void	PostSend(CClientContext*pClient,CIOBuffer*pBuffer);
			void	PostRead(CClientContext*pClient,CIOBuffer*pBuffer);

			bool	SendMsg(LPVOID pAddr,BYTE *data,int dataLen);
			bool	BroadCastMsg(LPVOID pAddr,BYTE *data,int dataLen);
		private:
			HANDLE m_hIOCP;
			bool m_bSvrRunning;
			SOCKET m_SockListen;
		private:
			//lock
			CLock			m_FreeIOBufferLock;
			CLock			m_UsedIOBufferLock;
			//free busy	/ IOBuffer
			IOBufferList	m_FreeIOBuffer;
			IOBufferList	m_UsedIOBuffer;

			//lock
			CLock		m_ContextLock;
			//free busy / clientcontext
			ClientContextList	m_FreeContextList;	
			//lock
			CLock		m_ContextMapLock;
			//used client map / clientcontext
			ClientContextMap	m_UsedContextMap;

			//order
			bool				m_bReadOrder;
			bool				m_bSendOrder;
			//maxOL
			UINT				m_iMaxOLUser;
			//nowOL
			UINT				m_iNowOLUser;

		};
	}
}


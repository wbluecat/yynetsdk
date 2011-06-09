#pragma once
#include "MsgQueue.h"

typedef list<SOCKET> ClientSocketList;
typedef ClientSocketList::iterator ClientSocketListIt;

namespace YYNetSDK
{
	namespace ServerSock
	{
		class _DLL CServerSock
		{
		public:
			CServerSock(void);
			~CServerSock(void);
		public:
			bool Start(int port);
			void Close();

			void SendMsg(CMsg msg);
		public:
			static DWORD WINAPI AcceptFunc(LPVOID param);
			static DWORD WINAPI SendFunc(LPVOID param);
			static DWORD WINAPI RecvFunc(LPVOID param);

		private:
			SOCKET m_sockSvr;
			int m_port;
			CMsgQueue m_recvList;
			CMsgQueue m_sendList;

			bool m_running;

			ClientSocketList m_clientList;

		};
	}
}

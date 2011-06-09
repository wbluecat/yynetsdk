#pragma once
#include "./MsgQueue.h"

//using namespace YYNetSDK;

namespace YYNetSDK
{
	namespace ClientSock
	{
		class _DLL CClientSock
		{
		public:
			CClientSock(void);
			~CClientSock(void);
		public:
			bool Connect(char*ip,int port);
			void Close();
		public:
			static DWORD WINAPI RecvFunc(LPVOID param);
			static DWORD WINAPI SendFunc(LPVOID param);
		public:
			CMsg GetMsg();
			void SendMsg(CMsg msg);
		private:
			SOCKET m_sock;
			char m_ip[32];
			int m_port;
			CMsgQueue m_recvList;
			CMsgQueue m_sendList;
			bool m_running;
		};
	}
}



#ifndef CSKSERVERSOCKET_H_
#define CSKSERVERSOCKET_H_

#include <string>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

using namespace std;

enum TSKServerSocketStatus 
{
	sssNone,
	//ssStartListeningUp,
	sssListening,
	sssShutdown
};

class ISKSocketCallback;

class CSKServerSocket 
{
	private:

		ISKSocketCallback* m_pSocketCallback;

		int m_nSocket;
		socklen_t m_AddrSize;

		TSKServerSocketStatus m_ServerSocketStatus;

		struct addrinfo m_AddrInfo;
		struct addrinfo* m_AddrRes;

	protected:
		void Close();

	public:
		CSKServerSocket();
		CSKServerSocket( ISKSocketCallback* pSocketCallback);
		virtual ~CSKServerSocket();

		void Listen(string strPort, int nBacklog = 20);
		int Accept(struct sockaddr_storage* pClientAddr);

		void ShutdownServer();
		void ShutdownClient(int nSocket);

		int Send(int nSocket, const unsigned char* pBuf, int nSize);
		//int Recv( unsigned char* pBuf, int nSize);

		TSKServerSocketStatus GetStatus();
};
#endif

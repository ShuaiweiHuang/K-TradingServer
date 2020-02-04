#ifndef CCVSERVERSOCKET_H_
#define CCVSERVERSOCKET_H_

#include <string>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

using namespace std;

enum TCVServerSocketStatus 
{
	sssNone,
	//ssStartListeningUp,
	sssListening,
	sssShutdown
};

class ICVSocketCallback;

class CCVServerSocket 
{
	private:

		ICVSocketCallback* m_pSocketCallback;

		int m_nSocket;
		socklen_t m_AddrSize;

		TCVServerSocketStatus m_ServerSocketStatus;

		struct addrinfo m_AddrInfo;
		struct addrinfo* m_AddrRes;

	protected:
		void Close();

	public:
		CCVServerSocket();
		CCVServerSocket( ICVSocketCallback* pSocketCallback);
		virtual ~CCVServerSocket();

		void Listen(string strPort, int nBacklog = 64);
		int Accept(struct sockaddr_storage* pClientAddr);

		void ShutdownServer();
		void ShutdownClient(int nSocket);

		int Send(int nSocket, const unsigned char* pBuf, int nSize);
		//int Recv( unsigned char* pBuf, int nSize);

		TCVServerSocketStatus GetStatus();
};
#endif

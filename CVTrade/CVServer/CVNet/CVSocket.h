#ifndef CCVSOCKET_H_
#define CCVSOCKET_H_

#include <string>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

using namespace std;

enum TCVSocketStatus 
{
	ssNone,
	ssConnecting,
	ssConnected,
	ssDisconnect
};

class ICVSocketCallback;

class CCVSocket 
{
	private:

		ICVSocketCallback* m_pSocketCallback;

		int m_nSocket;

		TCVSocketStatus m_SocketStatus;

		struct addrinfo m_AddrInfo;
		struct addrinfo* m_AddrRes;

	protected:
		void Close();

	public:
		CCVSocket();
		CCVSocket( ICVSocketCallback* pSocketCallback);
		virtual ~CCVSocket();

		void Connect(string strHost, string strPort);
		void Disconnect();

		int Send(const unsigned char* pBuf, int nSize);
		int Recv( unsigned char* pBuf, int nSize);

		int Recv();

		TCVSocketStatus GetStatus();
};
#endif /* CCVSOCKET_H_ */

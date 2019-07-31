/*
 * CSKSocket.h
 *
 *  Created on: 2015年10月26日
 *      Author: alex
 */

#ifndef CSKSOCKET_H_
#define CSKSOCKET_H_

#include <string>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

using namespace std;

enum TSKSocketStatus 
{
	ssNone,
	ssConnecting,
	ssConnected,
	ssDisconnect
};

class ISKSocketCallback;

class CSKSocket 
{
	private:

		ISKSocketCallback* m_pSocketCallback;

		int m_nSocket;

		TSKSocketStatus m_SocketStatus;

		struct addrinfo m_AddrInfo;
		struct addrinfo* m_AddrRes;

	protected:
		void Close();

	public:
		CSKSocket();
		CSKSocket( ISKSocketCallback* pSocketCallback);
		virtual ~CSKSocket();

		void Connect(string strHost, string strPort);
		void Disconnect();

		int Send(const unsigned char* pBuf, int nSize);
		int Recv( unsigned char* pBuf, int nSize);

		int Recv();

		TSKSocketStatus GetStatus();
};
#endif /* CSKSOCKET_H_ */

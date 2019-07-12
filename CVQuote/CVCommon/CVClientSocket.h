/*
 * CSKSocket.h
 *
 *  Created on: 2015年10月26日
 *      Author: alex
 */

#ifndef CSKCLIENTSOCKET_H_
#define CSKCLIENTSOCKET_H_

#include <string>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "CVWebSocket.h"

using namespace std;

enum TSKClientSocketStatus 
{
	cssNone,
	cssConnecting,
	cssConnected,
	cssDisconnect
};

class ISKClientSocketCallback;

class CSKClientSocket 
{
	private:

		ISKClientSocketCallback* m_pClientSocketCallback;

		int m_nSocket;

		TSKClientSocketStatus m_cssClientSocketStatus;

		struct addrinfo m_AddrInfo;
		struct addrinfo* m_AddrRes;

	protected:
		void Close();

	public:
		client m_cfd;
		CSKClientSocket();
		CSKClientSocket(ISKClientSocketCallback* pClientSocketCallback);
		virtual ~CSKClientSocket();

		void Connect(string strHost, string strPort, int type);
		void Disconnect();

		int Send(const unsigned char* pBuf, int nSize);
		int Recv( unsigned char* pBuf, int nSize);

		int Recv();

		TSKClientSocketStatus GetStatus();
};
#endif /* CSKSOCKET_H_ */

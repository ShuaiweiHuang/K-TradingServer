#include <string.h>
#include <sstream>
#include <stddef.h>
#include <sys/socket.h>
#include <string>
#include <unistd.h>
#include <errno.h>
#include <cstdio>

#include "../CVInterface/ICVSocketCallback.h"
#include "CVServerSocket.h"
#include <iostream>

using namespace std;

CCVServerSocket::CCVServerSocket() 
{
	m_pSocketCallback = NULL;

	m_nSocket = 0;

	m_ServerSocketStatus = sssNone;

	m_AddrRes = NULL;

	memset( &m_AddrInfo, 0 , sizeof( struct addrinfo));
}

CCVServerSocket::CCVServerSocket( ICVSocketCallback* pSocketCallback)
{
	m_pSocketCallback = pSocketCallback;

	m_ServerSocketStatus = sssNone;

	m_nSocket = 0;

	m_AddrSize = sizeof(sockaddr_storage);

	m_AddrRes = NULL;

	memset( &m_AddrInfo, 0 , sizeof( struct addrinfo));
}

CCVServerSocket::~CCVServerSocket() 
{
	// TODO Auto-generated destructor stub
}

void CCVServerSocket::Listen(string strPort, int nBacklog)
{
	//m_SocketStatus = sssStartListeningUp;

	m_AddrInfo.ai_family = AF_INET;
	m_AddrInfo.ai_socktype = SOCK_STREAM;
    m_AddrInfo.ai_flags = AI_PASSIVE;

	getaddrinfo( NULL, strPort.c_str(), &m_AddrInfo, &m_AddrRes);

	// make a socket:
	m_nSocket = socket( m_AddrRes->ai_family, m_AddrRes->ai_socktype, m_AddrRes->ai_protocol);

	// reuse addr!
	int yes = 1;
	setsockopt(m_nSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

	// bind!
	int nRs = bind(m_nSocket, m_AddrRes->ai_addr, m_AddrRes->ai_addrlen);
	if(nRs == -1)
	{
		perror("Bind socket error");
	}

	// listen!
	nRs = listen(m_nSocket, nBacklog);
	if(nRs == -1)
	{
		m_ServerSocketStatus = sssShutdown;
		perror("Listen error");
	}
	else
	{
		m_ServerSocketStatus = sssListening;

		if(m_pSocketCallback)
		{
			m_pSocketCallback->OnListening();
			cout << "Listening ..." << endl;
		}
	}
}

int CCVServerSocket::Accept(struct sockaddr_storage* pClientAddr)
{
	int nClientSocket = -1 ;

	nClientSocket = accept(m_nSocket, (struct sockaddr *)pClientAddr, &m_AddrSize);
	
	return nClientSocket;
}

void CCVServerSocket::Close()
{
	if ( m_ServerSocketStatus == sssListening)
	{
		close( m_nSocket);

		m_ServerSocketStatus = sssShutdown;

		m_nSocket = 0;
	}

	freeaddrinfo(m_AddrRes);
}

void CCVServerSocket::ShutdownServer()
{
	Close();
}

void CCVServerSocket::ShutdownClient(int nSocket)
{
	close(nSocket);
}

TCVServerSocketStatus CCVServerSocket::GetStatus()
{
	return m_ServerSocketStatus;
}

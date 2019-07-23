#include <iostream>
#include <string.h>
#include <sstream>
#include <stddef.h>
#include <sys/socket.h>
#include <string>
#include <unistd.h>
#include <errno.h>
#include <cstdio>

#include "ISKClientSocketCallback.h"
#include "CVClientSocket.h"

#include "../CVQueueNodes.h"
#include "../CVServer.h"
#include "../CVGlobal.h"

using namespace std;


CSKClientSocket::CSKClientSocket() 
{
	m_pClientSocketCallback = NULL;

	m_nSocket = 0;

	m_cssClientSocketStatus = cssNone;

	m_AddrRes = NULL;

	memset( &m_AddrInfo, 0 , sizeof( struct addrinfo));
}

CSKClientSocket::CSKClientSocket(ISKClientSocketCallback* pClientSocketCallback)
{
	m_pClientSocketCallback = pClientSocketCallback;

	m_cssClientSocketStatus = cssNone;

	m_nSocket = 0;

	m_AddrRes = NULL;

	memset( &m_AddrInfo, 0 , sizeof( struct addrinfo));
}

CSKClientSocket::~CSKClientSocket() 
{
}

void CSKClientSocket::Connect(string strHost, string strPara, string strName, int type)
{
	if(type == CONNECT_TCP)
	{
		m_cssClientSocketStatus = cssConnecting;

		m_AddrInfo.ai_family = AF_INET;
		m_AddrInfo.ai_socktype = SOCK_STREAM;


		getaddrinfo( strHost.c_str(), strPara.c_str(), &m_AddrInfo, &m_AddrRes);

		m_nSocket = socket( m_AddrRes->ai_family, m_AddrRes->ai_socktype, m_AddrRes->ai_protocol);

		int nRs = connect( m_nSocket, m_AddrRes->ai_addr, m_AddrRes->ai_addrlen);

		if ( nRs == 0)
		{
			m_cssClientSocketStatus = cssConnected;

			if ( m_pClientSocketCallback)
			{
				m_pClientSocketCallback->OnConnect();
			}
		}
		else
		{
			fprintf(stderr, "[%s][%d]", strerror(errno), errno);

			m_cssClientSocketStatus = cssDisconnect;

			if ( m_pClientSocketCallback)
			{
				m_pClientSocketCallback->OnDisconnect();
			}
		}
	}
	else if(type == CONNECT_WEBSOCK)
	{
		m_pClientSocketCallback->OnConnect();
	}
	else
	{
		fprintf(stderr, "CONNECT_TYPE_FAIL\n");
	}
}

void CSKClientSocket::Close()
{
	if ( m_cssClientSocketStatus == cssConnected)
	{
		shutdown(m_nSocket, SHUT_RDWR);

		close( m_nSocket);

		fprintf(stderr, "CLOSE_CLIENT_SOCKET:%d", m_nSocket);

		m_cssClientSocketStatus = cssDisconnect;

		m_nSocket = 0;
	}

	if(m_AddrRes)
	{
		freeaddrinfo(m_AddrRes);
		m_AddrRes = NULL;
	}
}

void CSKClientSocket::Disconnect()
{
	Close();
}

TSKClientSocketStatus CSKClientSocket::GetStatus()
{
	return m_cssClientSocketStatus;
}

int CSKClientSocket::Send(const unsigned char* pBuf, int nSize)
{
	if ( m_cssClientSocketStatus == cssConnected)
	{
		int nSend =  send( m_nSocket, pBuf, nSize, 0);

		if ( nSend == 0)
		{
			Close();
		}
		else if ( nSend == -1)
		{
			switch ( errno)
			{
			default: ;
			}
		}

		return nSend;
	}
	return 0;
}

int CSKClientSocket::Recv( unsigned char* pBuf, int nSize)
{
	if ( m_cssClientSocketStatus == cssConnected)
	{
		int nRecv =  recv( m_nSocket, pBuf, nSize, 0);

		if ( nRecv == 0)
		{
			Close();
		}
		else if ( nRecv == -1)
		{
			switch ( errno)
			{
			case 0:
			default:;
			}

			return nRecv;
		}

		return nRecv;
	}

	return 0;
}

int CSKClientSocket::Recv()
{
	unsigned char caBuf[1024];

	try
	{


	}
	catch(...)
	{

	}

	return 0;
}

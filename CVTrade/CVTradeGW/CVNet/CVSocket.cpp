#include <string.h>
#include <sstream>
#include <stddef.h>
#include <sys/socket.h>
#include <string>
#include <unistd.h>
#include <errno.h>

#include "../CVInterface/ICVSocketCallback.h"
#include "CVSocket.h"

#include   <iostream>

using namespace std;

CSKSocket::CSKSocket() 
{
	m_pSocketCallback = NULL;

	m_nSocket = 0;

	m_SocketStatus = ssNone;

	m_AddrRes = NULL;

	memset( &m_AddrInfo, 0 , sizeof( struct addrinfo));
}

CSKSocket::CSKSocket( ISKSocketCallback* pSocketCallback)
{
	m_pSocketCallback = pSocketCallback;

	m_SocketStatus = ssNone;

	m_nSocket = 0;

	m_AddrRes = NULL;

	memset( &m_AddrInfo, 0 , sizeof( struct addrinfo));
}

CSKSocket::~CSKSocket() 
{
	// TODO Auto-generated destructor stub
}

void CSKSocket::Connect(string strHost, string strPort)
{
	m_SocketStatus = ssConnecting;

	m_AddrInfo.ai_family = AF_INET;
	m_AddrInfo.ai_socktype = SOCK_STREAM;

	/*
	stringstream ss;

	ss << nPort;

	string strPort = ss.str();
	*/

	getaddrinfo( strHost.c_str(), strPort.c_str(), &m_AddrInfo, &m_AddrRes);

	// make a socket:
	m_nSocket = socket( m_AddrRes->ai_family, m_AddrRes->ai_socktype, m_AddrRes->ai_protocol);

	// connect!
	int nRs = connect( m_nSocket, m_AddrRes->ai_addr, m_AddrRes->ai_addrlen);

	if ( nRs == 0)
	{
		m_SocketStatus = ssConnected;

		if ( m_pSocketCallback)
		{
			m_pSocketCallback->OnConnect();
		}
	}
	else
	{
		m_SocketStatus = ssDisconnect;

		if ( m_pSocketCallback)
		{
			m_pSocketCallback->OnDisconnect();
		}
	}
}

void CSKSocket::Close()
{
	if ( m_SocketStatus == ssConnected)
	{
		shutdown(m_nSocket, SHUT_RDWR);
		close( m_nSocket);

		m_SocketStatus = ssDisconnect;

		m_nSocket = 0;
	}

	if(m_AddrRes)
	{
		freeaddrinfo(m_AddrRes);
		m_AddrRes = NULL;
	}
}

void CSKSocket::Disconnect()
{
	Close();
}

TSKSocketStatus CSKSocket::GetStatus()
{
	return m_SocketStatus;
}

int CSKSocket::Send(const unsigned char* pBuf, int nSize)
{
	if ( m_SocketStatus == ssConnected)
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

int CSKSocket::Recv( unsigned char* pBuf, int nSize)
{
	if ( m_SocketStatus == ssConnected)
	{
		int nRecv =  recv( m_nSocket, pBuf, nSize, 0);

		if ( nRecv == 0)
		{
			//Close();
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

int CSKSocket::Recv()
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

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
		string uri = strHost + strPara;
		try {
			m_cfd.set_access_channels(websocketpp::log::alevel::all);
			m_cfd.clear_access_channels(websocketpp::log::alevel::frame_payload);
			m_cfd.set_error_channels(websocketpp::log::elevel::all);

			m_cfd.init_asio();
			if(strName == "BITMEX")
				m_cfd.set_message_handler(&CB_Message_Bitmex);
			else if(strName == "BINANCE")
				m_cfd.set_message_handler(&CB_Message_Binance);

			m_cfd.set_tls_init_handler(std::bind(&CB_TLS_Init, strHost.c_str(), ::_1));

			websocketpp::lib::error_code errcode;

			client::connection_ptr con = m_cfd.get_connection(uri, errcode);

			if (errcode) {
				cout << "could not create connection because: " << errcode.message() << endl;
				exit(-1);
			}

			m_cfd.connect(con);
			m_cfd.get_alog().write(websocketpp::log::alevel::app, "Connecting to " + uri);
		} catch (websocketpp::exception const & ecp) {
			cout << ecp.what() << endl;
		}
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

context_ptr CSKClientSocket::CB_TLS_Init(const char * hostname, websocketpp::connection_hdl) {
    context_ptr ctx = websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);
    return ctx;
}


void CSKClientSocket::CB_Message_Bitmex(websocketpp::connection_hdl con, client::message_ptr msg) {
#if DEBUG
        printf("[on_message_bitmex]\n");
#endif
        static char netmsg[BUFFERSIZE];
        static char timemsg[9];

        string str = msg->get_payload();
        string price_str, size_str, side_str, time_str, symbol_str;
        json jtable = json::parse(str.c_str());
        static CSKClients* pClients = CSKClients::GetInstance();
        if(pClients == NULL)
                throw "GET_CLIENTS_ERROR";
        for(int i=0 ; i<jtable["data"].size() ; i++)
        {
                memset(netmsg, 0, BUFFERSIZE);
                memset(timemsg, 0, 8);
                static int tick_count=0;
                time_str   = jtable["data"][i]["timestamp"];
                symbol_str = jtable["data"][i]["symbol"];
                price_str  = to_string(jtable["data"][i]["price"]);
                size_str   = to_string(jtable["data"][i]["size"]);
                sprintf(timemsg, "%.2s%.2s%.2s%.2s", time_str.c_str()+11, time_str.c_str()+14, time_str.c_str()+17, time_str.c_str()+20);
                sprintf(netmsg, "01_ID=%s.BMEX,Time=%s,C=%s,V=%s,TC=%d,EPID=%s,",
                        symbol_str.c_str(), timemsg, price_str.c_str(), size_str.c_str(), tick_count++, pClients->m_strEPIDNum.c_str());
                int msglen = strlen(netmsg);
#ifdef DEBUG
                cout << setw(4) << jtable << endl;
                cout << netmsg << endl;
#endif
                netmsg[strlen(netmsg)] = GTA_TAIL_BYTE_1;
                netmsg[strlen(netmsg)] = GTA_TAIL_BYTE_2;

                vector<shared_ptr<CSKClient> >::iterator iter = pClients->m_vClient.begin();

                while(iter != pClients->m_vClient.end())
                {
                        CSKClient* pClient = (*iter).get();
                        if(pClient->GetStatus() == csOffline && (*iter).unique()) {
                                iter++;
                                continue;
                        }
                        pClient->SendAll(NULL, netmsg, strlen(netmsg));
                        iter++;
                }

        }
}

void CSKClientSocket::CB_Message_Binance(websocketpp::connection_hdl, client::message_ptr msg) {
#ifdef DEBUG
        printf("[on_message_binance]\n");
#endif
        static char netmsg[BUFFERSIZE];
        static char timemsg[9];

        string str = msg->get_payload();
        string price_str, size_str, side_str, time_str, symbol_str;
        json jtable = json::parse(str.c_str());

        static CSKClients* pClients = CSKClients::GetInstance();
        if(pClients == NULL)
                throw "GET_CLIENTS_ERROR";

        memset(netmsg, 0, BUFFERSIZE);
        memset(timemsg, 0, 8);

        static int tick_count_binance=0;
        time_str   = "00000000";
        symbol_str = to_string(jtable["s"]);
        symbol_str.erase(remove(symbol_str.begin(), symbol_str.end(), '\"'), symbol_str.end());
        price_str  = to_string(jtable["p"]);
        price_str.erase(remove(price_str.begin(), price_str.end(), '\"'), price_str.end());
        size_str   = to_string(jtable["q"]);
        size_str.erase(remove(size_str.begin(), size_str.end(), '\"'), size_str.end());
        int size_int = stof(size_str) * SCALE_VOL_BINANCE;
        size_str = to_string(size_int);

        sprintf(netmsg, "01_ID=%s.BINANCE,Time=%s,C=%s,V=%s,TC=%d,EPID=%s,",
                symbol_str.c_str(), time_str.c_str(), price_str.c_str(), size_str.c_str(), tick_count_binance++, pClients->m_strEPIDNum.c_str());

        int msglen = strlen(netmsg);
        netmsg[strlen(netmsg)] = GTA_TAIL_BYTE_1;
        netmsg[strlen(netmsg)] = GTA_TAIL_BYTE_2;

#if DEBUG
                cout << setw(4) << jtable << endl;
                cout << netmsg << endl;
#endif
        vector<shared_ptr<CSKClient> >::iterator iter = pClients->m_vClient.begin();

        while(iter != pClients->m_vClient.end())
        {
                CSKClient* pClient = (*iter).get();
                if(pClient->GetStatus() == csOffline && (*iter).unique()) {
                        iter++;
                        continue;
                }
                pClient->SendAll(NULL, netmsg, strlen(netmsg));
                iter++;
        }
}


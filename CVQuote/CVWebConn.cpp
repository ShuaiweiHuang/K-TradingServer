#include <iostream>
#include <unistd.h>
#include <ctime>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <sys/time.h>
#include <fstream>

#include "CVWebConns.h"
#include "CVWebConn.h"
#include "CVGlobal.h"
#include "Include/CVTSFormat.h"
#include "CVQueueNodes.h"

using namespace std;

extern void FprintfStderrLog(const char* pCause, int nError, int nData, const char* pFile = NULL, int nLine = 0, 
			     unsigned char* pMessage1 = NULL, int nMessage1Length = 0, unsigned char* pMessage2 = NULL, int nMessage2Length = 0);

CSKServer::CSKServer(string strWeb, string strQstr, string strName, TSKRequestMarket rmRequestMarket)
{
	m_shpClient = NULL;
	m_pHeartbeat = NULL;
	m_pRequest= NULL;

	m_strWeb = strWeb;
	m_strQstr = strQstr;
	m_strName = strName;

	m_ssServerStatus = ssNone;

	m_rmRequestMarket = rmRequestMarket;
	pthread_mutex_init(&m_pmtxServerStatusLock, NULL);

	try
	{
		m_pClientSocket = new CSKClientSocket(this);
		m_pClientSocket->Connect( m_strWeb, m_strQstr, m_strName, CONNECT_WEBSOCK);//start
	}
	catch (exception& e)
	{
		FprintfStderrLog("SERVER_NEW_SOCKET_ERROR", -1, 0, NULL, 0, (unsigned char*)e.what(), strlen(e.what()));
	}
}

CSKServer::~CSKServer() 
{
	m_shpClient = NULL;

	if(m_pClientSocket)
	{
		m_pClientSocket->Disconnect();

		delete m_pClientSocket;
		m_pClientSocket = NULL;
	}

	if(m_pHeartbeat)
	{
		delete m_pHeartbeat;
		m_pHeartbeat = NULL;
	}

	if(m_pRequest)
	{
		delete m_pRequest;
		m_pRequest = NULL;
	}
	pthread_mutex_destroy(&m_pmtxServerStatusLock);
}

void* CSKServer::Run()
{
	sprintf(m_caPthread_ID, "%020lu", Self());

        CSKClients* pClients = NULL;
        try
        {
                pClients = CSKClients::GetInstance();

                if(pClients == NULL)
                        throw "GET_CLIENTS_ERROR";
        }
        catch(const char* pErrorMessage)
        {
                FprintfStderrLog(pErrorMessage, -1, 0, __FILE__, __LINE__);
                return NULL;
        }
        try
        {
                m_pHeartbeat = new CSKHeartbeat(this);
                m_pHeartbeat->SetTimeInterval(HEARTBEAT_INTERVAL_WEB);
                m_pHeartbeat->Start();
		m_heartbeat_count = 0;
        }
        catch (exception& e)
        {
                FprintfStderrLog("NEW_HEARTBEAT_ERROR", -1, 0, __FILE__, __LINE__, (unsigned char*)m_caPthread_ID, sizeof(m_caPthread_ID), (unsigned char*)e.what(), strlen(e.what()));
                return NULL;
        }
        try
        {
		m_pClientSocket->m_cfd.run();
		printf("Websocket disconnect: %s.\n", m_strName.c_str());
		exit(-1);
	}
        catch (exception& e)
        {
                return NULL;
        }


 	return NULL;
}

void CSKServer::OnConnect()
{

	if(m_ssServerStatus == ssNone)
	{
		try {
			m_pClientSocket->m_cfd.set_access_channels(websocketpp::log::alevel::all);
			m_pClientSocket->m_cfd.clear_access_channels(websocketpp::log::alevel::frame_payload|websocketpp::log::alevel::frame_header|websocketpp::log::alevel::control);
			m_pClientSocket->m_cfd.set_error_channels(websocketpp::log::elevel::all);

			m_pClientSocket->m_cfd.init_asio();

			if(m_strName == "BITMEX") {
				m_pClientSocket->m_cfd.set_message_handler(&OnData_Bitmex);

			}
			else if(m_strName == "BINANCE") {
				m_pClientSocket->m_cfd.set_message_handler(&OnData_Binance);
			}
			string uri = m_strWeb + m_strQstr;

			m_pClientSocket->m_cfd.set_tls_init_handler(std::bind(&CB_TLS_Init, m_strWeb.c_str(), ::_1));

			websocketpp::lib::error_code errcode;

			m_pClientSocket->m_conn = m_pClientSocket->m_cfd.get_connection(uri, errcode);

			if (errcode) {
				cout << "could not create connection because: " << errcode.message() << endl;
				exit(-1);
			}

			m_pClientSocket->m_cfd.connect(m_pClientSocket->m_conn);
			m_pClientSocket->m_cfd.get_alog().write(websocketpp::log::alevel::app, "Connecting to " + uri);

		}  catch (websocketpp::exception const & ecp) {
			cout << ecp.what() << endl;
		}

		Start();
	}
	else if(m_ssServerStatus == ssReconnecting)
	{
		FprintfStderrLog("RECONNECT_SUCCESS", 0, 0, NULL, 0, (unsigned char*)m_caPthread_ID, sizeof(m_caPthread_ID));

		if(m_pHeartbeat)
		{
			m_pHeartbeat->TriggerGetReplyEvent();//reset heartbeat
		}
		else
		{
			FprintfStderrLog("HEARTBEAT_NULL_ERROR", -1, 0, __FILE__, __LINE__, (unsigned char*)m_caPthread_ID, sizeof(m_caPthread_ID));
		}
	}
	else
	{
		FprintfStderrLog("SERVER_STATUS_ERROR", -1, m_ssServerStatus, __FILE__, __LINE__, (unsigned char*)m_caPthread_ID, sizeof(m_caPthread_ID));
	}
}

void CSKServer::OnDisconnect()
{
	sleep(5);

	m_pClientSocket->Connect( m_strWeb, m_strQstr, m_strName, CONNECT_WEBSOCK);//start & reset heartbeat
}

void CSKServer::OnData_Bitmex(websocketpp::connection_hdl con, client::message_ptr msg)
{
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

	string strname = "BITMEX";
	static CSKServer* pServer = CSKServers::GetInstance()->GetServerByName(strname);
	pServer->m_heartbeat_count = 0;

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
		netmsg[strlen(netmsg)] = GTA_TAIL_BYTE_1;
		netmsg[strlen(netmsg)] = GTA_TAIL_BYTE_2;
		CSKQueueDAO* pQueueDAO = CSKQueueDAOs::GetInstance()->GetDAO();
		pServer->m_pHeartbeat->TriggerTerminateEvent();
		assert(pClients);
		pQueueDAO->SendData(netmsg, strlen(netmsg));
#ifdef DEBUG
		cout << setw(4) << jtable << endl;
		cout << netmsg << endl;
#endif
	}

}

void CSKServer::OnData_Binance(websocketpp::connection_hdl con, client::message_ptr msg)
{
#ifdef DEBUG
	printf("[on_message_binance]\n");
#endif
	static char netmsg[BUFFERSIZE];
	static char timemsg[9];

	string str = msg->get_payload();
	string price_str, size_str, side_str, time_str, symbol_str;
	json jtable = json::parse(str.c_str());

	static CSKClients* pClients = CSKClients::GetInstance();
	static int tick_count_binance=0;

	if(pClients == NULL)
		throw "GET_CLIENTS_ERROR";

	memset(netmsg, 0, BUFFERSIZE);
	memset(timemsg, 0, 8);

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
	CSKQueueDAO* pQueueDAO = CSKQueueDAOs::GetInstance()->GetDAO();
	assert(pClients);
	pQueueDAO->SendData(netmsg, strlen(netmsg));
#if DEBUG
	cout << setw(4) << jtable << endl;
	cout << netmsg << endl;
#endif

}

void CSKServer::OnHeartbeatLost()
{
}

void CSKServer::OnHeartbeatRequest()
{
	if(m_strName == "BITMEX") {
		if(m_heartbeat_count <= HTBT_COUNT_LIMIT) {
			auto msg = m_pClientSocket->m_conn->send("ping");
			cout << m_strName << " send \"PING\" and response: "<< msg.message() << endl;
			FprintfStderrLog("HEARTBEAT REQUEST", -1, 0, NULL, 0,  NULL, 0);
			if(msg.message() != "SUCCESS")
				exit(-1);
			m_heartbeat_count++;
		}
		else
			exit(-1);
	}
}

void CSKServer::OnHeartbeatError(int nData, const char* pErrorMessage)
{
}

void CSKServer::OnData(unsigned char* pBuf, int nSize)
{
}

void CSKServer::OnRequest()
{
}

void CSKServer::OnRequestError(int nData, const char* pErrorMessage)
{
}

bool CSKServer::RecvAll(const char* pWhat, unsigned char* pBuf, int nToRecv)
{
	return false;
}

bool CSKServer::SendAll(const char* pWhat, const unsigned char* pBuf, int nToSend)
{
	return false;
}

void CSKServer::ReconnectSocket()
{
	sleep(5);

	if(m_pClientSocket)
	{
		m_pClientSocket->Disconnect();

		m_pClientSocket->Connect( m_strWeb, m_strQstr, m_strName, CONNECT_TCP);//start & reset heartbeat
	}
}

void CSKServer::SetCallback(shared_ptr<CSKClient>& shpClient)
{
	m_shpClient = shpClient;
}

void CSKServer::SetRequestMessage(unsigned char* pRequestMessage, int nRequestMessageLength)
{
}

void CSKServer::SetStatus(TSKServerStatus ssServerStatus)
{
	pthread_mutex_lock(&m_pmtxServerStatusLock);

	m_ssServerStatus = ssServerStatus;

	pthread_mutex_unlock(&m_pmtxServerStatusLock);
}

TSKServerStatus CSKServer::GetStatus()
{
	return m_ssServerStatus;
}

context_ptr CSKServer::CB_TLS_Init(const char * hostname, websocketpp::connection_hdl) {
    context_ptr ctx = websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);
    return ctx;
}


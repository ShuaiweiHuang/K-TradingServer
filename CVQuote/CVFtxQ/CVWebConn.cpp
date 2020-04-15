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

CCVServer::CCVServer(string strWeb, string strQstr, string strName, TCVRequestMarket rmRequestMarket)
{
	m_shpClient = NULL;
	m_pHeartbeat = NULL;
	m_FTX_enable = false;
	m_strWeb = strWeb;
	m_strQstr = strQstr;
	m_strName = strName;
	m_ssServerStatus = ssNone;

	m_rmRequestMarket = rmRequestMarket;
	pthread_mutex_init(&m_pmtxServerStatusLock, NULL);

	try
	{
		m_pClientSocket = new CCVClientSocket(this);
		m_pClientSocket->Connect( m_strWeb, m_strQstr, m_strName, CONNECT_WEBSOCK);//start
	}
	catch (exception& e)
	{
		FprintfStderrLog("SERVER_NEW_SOCKET_ERROR", -1, 0, NULL, 0, (unsigned char*)e.what(), strlen(e.what()));
	}
}

CCVServer::~CCVServer() 
{
	m_shpClient = NULL;
	if(m_pClientSocket)
	{
		SetStatus(ssBreakdown);
		delete m_pClientSocket;
		m_pClientSocket = NULL;
	}

	if(m_pHeartbeat)
	{
		delete m_pHeartbeat;
		m_pHeartbeat = NULL;
	}

	pthread_mutex_destroy(&m_pmtxServerStatusLock);
}

void* CCVServer::Run()
{
	sprintf(m_caPthread_ID, "%020lu", Self());

	CCVClients* pClients = NULL;
	try
	{
		pClients = CCVClients::GetInstance();

		if(pClients == NULL)
			throw "GET_CLIENTS_ERROR";
	}
	catch(const char* pErrorMessage)
	{
		FprintfStderrLog(pErrorMessage, -1, 0, __FILE__, __LINE__);
		return NULL;
	}
	while(m_ssServerStatus != ssBreakdown)
	{
		try
		{
			SetStatus(ssNone);
			m_cfd.run();
			FprintfStderrLog("CFD error", -1, 0, NULL, 0,  NULL, 0);
			SetStatus(ssBreakdown);
			break;
		}
		catch (exception& e)
		{
			break;;
		}
	}
 	return NULL;
}

void CCVServer::OnConnect()
{
	unsigned char msg[BUFFERSIZE];

	if(m_ssServerStatus == ssNone)
	{
		try {
			m_cfd.set_access_channels(websocketpp::log::alevel::all);
			m_cfd.clear_access_channels(websocketpp::log::alevel::frame_payload|websocketpp::log::alevel::frame_header|websocketpp::log::alevel::control);
			m_cfd.set_error_channels(websocketpp::log::elevel::all);

			m_cfd.init_asio();

			try
			{
				m_pHeartbeat = new CCVHeartbeat(this);
				memset(msg, BUFFERSIZE, 0);
				sprintf((char*)msg, "add heartbeat in %s service.", m_strName.c_str());
				FprintfStderrLog("NEW_HEARTBEAT_CREATE", -1, 0, __FILE__, __LINE__, msg, strlen((char*)msg));
				m_heartbeat_count = 0;
			}
			catch (exception& e)
			{
				FprintfStderrLog("NEW_HEARTBEAT_ERROR", -1, 0, __FILE__, __LINE__,
					(unsigned char*)m_caPthread_ID, sizeof(m_caPthread_ID), (unsigned char*)e.what(), strlen(e.what()));
				SetStatus(ssBreakdown);
				return;
			}

			if(m_strName == "FTX") {
				sprintf((char*)msg, "set timer to %d sec.", HEARTBEAT_INTERVAL_SEC);
				FprintfStderrLog("HEARTBEAT_TIMER_CONFIG", -1, 0, __FILE__, __LINE__, msg, strlen((char*)msg));
				m_pHeartbeat->SetTimeInterval(HEARTBEAT_INTERVAL_FTX);
				m_cfd.set_message_handler(bind(&OnData_FTX,&m_cfd,::_1,::_2));

			}
			else {
				FprintfStderrLog("Exchange config error", -1, 0);	
			}
			string uri = m_strWeb + m_strQstr;

			m_cfd.set_tls_init_handler(std::bind(&CB_TLS_Init, m_strWeb.c_str(), ::_1));

			websocketpp::lib::error_code errcode;

			m_conn = m_cfd.get_connection(uri, errcode);

			if (errcode) {
				cout << "could not create connection because: " << errcode.message() << endl;
				SetStatus(ssBreakdown);
				return;
			}

			m_cfd.connect(m_conn);
			m_cfd.get_alog().write(websocketpp::log::alevel::app, "Connecting to " + uri);

		}  catch (websocketpp::exception const & ecp) {
			cout << ecp.what() << endl;
		}

		Start();
	}
	else if(m_ssServerStatus == ssReconnecting)
	{
		string uri = m_strWeb + m_strQstr;

		m_cfd.set_tls_init_handler(std::bind(&CB_TLS_Init, m_strWeb.c_str(), ::_1));

		websocketpp::lib::error_code errcode;

		m_conn = m_cfd.get_connection(uri, errcode);

		if (errcode) {
			cout << "could not create connection because: " << errcode.message() << endl;
			SetStatus(ssBreakdown);
			return;
		}

		m_cfd.connect(m_conn);
		m_cfd.get_alog().write(websocketpp::log::alevel::app, "Connecting to " + uri);

		if(m_pHeartbeat)
		{
			m_pHeartbeat->TriggerGetReplyEvent();//reset heartbeat
		}
		else
		{
			FprintfStderrLog("HEARTBEAT_NULL_ERROR", -1, 0, __FILE__, __LINE__, (unsigned char*)m_caPthread_ID, sizeof(m_caPthread_ID));
		}
		SetStatus(ssNone);
		FprintfStderrLog("RECONNECT_SUCCESS", 0, 0, NULL, 0, (unsigned char*)m_caPthread_ID, sizeof(m_caPthread_ID));
	}
	else
	{
		FprintfStderrLog("SERVER_STATUS_ERROR", -1, m_ssServerStatus, __FILE__, __LINE__, (unsigned char*)m_caPthread_ID, sizeof(m_caPthread_ID));
	}
}

void CCVServer::OnDisconnect()
{
	sleep(5);

	m_pClientSocket->Connect( m_strWeb, m_strQstr, m_strName, CONNECT_WEBSOCK);//start & reset heartbeat
}

void CCVServer::OnData_FTX(client* c, websocketpp::connection_hdl con, client::message_ptr msg)
{
#ifdef DEBUG
	printf("[on_message_FTX]\n");
#endif
	static char netmsg[BUFFERSIZE];
	static char timemsg[9];
	static char epochmsg[20];

	string str = msg->get_payload();
	string time_str, symbol_str, size_str;
	json jtable = json::parse(str.c_str());
	static CCVClients* pClients = CCVClients::GetInstance();
	tm tm_struct;

	if(pClients == NULL)
		throw "GET_CLIENTS_ERROR";

	string name_str = "FTX";
	static CCVServer* pServer = CCVServers::GetInstance()->GetServerByName(name_str);
	pServer->m_heartbeat_count = 0;
	pServer->m_pHeartbeat->TriggerGetReplyEvent();
#if 1
	symbol_str = jtable["market"];
	for(int i=0 ; i<jtable["data"].size() ; i++)
	{ 
		static int tick_count=0;

		memset(netmsg, 0, BUFFERSIZE);
		memset(timemsg, 0, 8);
		memset(epochmsg, 0, 20);

		time_str = jtable["data"][i]["time"].dump();
		time_str = time_str.substr(1, time_str.length()-2);
		size_str = jtable["data"][i]["size"].dump();
		size_str = to_string(atof(size_str.c_str())*10000);
		if(jtable["data"][i]["price"].dump() == "null")
			continue;

		sprintf(epochmsg, "%.10s %.2s:%.2s:%.2s", time_str.c_str(), time_str.c_str()+11, time_str.c_str()+14, time_str.c_str()+17);
		strptime(epochmsg, "%Y-%m-%d %H:%M:%S", &tm_struct);
		tm_struct.tm_isdst = 1;
		size_t epoch = std::mktime(&tm_struct);
		sprintf(epochmsg, "%d.%.3s", epoch, time_str.c_str()+20);
		sprintf(timemsg, "%.2s%.2s%.2s%.2s", time_str.c_str()+11, time_str.c_str()+14, time_str.c_str()+17, time_str.c_str()+20);
		sprintf(netmsg, "01_ID=%s.FTX,ECC.1=%d,Time=%s,C=%s,V=%s,TC=%d,EPID=%s,ECC.2=%d,EPOCH=%s,",
		symbol_str.c_str(), tick_count, timemsg, jtable["data"][i]["price"].dump().c_str(), size_str.c_str(), tick_count, pClients->m_strEPIDNum.c_str(), tick_count, epochmsg);
		tick_count++;

		int msglen = strlen(netmsg);
		netmsg[strlen(netmsg)] = GTA_TAIL_BYTE_1;
		netmsg[strlen(netmsg)] = GTA_TAIL_BYTE_2;
		CCVQueueDAO* pQueueDAO = CCVQueueDAOs::GetInstance()->GetDAO();
		assert(pClients);
		pQueueDAO->SendData(netmsg, strlen(netmsg));
#ifdef DEBUG
		cout << setw(4) << jtable << endl;
		cout << netmsg << endl;
#endif
	}
#endif

}

void CCVServer::OnHeartbeatLost()
{
	FprintfStderrLog("HEARTBEAT LOST", -1, 0, m_strName.c_str(), m_strName.length(),  NULL, 0);
	SetStatus(ssBreakdown);
}

void CCVServer::OnHeartbeatRequest()
{
	FprintfStderrLog("HEARTBEAT REQUEST", -1, 0, m_strName.c_str(), m_strName.length(),  NULL, 0);
	char replymsg[BUFFERSIZE];
	memset(replymsg, 0, BUFFERSIZE);

#if 1
	if(m_heartbeat_count <= HTBT_COUNT_LIMIT)
	{
		if(m_strName == "FTX")
		{
			if(!m_FTX_enable)
			{
				auto j = json::parse("{ \"op\": \"subscribe\", \"channel\": \"trades\", \"market\": \"BTC-PERP\" }");
				auto msg = m_conn->send(j.dump());
				j = json::parse("{ \"op\": \"subscribe\", \"channel\": \"trades\", \"market\": \"ETH-PERP\" }");
				msg = m_conn->send(j.dump());
				m_FTX_enable = true;
				sprintf(replymsg, "%s send subscribe message and response (%s)\n", m_strName.c_str(), msg.message().c_str());
				FprintfStderrLog("initial protocol", -1, 0, replymsg, strlen(replymsg),  NULL, 0);
			}
			else
			{
				auto j = json::parse("{ \"op\": \"subscribe\", \"channel\": \"trades\", \"market\": \"BTC-PERP\" }");
				auto msg = m_conn->send(j.dump());
				j = json::parse("{ \"op\": \"subscribe\", \"channel\": \"trades\", \"market\": \"ETH-PERP\" }");
				msg = m_conn->send(j.dump());
				//auto j = json::parse("{ \"op\": \"ping\"}");
				//auto msg = m_conn->send(j.dump());
				//j = json::parse("{ \"op\": \"subscribe\", \"channel\": \"trades\", \"market\": \"BTC-PERP\" }");
				//msg = m_conn->send(j.dump());
				sprintf(replymsg, "%s send PING message and response (%s)\n", m_strName.c_str(), msg.message().c_str());
				FprintfStderrLog("PING/PONG protocol", -1, 0, replymsg, strlen(replymsg),  NULL, 0);

				if(msg.message() != "Success")
				{
					FprintfStderrLog("Server PING/PONG Fail", -1, 0, m_strName.c_str(), m_strName.length(),  NULL, 0);
					SetStatus(ssBreakdown);
				}
				else
				{
					printf("ping/pong success\n");
					m_pHeartbeat->TriggerGetReplyEvent();
					m_heartbeat_count++;
				}

			}
		}
		else
		{
			auto msg = m_conn->send("ping");
			sprintf(replymsg, "%s send PING message and response (%s)\n", m_strName.c_str(), msg.message().c_str());
			FprintfStderrLog("PING/PONG protocol", -1, 0, replymsg, strlen(replymsg),  NULL, 0);

			if(msg.message() != "SUCCESS" && msg.message() != "Success")
			{
				FprintfStderrLog("Server PING/PONG Fail", -1, 0, m_strName.c_str(), m_strName.length(),  NULL, 0);
				SetStatus(ssBreakdown);
			}
			else
			{
				printf("ping/pong success\n");
				m_pHeartbeat->TriggerGetReplyEvent();
				m_heartbeat_count++;
			}
		}
	}
	else
	{

		FprintfStderrLog("Heartbeat limit exceed", -1, 0, m_strName.c_str(), m_strName.length(),  NULL, 0);
		SetStatus(ssBreakdown);
	}
#endif
}

void CCVServer::OnHeartbeatError(int nData, const char* pErrorMessage)
{
	FprintfStderrLog("HEARTBEAT ERROR", -1, 0, m_strName.c_str(), m_strName.length(),  NULL, 0);
	SetStatus(ssBreakdown);
}

bool CCVServer::RecvAll(const char* pWhat, unsigned char* pBuf, int nToRecv)
{
	return false;
}

bool CCVServer::SendAll(const char* pWhat, const unsigned char* pBuf, int nToSend)
{
	return false;
}

void CCVServer::ReconnectSocket()
{
	if(m_pClientSocket)
	{
		sleep(5);
		SetStatus(ssReconnecting);
		m_pClientSocket->Connect( m_strWeb, m_strQstr, m_strName, CONNECT_WEBSOCK);//start
	}
	else
	{
		printf("m_pClientSocket fail\n");
		SetStatus(ssBreakdown);
	}
}

void CCVServer::SetCallback(shared_ptr<CCVClient>& shpClient)
{
	m_shpClient = shpClient;
}

void CCVServer::SetRequestMessage(unsigned char* pRequestMessage, int nRequestMessageLength)
{
}

void CCVServer::SetStatus(TCVServerStatus ssServerStatus)
{
	pthread_mutex_lock(&m_pmtxServerStatusLock);

	m_ssServerStatus = ssServerStatus;

	pthread_mutex_unlock(&m_pmtxServerStatusLock);
}

TCVServerStatus CCVServer::GetStatus()
{
	return m_ssServerStatus;
}

context_ptr CCVServer::CB_TLS_Init(const char * hostname, websocketpp::connection_hdl) {
	context_ptr ctx = websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12 );
	return ctx;
}

void CCVServer::OnRequest()
{
}

void CCVServer::OnRequestError(int, const char*)
{
}

void CCVServer::OnData(unsigned char*, int)
{
}

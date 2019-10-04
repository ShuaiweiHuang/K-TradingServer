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
	while(1)
	{
		try
		{
			SetStatus(ssNone);
			m_pClientSocket->m_cfd.run();
			SetStatus(ssBreakdown);
			exit(-1);
		}
		catch (exception& e)
		{
			return NULL;
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
			m_pClientSocket->m_cfd.set_access_channels(websocketpp::log::alevel::all);
			m_pClientSocket->m_cfd.clear_access_channels(websocketpp::log::alevel::frame_payload|websocketpp::log::alevel::frame_header|websocketpp::log::alevel::control);
			m_pClientSocket->m_cfd.set_error_channels(websocketpp::log::elevel::all);

			m_pClientSocket->m_cfd.init_asio();

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
				exit(-1);
			}

			if(m_strName == "BITMEX") {
				sprintf((char*)msg, "set timer to %d", HEARTBEAT_INTERVAL_SEC);
				FprintfStderrLog("HEARTBEAT_TIMER_CONFIG", -1, 0, __FILE__, __LINE__, msg, strlen((char*)msg));
				m_pHeartbeat->SetTimeInterval(HEARTBEAT_INTERVAL_SEC);
				m_pClientSocket->m_cfd.set_message_handler(bind(&OnData_Bitmex,&m_pClientSocket->m_cfd,::_1,::_2));
			}
			else if(m_strName == "BITMEX_T") {
				sprintf((char*)msg, "set timer to %d", HEARTBEAT_INTERVAL_SEC);
				FprintfStderrLog("HEARTBEAT_TIMER_CONFIG", -1, 0, __FILE__, __LINE__, msg, strlen((char*)msg));
				m_pHeartbeat->SetTimeInterval(HEARTBEAT_INTERVAL_MIN);
				m_pClientSocket->m_cfd.set_message_handler(bind(&OnData_Bitmex_Test,&m_pClientSocket->m_cfd,::_1,::_2));
			}
			else if(m_strName == "BITMEXLESS") {
				sprintf((char*)msg, "set timer to %d", HEARTBEAT_INTERVAL_MIN);
				FprintfStderrLog("HEARTBEAT_TIMER_CONFIG", -1, 0, __FILE__, __LINE__, msg, strlen((char*)msg));
				m_pHeartbeat->SetTimeInterval(HEARTBEAT_INTERVAL_MIN);
				m_pClientSocket->m_cfd.set_message_handler(bind(&OnData_Bitmex_Less,&m_pClientSocket->m_cfd,::_1,::_2));
			}
			else if(m_strName == "BITMEXINDEX") {
				sprintf((char*)msg, "set timer to %d", HEARTBEAT_INTERVAL_MIN);
				FprintfStderrLog("HEARTBEAT_TIMER_CONFIG", -1, 0, __FILE__, __LINE__, msg, strlen((char*)msg));
				m_pHeartbeat->SetTimeInterval(HEARTBEAT_INTERVAL_MIN);
				m_pClientSocket->m_cfd.set_message_handler(bind(&OnData_Bitmex_Index,&m_pClientSocket->m_cfd,::_1,::_2));
			}
			else if(m_strName == "BITSTAMP") {
				sprintf((char*)msg, "set timer to %d", HEARTBEAT_INTERVAL_SEC);
				FprintfStderrLog("HEARTBEAT_TIMER_CONFIG", -1, 0, __FILE__, __LINE__, msg, strlen((char*)msg));
				m_pHeartbeat->SetTimeInterval(HEARTBEAT_INTERVAL_SEC);
				m_pClientSocket->m_cfd.set_message_handler(bind(&OnData_Bitstamp,&m_pClientSocket->m_cfd,::_1,::_2));
			}
			else if(m_strName == "BINANCE") {
				sprintf((char*)msg, "set timer to %d", HEARTBEAT_INTERVAL_SEC);
				FprintfStderrLog("HEARTBEAT_TIMER_CONFIG", -1, 0, __FILE__, __LINE__, msg, strlen((char*)msg));
				m_pHeartbeat->SetTimeInterval(HEARTBEAT_INTERVAL_SEC);
				m_pClientSocket->m_cfd.set_message_handler(bind(&OnData_Binance,&m_pClientSocket->m_cfd,::_1,::_2));
			}
			else if(m_strName == "BINANCE_FT") {
				sprintf((char*)msg, "set timer to %d", HEARTBEAT_INTERVAL_SEC);
				FprintfStderrLog("HEARTBEAT_TIMER_CONFIG", -1, 0, __FILE__, __LINE__, msg, strlen((char*)msg));
				m_pHeartbeat->SetTimeInterval(HEARTBEAT_INTERVAL_SEC);
				m_pClientSocket->m_cfd.set_message_handler(bind(&OnData_Binance_FT,&m_pClientSocket->m_cfd,::_1,::_2));
			}
			else if(m_strName == "BINANCE_F") {
				sprintf((char*)msg, "set timer to %d", HEARTBEAT_INTERVAL_MIN);
				FprintfStderrLog("HEARTBEAT_TIMER_CONFIG", -1, 0, __FILE__, __LINE__, msg, strlen((char*)msg));
				m_pHeartbeat->SetTimeInterval(HEARTBEAT_INTERVAL_MIN);
				m_pClientSocket->m_cfd.set_message_handler(bind(&OnData_Binance_F,&m_pClientSocket->m_cfd,::_1,::_2));
			}

			else {
				
			}
			string uri = m_strWeb + m_strQstr;

			m_pClientSocket->m_cfd.set_tls_init_handler(std::bind(&CB_TLS_Init, m_strWeb.c_str(), ::_1));

			websocketpp::lib::error_code errcode;

			m_pClientSocket->m_conn = m_pClientSocket->m_cfd.get_connection(uri, errcode);

			if(m_strName == "BITSTAMP") {
				m_pClientSocket->m_conn->send("{\"event\": \"bts:subscribe\",\"data\":{\"channel\": \"[channel_name]\"}");
			}
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
		if(m_strName == "BITMEX") {
			m_pClientSocket->m_cfd.set_message_handler(bind(&OnData_Bitmex,&m_pClientSocket->m_cfd,::_1,::_2));

		}
		else if(m_strName == "BINANCE") {
			m_pClientSocket->m_cfd.set_message_handler(bind(&OnData_Binance,&m_pClientSocket->m_cfd,::_1,::_2));
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

void CCVServer::OnData_Bitmex_Index(client* c, websocketpp::connection_hdl con, client::message_ptr msg)
{
//	printf("[on_message_bitmexIndex]\n");
	static char netmsg[BUFFERSIZE];
	static char timemsg[9];

	string str = msg->get_payload();
	string price_str, size_str, side_str, time_str, symbol_str;
	json jtable = json::parse(str.c_str());
	static CCVClients* pClients = CCVClients::GetInstance();

	if(pClients == NULL)
		throw "GET_CLIENTS_ERROR";

	string strname = "BITMEXINDEX";
	static CCVServer* pServer = CCVServers::GetInstance()->GetServerByName(strname);
	pServer->m_heartbeat_count = 0;
	if(pServer->GetStatus() == ssBreakdown) {
		c->close(con,websocketpp::close::status::normal,"");
		printf("Bitmex breakdown\n");
		exit(-1);
	}
	pServer->m_pHeartbeat->TriggerGetReplyEvent();
	for(int i=0 ; i<jtable["data"].size() ; i++)
	{ 
		memset(netmsg, 0, BUFFERSIZE);
		memset(timemsg, 0, 8);
		static int tick_count=0;
		time_str   = jtable["data"][i]["timestamp"];
		symbol_str = jtable["data"][i]["symbol"];
		price_str  = to_string(static_cast<float>(jtable["data"][i]["lastPrice"]));
		size_str   = "0";//to_string(jtable["data"][i]["size"]);
		if(price_str == "null")
			return;
		sprintf(timemsg, "%.2s%.2s%.2s%.2s", time_str.c_str()+11, time_str.c_str()+14, time_str.c_str()+17, time_str.c_str()+20);
		sprintf(netmsg, "01_ID=%s.BMEX,Time=%s,C=%s,V=%s,TC=%d,EPID=%s,",
			symbol_str.c_str(), timemsg, price_str.c_str(), size_str.c_str(), tick_count++, pClients->m_strEPIDNum.c_str());
		int msglen = strlen(netmsg);
		netmsg[strlen(netmsg)] = GTA_TAIL_BYTE_1;
		netmsg[strlen(netmsg)] = GTA_TAIL_BYTE_2;
		CCVQueueDAO* pQueueDAO = CCVQueueDAOs::GetInstance()->GetDAO();
		assert(pClients);
		pQueueDAO->SendData(netmsg, strlen(netmsg));
//		printf("%s\n", netmsg);
#ifdef DEBUG
		cout << setw(4) << jtable << endl;
		cout << netmsg << endl;
#endif
	}

}


void CCVServer::OnData_Bitmex_Less(client* c, websocketpp::connection_hdl con, client::message_ptr msg)
{
//	printf("[on_message_bitmexLess]\n");
	static char netmsg[BUFFERSIZE];
	static char timemsg[9];

	string str = msg->get_payload();
	string price_str, size_str, side_str, time_str, symbol_str;
	json jtable = json::parse(str.c_str());
	static CCVClients* pClients = CCVClients::GetInstance();

	if(pClients == NULL)
		throw "GET_CLIENTS_ERROR";

	string strname = "BITMEXLESS";
	static CCVServer* pServer = CCVServers::GetInstance()->GetServerByName(strname);
	pServer->m_heartbeat_count = 0;
	if(pServer->GetStatus() == ssBreakdown) {
		c->close(con,websocketpp::close::status::normal,"");
		printf("Bitmex breakdown\n");
		exit(-1);
	}
	pServer->m_pHeartbeat->TriggerGetReplyEvent();
	for(int i=0 ; i<jtable["data"].size() ; i++)
	{ 
		memset(netmsg, 0, BUFFERSIZE);
		memset(timemsg, 0, 8);
		static int tick_count=0;
		time_str   = jtable["data"][i]["timestamp"];
		symbol_str = jtable["data"][i]["symbol"];
		price_str  = to_string(static_cast<float>(jtable["data"][i]["price"]));
		size_str   = to_string(static_cast<int>(jtable["data"][i]["size"]));
		sprintf(timemsg, "%.2s%.2s%.2s%.2s", time_str.c_str()+11, time_str.c_str()+14, time_str.c_str()+17, time_str.c_str()+20);
		sprintf(netmsg, "01_ID=%s.BMEX,Time=%s,C=%s,V=%s,TC=%d,EPID=%s,",
			symbol_str.c_str(), timemsg, price_str.c_str(), size_str.c_str(), tick_count++, pClients->m_strEPIDNum.c_str());
		int msglen = strlen(netmsg);
		netmsg[strlen(netmsg)] = GTA_TAIL_BYTE_1;
		netmsg[strlen(netmsg)] = GTA_TAIL_BYTE_2;
		CCVQueueDAO* pQueueDAO = CCVQueueDAOs::GetInstance()->GetDAO();
		assert(pClients);
		pQueueDAO->SendData(netmsg, strlen(netmsg));
//		printf("%s\n", netmsg);
#ifdef DEBUG
		cout << setw(4) << jtable << endl;
		cout << netmsg << endl;
#endif
	}

}

void CCVServer::OnData_Bitmex_Test(client* c, websocketpp::connection_hdl con, client::message_ptr msg)
{
	//printf("[on_message_bitmex_T]\n");
	static char netmsg[BUFFERSIZE];
	static char timemsg[9];

	string str = msg->get_payload();
	string price_str, size_str, side_str, time_str, symbol_str;
	json jtable = json::parse(str.c_str());
	static CCVClients* pClients = CCVClients::GetInstance();

	if(pClients == NULL)
		throw "GET_CLIENTS_ERROR";

	string strname = "BITMEX_T";
	static CCVServer* pServer = CCVServers::GetInstance()->GetServerByName(strname);
	pServer->m_heartbeat_count = 0;
	pServer->m_pHeartbeat->TriggerGetReplyEvent();
	if(pServer->GetStatus() == ssBreakdown) {
		c->close(con,websocketpp::close::status::normal,"");
		printf("Bitmex breakdown\n");
		exit(-1);
	}

	for(int i=0 ; i<jtable["data"].size() ; i++)
	{ 
		memset(netmsg, 0, BUFFERSIZE);
		memset(timemsg, 0, 8);
		static int tick_count=0;
		time_str   = jtable["data"][i]["timestamp"];
		symbol_str = jtable["data"][i]["symbol"];
		price_str  = to_string(static_cast<float>(jtable["data"][i]["price"]));
		size_str   = to_string(static_cast<int>(jtable["data"][i]["size"]));
		sprintf(timemsg, "%.2s%.2s%.2s%.2s", time_str.c_str()+11, time_str.c_str()+14, time_str.c_str()+17, time_str.c_str()+20);
		sprintf(netmsg, "01_ID=%s.BITMEX_T,Time=%s,C=%s,V=%s,TC=%d,EPID=%s,",
			symbol_str.c_str(), timemsg, price_str.c_str(), size_str.c_str(), tick_count++, pClients->m_strEPIDNum.c_str());
		int msglen = strlen(netmsg);
		netmsg[strlen(netmsg)] = GTA_TAIL_BYTE_1;
		netmsg[strlen(netmsg)] = GTA_TAIL_BYTE_2;
		CCVQueueDAO* pQueueDAO = CCVQueueDAOs::GetInstance()->GetDAO();
		assert(pClients);
		pQueueDAO->SendData(netmsg, strlen(netmsg));
		//printf("%s\n", netmsg);
#ifdef DEBUG
		cout << setw(4) << jtable << endl;
		cout << netmsg << endl;
#endif
	}

}


void CCVServer::OnData_Bitmex(client* c, websocketpp::connection_hdl con, client::message_ptr msg)
{
	printf("[on_message_bitmex]\n");
	static char netmsg[BUFFERSIZE];
	static char timemsg[9];

	string str = msg->get_payload();
	string price_str, size_str, side_str, time_str, symbol_str;
	json jtable = json::parse(str.c_str());
	static CCVClients* pClients = CCVClients::GetInstance();

	if(pClients == NULL)
		throw "GET_CLIENTS_ERROR";

	string strname = "BITMEX";
	static CCVServer* pServer = CCVServers::GetInstance()->GetServerByName(strname);
	pServer->m_heartbeat_count = 0;
	pServer->m_pHeartbeat->TriggerGetReplyEvent();
	if(pServer->GetStatus() == ssBreakdown) {
		c->close(con,websocketpp::close::status::normal,"");
		printf("Bitmex breakdown\n");
		exit(-1);
	}

	for(int i=0 ; i<jtable["data"].size() ; i++)
	{ 
		memset(netmsg, 0, BUFFERSIZE);
		memset(timemsg, 0, 8);
		static int tick_count=0;
		time_str   = jtable["data"][i]["timestamp"];
		symbol_str = jtable["data"][i]["symbol"];
		price_str  = to_string(static_cast<float>(jtable["data"][i]["price"]));
		size_str   = to_string(static_cast<int>(jtable["data"][i]["size"]));
		sprintf(timemsg, "%.2s%.2s%.2s%.2s", time_str.c_str()+11, time_str.c_str()+14, time_str.c_str()+17, time_str.c_str()+20);
		sprintf(netmsg, "01_ID=%s.BMEX,Time=%s,C=%s,V=%s,TC=%d,EPID=%s,",
			symbol_str.c_str(), timemsg, price_str.c_str(), size_str.c_str(), tick_count++, pClients->m_strEPIDNum.c_str());
		int msglen = strlen(netmsg);
		netmsg[strlen(netmsg)] = GTA_TAIL_BYTE_1;
		netmsg[strlen(netmsg)] = GTA_TAIL_BYTE_2;
		CCVQueueDAO* pQueueDAO = CCVQueueDAOs::GetInstance()->GetDAO();
		assert(pClients);
		pQueueDAO->SendData(netmsg, strlen(netmsg));
		//printf("%s\n", netmsg);
#ifdef DEBUG
		cout << setw(4) << jtable << endl;
		cout << netmsg << endl;
#endif
	}

}

void CCVServer::OnData_Bitstamp(client* c, websocketpp::connection_hdl con, client::message_ptr msg)
{
//	printf("[on_message_bitstamp]\n");
	static char netmsg[BUFFERSIZE];
	static char timemsg[9];

	string str = msg->get_payload();
	string price_str, size_str, side_str, time_str, symbol_str;
	json jtable = json::parse(str.c_str());
	static CCVClients* pClients = CCVClients::GetInstance();

	if(pClients == NULL)
		throw "GET_CLIENTS_ERROR";

	string strname = "BITMEX";
	static CCVServer* pServer = CCVServers::GetInstance()->GetServerByName(strname);
	pServer->m_pHeartbeat->TriggerGetReplyEvent();
	if(pServer->GetStatus() == ssBreakdown) {
		c->close(con,websocketpp::close::status::normal,"");
		printf("Bitstamp breakdown\n");
		exit(-1);
	}

	for(int i=0 ; i<jtable["data"].size() ; i++)
	{ 
		memset(netmsg, 0, BUFFERSIZE);
		memset(timemsg, 0, 8);
		static int tick_count=0;
		time_str   = jtable["data"][i]["timestamp"];
		symbol_str = jtable["data"][i]["symbol"];
		price_str  = to_string(static_cast<float>(jtable["data"][i]["price"]));
		size_str   = to_string(static_cast<int>(jtable["data"][i]["size"]));
		sprintf(timemsg, "%.2s%.2s%.2s%.2s", time_str.c_str()+11, time_str.c_str()+14, time_str.c_str()+17, time_str.c_str()+20);
		sprintf(netmsg, "01_ID=%s.BITSTAMP,Time=%s,C=%s,V=%s,TC=%d,EPID=%s,",
			symbol_str.c_str(), timemsg, price_str.c_str(), size_str.c_str(), tick_count++, pClients->m_strEPIDNum.c_str());
		int msglen = strlen(netmsg);
		netmsg[strlen(netmsg)] = GTA_TAIL_BYTE_1;
		netmsg[strlen(netmsg)] = GTA_TAIL_BYTE_2;
		CCVQueueDAO* pQueueDAO = CCVQueueDAOs::GetInstance()->GetDAO();
		assert(pClients);
		pQueueDAO->SendData(netmsg, strlen(netmsg));
		//printf("%s\n", netmsg);
		cout << setw(4) << jtable << endl;
		cout << netmsg << endl;
	}

}
void CCVServer::OnData_Binance(client* c, websocketpp::connection_hdl con, client::message_ptr msg)
{
	//printf("[on_message_binance]\n");
	static char netmsg[BUFFERSIZE];
	static char timemsg[9];
	string str = msg->get_payload();
	string price_str, size_str, side_str, time_str, symbol_str;
	json jtable = json::parse(str.c_str());

	static CCVClients* pClients = CCVClients::GetInstance();
	static int tick_count_binance=0;

	if(pClients == NULL)
		throw "GET_CLIENTS_ERROR";

	string strname = "BINANCE";
	static CCVServer* pServer = CCVServers::GetInstance()->GetServerByName(strname);
	pServer->m_heartbeat_count = 0;
	if(pServer->GetStatus() == ssBreakdown) {
		c->close(con,websocketpp::close::status::normal,"");
		printf("Binance breakdown\n");
		exit(-1);
	}
	pServer->m_pHeartbeat->TriggerGetReplyEvent();

	memset(netmsg, 0, BUFFERSIZE);
	memset(timemsg, 0, 8);

	time_str   = "00000000";
	symbol_str = jtable["s"];
	symbol_str.erase(remove(symbol_str.begin(), symbol_str.end(), '\"'), symbol_str.end());
	price_str  = to_string(static_cast<float>(jtable["p"]));
	price_str.erase(remove(price_str.begin(), price_str.end(), '\"'), price_str.end());
	size_str   = to_string(static_cast<float>(jtable["q"]));
	size_str.erase(remove(size_str.begin(), size_str.end(), '\"'), size_str.end());

	int size_int = stof(size_str) * SCALE_VOL_BINANCE;
	size_str = to_string(size_int);

	sprintf(netmsg, "01_ID=%s.BINANCE,Time=%s,C=%s,V=%s,TC=%d,EPID=%s,",
		symbol_str.c_str(), time_str.c_str(), price_str.c_str(), size_str.c_str(), tick_count_binance++, pClients->m_strEPIDNum.c_str());

	int msglen = strlen(netmsg);
	netmsg[strlen(netmsg)] = GTA_TAIL_BYTE_1;
	netmsg[strlen(netmsg)] = GTA_TAIL_BYTE_2;
	CCVQueueDAO* pQueueDAO = CCVQueueDAOs::GetInstance()->GetDAO();
	assert(pClients);
	pQueueDAO->SendData(netmsg, strlen(netmsg));
	//printf("%s\n", netmsg);
#if DEBUG
	cout << setw(4) << jtable << endl;
	cout << netmsg << endl;
#endif
}
void CCVServer::OnData_Binance_F(client* c, websocketpp::connection_hdl con, client::message_ptr msg)
{
	//printf("[on_message_binance_F]\n");
	static char netmsg[BUFFERSIZE];
	static char timemsg[9];
	string str = msg->get_payload();
	string price_str, size_str, side_str, time_str, symbol_str;
	json jtable = json::parse(str.c_str());
#if 0
	cout << setw(4) << jtable << endl;
	cout << netmsg << endl;
#endif
	static CCVClients* pClients = CCVClients::GetInstance();
	static int tick_count_binance=0;

	if(pClients == NULL)
		throw "GET_CLIENTS_ERROR";

	string strname = "BINANCE_F";
	static CCVServer* pServer = CCVServers::GetInstance()->GetServerByName(strname);
	pServer->m_heartbeat_count = 0;
	if(pServer->GetStatus() == ssBreakdown) {
		c->close(con,websocketpp::close::status::normal,"");
		printf("Binance breakdown\n");
		exit(-1);
	}
	pServer->m_pHeartbeat->TriggerGetReplyEvent();

	memset(netmsg, 0, BUFFERSIZE);
	memset(timemsg, 0, 8);

	time_str   = "00000000";
	symbol_str = jtable["data"]["s"];
	symbol_str.erase(remove(symbol_str.begin(), symbol_str.end(), '\"'), symbol_str.end());
	price_str  = to_string(static_cast<float>(jtable["data"]["p"]));
	price_str.erase(remove(price_str.begin(), price_str.end(), '\"'), price_str.end());
	size_str   = to_string(static_cast<float>(jtable["data"]["q"]));
	size_str.erase(remove(size_str.begin(), size_str.end(), '\"'), size_str.end());

	int size_int = stof(size_str) * SCALE_VOL_BINANCE_F;
	size_str = to_string(size_int);

	sprintf(netmsg, "01_ID=%s.BINANCE_F,Time=%s,C=%s,V=%s,TC=%d,EPID=%s,",
		symbol_str.c_str(), time_str.c_str(), price_str.c_str(), size_str.c_str(), tick_count_binance++, pClients->m_strEPIDNum.c_str());

	int msglen = strlen(netmsg);
	netmsg[strlen(netmsg)] = GTA_TAIL_BYTE_1;
	netmsg[strlen(netmsg)] = GTA_TAIL_BYTE_2;
	CCVQueueDAO* pQueueDAO = CCVQueueDAOs::GetInstance()->GetDAO();
	assert(pClients);
	pQueueDAO->SendData(netmsg, strlen(netmsg));
	//printf("%s\n", netmsg);
}

void CCVServer::OnData_Binance_FT(client* c, websocketpp::connection_hdl con, client::message_ptr msg)
{
	//printf("[on_message_binance_FT]\n");
	static char netmsg[BUFFERSIZE];
	static char timemsg[9];
	string str = msg->get_payload();
	string price_str, size_str, side_str, time_str, symbol_str;
	json jtable = json::parse(str.c_str());
#if 0
	cout << setw(4) << jtable << endl;
	cout << netmsg << endl;
#endif
	static CCVClients* pClients = CCVClients::GetInstance();
	static int tick_count_binance=0;

	if(pClients == NULL)
		throw "GET_CLIENTS_ERROR";

	string strname = "BINANCE_FT";
	static CCVServer* pServer = CCVServers::GetInstance()->GetServerByName(strname);
	pServer->m_heartbeat_count = 0;
	if(pServer->GetStatus() == ssBreakdown) {
		c->close(con,websocketpp::close::status::normal,"");
		printf("Binance breakdown\n");
		exit(-1);
	}
	pServer->m_pHeartbeat->TriggerGetReplyEvent();

	memset(netmsg, 0, BUFFERSIZE);
	memset(timemsg, 0, 8);

	time_str   = "00000000";
	symbol_str = jtable["data"]["s"];
	symbol_str.erase(remove(symbol_str.begin(), symbol_str.end(), '\"'), symbol_str.end());
	price_str  = to_string(static_cast<float>(jtable["data"]["p"]));
	price_str.erase(remove(price_str.begin(), price_str.end(), '\"'), price_str.end());
	size_str   = to_string(static_cast<float>(jtable["data"]["q"]));
	size_str.erase(remove(size_str.begin(), size_str.end(), '\"'), size_str.end());

	int size_int = stof(size_str) * SCALE_VOL_BINANCE_F;
	size_str = to_string(size_int);

	sprintf(netmsg, "01_ID=%s.BINANCE_FT,Time=%s,C=%s,V=%s,TC=%d,EPID=%s,",
		symbol_str.c_str(), time_str.c_str(), price_str.c_str(), size_str.c_str(), tick_count_binance++, pClients->m_strEPIDNum.c_str());

	int msglen = strlen(netmsg);
	netmsg[strlen(netmsg)] = GTA_TAIL_BYTE_1;
	netmsg[strlen(netmsg)] = GTA_TAIL_BYTE_2;
	CCVQueueDAO* pQueueDAO = CCVQueueDAOs::GetInstance()->GetDAO();
	assert(pClients);
	pQueueDAO->SendData(netmsg, strlen(netmsg));
	//printf("%s\n", netmsg);
}

void CCVServer::OnHeartbeatLost()
{
	FprintfStderrLog("HEARTBEAT LOST", -1, 0, m_strName.c_str(), m_strName.length(),  NULL, 0);
	exit(-1);
}

void CCVServer::OnHeartbeatRequest()
{
	FprintfStderrLog("HEARTBEAT REQUEST", -1, 0, m_strName.c_str(), m_strName.length(),  NULL, 0);
	char replymsg[BUFFERSIZE];
	memset(replymsg, 0, BUFFERSIZE);

	if(m_heartbeat_count <= HTBT_COUNT_LIMIT)
	{
		auto msg = m_pClientSocket->m_conn->send("ping");
		sprintf(replymsg, "%s send PING message and response (%s)\n", m_strName.c_str(), msg.message().c_str());
		FprintfStderrLog("PING/PONG protocol", -1, 0, replymsg, strlen(replymsg),  NULL, 0);

		if(msg.message() != "SUCCESS" && msg.message() != "Success")
		{
			FprintfStderrLog("Server PING/PONG Fail", -1, 0, m_strName.c_str(), m_strName.length(),  NULL, 0);
			exit(-1);
		}
		else
		{
			m_heartbeat_count++;
		}
	}
	else
	{
		exit(-1);
	}
}

void CCVServer::OnHeartbeatError(int nData, const char* pErrorMessage)
{
	exit(-1);
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
		exit(-1);
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
#if 0
	try {
	ctx->set_options(boost::asio::ssl::context::default_workarounds |
		 boost::asio::ssl::context::no_sslv2 |
		 boost::asio::ssl::context::no_sslv3 |
		 boost::asio::ssl::context::tlsv12  |
		 boost::asio::ssl::context::single_dh_use);
	} catch (std::exception &e) {
		std::cout << "Error in context pointer: " << e.what() << std::endl;
	}
#endif
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

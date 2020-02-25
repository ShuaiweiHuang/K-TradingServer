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
#ifdef EXIT_VERSION
			exit(-1);
#endif
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

			if(m_strName == "BITMEX") {
				CCVServers::GetInstance()->m_ServerBitmex = this;
				sprintf((char*)msg, "set timer to %d sec.", HEARTBEAT_INTERVAL_SEC);
				FprintfStderrLog("HEARTBEAT_TIMER_CONFIG", -1, 0, __FILE__, __LINE__, msg, strlen((char*)msg));
				m_pHeartbeat->SetTimeInterval(HEARTBEAT_INTERVAL_SEC);
				m_cfd.set_message_handler(bind(&OnData_Bitmex,&m_cfd,::_1,::_2));
			}
			else if(m_strName == "BITMEX_T") {
				sprintf((char*)msg, "set timer to %d sec.", HEARTBEAT_INTERVAL_SEC);
				FprintfStderrLog("HEARTBEAT_TIMER_CONFIG", -1, 0, __FILE__, __LINE__, msg, strlen((char*)msg));
				m_pHeartbeat->SetTimeInterval(HEARTBEAT_INTERVAL_MIN);
				m_cfd.set_message_handler(bind(&OnData_Bitmex_Test,&m_cfd,::_1,::_2));
			}
			else if(m_strName == "BITMEXINDEX") {
				sprintf((char*)msg, "set timer to %d sec.", HEARTBEAT_INTERVAL_MIN);
				FprintfStderrLog("HEARTBEAT_TIMER_CONFIG", -1, 0, __FILE__, __LINE__, msg, strlen((char*)msg));
				m_pHeartbeat->SetTimeInterval(HEARTBEAT_INTERVAL_MIN);
				m_cfd.set_message_handler(bind(&OnData_Bitmex_Index,&m_cfd,::_1,::_2));
			}
			else if(m_strName == "BITMEXFUND") {
				sprintf((char*)msg, "set timer to %d sec.", HEARTBEAT_INTERVAL_MIN/6);
				FprintfStderrLog("HEARTBEAT_TIMER_CONFIG", -1, 0, __FILE__, __LINE__, msg, strlen((char*)msg));
				m_pHeartbeat->SetTimeInterval(HEARTBEAT_INTERVAL_MIN);
				m_cfd.set_message_handler(bind(&OnData_Bitmex_Funding,&m_cfd,::_1,::_2));
			}
			else if(m_strName == "BINANCE") {
				sprintf((char*)msg, "set timer to %d sec.", HEARTBEAT_INTERVAL_SEC);
				FprintfStderrLog("HEARTBEAT_TIMER_CONFIG", -1, 0, __FILE__, __LINE__, msg, strlen((char*)msg));
				m_pHeartbeat->SetTimeInterval(HEARTBEAT_INTERVAL_SEC);
				m_cfd.set_message_handler(bind(&OnData_Binance,&m_cfd,::_1,::_2));
			}
			else if(m_strName == "BINANCE_FT") {
				sprintf((char*)msg, "set timer to %d sec.", HEARTBEAT_INTERVAL_SEC);
				FprintfStderrLog("HEARTBEAT_TIMER_CONFIG", -1, 0, __FILE__, __LINE__, msg, strlen((char*)msg));
				m_pHeartbeat->SetTimeInterval(HEARTBEAT_INTERVAL_SEC);
				m_cfd.set_message_handler(bind(&OnData_Binance_FT,&m_cfd,::_1,::_2));
			}
			else if(m_strName == "BINANCE_F") {
				sprintf((char*)msg, "set timer to %d sec.", HEARTBEAT_INTERVAL_MIN);
				FprintfStderrLog("HEARTBEAT_TIMER_CONFIG", -1, 0, __FILE__, __LINE__, msg, strlen((char*)msg));
				m_pHeartbeat->SetTimeInterval(HEARTBEAT_INTERVAL_MIN);
				m_cfd.set_message_handler(bind(&OnData_Binance_F,&m_cfd,::_1,::_2));
			}
			else if(m_strName == "FTX") {
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
		if(m_strName == "BITMEX") {
			m_cfd.set_message_handler(bind(&OnData_Bitmex,&m_cfd,::_1,::_2));

		}
		else if(m_strName == "BINANCE") {
			m_cfd.set_message_handler(bind(&OnData_Binance,&m_cfd,::_1,::_2));
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

		time_str   = jtable["data"][i]["time"];
		size_str   = "0";

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


void CCVServer::OnData_Bitmex_Index(client* c, websocketpp::connection_hdl con, client::message_ptr msg)
{
#ifdef DEBUG
	printf("[on_message_bitmexIndex]\n");
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

	string name_str = "BITMEXINDEX";
	static CCVServer* pServer = CCVServers::GetInstance()->GetServerByName(name_str);
	pServer->m_heartbeat_count = 0;
	pServer->m_pHeartbeat->TriggerGetReplyEvent();

	for(int i=0 ; i<jtable["data"].size() ; i++)
	{ 
		static int tick_count=0;

		memset(netmsg, 0, BUFFERSIZE);
		memset(timemsg, 0, 8);
		memset(epochmsg, 0, 20);

		time_str   = jtable["data"][i]["timestamp"];
		symbol_str = jtable["data"][i]["symbol"];
		size_str   = "0";

		if(jtable["data"][i]["lastPrice"].dump() == "null")
			continue;

		sprintf(epochmsg, "%.10s %.2s:%.2s:%.2s", time_str.c_str(), time_str.c_str()+11, time_str.c_str()+14, time_str.c_str()+17);
		strptime(epochmsg, "%Y-%m-%d %H:%M:%S", &tm_struct);
		tm_struct.tm_isdst = 1;
		size_t epoch = std::mktime(&tm_struct);
		sprintf(epochmsg, "%d.%.3s", epoch, time_str.c_str()+20);
		sprintf(timemsg, "%.2s%.2s%.2s%.2s", time_str.c_str()+11, time_str.c_str()+14, time_str.c_str()+17, time_str.c_str()+20);
		sprintf(netmsg, "01_ID=%s.BMEX,ECC.1=%d,Time=%s,C=%s,V=%s,TC=%d,EPID=%s,ECC.2=%d,EPOCH=%s,",
			symbol_str.c_str(), tick_count, timemsg, jtable["data"][i]["lastPrice"].dump().c_str(), size_str.c_str(), tick_count, pClients->m_strEPIDNum.c_str(), tick_count, epochmsg);
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

}

void CCVServer::OnData_Bitmex_Funding(client* c, websocketpp::connection_hdl con, client::message_ptr msg)
{
#ifdef DEBUG
	printf("[on_message_bitmex_funding]\n");
#endif
	static char fundmsg[BUFFERSIZE];
	static char timemsg[9];
	static char epochmsg[20];

	string str = msg->get_payload();
	string time_str, symbol_str, size_str;
	json jtable = json::parse(str.c_str());
	static CCVClients* pClients = CCVClients::GetInstance();
	tm tm_struct;

	if(pClients == NULL)
		throw "GET_CLIENTS_ERROR";

	string name_str = "BITMEXFUND";
	static CCVServer* pServer = CCVServers::GetInstance()->GetServerByName(name_str);
	pServer->m_heartbeat_count = 0;
	pServer->m_pHeartbeat->TriggerGetReplyEvent();

	for(int i=0 ; i<jtable["data"].size() ; i++)
	{ 
		static int tick_count=0;

		//if(jtable["data"][i]["symbol"] != "XBTUSD" && jtable["data"][i]["symbol"] != "ETHUSD")
		//	continue;

		memset(fundmsg, 0, BUFFERSIZE);
		memset(timemsg, 0, 8);
		memset(epochmsg, 0, 20);

		time_str   = jtable["data"][i]["timestamp"];
		symbol_str = jtable["data"][i]["symbol"];
		size_str   = "0";
		istringstream science_notation(jtable["data"][i]["fundingRate"].dump());
		science_notation.precision(8);
		double funding_rate;
		science_notation >> funding_rate;
		funding_rate += 1;
		sprintf(epochmsg, "%.10s %.2s:%.2s:%.2s", time_str.c_str(), time_str.c_str()+11, time_str.c_str()+14, time_str.c_str()+17);
		strptime(epochmsg, "%Y-%m-%d %H:%M:%S", &tm_struct);
		tm_struct.tm_isdst = 1;
		size_t epoch = std::mktime(&tm_struct);
		sprintf(epochmsg, "%d.%.3s", epoch, time_str.c_str()+20);
		sprintf(timemsg, "%.2s%.2s%.2s%.2s", time_str.c_str()+11, time_str.c_str()+14, time_str.c_str()+17, time_str.c_str()+20);
		sprintf(fundmsg, "01_ID=%s_FR.BMEX,ECC.1=%d,Time=%s,C=%lf,V=%s,TC=%d,EPID=%s,ECC.2=%d,EPOCH=%s,",
			symbol_str.c_str(), tick_count, timemsg, funding_rate, size_str.c_str(), tick_count, pClients->m_strEPIDNum.c_str(), tick_count, epochmsg);
		printf("%s\n", fundmsg);
		tick_count++;

		int msglen = strlen(fundmsg);
		fundmsg[strlen(fundmsg)] = GTA_TAIL_BYTE_1;
		fundmsg[strlen(fundmsg)] = GTA_TAIL_BYTE_2;
		CCVQueueDAO* pQueueDAO = CCVQueueDAOs::GetInstance()->GetDAO();
		assert(pClients);
		pQueueDAO->SendData(fundmsg, strlen(fundmsg));
#ifdef DEBUG
		cout << setw(4) << jtable << endl;
		cout << fundmsg << endl;
#endif
	}

}

void CCVServer::OnData_Bitmex_Test(client* c, websocketpp::connection_hdl con, client::message_ptr msg)
{
#ifdef DEBUG
	printf("[on_message_bitmex_T]\n");
#endif
	static char netmsg[BUFFERSIZE];
	static char timemsg[9];
	static char epochmsg[20];

	string str = msg->get_payload();
	string time_str, symbol_str;
	json jtable = json::parse(str.c_str());
	static CCVClients* pClients = CCVClients::GetInstance();
	tm tm_struct;

	if(pClients == NULL)
		throw "GET_CLIENTS_ERROR";

	string name_str = "BITMEX_T";
	static CCVServer* pServer = CCVServers::GetInstance()->GetServerByName(name_str);
	pServer->m_heartbeat_count = 0;
	pServer->m_pHeartbeat->TriggerGetReplyEvent();
	for(int i=0 ; i<jtable["data"].size() ; i++)
	{ 
		static int tick_count=0;

		memset(netmsg, 0, BUFFERSIZE);
		memset(timemsg, 0, 8);
		memset(epochmsg, 0, 20);

		time_str   = jtable["data"][i]["timestamp"];
		symbol_str = jtable["data"][i]["symbol"];

		if(jtable["data"][i]["price"].dump() == "null")
			continue;

		sprintf(epochmsg, "%.10s %.2s:%.2s:%.2s", time_str.c_str(), time_str.c_str()+11, time_str.c_str()+14, time_str.c_str()+17);
		strptime(epochmsg, "%Y-%m-%d %H:%M:%S", &tm_struct);
		tm_struct.tm_isdst = 1;
		size_t epoch = std::mktime(&tm_struct);
		sprintf(epochmsg, "%d.%.3s", epoch, time_str.c_str()+20);
		sprintf(timemsg, "%.2s%.2s%.2s%.2s", time_str.c_str()+11, time_str.c_str()+14, time_str.c_str()+17, time_str.c_str()+20);
		sprintf(netmsg, "01_ID=%s.BITMEX_T,ECC.1=%d,Time=%s,C=%s,V=%s,TC=%d,EPID=%s,ECC.2=%d,EPOCH=%s,",
			symbol_str.c_str(), tick_count, timemsg, jtable["data"][i]["price"].dump().c_str(),
		jtable["data"][i]["size"].dump().c_str(), tick_count, pClients->m_strEPIDNum.c_str(), tick_count, epochmsg);
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

}


void CCVServer::OnData_Bitmex(client* c, websocketpp::connection_hdl con, client::message_ptr msg)
{
#ifdef DEBUG
	printf("[on_message_bitmex]\n");
#endif
	static char netmsg[BUFFERSIZE];
	static char timemsg[9];
	static char epochmsg[20];
	

	string data_str = msg->get_payload();
	string time_str, symbol_str, epoch_str;
	string name_str = "BITMEX";
	int vol_count = 0;
	static int aa = 0, bb = 0;
	json jtable = json::parse(data_str.c_str());
	static CCVClients* pClients = CCVClients::GetInstance();
	tm tm_struct;

	if(pClients == NULL)
		throw "GET_CLIENTS_ERROR";

	static CCVServer* pServer = CCVServers::GetInstance()->GetServerByName(name_str);
	pServer->m_heartbeat_count = 0;
	pServer->m_pHeartbeat->TriggerGetReplyEvent();
	static long int xbt_last_price = -1, eth_last_price = -1;
	for(int i=0 ; i<jtable["data"].size() ; i++)
	{ 
		static int tick_count = 0;

		memset(netmsg, 0, BUFFERSIZE);
		memset(timemsg, 0, 9);
		memset(epochmsg, 0, 20);

		vol_count  += atoi(jtable["data"][i]["size"].dump().c_str());
		time_str   = jtable["data"][i]["timestamp"];
		symbol_str = jtable["data"][i]["symbol"];

		if(jtable["data"][i]["price"].dump() == "null")
			continue;


		if(i<jtable["data"].size()-1 && jtable["data"][i]["price"].dump() == jtable["data"][i+1]["price"].dump())
		{
			//printf("keanu merge: %s:%d\n", jtable["data"][i]["price"].dump().c_str(), vol_count);
			continue;
		}
		else
		{
			if(symbol_str == "ETHUSD")
			{

				if(eth_last_price < 0)
					eth_last_price = atol(jtable["data"][i]["price"].dump().c_str());

				if(atol(jtable["data"][i]["price"].dump().c_str()) < eth_last_price*0.5 || atol(jtable["data"][i]["price"].dump().c_str()) > eth_last_price*1.5)
				{
					printf("%s: Price long int = %ld/%ld\n", symbol_str.c_str(), eth_last_price, atol(jtable["data"][i]["price"].dump().c_str()));
					continue;
				}
				else
					eth_last_price = atol(jtable["data"][i]["price"].dump().c_str());
			}
			if(symbol_str == "XBTUSD")
			{

				if(xbt_last_price < 0)
					xbt_last_price = atol(jtable["data"][i]["price"].dump().c_str());

				if(atol(jtable["data"][i]["price"].dump().c_str()) < xbt_last_price*0.5 || atol(jtable["data"][i]["price"].dump().c_str()) > xbt_last_price*1.5)
				{
					printf("%s: Price long int = %ld/%ld\n", symbol_str.c_str(), xbt_last_price, atol(jtable["data"][i]["price"].dump().c_str()));
					continue;
				}
				else
					xbt_last_price = atol(jtable["data"][i]["price"].dump().c_str());
			}

			//printf("keanu dump: %s:%d\n", jtable["data"][i]["price"].dump().c_str(), vol_count);
			sprintf(epochmsg, "%.10s %.2s:%.2s:%.2s", time_str.c_str(), time_str.c_str()+11, time_str.c_str()+14, time_str.c_str()+17);
			strptime(epochmsg, "%Y-%m-%d %H:%M:%S", &tm_struct);
			tm_struct.tm_isdst = 1;
			size_t epoch = std::mktime(&tm_struct);

			sprintf(epochmsg, "%d.%.3s", epoch, time_str.c_str()+20);
			sprintf(timemsg, "%.2s%.2s%.2s%.2s", time_str.c_str()+11, time_str.c_str()+14, time_str.c_str()+17, time_str.c_str()+20);
			sprintf(netmsg, "01_ID=%s.BMEX,ECC.1=%d,Time=%s,C=%s,V=%d,TC=%d,EPID=%s,ECC.2=%d,EPOCH=%s,",
				symbol_str.c_str(), tick_count, timemsg, jtable["data"][i]["price"].dump().c_str(), vol_count, //jtable["data"][i]["size"].dump().c_str(),
				tick_count, pClients->m_strEPIDNum.c_str(), tick_count, epochmsg);
			tick_count++;
			int msglen = strlen(netmsg);

			netmsg[strlen(netmsg)] = GTA_TAIL_BYTE_1;
			netmsg[strlen(netmsg)] = GTA_TAIL_BYTE_2;

			CCVQueueDAO* pQueueDAO = CCVQueueDAOs::GetInstance()->GetDAO();
			pQueueDAO->SendData(netmsg, strlen(netmsg));
			vol_count = 0;
		}
#ifdef DEBUG
		cout << setw(4) << jtable << endl;
		cout << netmsg << endl;
#endif
	}

}

void CCVServer::OnData_Binance(client* c, websocketpp::connection_hdl con, client::message_ptr msg)
{
#ifdef DEBUG
	printf("[on_message_binance]\n");
#endif
	static char netmsg[BUFFERSIZE];
	static char timemsg[9];

	string str = msg->get_payload();
	string price_str, size_str, time_str, symbol_str;
	json jtable = json::parse(str.c_str());

	static CCVClients* pClients = CCVClients::GetInstance();
	static int tick_count_binance=0;

	if(pClients == NULL)
		throw "GET_CLIENTS_ERROR";

	string name_str = "BINANCE";
	static CCVServer* pServer = CCVServers::GetInstance()->GetServerByName(name_str);
	pServer->m_heartbeat_count = 0;
	pServer->m_pHeartbeat->TriggerGetReplyEvent();

	memset(netmsg, 0, BUFFERSIZE);
	memset(timemsg, 0, 8);

	time_str   = "00000000";
	symbol_str = jtable["s"];
	symbol_str.erase(remove(symbol_str.begin(), symbol_str.end(), '\"'), symbol_str.end());
	price_str  = jtable["p"];
	price_str.erase(remove(price_str.begin(), price_str.end(), '\"'), price_str.end());
	size_str   = jtable["q"];
	size_str.erase(remove(size_str.begin(), size_str.end(), '\"'), size_str.end());

	int size_int = stof(size_str) * SCALE_VOL_BINANCE;
	size_str = to_string(size_int);

	sprintf(netmsg, "01_ID=%s.BINANCE,ECC.1=%d,Time=%s,C=%s,V=%s,TC=%d,EPID=%s,ECC.2=%d,",
		symbol_str.c_str(), tick_count_binance, time_str.c_str(), price_str.c_str(), size_str.c_str(), tick_count_binance, pClients->m_strEPIDNum.c_str(), tick_count_binance);
	tick_count_binance++;
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
void CCVServer::OnData_Binance_F(client* c, websocketpp::connection_hdl con, client::message_ptr msg)
{

#ifdef DEBUG
	printf("[on_message_binance_F]\n");
#endif
	static char netmsg[BUFFERSIZE];
	static char timemsg[9];
	string str = msg->get_payload();
	string price_str, size_str, time_str, symbol_str;
	json jtable = json::parse(str.c_str());
	static CCVClients* pClients = CCVClients::GetInstance();
	static int tick_count_binance_F=0;

	if(pClients == NULL)
		throw "GET_CLIENTS_ERROR";

	string name_str = "BINANCE_F";
	static CCVServer* pServer = CCVServers::GetInstance()->GetServerByName(name_str);
	pServer->m_heartbeat_count = 0;
	pServer->m_pHeartbeat->TriggerGetReplyEvent();

	memset(netmsg, 0, BUFFERSIZE);
	memset(timemsg, 0, 8);

	time_str   = "00000000";
	symbol_str = jtable["data"]["s"];
	symbol_str.erase(remove(symbol_str.begin(), symbol_str.end(), '\"'), symbol_str.end());
	price_str  = jtable["data"]["p"];
	price_str.erase(remove(price_str.begin(), price_str.end(), '\"'), price_str.end());
	size_str   = jtable["data"]["q"];
	size_str.erase(remove(size_str.begin(), size_str.end(), '\"'), size_str.end());

	int size_int = stof(size_str) * SCALE_VOL_BINANCE_F;
	size_str = to_string(size_int);

	sprintf(netmsg, "01_ID=%s.BINANCE_F,ECC.1=%d,Time=%s,C=%s,V=%s,TC=%d,EPID=%s,ECC.2=%d,",
		symbol_str.c_str(), tick_count_binance_F, time_str.c_str(), price_str.c_str(), size_str.c_str(), tick_count_binance_F, pClients->m_strEPIDNum.c_str(), tick_count_binance_F);
	tick_count_binance_F++;
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

void CCVServer::OnData_Binance_FT(client* c, websocketpp::connection_hdl con, client::message_ptr msg)
{
#ifdef DEBUG
	printf("[on_message_binance_FT]\n");
#endif
	static char netmsg[BUFFERSIZE];
	static char timemsg[9];
	string str = msg->get_payload();
	string price_str, size_str, time_str, symbol_str;
	json jtable = json::parse(str.c_str());
	static CCVClients* pClients = CCVClients::GetInstance();
	static int tick_count_binance_FT=0;

	if(pClients == NULL)
		throw "GET_CLIENTS_ERROR";

	string name_str = "BINANCE_FT";
	static CCVServer* pServer = CCVServers::GetInstance()->GetServerByName(name_str);
	pServer->m_heartbeat_count = 0;
#if 0
	if(pServer->GetStatus() == ssBreakdown) {
		websocketpp::lib::error_code ec;
		c->close(con, websocketpp::close::status::going_away, "", ec);
		if (ec)
			std::cout << "> Error closing connection " << it->second->get_id() << ": " << ec.message() << std::endl;
		printf("Binance breakdown\n");
	}
#endif
	pServer->m_pHeartbeat->TriggerGetReplyEvent();

	memset(netmsg, 0, BUFFERSIZE);
	memset(timemsg, 0, 8);

	time_str   = "00000000";
	symbol_str = jtable["data"]["s"];
	symbol_str.erase(remove(symbol_str.begin(), symbol_str.end(), '\"'), symbol_str.end());
	price_str  = jtable["data"]["p"];
	price_str.erase(remove(price_str.begin(), price_str.end(), '\"'), price_str.end());
	size_str   = jtable["data"]["q"];
	size_str.erase(remove(size_str.begin(), size_str.end(), '\"'), size_str.end());

	int size_int = stof(size_str) * SCALE_VOL_BINANCE_F;
	size_str = to_string(size_int);

	sprintf(netmsg, "01_ID=%s.BINANCE_FT,ECC.1=%d,Time=%s,C=%s,V=%s,TC=%d,EPID=%s,ECC.2=%d,",symbol_str.c_str(),
		tick_count_binance_FT, time_str.c_str(), price_str.c_str(), size_str.c_str(), tick_count_binance_FT, pClients->m_strEPIDNum.c_str(), tick_count_binance_FT);
	tick_count_binance_FT++;
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

void CCVServer::OnHeartbeatLost()
{
	FprintfStderrLog("HEARTBEAT LOST", -1, 0, m_strName.c_str(), m_strName.length(),  NULL, 0);
#ifdef EXIT_VERSION
	exit(-1);
#endif
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
				m_FTX_enable = true;
				sprintf(replymsg, "%s send subscribe message and response (%s)\n", m_strName.c_str(), msg.message().c_str());
				FprintfStderrLog("initial protocol", -1, 0, replymsg, strlen(replymsg),  NULL, 0);
			}
			else
			{
				auto j = json::parse("{ \"op\": \"subscribe\", \"channel\": \"trades\", \"market\": \"BTC-PERP\" }");
				auto msg = m_conn->send(j.dump());
				//auto j = json::parse("{ \"op\": \"ping\"}");
				//auto msg = m_conn->send(j.dump());
				//j = json::parse("{ \"op\": \"subscribe\", \"channel\": \"trades\", \"market\": \"BTC-PERP\" }");
				//msg = m_conn->send(j.dump());
				sprintf(replymsg, "%s send PING message and response (%s)\n", m_strName.c_str(), msg.message().c_str());
				FprintfStderrLog("PING/PONG protocol", -1, 0, replymsg, strlen(replymsg),  NULL, 0);

				if(msg.message() != "Success")
				{
					FprintfStderrLog("Server PING/PONG Fail", -1, 0, m_strName.c_str(), m_strName.length(),  NULL, 0);
#ifdef EXIT_VERSION
					exit(-1);
#endif
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
#ifdef EXIT_VERSION
				exit(-1);
#endif
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
#ifdef EXIT_VERSION
		exit(-1);
#endif
		SetStatus(ssBreakdown);
	}
#endif
}

void CCVServer::OnHeartbeatError(int nData, const char* pErrorMessage)
{
	FprintfStderrLog("HEARTBEAT ERROR", -1, 0, m_strName.c_str(), m_strName.length(),  NULL, 0);
#ifdef EXIT_VERSION
	exit(-1);
#endif
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

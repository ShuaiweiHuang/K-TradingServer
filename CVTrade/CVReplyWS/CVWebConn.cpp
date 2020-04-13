#include <iostream>
#include <unistd.h>
#include <ctime>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <sys/time.h>
#include <fstream>
#include <curl/curl.h>

#include "CVWebConns.h"
#include "CVWebConn.h"
#include "CVGlobal.h"
#include "Include/CVTSFormat.h"
#include "CVQueueNodes.h"

using namespace std;

extern void FprintfStderrLog(const char* pCause, int nError, int nData, const char* pFile = NULL, int nLine = 0, 
			     unsigned char* pMessage1 = NULL, int nMessage1Length = 0, unsigned char* pMessage2 = NULL, int nMessage2Length = 0);

static size_t getResponse(char *contents, size_t size, size_t nmemb, void *userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}


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
	while(m_ssServerStatus != ssBreakdown)
	{
		try
		{
			SetStatus(ssNone);
			m_cfd.run();
			exit(-1);
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

			if(m_strName == "ORDER_REPLY") {
				sprintf((char*)msg, "set timer to %d sec.", HEARTBEAT_INTERVAL_MIN);
				FprintfStderrLog("HEARTBEAT_TIMER_CONFIG", -1, 0, __FILE__, __LINE__, msg, strlen((char*)msg));
				m_pHeartbeat->SetTimeInterval(HEARTBEAT_INTERVAL_MIN);
				m_cfd.set_message_handler(bind(&OnData_Order_Reply,&m_cfd,::_1,::_2));
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
		if(m_strName == "ORDER_REPLY") {
			m_cfd.set_message_handler(bind(&OnData_Order_Reply,&m_cfd,::_1,::_2));

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

void CCVServer::Binance_Update(json* jtable)
{
	char update_str[BUFFERSIZE];
	string response, exchange_data[30];

	if((*jtable)["e"] == "ORDER_TRADE_UPDATE")
	{
		exchange_data[0] = ((*jtable)["accountId"].dump());
		exchange_data[0] = exchange_data[0].substr(0, exchange_data[0].length());

		exchange_data[1] = ((*jtable)["o"]["i"].dump());
		exchange_data[1] = exchange_data[1].substr(0, exchange_data[1].length());

		exchange_data[2] = ((*jtable)["o"]["s"].dump());
		exchange_data[2] = exchange_data[2].substr(1, exchange_data[2].length()-2);

		exchange_data[3] = ((*jtable)["o"]["X"].dump());
		exchange_data[3] = exchange_data[3].substr(1, exchange_data[3].length()-2);

		exchange_data[4] = ((*jtable)["o"]["c"].dump());
		exchange_data[4] = exchange_data[4].substr(1, exchange_data[4].length()-2);

		exchange_data[5] = ((*jtable)["o"]["ap"].dump());
		exchange_data[5] = exchange_data[5].substr(1, exchange_data[5].length()-2);

		exchange_data[6] = ((*jtable)["o"]["q"].dump());
		exchange_data[6] = exchange_data[6].substr(1, exchange_data[6].length()-2);

		exchange_data[7] = ((*jtable)["o"]["z"].dump());
		exchange_data[7] = exchange_data[7].substr(1, exchange_data[7].length()-2);

		exchange_data[8] = ((*jtable)["o"]["l"].dump());
		exchange_data[8] = exchange_data[8].substr(1, exchange_data[8].length()-2);

		exchange_data[9] = ((*jtable)["o"]["f"].dump());
		exchange_data[9] = exchange_data[9].substr(1, exchange_data[9].length()-2);

		exchange_data[10] = ((*jtable)["o"]["o"].dump());
		exchange_data[10] = exchange_data[10].substr(1, exchange_data[10].length()-2);

		exchange_data[11] = ((*jtable)["o"]["S"].dump());
		exchange_data[11] = exchange_data[11].substr(1, exchange_data[11].length()-2);

		exchange_data[12] = ((*jtable)["o"]["sp"].dump());
		exchange_data[12] = exchange_data[12].substr(1, exchange_data[12].length()-2);

		exchange_data[13] = ((*jtable)["o"]["T"].dump());
		exchange_data[13] = exchange_data[13].substr(0, exchange_data[13].length());

		exchange_data[14] = ((*jtable)["o"]["x"].dump());
		exchange_data[14] = exchange_data[14].substr(1, exchange_data[14].length()-2);

		time_t tt_time = atol(exchange_data[13].c_str())/1000;
		char time_str[30];
		struct tm *tm_time  = localtime(&tt_time);
		strftime(time_str, 30, "%Y-%m-%d %H:%M:%S", tm_time);
		sprintf(update_str, "https://127.0.0.1:2012/mysql/?query=update%%20binance_order_history%%20set%%20order_status=%%27%s%%27,match_qty=%%27%s%%27,match_price=%%27%s%%27,update_user=%%27reply.server%%27%20where%%20order_no=%%27%s%%27", exchange_data[3].c_str(), exchange_data[7].c_str(), exchange_data[5].c_str(), exchange_data[1].c_str());
		printf("[UpdateSQL]:%s\n", update_str);
		CURL *curl = curl_easy_init();
		curl_easy_setopt(curl, CURLOPT_URL, update_str);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, getResponse);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
		curl_easy_perform(curl);
		curl_easy_cleanup(curl);

		if(exchange_data[14] == "TRADE")
		{
			memset(&m_trade_reply, 0, sizeof(m_trade_reply));
			memcpy(m_trade_reply.status_code, "1000", 4);
			memcpy(m_trade_reply.bookno,		(*jtable)["o"]["i"].dump().c_str(),	(*jtable)["o"]["i"].dump().length());
			memcpy(m_trade_reply.price,		(*jtable)["o"]["p"].dump().c_str()+1,	(*jtable)["o"]["p"].dump().length()-2);
			memcpy(m_trade_reply.avgPx,		(*jtable)["o"]["ap"].dump().c_str()+1,	(*jtable)["o"]["ap"].dump().length()-2);
			memcpy(m_trade_reply.orderQty,		(*jtable)["o"]["q"].dump().c_str()+1,	(*jtable)["o"]["q"].dump().length()-2);
			memcpy(m_trade_reply.lastQty,		(*jtable)["o"]["l"].dump().c_str()+1,	(*jtable)["o"]["l"].dump().length()-2);
			memcpy(m_trade_reply.cumQty,		(*jtable)["o"]["z"].dump().c_str()+1,	(*jtable)["o"]["z"].dump().length()-2);
			memcpy(m_trade_reply.key_id,		(*jtable)["o"]["c"].dump().c_str()+1,	(*jtable)["o"]["c"].dump().length()-2);
			memcpy(m_trade_reply.transactTime, time_str, strlen(time_str));
			sprintf(m_trade_reply.reply_msg, "trade reply - [%s]", (*jtable)["data"][0]["X"].dump().c_str());

			printf("Dump reply message:\n");
			printf("[KEYID] %.13s\n", m_trade_reply.key_id);
			printf("[BOOKNO] %s\n", m_trade_reply.bookno);
			printf("[PRICE] %s\n", m_trade_reply.price);
			printf("[MATCHPRICE] %s\n", m_trade_reply.avgPx);
			printf("[ORDERQTY] %s\n", m_trade_reply.orderQty);
			printf("[MATCHQTY] %s\n", m_trade_reply.lastQty);
			printf("[CUMQTY] %s\n", m_trade_reply.cumQty);
			printf("[TIME] %s\n", m_trade_reply.transactTime);

			CCVQueueDAO* pQueueDAO = CCVQueueDAOs::GetInstance()->GetDAO();
			pQueueDAO->SendData((char*)&m_trade_reply, sizeof(m_trade_reply));
		}
	}
}

void CCVServer::Bitmex_Update(json* jtable)
{
	char update_match_str[BUFFERSIZE], insert_str[BUFFERSIZE], update_order_str[BUFFERSIZE];
	string response, exchange_data[30];
	CURLcode res;

	if((*jtable)["table"] == "execution")
	{
		for(int i=0 ; i<(*jtable)["data"].size() ; i++)
		{
			if(((*jtable)["data"][i]["execType"].dump()) != "\"Trade\"")
				continue;

			exchange_data[0] = ((*jtable)["data"][i]["account"].dump());
			exchange_data[0] = exchange_data[0].substr(0, exchange_data[0].length());

			exchange_data[1] = ((*jtable)["data"][i]["execID"].dump());
			exchange_data[1] = exchange_data[1].substr(1, exchange_data[1].length()-2);

			exchange_data[2] = ((*jtable)["data"][i]["symbol"].dump());
			exchange_data[2] = exchange_data[2].substr(1, exchange_data[2].length()-2);

			exchange_data[3] = ((*jtable)["data"][i]["side"].dump());
			exchange_data[3] = exchange_data[3].substr(1, exchange_data[3].length()-2);

			exchange_data[4] = ((*jtable)["data"][i]["lastPx"].dump());
			exchange_data[4] = exchange_data[4].substr(0, exchange_data[4].length());

			exchange_data[5] = ((*jtable)["data"][i]["lastQty"].dump());
			exchange_data[5] = exchange_data[5].substr(0, exchange_data[5].length());

			exchange_data[6] = ((*jtable)["data"][i]["cumQty"].dump());
			exchange_data[6] = exchange_data[6].substr(0, exchange_data[6].length());

			exchange_data[7] = ((*jtable)["data"][i]["leavesQty"].dump());
			exchange_data[7] = exchange_data[7].substr(0, exchange_data[7].length());

			exchange_data[8] = ((*jtable)["data"][i]["execType"].dump());
			exchange_data[8] = exchange_data[8].substr(1, exchange_data[8].length()-2);

			exchange_data[9] = ((*jtable)["data"][i]["transactTime"].dump());
			exchange_data[9] = exchange_data[9].substr(1, 19);

			exchange_data[10] = ((*jtable)["data"][i]["commission"].dump());
			exchange_data[10] = exchange_data[10].substr(0, exchange_data[10].length());

			exchange_data[11] = ((*jtable)["data"][i]["execComm"].dump());
			exchange_data[11] = exchange_data[11].substr(0, exchange_data[11].length());

			exchange_data[12] = ((*jtable)["data"][i]["orderID"].dump());
			exchange_data[12] = exchange_data[12].substr(1, exchange_data[12].length()-2);

			exchange_data[13] = ((*jtable)["data"][i]["price"].dump());
			exchange_data[13] = exchange_data[13].substr(0, exchange_data[13].length());

			exchange_data[14] = ((*jtable)["data"][i]["orderQty"].dump());
			exchange_data[14] = exchange_data[14].substr(0, exchange_data[14].length());

			exchange_data[15] = ((*jtable)["data"][i]["ordType"].dump());
			exchange_data[15] = exchange_data[15].substr(1, exchange_data[15].length()-2);

			exchange_data[16] = ((*jtable)["data"][i]["ordStatus"].dump());
			exchange_data[16] = exchange_data[16].substr(1, exchange_data[16].length()-2);

			exchange_data[17] = ((*jtable)["data"][i]["currency"].dump());
			exchange_data[17] = exchange_data[17].substr(1, exchange_data[17].length()-2);

			exchange_data[18] = ((*jtable)["data"][i]["settlCurrency"].dump());
			exchange_data[18] = exchange_data[18].substr(1, exchange_data[18].length()-2);

			exchange_data[19] = ((*jtable)["data"][i]["clOrdID"].dump());
			exchange_data[19] = exchange_data[19].substr(1, exchange_data[19].length()-2);

			exchange_data[20] = ((*jtable)["data"][i]["text"].dump());
			exchange_data[20] = exchange_data[20].substr(1, exchange_data[20].length()-2);
			sprintf(insert_str, "https://127.0.0.1:2012/mysql/?query=insert%%20into%%20bitmex_match_history%%20set%%20exchange=%%27BITMEX%%27,account=%%27%s%%27,match_no=%%27%s%%27,symbol=%%27%s%%27,side=%%27%s%%27,match_cum_qty=%%27%s%%27,remaining_qty=%%27%s%%27,match_type=%%27%s%%27,match_time=%%27%s%%27,order_no=%%27%s%%27,order_qty=%%27%s%%27,order_type=%%27%s%%27,order_status=%%27%s%%27,quote_currency=%%27%s%%27,settlement_currency=%%27%s%%27,serial_no=%%27%s%%27,remark=%%27%s%%27", exchange_data[0].c_str(), exchange_data[1].c_str(), exchange_data[2].c_str(), exchange_data[3].c_str(), exchange_data[6].c_str(), exchange_data[7].c_str(), exchange_data[8].c_str(), exchange_data[9].c_str(), exchange_data[12].c_str(), exchange_data[14].c_str(), exchange_data[15].c_str(), exchange_data[16].c_str(), exchange_data[17].c_str(), exchange_data[18].c_str(), exchange_data[19].c_str(), exchange_data[20].c_str());

			if(exchange_data[4] != "null")
				sprintf(insert_str, "%s,match_price=%%27%s%%27", insert_str, exchange_data[4].c_str());
			if(exchange_data[5] != "null")
				sprintf(insert_str, "%s,match_qty=%%27%s%%27", insert_str, exchange_data[5].c_str());
			if(exchange_data[10] != "null")
				sprintf(insert_str, "%s,commission_rate=%%27%s%%27", insert_str, exchange_data[10].c_str());
			if(exchange_data[11] != "null")
				sprintf(insert_str, "%s,commission=%%27%s%%27", insert_str, exchange_data[11].c_str());
			if(exchange_data[13] != "null")
				sprintf(insert_str, "%s,order_price=%%27%s%%27", insert_str, exchange_data[13].c_str());

			sprintf(insert_str, "%s,update_user=USER()", insert_str);
			sprintf(update_match_str, "https://127.0.0.1:2012/mysql/?query=update%%20bitmex_match_history%%20set%%20match_cum_qty=%%27%s%%27,remaining_qty=%%27%s%%27,match_type=%%27%s%%27,match_time=%%27%s%%27,order_status=%%27%s%%27,remark=%%27%s%%27", exchange_data[6].c_str(), exchange_data[7].c_str(), exchange_data[8].c_str(), exchange_data[9].c_str(), exchange_data[16].c_str(), exchange_data[20].c_str());
			if(exchange_data[4] != "null")
				sprintf(update_match_str, "%s,match_price=%%27%s%%27", update_match_str, exchange_data[4].c_str());
			if(exchange_data[5] != "null")
				sprintf(update_match_str, "%s,match_qty=%%27%s%%27", update_match_str, exchange_data[5].c_str());
			if(exchange_data[10] != "null")
				sprintf(update_match_str, "%s,commission_rate=%%27%s%%27", update_match_str, exchange_data[10].c_str());
			if(exchange_data[11] != "null")
				sprintf(update_match_str, "%s,commission=%%27%s%%27", update_match_str, exchange_data[11].c_str());
			sprintf(update_match_str, "%s,update_user=USER()", update_match_str);
			sprintf(update_match_str, "%s%%20where%%20match_no=%%27%s%%27", update_match_str, exchange_data[1].c_str());

			sprintf(update_order_str, "https://127.0.0.1:2012/mysql/?query=update%%20bitmex_order_history%%20set%%20remaining_qty=%%27%s%%27,order_status=%%27%s%%27,remark=%%27%s%%27", exchange_data[7].c_str(), exchange_data[16].c_str(), exchange_data[20].c_str());
			if(exchange_data[4] != "null")
				sprintf(update_order_str, "%s,match_price=%%27%s%%27", update_order_str, exchange_data[4].c_str());
			if(exchange_data[5] != "null")
				sprintf(update_order_str, "%s,match_qty=%%27%s%%27", update_order_str, exchange_data[5].c_str());
			sprintf(update_order_str, "%s,update_user=USER()", update_order_str);
			sprintf(update_order_str, "%s%%20where%%20order_no=%%27%s%%27", update_order_str, exchange_data[12].c_str());

			CURL *curl = curl_easy_init();

			for(int i=0 ; i<strlen(insert_str) ; i++)
			{
				if(insert_str[i] == ' ')
					insert_str[i] = '+';
			}
			for(int i=0 ; i<strlen(update_match_str) ; i++)
			{
				if(update_match_str[i] == ' ')
					update_match_str[i] = '+';
			}
			for(int i=0 ; i<strlen(update_order_str) ; i++)
			{
				if(update_order_str[i] == ' ')
					update_order_str[i] = '+';
			}

			printf("=============insert order BITMEX:\n%s\n=============\n", insert_str);
			curl_easy_setopt(curl, CURLOPT_URL, insert_str);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, getResponse);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
			res = curl_easy_perform(curl);
			if(res != CURLE_OK) {
				fprintf(stderr, "CVReplyWS:Bitmex_Update:curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
			}

			printf("=============update order BITMEX:\n%s\n=============\n", update_order_str);
			curl_easy_setopt(curl, CURLOPT_URL, update_order_str);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, getResponse);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
			res = curl_easy_perform(curl);
			if(res != CURLE_OK) {
				fprintf(stderr, "CVReplyWS:Bitmex_Update:curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
			}

			curl_easy_cleanup(curl);

			//if(exchange_data[8] == "Trade")
			{
				memset(&m_trade_reply, 0, sizeof(m_trade_reply));

				string text = (*jtable)["data"][i]["error"].dump();
				if(text != "null")
				{
					memcpy(m_trade_reply.status_code, "1001", 4);
					sprintf(m_trade_reply.reply_msg, "reply fail, error message - [%s]", text.c_str());
					memcpy(m_trade_reply.bookno, (*jtable)["data"][i]["orderID"].dump().c_str()+1, 36);
					printf("[CVReply ErrMsg] %s\n", m_trade_reply.reply_msg);
				}
				else
				{
					memcpy(m_trade_reply.status_code, "1000", 4);
					memcpy(m_trade_reply.price,         (*jtable)["data"][i]["price"].dump().c_str(),      (*jtable)["data"][i]["price"].dump().length());

					if((*jtable)["data"][i]["avgPx"].dump() == "null")
						memcpy(m_trade_reply.avgPx, (*jtable)["data"][i]["stopPx"].dump().c_str(),         (*jtable)["data"][i]["stopPx"].dump().length());
					else
						memcpy(m_trade_reply.avgPx, (*jtable)["data"][i]["avgPx"].dump().c_str(),          (*jtable)["data"][i]["avgPx"].dump().length());
					memcpy(m_trade_reply.orderQty,      (*jtable)["data"][i]["orderQty"].dump().c_str(),       (*jtable)["data"][i]["orderQty"].dump().length());
					memcpy(m_trade_reply.lastQty,       (*jtable)["data"][i]["lastQty"].dump().c_str(),        (*jtable)["data"][i]["lastQty"].dump().length());
					memcpy(m_trade_reply.cumQty,        (*jtable)["data"][i]["cumQty"].dump().c_str(),         (*jtable)["data"][i]["cumQty"].dump().length());
					memcpy(m_trade_reply.key_id,        (*jtable)["data"][i]["clOrdID"].dump().c_str()+1,      (*jtable)["data"][i]["clOrdID"].dump().length()-2);
					memcpy(m_trade_reply.bookno,        (*jtable)["data"][i]["orderID"].dump().c_str()+1, 36);
					memcpy(m_trade_reply.transactTime,  (*jtable)["data"][i]["transactTime"].dump().c_str()+1, (*jtable)["data"][i]["transactTime"].dump().length()-2);
					memcpy(m_trade_reply.symbol,		(*jtable)["data"][i]["symbol"].dump().c_str()+1,	(*jtable)["data"][i]["symbol"].dump().length()-2);
					memcpy(m_trade_reply.buysell,		(*jtable)["data"][i]["side"].dump().c_str()+1, 1);
	
					if((*jtable)["data"][i]["execComm"].dump() != "null")
						memcpy(m_trade_reply.commission,	(*jtable)["data"][i]["execComm"].dump().c_str(), (*jtable)["data"][i]["execComm"].dump().length());
	
					sprintf(m_trade_reply.reply_msg,    "trade reply - [%s, (%s/%s)]",
					                                    (*jtable)["data"][i]["text"].dump().c_str(), m_trade_reply.cumQty, m_trade_reply.orderQty);
					sprintf(m_trade_reply.exchange_name, "BITMEX");
					CCVQueueDAO* pQueueDAO = CCVQueueDAOs::GetInstance()->GetDAO();
					m_trade_reply.trail[0] = '\r';
					m_trade_reply.trail[1] = '\n';
					pQueueDAO->SendData((char*)&m_trade_reply, sizeof(m_trade_reply));
				}
			}//trade
		}//for
	}//execution
}

void CCVServer::FTX_Update(json* jtable)
{
	char update_match_str[BUFFERSIZE], insert_str[BUFFERSIZE], update_order_str[BUFFERSIZE];
	string response, exchange_data[30];
	CURLcode res;

	if((*jtable)["table"] == "execution")
	{
		for(int i=0 ; i<(*jtable)["data"].size() ; i++)
		{
			if(((*jtable)["data"][i]["cv_data_type"].dump()) != "\"match\"")
				continue;

			exchange_data[0] = ((*jtable)["data"][i]["account"].dump());
			exchange_data[0] = exchange_data[0].substr(1, exchange_data[0].length()-2);

			exchange_data[1] = ((*jtable)["data"][i]["execID"].dump());
			exchange_data[1] = exchange_data[1].substr(1, exchange_data[1].length()-2);

			exchange_data[2] = ((*jtable)["data"][i]["symbol"].dump());
			exchange_data[2] = exchange_data[2].substr(1, exchange_data[2].length()-2);

			exchange_data[3] = ((*jtable)["data"][i]["side"].dump());
			exchange_data[3] = exchange_data[3].substr(1, exchange_data[3].length()-2);

			exchange_data[4] = ((*jtable)["data"][i]["lastPx"].dump());
			exchange_data[4] = exchange_data[4].substr(0, exchange_data[4].length());

			exchange_data[5] = ((*jtable)["data"][i]["lastQty"].dump());
			exchange_data[5] = exchange_data[5].substr(0, exchange_data[5].length());

			exchange_data[6] = ((*jtable)["data"][i]["cumQty"].dump());
			exchange_data[6] = exchange_data[6].substr(0, exchange_data[6].length());

			exchange_data[7] = ((*jtable)["data"][i]["leavesQty"].dump());
			exchange_data[7] = exchange_data[7].substr(0, exchange_data[7].length());

			exchange_data[8] = ((*jtable)["data"][i]["execType"].dump());
			exchange_data[8] = exchange_data[8].substr(1, exchange_data[8].length()-2);

			exchange_data[9] = ((*jtable)["data"][i]["transactTime"].dump());
			exchange_data[9] = exchange_data[9].substr(1, 19);

			exchange_data[10] = ((*jtable)["data"][i]["commission"].dump());
			exchange_data[10] = exchange_data[10].substr(0, exchange_data[10].length());

			exchange_data[11] = ((*jtable)["data"][i]["execComm"].dump());
			exchange_data[11] = exchange_data[11].substr(0, exchange_data[11].length());

			exchange_data[12] = ((*jtable)["data"][i]["orderID"].dump());
			exchange_data[12] = exchange_data[12].substr(1, exchange_data[12].length()-2);

			exchange_data[13] = ((*jtable)["data"][i]["price"].dump());
			exchange_data[13] = exchange_data[13].substr(0, exchange_data[13].length());

			exchange_data[14] = ((*jtable)["data"][i]["orderQty"].dump());
			exchange_data[14] = exchange_data[14].substr(0, exchange_data[14].length());

			exchange_data[15] = ((*jtable)["data"][i]["ordType"].dump());
			exchange_data[15] = exchange_data[15].substr(1, exchange_data[15].length()-2);

			exchange_data[16] = ((*jtable)["data"][i]["ordStatus"].dump());
			exchange_data[16] = exchange_data[16].substr(1, exchange_data[16].length()-2);

			exchange_data[17] = ((*jtable)["data"][i]["currency"].dump());
			exchange_data[17] = exchange_data[17].substr(1, exchange_data[17].length()-2);

			exchange_data[18] = ((*jtable)["data"][i]["settlCurrency"].dump());
			exchange_data[18] = exchange_data[18].substr(1, exchange_data[18].length()-2);

			exchange_data[19] = ((*jtable)["data"][i]["clOrdID"].dump());
			exchange_data[19] = exchange_data[19].substr(1, exchange_data[19].length()-2);

			exchange_data[20] = ((*jtable)["data"][i]["text"].dump());
			exchange_data[20] = exchange_data[20].substr(1, exchange_data[20].length()-2);
			sprintf(insert_str, "https://127.0.0.1:2012/mysql/?query=insert%%20into%%20ftx_match_history%%20set%%20exchange=%%27FTX%%27,account=%%27%s%%27,match_no=%%27%s%%27,symbol=%%27%s%%27,side=%%27%s%%27,match_cum_qty=%%27%s%%27,remaining_qty=%%27%s%%27,match_type=%%27%s%%27,match_time=%%27%s%%27,order_no=%%27%s%%27,order_qty=%%27%s%%27,order_type=%%27%s%%27,order_status=%%27%s%%27,quote_currency=%%27%s%%27,settlement_currency=%%27%s%%27,serial_no=%%27%s%%27,remark=%%27%s%%27", exchange_data[0].c_str(), exchange_data[1].c_str(), exchange_data[2].c_str(), exchange_data[3].c_str(), exchange_data[6].c_str(), exchange_data[7].c_str(), exchange_data[8].c_str(), exchange_data[9].c_str(), exchange_data[12].c_str(), exchange_data[14].c_str(), exchange_data[15].c_str(), exchange_data[16].c_str(), exchange_data[17].c_str(), exchange_data[18].c_str(), exchange_data[19].c_str(), exchange_data[20].c_str());

			if(exchange_data[4] != "null")
				sprintf(insert_str, "%s,match_price=%%27%s%%27", insert_str, exchange_data[4].c_str());
			if(exchange_data[5] != "null")
				sprintf(insert_str, "%s,match_qty=%%27%s%%27", insert_str, exchange_data[5].c_str());
			if(exchange_data[10] != "null")
				sprintf(insert_str, "%s,commission_rate=%%27%s%%27", insert_str, exchange_data[10].c_str());
			if(exchange_data[11] != "null")
				sprintf(insert_str, "%s,commission=%%27%s%%27", insert_str, exchange_data[11].c_str());
			if(exchange_data[13] != "null")
				sprintf(insert_str, "%s,order_price=%%27%s%%27", insert_str, exchange_data[13].c_str());

			sprintf(insert_str, "%s,update_user=USER()", insert_str);
			sprintf(update_match_str, "https://127.0.0.1:2012/mysql/?query=update%%20ftx_match_history%%20set%%20match_cum_qty=%%27%s%%27,remaining_qty=%%27%s%%27,match_type=%%27%s%%27,match_time=%%27%s%%27,order_status=%%27%s%%27,remark=%%27%s%%27", exchange_data[6].c_str(), exchange_data[7].c_str(), exchange_data[8].c_str(), exchange_data[9].c_str(), exchange_data[16].c_str(), exchange_data[20].c_str());
			if(exchange_data[4] != "null")
				sprintf(update_match_str, "%s,match_price=%%27%s%%27", update_match_str, exchange_data[4].c_str());
			if(exchange_data[5] != "null")
				sprintf(update_match_str, "%s,match_qty=%%27%s%%27", update_match_str, exchange_data[5].c_str());
			if(exchange_data[10] != "null")
				sprintf(update_match_str, "%s,commission_rate=%%27%s%%27", update_match_str, exchange_data[10].c_str());
			if(exchange_data[11] != "null")
				sprintf(update_match_str, "%s,commission=%%27%s%%27", update_match_str, exchange_data[11].c_str());
			sprintf(update_match_str, "%s,update_user=USER()", update_match_str);
			sprintf(update_match_str, "%s%%20where%%20match_no=%%27%s%%27", update_match_str, exchange_data[1].c_str());

			sprintf(update_order_str, "https://127.0.0.1:2012/mysql/?query=update%%20ftx_order_history%%20set%%20remaining_qty=%%27%s%%27,order_status=%%27%s%%27,remark=%%27%s%%27", exchange_data[7].c_str(), exchange_data[16].c_str(), exchange_data[20].c_str());
			if(exchange_data[4] != "null")
				sprintf(update_order_str, "%s,match_price=%%27%s%%27", update_order_str, exchange_data[4].c_str());
			if(exchange_data[5] != "null")
				sprintf(update_order_str, "%s,match_qty=%%27%s%%27", update_order_str, exchange_data[5].c_str());
			sprintf(update_order_str, "%s,update_user=USER()", update_order_str);
			sprintf(update_order_str, "%s%%20where%%20order_no=%%27%s%%27", update_order_str, exchange_data[12].c_str());

			CURL *curl = curl_easy_init();

			for(int i=0 ; i<strlen(insert_str) ; i++)
			{
				if(insert_str[i] == ' ')
					insert_str[i] = '+';
			}
			for(int i=0 ; i<strlen(update_match_str) ; i++)
			{
				if(update_match_str[i] == ' ')
					update_match_str[i] = '+';
			}
			for(int i=0 ; i<strlen(update_order_str) ; i++)
			{
				if(update_order_str[i] == ' ')
					update_order_str[i] = '+';
			}

			printf("=============insert match FTX:\n%s\n=============\n", insert_str);
			curl_easy_setopt(curl, CURLOPT_URL, insert_str);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, getResponse);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
			res = curl_easy_perform(curl);
			if(res != CURLE_OK) {
				fprintf(stderr, "CVReplyWS:FTX_Update:curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
			}

			printf("=============update order FTX:\n%s\n=============\n", update_order_str);
			curl_easy_setopt(curl, CURLOPT_URL, update_order_str);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, getResponse);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
			res = curl_easy_perform(curl);
			if(res != CURLE_OK) {
				fprintf(stderr, "CVReplyWS:FTX_Update:curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
			}

			curl_easy_cleanup(curl);

			//if(exchange_data[8] == "Trade")
			{
				memset(&m_trade_reply, 0, sizeof(m_trade_reply));

				string text = (*jtable)["data"][i]["error"].dump();
				if(text != "null")
				{
					memcpy(m_trade_reply.status_code, "1001", 4);
					sprintf(m_trade_reply.reply_msg, "reply fail, error message - [%s]", text.c_str());
					memcpy(m_trade_reply.bookno, (*jtable)["data"][i]["orderID"].dump().c_str()+1, (*jtable)["data"][i]["orderID"].dump().length()-2);
					printf("[CVReply ErrMsg] %s\n", m_trade_reply.reply_msg);
				}
				else
				{
					memcpy(m_trade_reply.status_code, "1000", 4);
					memcpy(m_trade_reply.price,         (*jtable)["data"][i]["price"].dump().c_str(),      (*jtable)["data"][i]["price"].dump().length());

					if((*jtable)["data"][i]["avgPx"].dump() == "null")
						memcpy(m_trade_reply.avgPx, (*jtable)["data"][i]["stopPx"].dump().c_str(),         (*jtable)["data"][i]["stopPx"].dump().length());
					else
						memcpy(m_trade_reply.avgPx, (*jtable)["data"][i]["avgPx"].dump().c_str(),          (*jtable)["data"][i]["avgPx"].dump().length());
					memcpy(m_trade_reply.orderQty,      (*jtable)["data"][i]["orderQty"].dump().c_str(),       (*jtable)["data"][i]["orderQty"].dump().length());
					memcpy(m_trade_reply.lastQty,       (*jtable)["data"][i]["lastQty"].dump().c_str(),        (*jtable)["data"][i]["lastQty"].dump().length());
					memcpy(m_trade_reply.cumQty,        (*jtable)["data"][i]["cumQty"].dump().c_str(),         (*jtable)["data"][i]["cumQty"].dump().length());
					memcpy(m_trade_reply.key_id,        (*jtable)["data"][i]["clOrdID"].dump().c_str()+1,      (*jtable)["data"][i]["clOrdID"].dump().length()-2);
					memcpy(m_trade_reply.bookno,        (*jtable)["data"][i]["orderID"].dump().c_str()+1, (*jtable)["data"][i]["orderID"].dump().length()-2);
					memcpy(m_trade_reply.transactTime,  (*jtable)["data"][i]["transactTime"].dump().c_str()+1, (*jtable)["data"][i]["transactTime"].dump().length()-2);
					memcpy(m_trade_reply.symbol,		(*jtable)["data"][i]["symbol"].dump().c_str()+1,	(*jtable)["data"][i]["symbol"].dump().length()-2);
					memcpy(m_trade_reply.buysell,		(*jtable)["data"][i]["side"].dump().c_str()+1, 1);
	
					if((*jtable)["data"][i]["execComm"].dump() != "null")
						memcpy(m_trade_reply.commission,	(*jtable)["data"][i]["execComm"].dump().c_str(), (*jtable)["data"][i]["execComm"].dump().length());
	
					sprintf(m_trade_reply.reply_msg,    "trade reply - [%s, (%s/%s)]",
					                                    (*jtable)["data"][i]["text"].dump().c_str(), m_trade_reply.cumQty, m_trade_reply.orderQty);
					sprintf(m_trade_reply.exchange_name, "FTX");
					CCVQueueDAO* pQueueDAO = CCVQueueDAOs::GetInstance()->GetDAO();
					m_trade_reply.trail[0] = '\r';
					m_trade_reply.trail[1] = '\n';
					pQueueDAO->SendData((char*)&m_trade_reply, sizeof(m_trade_reply));
				}
			}//trade
		}//for
	}//execution
}


#if 0
void CCVServer::FTX_Update(json* jtable)
{
	char update_match_str[BUFFERSIZE], insert_str[BUFFERSIZE], update_order_str[BUFFERSIZE];
	string response, exchange_data[30];
	CURLcode res;

	if((*jtable)["channel"] == "fills")
	{
		exchange_data[0] = ((*jtable)["accountId"].dump());
		exchange_data[0] = exchange_data[0].substr(1, exchange_data[0].length()-2);

		if((*jtable)["data"]["type"] != "order")
			return;

		exchange_data[1] = ((*jtable)["data"]["id"].dump());
		exchange_data[1] = exchange_data[1].substr(0, exchange_data[1].length());

		exchange_data[2] = ((*jtable)["data"]["market"].dump());
		exchange_data[2] = exchange_data[2].substr(1, exchange_data[2].length()-2);

		exchange_data[3] = ((*jtable)["data"]["side"].dump());
		exchange_data[3] = exchange_data[3].substr(1, exchange_data[3].length()-2);

		exchange_data[4] = ((*jtable)["data"]["price"].dump());
		exchange_data[4] = exchange_data[4].substr(0, exchange_data[4].length());

		exchange_data[5] = ((*jtable)["data"]["size"].dump());
		exchange_data[5] = exchange_data[5].substr(0, exchange_data[5].length());

		exchange_data[6] = ((*jtable)["data"]["type"].dump());
		exchange_data[6] = exchange_data[6].substr(1, exchange_data[6].length()-2);

		exchange_data[7] = ((*jtable)["data"]["time"].dump());
		exchange_data[7] = exchange_data[7].substr(1, 19);

		exchange_data[8] = ((*jtable)["data"]["feeRate"].dump());
		exchange_data[8] = exchange_data[8].substr(0, exchange_data[8].length());

		exchange_data[9] = ((*jtable)["data"]["fee"].dump());
		exchange_data[9] = exchange_data[9].substr(0, exchange_data[9].length());

		exchange_data[10] = ((*jtable)["data"]["orderId"].dump());
		exchange_data[10] = exchange_data[10].substr(0, exchange_data[10].length());

		exchange_data[11] = ((*jtable)["data"]["quoteCurrency"].dump());
		exchange_data[11] = exchange_data[11].substr(0, exchange_data[11].length());

		exchange_data[12] = ((*jtable)["data"]["baseCurrency"].dump());
		exchange_data[12] = exchange_data[12].substr(0, exchange_data[12].length());

		exchange_data[13] = ((*jtable)["data"]["remainingSize"].dump());
		exchange_data[13] = exchange_data[13].substr(0, exchange_data[13].length());

		exchange_data[14] = ((*jtable)["data"]["status"].dump());
		exchange_data[14] = exchange_data[14].substr(1, exchange_data[14].length()-2);

		sprintf(insert_str, "https://127.0.0.1:2012/mysql/?query=insert%%20into%%20ftx_match_history%%20set%%20exchange=%%27FTX%%27,account=%%27%s%%27,match_no=%%27%s%%27,symbol=%%27%s%%27,side=%%27%s%%27,match_price=%%27%s%%27,match_qty=%%27%s%%27,match_type=%%27%s%%27,match_time=%%27%s%%27,commission_rate=%%27%s%%27,commission=%%27%s%%27,order_no=%%27%s%%27",
		exchange_data[0].c_str(),
		exchange_data[1].c_str(),
		exchange_data[2].c_str(),
		exchange_data[3].c_str(),
		exchange_data[4].c_str(),
		exchange_data[5].c_str(),
		exchange_data[6].c_str(),
		exchange_data[7].c_str(),
		exchange_data[8].c_str(),
		exchange_data[9].c_str(),
		exchange_data[10].c_str());

		printf("=============insert=============\n%s\n", insert_str);

		if(exchange_data[11] != "null")
			sprintf(insert_str, "%s,quote_currency=%%27%s%%27", insert_str, exchange_data[11].c_str());
		if(exchange_data[12] != "null")
			sprintf(insert_str, "%s,settlement_currency=%%27%s%%27", insert_str, exchange_data[12].c_str());

		sprintf(insert_str, "%s,update_user=USER()", insert_str);

		printf("=============insert=============\n%s\n", insert_str);

		sprintf(update_order_str, "https://127.0.0.1:2012/mysql/?query=update%%20bitmex_order_history%%20set%%20remaining_qty=%%27%s%%27,order_status=%%27%s%%27", exchange_data[13].c_str(), exchange_data[14].c_str());

		if(exchange_data[4] != "null")
			sprintf(update_order_str, "%s,match_price=%%27%s%%27", update_order_str, exchange_data[4].c_str());

		if(exchange_data[5] != "null")
			sprintf(update_order_str, "%s,match_qty=%%27%s%%27", update_order_str, exchange_data[5].c_str());

		sprintf(update_order_str, "%s,update_user=USER()", update_order_str);
		sprintf(update_order_str, "%s%%20where%%20order_no=%%27%s%%27", update_order_str, exchange_data[10].c_str());

		printf("=============update order==========\n%s\n=============\n", update_order_str);

		CURL *curl = curl_easy_init();
		curl_easy_setopt(curl, CURLOPT_URL, update_order_str);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, getResponse);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
		res = curl_easy_perform(curl);

		if(res != CURLE_OK)
			fprintf(stderr, "CVReplyWS:FTX_Update:curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

		for(int i=0 ; i<strlen(insert_str) ; i++)
		{
			if(insert_str[i] == ' ')
				insert_str[i] = '+';
		}
#if 0
		for(int i=0 ; i<strlen(update_match_str) ; i++)
		{
			if(update_match_str[i] == ' ')
				update_match_str[i] = '+';
		}
#endif
		for(int i=0 ; i<strlen(update_order_str) ; i++)
		{
			if(update_order_str[i] == ' ')
				update_order_str[i] = '+';
		}
		printf("=============insert:\n%s\n=============\n", insert_str);
		curl_easy_setopt(curl, CURLOPT_URL, insert_str);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, getResponse);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
		res = curl_easy_perform(curl);
		curl_easy_setopt(curl, CURLOPT_URL, insert_str);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, getResponse);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
		res = curl_easy_perform(curl);

		if(res != CURLE_OK)
			fprintf(stderr, "CVReplyWS:FTX_Update:curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

		curl_easy_cleanup(curl);

		//if(exchange_data[8] == "Trade")
		{
			memset(&m_trade_reply, 0, sizeof(m_trade_reply));

			string text = (*jtable)["data"]["error"].dump();
			if(text != "null")
			{
				memcpy(m_trade_reply.status_code, "1001", 4);
				sprintf(m_trade_reply.reply_msg, "reply fail, error message - [%s]", text.c_str());
				memcpy(m_trade_reply.bookno, (*jtable)["data"]["orderID"].dump().c_str()+1, 36);
				printf("[CVReply ErrMsg] %s\n", m_trade_reply.reply_msg);
			}
			else
			{
				memcpy(m_trade_reply.status_code, "1000", 4);
				memcpy(m_trade_reply.price,         (*jtable)["data"]["price"].dump().c_str(),      (*jtable)["data"]["price"].dump().length());

				if((*jtable)["data"]["avgPx"].dump() == "null")
					memcpy(m_trade_reply.avgPx, (*jtable)["data"]["stopPx"].dump().c_str(),         (*jtable)["data"]["stopPx"].dump().length());
				else
					memcpy(m_trade_reply.avgPx, (*jtable)["data"]["avgPx"].dump().c_str(),          (*jtable)["data"]["avgPx"].dump().length());
				memcpy(m_trade_reply.orderQty,      (*jtable)["data"]["orderQty"].dump().c_str(),       (*jtable)["data"]["orderQty"].dump().length());
				memcpy(m_trade_reply.lastQty,       (*jtable)["data"]["lastQty"].dump().c_str(),        (*jtable)["data"]["lastQty"].dump().length());
				memcpy(m_trade_reply.cumQty,        (*jtable)["data"]["cumQty"].dump().c_str(),         (*jtable)["data"]["cumQty"].dump().length());
				memcpy(m_trade_reply.key_id,        (*jtable)["data"]["clOrdID"].dump().c_str()+1,      (*jtable)["data"]["clOrdID"].dump().length()-2);
				memcpy(m_trade_reply.bookno,        (*jtable)["data"]["orderID"].dump().c_str()+1, 36);
				memcpy(m_trade_reply.transactTime,  (*jtable)["data"]["transactTime"].dump().c_str()+1, (*jtable)["data"]["transactTime"].dump().length()-2);
				memcpy(m_trade_reply.symbol,		(*jtable)["data"]["symbol"].dump().c_str()+1,	(*jtable)["data"]["symbol"].dump().length()-2);
				memcpy(m_trade_reply.buysell,		(*jtable)["data"]["side"].dump().c_str()+1, 1);

				if((*jtable)["data"]["execComm"].dump() != "null")
					memcpy(m_trade_reply.commission,	(*jtable)["data"]["execComm"].dump().c_str(), (*jtable)["data"]["execComm"].dump().length());

				sprintf(m_trade_reply.reply_msg,    "trade reply - [%s, (%s/%s)]",
								    (*jtable)["data"]["text"].dump().c_str(), m_trade_reply.cumQty, m_trade_reply.orderQty);
				sprintf(m_trade_reply.exchange_name, "BITMEX");
				CCVQueueDAO* pQueueDAO = CCVQueueDAOs::GetInstance()->GetDAO();
				m_trade_reply.trail[0] = '\r';
				m_trade_reply.trail[1] = '\n';
				pQueueDAO->SendData((char*)&m_trade_reply, sizeof(m_trade_reply));
			}
		}//trade
	}//fills

	if((*jtable)["channel"] == "orders")
	{
		exchange_data[0] = ((*jtable)["accountId"].dump());
		exchange_data[0] = exchange_data[0].substr(1, exchange_data[0].length()-2);

		if((*jtable)["data"]["type"] != "order")
			return;

		exchange_data[1] = ((*jtable)["data"]["id"].dump());
		exchange_data[1] = exchange_data[1].substr(0, exchange_data[1].length());

		exchange_data[2] = ((*jtable)["data"]["market"].dump());
		exchange_data[2] = exchange_data[2].substr(1, exchange_data[2].length()-2);

		exchange_data[3] = ((*jtable)["data"]["side"].dump());
		exchange_data[3] = exchange_data[3].substr(1, exchange_data[3].length()-2);

		exchange_data[4] = ((*jtable)["data"]["price"].dump());
		exchange_data[4] = exchange_data[4].substr(0, exchange_data[4].length());

		exchange_data[5] = ((*jtable)["data"]["size"].dump());
		exchange_data[5] = exchange_data[5].substr(0, exchange_data[5].length());

		exchange_data[6] = ((*jtable)["data"]["type"].dump());
		exchange_data[6] = exchange_data[6].substr(1, exchange_data[6].length()-2);

		exchange_data[7] = ((*jtable)["data"]["time"].dump());
		exchange_data[7] = exchange_data[7].substr(1, 19);

		exchange_data[8] = ((*jtable)["data"]["feeRate"].dump());
		exchange_data[8] = exchange_data[8].substr(0, exchange_data[8].length());

		exchange_data[9] = ((*jtable)["data"]["fee"].dump());
		exchange_data[9] = exchange_data[9].substr(0, exchange_data[9].length());

		exchange_data[10] = ((*jtable)["data"]["orderId"].dump());
		exchange_data[10] = exchange_data[10].substr(0, exchange_data[10].length());

		exchange_data[11] = ((*jtable)["data"]["quoteCurrency"].dump());
		exchange_data[11] = exchange_data[11].substr(0, exchange_data[11].length());

		exchange_data[12] = ((*jtable)["data"]["baseCurrency"].dump());
		exchange_data[12] = exchange_data[12].substr(0, exchange_data[12].length());

		exchange_data[13] = ((*jtable)["data"]["remainingSize"].dump());
		exchange_data[13] = exchange_data[13].substr(0, exchange_data[13].length());

		exchange_data[14] = ((*jtable)["data"]["status"].dump());
		exchange_data[14] = exchange_data[14].substr(1, exchange_data[14].length()-2);

		sprintf(insert_str, "https://127.0.0.1:2012/mysql/?query=insert%%20into%%20ftx_match_history%%20set%%20exchange=%%27FTX%%27,account=%%27%s%%27,match_no=%%27%s%%27,symbol=%%27%s%%27,side=%%27%s%%27,match_price=%%27%s%%27,match_qty=%%27%s%%27,match_type=%%27%s%%27,match_time=%%27%s%%27,commission_rate=%%27%s%%27,commission=%%27%s%%27,order_no=%%27%s%%27",
		exchange_data[0].c_str(),
		exchange_data[1].c_str(),
		exchange_data[2].c_str(),
		exchange_data[3].c_str(),
		exchange_data[4].c_str(),
		exchange_data[5].c_str(),
		exchange_data[6].c_str(),
		exchange_data[7].c_str(),
		exchange_data[8].c_str(),
		exchange_data[9].c_str(),
		exchange_data[10].c_str());

		printf("=============insert=============\n%s\n", insert_str);

		if(exchange_data[11] != "null")
		{
			exchange_data[11] = exchange_data[11].substr(1, exchange_data[11].length()-2);
			sprintf(insert_str, "%s,quote_currency=%%27%s%%27", insert_str, exchange_data[11].c_str());
		}
		if(exchange_data[12] != "null")
		{
			exchange_data[12] = exchange_data[12].substr(1, exchange_data[12].length()-2);
			sprintf(insert_str, "%s,settlement_currency=%%27%s%%27", insert_str, exchange_data[12].c_str());
		}
		sprintf(insert_str, "%s,update_user=USER()", insert_str);

		printf("=============insert=============\n%s\n", insert_str);

		sprintf(update_order_str, "https://127.0.0.1:2012/mysql/?query=update%%20bitmex_order_history%%20set%%20remaining_qty=%%27%s%%27,order_status=%%27%s%%27", exchange_data[13].c_str(), exchange_data[14].c_str());

		if(exchange_data[4] != "null")
			sprintf(update_order_str, "%s,match_price=%%27%s%%27", update_order_str, exchange_data[4].c_str());

		if(exchange_data[5] != "null")
			sprintf(update_order_str, "%s,match_qty=%%27%s%%27", update_order_str, exchange_data[5].c_str());

		sprintf(update_order_str, "%s,update_user=USER()", update_order_str);
		sprintf(update_order_str, "%s%%20where%%20order_no=%%27%s%%27", update_order_str, exchange_data[10].c_str());

		printf("=============update order==========\n%s\n=============\n", update_order_str);

		CURL *curl = curl_easy_init();
		curl_easy_setopt(curl, CURLOPT_URL, update_order_str);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, getResponse);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
		res = curl_easy_perform(curl);

		if(res != CURLE_OK)
			fprintf(stderr, "CVReplyWS:FTX_Update:curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

		for(int i=0 ; i<strlen(insert_str) ; i++)
		{
			if(insert_str[i] == ' ')
				insert_str[i] = '+';
		}
#if 0
		for(int i=0 ; i<strlen(update_match_str) ; i++)
		{
			if(update_match_str[i] == ' ')
				update_match_str[i] = '+';
		}
#endif
		for(int i=0 ; i<strlen(update_order_str) ; i++)
		{
			if(update_order_str[i] == ' ')
				update_order_str[i] = '+';
		}
		printf("=============insert:\n%s\n=============\n", insert_str);
		curl_easy_setopt(curl, CURLOPT_URL, insert_str);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, getResponse);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
		res = curl_easy_perform(curl);
		curl_easy_setopt(curl, CURLOPT_URL, insert_str);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, getResponse);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
		res = curl_easy_perform(curl);

		if(res != CURLE_OK)
			fprintf(stderr, "CVReplyWS:FTX_Update:curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

		curl_easy_cleanup(curl);

		//if(exchange_data[8] == "Trade")
		{
			memset(&m_trade_reply, 0, sizeof(m_trade_reply));

			string text = (*jtable)["data"]["error"].dump();
			if(text != "null")
			{
				memcpy(m_trade_reply.status_code, "1001", 4);
				sprintf(m_trade_reply.reply_msg, "reply fail, error message - [%s]", text.c_str());
				memcpy(m_trade_reply.bookno, (*jtable)["data"]["orderID"].dump().c_str()+1, 36);
				printf("[CVReply ErrMsg] %s\n", m_trade_reply.reply_msg);
			}
			else
			{
				memcpy(m_trade_reply.status_code, "1000", 4);
				memcpy(m_trade_reply.price,         (*jtable)["data"]["price"].dump().c_str(),      (*jtable)["data"]["price"].dump().length());

				if((*jtable)["data"]["avgPx"].dump() == "null")
					memcpy(m_trade_reply.avgPx, (*jtable)["data"]["stopPx"].dump().c_str(),         (*jtable)["data"]["stopPx"].dump().length());
				else
					memcpy(m_trade_reply.avgPx, (*jtable)["data"]["avgPx"].dump().c_str(),          (*jtable)["data"]["avgPx"].dump().length());
				memcpy(m_trade_reply.orderQty,      (*jtable)["data"]["orderQty"].dump().c_str(),       (*jtable)["data"]["orderQty"].dump().length());
				memcpy(m_trade_reply.lastQty,       (*jtable)["data"]["lastQty"].dump().c_str(),        (*jtable)["data"]["lastQty"].dump().length());
				memcpy(m_trade_reply.cumQty,        (*jtable)["data"]["cumQty"].dump().c_str(),         (*jtable)["data"]["cumQty"].dump().length());
				memcpy(m_trade_reply.key_id,        (*jtable)["data"]["clOrdID"].dump().c_str()+1,      (*jtable)["data"]["clOrdID"].dump().length()-2);
				memcpy(m_trade_reply.bookno,        (*jtable)["data"]["orderID"].dump().c_str()+1, 36);
				memcpy(m_trade_reply.transactTime,  (*jtable)["data"]["transactTime"].dump().c_str()+1, (*jtable)["data"]["transactTime"].dump().length()-2);
				memcpy(m_trade_reply.symbol,		(*jtable)["data"]["symbol"].dump().c_str()+1,	(*jtable)["data"]["symbol"].dump().length()-2);
				memcpy(m_trade_reply.buysell,		(*jtable)["data"]["side"].dump().c_str()+1, 1);

				if((*jtable)["data"]["execComm"].dump() != "null")
					memcpy(m_trade_reply.commission,	(*jtable)["data"]["execComm"].dump().c_str(), (*jtable)["data"]["execComm"].dump().length());

				sprintf(m_trade_reply.reply_msg,    "trade reply - [%s, (%s/%s)]",
								    (*jtable)["data"]["text"].dump().c_str(), m_trade_reply.cumQty, m_trade_reply.orderQty);
				sprintf(m_trade_reply.exchange_name, "BITMEX");
				CCVQueueDAO* pQueueDAO = CCVQueueDAOs::GetInstance()->GetDAO();
				m_trade_reply.trail[0] = '\r';
				m_trade_reply.trail[1] = '\n';
				pQueueDAO->SendData((char*)&m_trade_reply, sizeof(m_trade_reply));
			}
		}//trade
	}//fills

}
#endif

void CCVServer::OnData_Order_Reply(client* c, websocketpp::connection_hdl con, client::message_ptr msg)
{
	static char netmsg[BUFFERSIZE];
	static char timemsg[9];
	string symbol_str;
	string strname = "ORDER_REPLY";
	string str = msg->get_payload();
	static CCVServer* pServer = CCVServers::GetInstance()->GetServerByName(strname);
	pServer->m_heartbeat_count = 0;
	pServer->m_pHeartbeat->TriggerGetReplyEvent();

	if(str[0] == '{') {
		json jtable = json::parse(str.c_str());

		if(jtable["table"] != "margin" && jtable["table"] != "position") {
			if(jtable["exchange"] == "BINANCE") {
				pServer->Binance_Update(&jtable);
			}
			else if(jtable["exchange"] == "BITMEX") {
				cout << setw(4) << jtable << endl;
				pServer->Bitmex_Update(&jtable);
			}
			else if(jtable["exchange"] == "FTX") {
				cout << setw(4) << jtable << endl;
				pServer->FTX_Update(&jtable);
			}
			else {
				FprintfStderrLog("[Unknown]", -1, 0, jtable["exchange"].dump().c_str(), jtable["exchange"].dump().length(),  NULL, 0);
			}
		}
			
	}

}

void CCVServer::OnHeartbeatLost()
{
	FprintfStderrLog("HEARTBEAT LOST", -1, 0, m_strName.c_str(), m_strName.length(),  NULL, 0);
	exit(-1);
	SetStatus(ssBreakdown);
}

void CCVServer::OnHeartbeatRequest()
{
	FprintfStderrLog("HEARTBEAT REQUEST", -1, 0, m_strName.c_str(), m_strName.length(),  NULL, 0);
	char replymsg[BUFFERSIZE];
	memset(replymsg, 0, BUFFERSIZE);

	if(m_heartbeat_count <= HTBT_COUNT_LIMIT)
	{
		auto msg = m_conn->send("ping");
		sprintf(replymsg, "%s send PING message and response (%s)\n", m_strName.c_str(), msg.message().c_str());
		FprintfStderrLog("PING/PONG protocol", -1, 0, replymsg, strlen(replymsg),  NULL, 0);

		if(msg.message() != "SUCCESS" && msg.message() != "Success")
		{
			FprintfStderrLog("Server PING/PONG Fail", -1, 0, m_strName.c_str(), m_strName.length(),  NULL, 0);
			exit(-1);
			SetStatus(ssBreakdown);
		}
		else
		{
			m_heartbeat_count++;
		}
	}
	else
	{
		exit(-1);
		SetStatus(ssBreakdown);
	}
}

void CCVServer::OnHeartbeatError(int nData, const char* pErrorMessage)
{
	exit(-1);
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
		printf("[ERROR] Socket fail.\n");
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

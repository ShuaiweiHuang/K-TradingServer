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

extern void FprintfStderrLog(const char *pCause, int nError, int nData, const char *pFile = NULL, int nLine = 0, unsigned char *pMessage1 = NULL,
                             int nMessage1Length = 0, unsigned char *pMessage2 = NULL, int nMessage2Length = 0);

CCVServer::CCVServer(string strWeb, string strQstr, string strName, TCVRequestMarket rmRequestMarket) {

	m_shpClient = NULL;
	m_pHeartbeat = NULL;

	m_strWeb = strWeb;
	m_strQstr = strQstr;
	m_strName = strName;
	m_ssServerStatus = ssNone;

	m_rmRequestMarket = rmRequestMarket;
	pthread_mutex_init(&m_pmtxServerStatusLock, NULL);

	try {
		m_pClientSocket = new CCVClientSocket(this);
		m_pClientSocket->Connect(m_strWeb, m_strQstr, m_strName, CONNECT_WEBSOCK); // start
	}
	catch (exception &e) {

		FprintfStderrLog("SERVER_NEW_SOCKET_ERROR", -1, 0, NULL, 0, (unsigned char *)e.what(), strlen(e.what()));
	}
}

CCVServer::~CCVServer() {

	m_shpClient = NULL;

	if (m_pClientSocket) {

		SetStatus(ssBreakdown);
		delete m_pClientSocket;
		m_pClientSocket = NULL;
	}

	if (m_pHeartbeat) {

		delete m_pHeartbeat;
		m_pHeartbeat = NULL;
	}

	pthread_mutex_destroy(&m_pmtxServerStatusLock);
}

void *CCVServer::Run() {

	sprintf(m_caPthread_ID, "%020lu", Self());

	CCVClients *pClients = NULL;
	try {
		pClients = CCVClients::GetInstance();

		if (pClients == NULL)
			throw "GET_CLIENTS_ERROR";
	}
	catch (const char *pErrorMessage) {
		FprintfStderrLog(pErrorMessage, -1, 0, __FILE__, __LINE__);
		return NULL;
	}
	while (m_ssServerStatus != ssBreakdown) {
		try {
			SetStatus(ssNone);
			m_cfd.run();
			exit(-1);
			SetStatus(ssBreakdown);
			break;
		}
		catch (exception &e) {
			break;
			;
		}
	}
	return NULL;
}

void CCVServer::OnConnect() {

	unsigned char msg[BUFFERSIZE];

	if (m_ssServerStatus == ssNone) {

		try {
			m_cfd.set_access_channels(websocketpp::log::alevel::all);
			m_cfd.clear_access_channels(websocketpp::log::alevel::frame_payload | websocketpp::log::alevel::frame_header |
			                            websocketpp::log::alevel::control);
			m_cfd.set_error_channels(websocketpp::log::elevel::all);

			m_cfd.init_asio();

			try {
				m_pHeartbeat = new CCVHeartbeat(this);
				memset(msg, BUFFERSIZE, 0);
				sprintf((char *)msg, "add heartbeat in %s service.", m_strName.c_str());
				FprintfStderrLog("NEW_HEARTBEAT_CREATE", -1, 0, __FILE__, __LINE__, msg, strlen((char *)msg));
				m_heartbeat_count = 0;
			}
			catch (exception &e) {

				FprintfStderrLog("NEW_HEARTBEAT_ERROR", -1, 0, __FILE__, __LINE__, (unsigned char *)m_caPthread_ID, sizeof(m_caPthread_ID),
				                 (unsigned char *)e.what(), strlen(e.what()));
				SetStatus(ssBreakdown);
				return;
			}

			if (m_strName == "ORDER_REPLY") {

				sprintf((char *)msg, "set timer to %d sec.", HEARTBEAT_INTERVAL_MIN);
				FprintfStderrLog("HEARTBEAT_TIMER_CONFIG", -1, 0, __FILE__, __LINE__, msg, strlen((char *)msg));
				m_pHeartbeat->SetTimeInterval(HEARTBEAT_INTERVAL_MIN);
				m_cfd.set_message_handler(bind(&OnData_Order_Reply, &m_cfd, ::_1, ::_2));
			} else {
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
		}
		catch (websocketpp::exception const &ecp) {

			cout << ecp.what() << endl;
		}

		Start();
	} else if (m_ssServerStatus == ssReconnecting) {

		if (m_strName == "ORDER_REPLY") {

			m_cfd.set_message_handler(bind(&OnData_Order_Reply, &m_cfd, ::_1, ::_2));
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

		if (m_pHeartbeat) {

			m_pHeartbeat->TriggerGetReplyEvent(); // reset heartbeat
		} else {
			FprintfStderrLog("HEARTBEAT_NULL_ERROR", -1, 0, __FILE__, __LINE__, (unsigned char *)m_caPthread_ID, sizeof(m_caPthread_ID));
		}
		SetStatus(ssNone);
		FprintfStderrLog("RECONNECT_SUCCESS", 0, 0, NULL, 0, (unsigned char *)m_caPthread_ID, sizeof(m_caPthread_ID));
	} else {
		FprintfStderrLog("SERVER_STATUS_ERROR", -1, m_ssServerStatus, __FILE__, __LINE__, (unsigned char *)m_caPthread_ID, sizeof(m_caPthread_ID));
	}
}

void CCVServer::OnDisconnect() {

	sleep(5);
	m_pClientSocket->Connect(m_strWeb, m_strQstr, m_strName, CONNECT_WEBSOCK); // start & reset heartbeat
}

void CCVServer::Transaction_Update(json *jtable) {

	char update_match_str[BUFFERSIZE], insert_str[BUFFERSIZE], update_order_str[BUFFERSIZE];
	string response, exchange_data[30];
	CURLcode res;

	if ((*jtable)["table"] == "execution") {

		for (int i = 0; i < (*jtable)["data"].size(); i++) {

			if (((*jtable)["data"][i]["cv_data_type"].dump()) != "\"match\"")
				continue;

			exchange_data[0] = ((*jtable)["data"][i]["account"].dump());
			exchange_data[0] = exchange_data[0].substr(0, exchange_data[0].length());

			exchange_data[1] = ((*jtable)["data"][i]["execID"].dump());
			exchange_data[1] = exchange_data[1].substr(1, exchange_data[1].length() - 2);

			exchange_data[2] = ((*jtable)["data"][i]["symbol"].dump());
			exchange_data[2] = exchange_data[2].substr(1, exchange_data[2].length() - 2);

			exchange_data[3] = ((*jtable)["data"][i]["side"].dump());
			exchange_data[3] = exchange_data[3].substr(1, exchange_data[3].length() - 2);

			exchange_data[4] = ((*jtable)["data"][i]["lastPx"].dump());
			exchange_data[4] = exchange_data[4].substr(0, exchange_data[4].length());

			exchange_data[5] = ((*jtable)["data"][i]["lastQty"].dump());
			exchange_data[5] = exchange_data[5].substr(0, exchange_data[5].length());

			exchange_data[6] = ((*jtable)["data"][i]["cumQty"].dump());
			exchange_data[6] = exchange_data[6].substr(0, exchange_data[6].length());

			exchange_data[7] = ((*jtable)["data"][i]["leavesQty"].dump());
			exchange_data[7] = exchange_data[7].substr(0, exchange_data[7].length());

			exchange_data[8] = ((*jtable)["data"][i]["execType"].dump());
			exchange_data[8] = exchange_data[8].substr(1, exchange_data[8].length() - 2);

			exchange_data[9] = ((*jtable)["data"][i]["cv_transactTime"].dump());
			exchange_data[9] = exchange_data[9].substr(1, exchange_data[9].length() - 2);

			exchange_data[10] = ((*jtable)["data"][i]["commission"].dump());
			exchange_data[10] = exchange_data[10].substr(0, exchange_data[10].length());

			exchange_data[11] = ((*jtable)["data"][i]["execComm"].dump());
			exchange_data[11] = exchange_data[11].substr(0, exchange_data[11].length());

			exchange_data[12] = ((*jtable)["data"][i]["orderID"].dump());
			exchange_data[12] = exchange_data[12].substr(1, exchange_data[12].length() - 2);

			exchange_data[13] = ((*jtable)["data"][i]["price"].dump());
			exchange_data[13] = exchange_data[13].substr(0, exchange_data[13].length());

			exchange_data[14] = ((*jtable)["data"][i]["orderQty"].dump());
			exchange_data[14] = exchange_data[14].substr(0, exchange_data[14].length());

			exchange_data[15] = ((*jtable)["data"][i]["ordType"].dump());
			exchange_data[15] = exchange_data[15].substr(1, exchange_data[15].length() - 2);

			exchange_data[16] = ((*jtable)["data"][i]["ordStatus"].dump());
			exchange_data[16] = exchange_data[16].substr(1, exchange_data[16].length() - 2);

			exchange_data[17] = ((*jtable)["data"][i]["currency"].dump());
			exchange_data[17] = exchange_data[17].substr(1, exchange_data[17].length() - 2);

			exchange_data[18] = ((*jtable)["data"][i]["settlCurrency"].dump());
			exchange_data[18] = exchange_data[18].substr(1, exchange_data[18].length() - 2);

			exchange_data[19] = ((*jtable)["data"][i]["clOrdID"].dump());
			exchange_data[19] = exchange_data[19].substr(1, exchange_data[19].length() - 2);

			exchange_data[20] = ((*jtable)["data"][i]["text"].dump());
			exchange_data[20] = exchange_data[20].substr(1, exchange_data[20].length() - 2);

			memset(&m_trade_reply, 0, sizeof(m_trade_reply));

			string text = (*jtable)["data"][i]["error"].dump();

			if (text != "null") {

				memcpy(m_trade_reply.status_code, "1001", 4);
				sprintf(m_trade_reply.reply_msg, "reply fail, error message - [%s]", text.c_str());
				memcpy(m_trade_reply.bookno, (*jtable)["data"][i]["orderID"].dump().c_str() + 1, 36);
				printf("[CVReply ErrMsg] %s\n", m_trade_reply.reply_msg);
			} else {

				memcpy(m_trade_reply.status_code, "1000", 4);
				memcpy(m_trade_reply.price, (*jtable)["data"][i]["price"].dump().c_str(), (*jtable)["data"][i]["price"].dump().length());

				if ((*jtable)["data"][i]["avgPx"].dump() == "null")
					memcpy(m_trade_reply.avgPx, (*jtable)["data"][i]["stopPx"].dump().c_str(),
					       (*jtable)["data"][i]["stopPx"].dump().length());
				else
					memcpy(m_trade_reply.avgPx, (*jtable)["data"][i]["avgPx"].dump().c_str(),
					       (*jtable)["data"][i]["avgPx"].dump().length());

				memcpy(m_trade_reply.orderQty, (*jtable)["data"][i]["orderQty"].dump().c_str(),
				       (*jtable)["data"][i]["orderQty"].dump().length());
				memcpy(m_trade_reply.lastQty, (*jtable)["data"][i]["lastQty"].dump().c_str(), (*jtable)["data"][i]["lastQty"].dump().length());
				memcpy(m_trade_reply.cumQty, (*jtable)["data"][i]["cumQty"].dump().c_str(), (*jtable)["data"][i]["cumQty"].dump().length());
				memcpy(m_trade_reply.key_id, (*jtable)["data"][i]["clOrdID"].dump().c_str() + 1,
				       (*jtable)["data"][i]["clOrdID"].dump().length() - 2);
				memcpy(m_trade_reply.bookno, (*jtable)["data"][i]["orderID"].dump().c_str()+1, (*jtable)["data"][i]["orderID"].dump().length()-2);
				memcpy(m_trade_reply.transactTime, (*jtable)["data"][i]["cv_transactTime"].dump().c_str() + 1,
				       (*jtable)["data"][i]["cv_transactTime"].dump().length() - 2);
				memcpy(m_trade_reply.symbol, (*jtable)["data"][i]["symbol"].dump().c_str() + 1,
				       (*jtable)["data"][i]["symbol"].dump().length() - 2);
				memcpy(m_trade_reply.buysell, (*jtable)["data"][i]["side"].dump().c_str() + 1, 1);

				if ((*jtable)["data"][i]["execComm"].dump() != "null")
					memcpy(m_trade_reply.commission, (*jtable)["data"][i]["execComm"].dump().c_str(),
					       (*jtable)["data"][i]["execComm"].dump().length());

				sprintf(m_trade_reply.reply_msg, "transaction reply - [%s, (%s/%s)]", (*jtable)["data"][i]["text"].dump().c_str(),
				        m_trade_reply.cumQty, m_trade_reply.orderQty);

				sprintf(m_trade_reply.exchange_name, (*jtable)["cv_exchange"].dump().c_str());

				CCVQueueDAO *pQueueDAO = CCVQueueDAOs::GetInstance()->GetDAO();

				m_trade_reply.trail[0] = '\r';
				m_trade_reply.trail[1] = '\n';
				pQueueDAO->SendData((char *)&m_trade_reply, sizeof(m_trade_reply));
			}
		} // for
	}         // execution
}

void CCVServer::OnData_Order_Reply(client *c, websocketpp::connection_hdl con, client::message_ptr msg) {

	static char netmsg[BUFFERSIZE];
	static char timemsg[9];
	string symbol_str;
	string strname = "ORDER_REPLY";
	string str = msg->get_payload();
	static CCVServer *pServer = CCVServers::GetInstance()->GetServerByName(strname);
	pServer->m_heartbeat_count = 0;
	pServer->m_pHeartbeat->TriggerGetReplyEvent();

	if (str[0] == '{') {

		json jtable = json::parse(str.c_str());
		if (jtable["table"] != "margin" && jtable["table"] != "position" && jtable["cv_exchange"].dump() != "null") {

			if (jtable["cv_exchange"] == "BINANCE" || jtable["cv_exchange"] == "BITMEX" || jtable["cv_exchange"] == "FTX" ||
			    jtable["cv_exchange"] == "BYBIT") {

				pServer->Transaction_Update(&jtable);

			} else {
				FprintfStderrLog("[UnknownExchange]", -1, 0, jtable["cv_exchange"].dump().c_str(), jtable["cv_exchange"].dump().length(), NULL,
				                 0);
			}
		}
	}
}

void CCVServer::OnHeartbeatLost() {

	FprintfStderrLog("HEARTBEAT LOST", -1, 0, m_strName.c_str(), m_strName.length(), NULL, 0);
	exit(-1);
	SetStatus(ssBreakdown);
}

void CCVServer::OnHeartbeatRequest() {

	FprintfStderrLog("HEARTBEAT REQUEST", -1, 0, m_strName.c_str(), m_strName.length(), NULL, 0);
	char replymsg[BUFFERSIZE];
	memset(replymsg, 0, BUFFERSIZE);

	if (m_heartbeat_count <= HTBT_COUNT_LIMIT) {

		auto msg = m_conn->send("ping");
		sprintf(replymsg, "%s send PING message and response (%s)\n", m_strName.c_str(), msg.message().c_str());
		FprintfStderrLog("PING/PONG protocol", -1, 0, replymsg, strlen(replymsg), NULL, 0);

		if (msg.message() != "SUCCESS" && msg.message() != "Success") {

			FprintfStderrLog("Server PING/PONG Fail", -1, 0, m_strName.c_str(), m_strName.length(), NULL, 0);
			exit(-1);
			SetStatus(ssBreakdown);
		} else {
			m_heartbeat_count++;
		}
	} else {
		exit(-1);
		SetStatus(ssBreakdown);
	}
}

void CCVServer::OnHeartbeatError(int nData, const char *pErrorMessage) {
	exit(-1);
	SetStatus(ssBreakdown);
}

bool CCVServer::RecvAll(const char *pWhat, unsigned char *pBuf, int nToRecv) { return false; }

bool CCVServer::SendAll(const char *pWhat, const unsigned char *pBuf, int nToSend) { return false; }

void CCVServer::ReconnectSocket() {

	if (m_pClientSocket) {

		sleep(5);
		SetStatus(ssReconnecting);
		m_pClientSocket->Connect(m_strWeb, m_strQstr, m_strName, CONNECT_WEBSOCK); // start
	} else {
		printf("[ERROR] Socket fail.\n");
		SetStatus(ssBreakdown);
	}
}

void CCVServer::SetCallback(shared_ptr<CCVClient> &shpClient) { m_shpClient = shpClient; }

void CCVServer::SetRequestMessage(unsigned char *pRequestMessage, int nRequestMessageLength) {
	// TODO
}

void CCVServer::SetStatus(TCVServerStatus ssServerStatus) {

	pthread_mutex_lock(&m_pmtxServerStatusLock);

	m_ssServerStatus = ssServerStatus;

	pthread_mutex_unlock(&m_pmtxServerStatusLock);
}

TCVServerStatus CCVServer::GetStatus() { return m_ssServerStatus; }

context_ptr CCVServer::CB_TLS_Init(const char *hostname, websocketpp::connection_hdl) {

	context_ptr ctx = websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12);
	return ctx;
}

void CCVServer::OnRequest() {
	// TODO
}

void CCVServer::OnRequestError(int, const char *) {
	// TODO
}

void CCVServer::OnData(unsigned char *, int) {
	// TOOD
}

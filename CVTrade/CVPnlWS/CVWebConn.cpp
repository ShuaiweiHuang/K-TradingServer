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

			if (m_strName == "ORDER_RTPNL") {

				sprintf((char *)msg, "set timer to %d sec.", HEARTBEAT_INTERVAL_MIN);
				FprintfStderrLog("HEARTBEAT_TIMER_CONFIG", -1, 0, __FILE__, __LINE__, msg, strlen((char *)msg));
				m_pHeartbeat->SetTimeInterval(HEARTBEAT_INTERVAL_MIN);
				m_cfd.set_message_handler(bind(&OnData_Order_Pnl, &m_cfd, ::_1, ::_2));
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

		if (m_strName == "ORDER_RTPNL") {

			m_cfd.set_message_handler(bind(&OnData_Order_Pnl, &m_cfd, ::_1, ::_2));
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

void CCVServer::Position_Update(json *jtable) {

	char update_match_str[BUFFERSIZE], insert_str[BUFFERSIZE], update_order_str[BUFFERSIZE];
	string response, accounting_no, strategy, position, symbol, exchange;
	CURLcode res;

	for (int i = 0; i < (*jtable)["data"].size(); i++)
	{
		accounting_no = ((*jtable)["data"][i]["accounting_no"].dump());
		accounting_no = accounting_no.substr(1, accounting_no.length() - 2);
		strategy = ((*jtable)["data"][i]["strategy"].dump());
		strategy = accounting_no.substr(1, strategy.length() - 2);
		symbol = ((*jtable)["data"][i]["symbol"].dump());
		symbol = symbol.substr(1, symbol.length() - 2);
		exchange = ((*jtable)["data"][i]["exchange"].dump());
		exchange = exchange.substr(1, exchange.length() - 2);
		position = ((*jtable)["data"][i]["position"].dump());

		memset(&m_pnl_reply, 0, sizeof(m_pnl_reply));
		memcpy(m_pnl_reply.status_code, "9999", 4);
		memcpy(m_pnl_reply.accounting_no, accounting_no.c_str(), accounting_no.length());
		memcpy(m_pnl_reply.strategy, strategy.c_str(), strategy.length());
		memcpy(m_pnl_reply.position, position.c_str(), position.length());
		memcpy(m_pnl_reply.symbol, symbol.c_str(), symbol.length());
		memcpy(m_pnl_reply.exchange, exchange.c_str(), exchange.length());


		CCVQueueDAO *pQueueDAO = CCVQueueDAOs::GetInstance()->GetDAO();

		m_pnl_reply.trail[0] = '\r';
		m_pnl_reply.trail[1] = '\n';
		pQueueDAO->SendData((char *)&m_pnl_reply, sizeof(m_pnl_reply));
		//printf("%s\n", m_pnl_reply);
	} // for
}

void CCVServer::OnData_Order_Pnl(client *c, websocketpp::connection_hdl con, client::message_ptr msg) {

	static char netmsg[BUFFERSIZE];
	static char timemsg[9];
	string symbol_str;
	string strname = "ORDER_RTPNL";
	string str = msg->get_payload();
	static CCVServer *pServer = CCVServers::GetInstance()->GetServerByName(strname);
	pServer->m_heartbeat_count = 0;
	pServer->m_pHeartbeat->TriggerGetReplyEvent();

	cout << setw(4) << str << endl;

	if (str[0] == '{') {

		json jtable = json::parse(str.c_str());

		cout << setw(4) << jtable << endl;

		if (jtable["cv_subscribe"] == "CV.StrategyPosition") {

			pServer->Position_Update(&jtable);

		} else {

			FprintfStderrLog("[UnknownExchange]", -1, 0, jtable["cv_exchange"].dump().c_str(), jtable["cv_exchange"].dump().length(), NULL,
					 0);
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

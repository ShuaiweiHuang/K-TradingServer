#ifndef CVSERVER_H_
#define CVSERVER_H_

#ifdef _WIN32
#include <Windows.h>
#endif

#include <string>
#include <memory>
#include <iostream>

#include "CVCommon/CVThread.h"
#include "CVCommon/ICVClientSocketCallback.h"
#include "CVCommon/ICVHeartbeatCallback.h"
#include "CVCommon/ICVRequestCallback.h"
#include "CVCommon/CVClientSocket.h"
#include "CVHeartbeat.h"
#include "CVRequest.h"
#include "CVServer.h"

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <nlohmann/json.hpp>

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> context_ptr;

using json = nlohmann::json;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;
using namespace std;

#define MAX_DATA_LENGTH 4096 
#define SYMSIZE 10

class CCVClient;

enum TCVServerStatus
{
	ssNone,
	ssFree,
	ssInuse,
	ssBreakdown,
	ssReconnecting
};

struct CV_StructTSOrderReply
{
	char status_code[4];
	char key_id[13];
	char bookno[36];
	char price[10];
	char avgPx[10];
	char orderQty[10];
	char lastQty[10];
	char cumQty[10];
	char transactTime[24];
	char reply_msg[129];
	char symbol[10];
	char buysell[1];
	char exchange_name[10];
	char commission[10];
	char trail[2];
};

using namespace std;

class CCVServer: public CCVThread, public ICVClientSocketCallback, public ICVHeartbeatCallback, public ICVRequestCallback
{
	private:
		friend void CCVClient:: TriggerSendRequestEvent(CCVServer* pServer, unsigned char* pRequestMessage, int nRequestMessageLength);

		client m_cfd;
		client::connection_ptr m_conn;
		char m_caPthread_ID[21];
		shared_ptr<CCVClient> m_shpClient;

		CCVClientSocket* m_pClientSocket;
		CCVHeartbeat* m_pHeartbeat;
		CCVRequest* m_pRequest;


		TCVRequestMarket m_rmRequestMarket;
		unsigned char m_uncaSecondByte;
		int m_nReplyMessageLength;

		unsigned char m_uncaRequestMessage[MAX_DATA_LENGTH];
		int m_nRequestMessageLength;

		int m_nOriginalOrderLength;
		int m_nReplyMsgLength;//for reply_msg[78] reply_msg[80]
		int m_nPoolIndex;
		int m_heartbeat_count;

		pthread_mutex_t m_pmtxServerStatusLock;
		static context_ptr CB_TLS_Init(const char *, websocketpp::connection_hdl);
		static void OnData_Order_Reply(client* c, websocketpp::connection_hdl, client::message_ptr msg);
		void Transaction_Update(json*);

	protected:
		void* Run();

		void OnConnect();
		void OnDisconnect();

		void OnHeartbeatLost();
		void OnHeartbeatRequest();
		void OnHeartbeatError(int nData, const char* pErrorMessage);

		bool RecvAll(const char* pWhat, unsigned char* pBuf, int nToRecv);
		bool SendAll(const char* pWhat, const unsigned char* pBuf, int nToSend);

		void ReconnectSocket();
		void OnRequest();
		void OnRequestError(int, const char*);
		void OnData(unsigned char*, int);
		struct CV_StructTSOrderReply m_trade_reply;

	public:
		TCVServerStatus m_ssServerStatus;
		CCVServer(string strHost, string strPort, string strName, TCVRequestMarket rmRequestMarket);
		virtual ~CCVServer();

		void SetCallback(shared_ptr<CCVClient>& shpClient);
		void SetRequestMessage(unsigned char* pRequestMessage, int nRequestMessageLength);

		void SetStatus(TCVServerStatus ssServerStatus);
		TCVServerStatus GetStatus();
		string m_strWeb;
		string m_strQstr;
		string m_strName;
		string m_strEPID;
};
#endif

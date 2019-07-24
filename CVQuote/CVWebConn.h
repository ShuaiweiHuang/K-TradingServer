#ifndef SKSERVER_H_
#define SKSERVER_H_

#ifdef _WIN32
#include <Windows.h>
#endif

#include <string>
#include <memory>
#include <iostream>

#include "CVCommon/CVThread.h"
#include "CVCommon/ISKClientSocketCallback.h"
#include "CVCommon/ISKHeartbeatCallback.h"
#include "CVCommon/ISKRequestCallback.h"
#include "CVCommon/CVClientSocket.h"
#include "CVHeartbeat.h"
#include "CVRequest.h"
#include "CVServer.h"

#define MAX_DATA_LENGTH 4096 

class CSKClient;

enum TSKServerStatus
{
	ssNone,
	ssFree,
	ssInuse,
	ssBreakdown,
	ssReconnecting
};


using namespace std;

class CSKServer: public CSKThread, public ISKClientSocketCallback, public ISKHeartbeatCallback, public ISKRequestCallback
{
	private:
		friend void CSKClient:: TriggerSendRequestEvent(CSKServer* pServer, unsigned char* pRequestMessage, int nRequestMessageLength);

		char m_caPthread_ID[21];
		shared_ptr<CSKClient> m_shpClient;

		CSKClientSocket* m_pClientSocket;
		CSKHeartbeat* m_pHeartbeat;
		CSKRequest* m_pRequest;

		TSKServerStatus m_ssServerStatus;

		TSKRequestMarket m_rmRequestMarket;
		unsigned char m_uncaSecondByte;
		int m_nReplyMessageLength;

		unsigned char m_uncaRequestMessage[MAX_DATA_LENGTH];
		int m_nRequestMessageLength;

		int m_nOriginalOrderLength;
		int m_nReplyMsgLength;//for reply_msg[78] reply_msg[80]

		int m_nPoolIndex;

		pthread_mutex_t m_pmtxServerStatusLock;
		static context_ptr CB_TLS_Init(const char *, websocketpp::connection_hdl);
		static void OnData_Bitmex(websocketpp::connection_hdl, client::message_ptr msg);
		static void OnData_Binance(websocketpp::connection_hdl, client::message_ptr msg);

	protected:
		void* Run();

		void OnConnect();
		void OnDisconnect();

		void OnHeartbeatLost();
		void OnHeartbeatRequest();
		void OnHeartbeatError(int nData, const char* pErrorMessage);

		void OnRequest();
		void OnRequestError(int nData, const char* pErrorMessage);

		bool RecvAll(const char* pWhat, unsigned char* pBuf, int nToRecv);
		bool SendAll(const char* pWhat, const unsigned char* pBuf, int nToSend);
		void OnData(unsigned char* pBuf, int nSize);

		void ReconnectSocket();

	public:
		CSKServer(string strHost, string strPort, string strName, TSKRequestMarket rmRequestMarket);
		virtual ~CSKServer();

		void SetCallback(shared_ptr<CSKClient>& shpClient);
		void SetRequestMessage(unsigned char* pRequestMessage, int nRequestMessageLength);

		void SetStatus(TSKServerStatus ssServerStatus);
		TSKServerStatus GetStatus();
		string m_strWeb;
		string m_strQstr;
		string m_strName;
		string m_strEPID;
};
#endif

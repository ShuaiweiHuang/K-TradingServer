#ifndef CVCLIENT_H_
#define CVCLIENT_H_

#include <string>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <vector>
#include <map>
#include <memory>
#include <openssl/aes.h>
#include <openssl/rsa.h>
#include <openssl/bn.h>

#include "CVCommon/ICVHeartbeatCallback.h"
#include "CVCommon/CVThread.h"
#include "CVHeartbeat.h"
#include "CVGlobal.h"
#include "Include/CVLogonFormat.h"

class CCVServer;

using namespace std;

union L
{
    unsigned char uncaByte[16];
    short value;
};

struct TCVClientAddrInfo
{
    int nSocket;
	char caIP[IPLEN];
    struct sockaddr_storage ClientAddr;
};

struct TCVAccountRecvBuf
{
	int nRecved;
	unsigned char uncaRecvBuf[BUFFERSIZE];
};

enum TCVClientStauts
{
	csNone,
	csLogoning,
	csOnline,
	csOffline
};

enum TCVRequestMarket
{
	rmBitmex,
	rmNum
};

struct TCVBranchAccountInfo
{
	const char* pMarket;

	int nBranchPosition;
	int nAccountPosition;
	int nSubAccountPosition;

	int nBranchLength;
	int nAccountLength;
	int nSubAccountLength;
};

class CCVClient: public CCVThread, public ICVHeartbeatCallback, public enable_shared_from_this<CCVClient>
{
	private:
		struct TCVClientAddrInfo m_ClientAddrInfo;
		unsigned char m_uncaLogonID[10];


		TCVClientStauts m_csClientStatus;

		int m_nOriginalOrderLength;
		int m_nTSReplyMsgLength;//78
		int m_nTFReplyMsgLength;//80
		int m_nOFReplyMsgLength;//80
		int m_nOSReplyMsgLength;//78
		map<string, string> m_mMarketBranchAccount;

		vector<struct TCVBranchAccountInfo*> m_vBranchAccountInfo;

		pthread_mutex_t m_pmtxClientStatusLock;
		string m_strEPID;
	protected:
		void* Run();

		void OnHeartbeatRequest();
		void OnHeartbeatLost();
		void OnHeartbeatError(int nData, const char* pErrorMessage);

		bool Logon(char* pID, char* pPasswd, struct TCVLogonReply &struLogonReply);
		bool GetAccount(char* pID, char* pAgent, char* pVersion, vector<char*> &vAccountData);

		bool RecvAll(const char* pWhat, unsigned char* pBuf, int nToRecv);

		void InitialBranchAccountInfo();
		bool CheckBranchAccount(TCVRequestMarket rmRequestMarket, unsigned char* pRequstMessage);

	public:
		CCVClient(struct TCVClientAddrInfo &ClientAddrInfo);
		bool SendAll(const char* pWhat, char* pBuf, int nToSend);
		virtual ~CCVClient();

		void TriggerSendRequestEvent(CCVServer* pServer, unsigned char* pRequestMessage, int nRequestMessageLength);

		bool SendLogonReply(struct TCVLogonReply &struLogonReply);
		bool SendAccountCount(short sCount);
		bool SendAccountData(vector<char*> &vAccountData);
		bool SendRequestReply(unsigned char uncaSecondByte, unsigned char* unpRequestReplyMessage, int nRequestReplyMessageLength);
		bool SendRequestErrorReply(unsigned char uncaSecondByte,unsigned char* pOriginalRequstMessage,int nOriginalRequestMessageLength,const char* pErrorMessage,short nErrorCode);

		void SetStatus(TCVClientStauts csStatus);
		TCVClientStauts GetStatus();

		int GetClientSocket();
		CCVHeartbeat* m_pHeartbeat;
};
#endif

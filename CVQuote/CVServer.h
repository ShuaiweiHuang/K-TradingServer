#ifndef SKCLIENT_H_
#define SKCLIENT_H_

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

#include "CVCommon/ISKHeartbeatCallback.h"
#include "CVCommon/CVThread.h"
#include "CVHeartbeat.h"
#include "Include/CVLogonFormat.h"

#define BUFFERSIZE	1024
#define IPLEN		16
#define UIDLEN		10

class CSKServer;

using namespace std;

#ifdef MNTRMSG
#define QUEUESIZE 10
struct LogMesssage
{
	int logon_index;
	int logon_msg_len[QUEUESIZE];
	char logon_msg_buf[QUEUESIZE][BUFFERSIZE];
};
#endif
union L
{
    unsigned char uncaByte[16];
    short value;
};

struct TSKClientAddrInfo
{
    int nSocket;
	char caIP[IPLEN];
    struct sockaddr_storage ClientAddr;
};

struct TSKAccountRecvBuf
{
	int nRecved;
	unsigned char uncaRecvBuf[BUFFERSIZE];
};

enum TSKClientStauts
{
	csNone,
	csLogoning,
	csOnline,
	csOffline
};

enum TSKRequestMarket
{
	rmBitmex,
	rmNum
};

struct TSKBranchAccountInfo
{
	const char* pMarket;

	int nBranchPosition;
	int nAccountPosition;
	int nSubAccountPosition;

	int nBranchLength;
	int nAccountLength;
	int nSubAccountLength;
};

class CSKClient: public CSKThread, public ISKHeartbeatCallback, public enable_shared_from_this<CSKClient>
{
	private:
		struct TSKClientAddrInfo m_ClientAddrInfo;
		unsigned char m_uncaLogonID[10];

		CSKHeartbeat* m_pHeartbeat;

		TSKClientStauts m_csClientStatus;

		int m_nOriginalOrderLength;
		int m_nTSReplyMsgLength;//78
		int m_nTFReplyMsgLength;//80
		int m_nOFReplyMsgLength;//80
		int m_nOSReplyMsgLength;//78
#ifdef MNTRMSG
		char m_userID[UIDLEN];
#endif
		map<string, string> m_mMarketBranchAccount;

		vector<struct TSKBranchAccountInfo*> m_vBranchAccountInfo;

		pthread_mutex_t m_pmtxClientStatusLock;
#ifdef SSLTLS
		SSL* m_ssl;
		BIO* m_accept_bio;
		BIO* m_bio;
#endif
	protected:
		void* Run();

		void OnHeartbeatRequest();
		void OnHeartbeatLost();
		void OnHeartbeatError(int nData, const char* pErrorMessage);

		bool Logon(char* pID, char* pPasswd, struct TSKLogonReply &struLogonReply);
		bool GetAccount(char* pID, char* pAgent, char* pVersion, vector<char*> &vAccountData);

		bool RecvAll(const char* pWhat, unsigned char* pBuf, int nToRecv);

		void InitialBranchAccountInfo();
		bool CheckBranchAccount(TSKRequestMarket rmRequestMarket, unsigned char* pRequstMessage);

	public:
		CSKClient(struct TSKClientAddrInfo &ClientAddrInfo);
		bool SendAll(const char* pWhat, const unsigned char* pBuf, int nToSend);
		virtual ~CSKClient();

		void TriggerSendRequestEvent(CSKServer* pServer, unsigned char* pRequestMessage, int nRequestMessageLength);

		bool SendLogonReply(struct TSKLogonReply &struLogonReply);
		bool SendAccountCount(short sCount);
		bool SendAccountData(vector<char*> &vAccountData);
		bool SendRequestReply(unsigned char uncaSecondByte, unsigned char* unpRequestReplyMessage, int nRequestReplyMessageLength);
		bool SendRequestErrorReply(unsigned char uncaSecondByte, unsigned char* pOriginalRequstMessage, int nOriginalRequestMessageLength, const char* pErrorMessage, short nErrorCode);

		void SetStatus(TSKClientStauts csStatus);
		TSKClientStauts GetStatus();

		int GetClientSocket();
#ifdef SSLTLS
		EVP_PKEY    *generatePrivateKey();
		X509        *generateCertificate(EVP_PKEY *pkey);
		void		init_openssl();
#endif
#ifdef MNTRMSG
		struct LogMesssage m_LogMessageQueue;
#endif
};
#endif

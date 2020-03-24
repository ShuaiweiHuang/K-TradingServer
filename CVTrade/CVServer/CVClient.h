#ifndef CCVCLIENT_H_
#define CCVCLIENT_H_

#include <string>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <vector>
#include <map>
#include <openssl/aes.h>
#include <openssl/rsa.h>
#include <openssl/bn.h>

#include "CVCommon/CVThread.h"
#include "CVHeartbeat.h"

using namespace std;

#define BUFFERSIZE	1024
#define IPLEN		16
#define UIDLEN		10
#define QUEUESIZE	10
#define MAX_TIME_LIMIT	120


struct CV_StructLogon
{
        char header_bit[2];
        char logon_id[20];
        char password[30];
        char source[2];
        char Version[10];
};

struct TCVClientAddrInfo
{
	int nSocket;
	char caIP[IPLEN];
	struct sockaddr_storage ClientAddr;
};

struct CVOriginalOrder
{
	unsigned char uncaBuf[BUFFERSIZE];
};

enum TCVClientStauts
{
	csNone,
	csLogoning,
	csOnline,
	csOffline
};

struct AccountData
{
	string api_id;
	string api_key;
	string broker_id;
	string exchange_name;
	string account;
	string trader_name;
};

struct RiskctlData
{
	int riskctl_limit;
	int riskctl_side_limit;
	int riskctl_side_limit_current;
	int riskctl_cum_limit;
	int riskctl_time_limit;
};

class CCVClient: public CCVThread
{
	private:
		unsigned char m_uncaLogonID[20];
		int m_nLengthOfLogonMessage;
		int m_nLengthOfHeartbeatMessage;
		int m_nLengthOfOrderMessage;
		int m_nLengthOfLogoutMessage;
		int m_nLengthOfLogonReplyMessage;
		int m_nLengthOfAccountNum;
		bool m_bIsProxy;
		friend class CCVHeartbeat;
		struct TCVClientAddrInfo m_ClientAddrInfo;
		TCVClientStauts m_ClientStatus;
		map<long, struct CVOriginalOrder> m_mOriginalOrder;
		map<string, struct AccountData> m_mBranchAccount;
		string m_strService;
		CCVHeartbeat* m_pHeartbeat;
		pthread_mutex_t m_MutexLockOnClientStatus;
		char m_username[20];
#ifdef SSLTLS
		SSL* m_ssl;
		BIO* m_accept_bio;
		BIO* m_bio;
#endif
	protected:
		void* Run();
		bool LogonAuth(char* p_username, char* p_password, struct CV_StructLogonReply &logon_reply);
		void ReplyAccountContents();
		void ReplyAccountNum();

	public:
		void LoadRiskControl(char* p_username);
		CCVClient(struct TCVClientAddrInfo &ClientAddrInfo, string strService);
		TCVClientStauts GetStatus();
		virtual ~CCVClient();
		bool SendData(const unsigned char* pBuf, int nSize);
		bool SendAll(const unsigned char* pBuf, int nSize);
		bool RecvAll(unsigned char* pBuf, int nToRecv);
		void SetStatus(TCVClientStauts csStatus);
		void GetOriginalOrder(long nOrderNumber, int nOrderSize, union CV_ORDER_REPLY &cv_order_reply);
		struct CV_StructLogon m_logon_type;
		int GetClientSocket();
		int m_riskctl_side_limit_current;
		int m_riskctl_time_limit_current;
		int m_order_timestamp[MAX_TIME_LIMIT];
		int m_order_index;
		int HmacEncodeSHA256( const char * key, unsigned int key_length, const char * input, unsigned int input_length, unsigned char * &output, unsigned int &output_length);
		map<string, struct RiskctlData> m_mRiskControl;
		map<string, struct RiskctlData>::iterator m_iter;
#ifdef SSLTLS
		EVP_PKEY *generatePrivateKey();
		X509	*generateCertificate(EVP_PKEY *pkey);
		void	init_openssl();
#endif

};
#endif

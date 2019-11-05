#ifndef CCVCLIENT_H_
#define CCVCLIENT_H_

#include <string>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <vector>
#include <map>

#include "CVCommon/CVThread.h"
#include "CVHeartbeat.h"

using namespace std;

#define BUFFERSIZE	1024
#define IPLEN		16
#define UIDLEN		10
#define QUEUESIZE	10
#ifdef  MNTRMSG

#endif

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
};

struct RiskctlData
{
	int bitmex_limit;
	int bitmex_cum;
	int bitmex_limit_cum;
	float binance_limit;
	float binance_cum;
	float binance_limit_cum;
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
		map<string, struct RiskctlData> m_mRiskControl;
		string m_strService;
		CCVHeartbeat* m_pHeartbeat;
		pthread_mutex_t m_MutexLockOnClientStatus;
		char m_username[20];
	protected:
		void* Run();
		bool LogonAuth(char* p_username, char* p_password, struct CV_StructLogonReply &logon_reply);
		void ReplyAccountContents();
		void ReplyAccountNum();
		void LoadRiskControl(char* p_username);

	public:
		CCVClient(struct TCVClientAddrInfo &ClientAddrInfo, string strService);
		TCVClientStauts GetStatus();
		virtual ~CCVClient();
		bool SendData(const unsigned char* pBuf, int nSize);
		bool SendAll(const unsigned char* pBuf, int nSize);
		bool RecvAll(unsigned char* pBuf, int nToRecv);
		void SetStatus(TCVClientStauts csStatus);
		void GetOriginalOrder(long nOrderNumber, int nOrderSize, union CV_ORDER_REPLY &cv_order_reply);
		int GetClientSocket();
		int HmacEncodeSHA256( const char * key, unsigned int key_length, const char * input, unsigned int input_length, unsigned char * &output, unsigned int &output_length);

};
#endif

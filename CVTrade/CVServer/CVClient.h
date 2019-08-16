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

#ifdef MNTRMSG
#define QUEUESIZE 10
struct LogMesssage
{
	int order_index;
	int reply_index;
	int order_msg_len[QUEUESIZE];
	int reply_msg_len[QUEUESIZE];
	char order_msg_buf[QUEUESIZE][BUFFERSIZE];
	char reply_msg_buf[QUEUESIZE][BUFFERSIZE];
};
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

struct AccountRecvBuf
{
	int nRecved;
	unsigned char uncaRecvBuf[BUFFERSIZE];
};

struct AccountItem
{
	char caItem[128];
};

struct AccountMessage
{
	char caMessage[30];
};

enum TCVClientStauts
{
	csNone,
	csLogoning,
	csOnline,
	csOffline
};

class CCVClient: public CCVThread
{
	private:
		unsigned char m_uncaLogonID[10];
		int m_nLengthOfLogonMessage;
		int m_nLengthOfHeartbeatMessage;
		int m_nLengthOfOrderMessage;
		int m_nLengthOfLogoutMessage;
		int m_nLengthOfAccountNum;
		bool m_bIsProxy;
		friend class CCVHeartbeat;
		struct TCVClientAddrInfo m_ClientAddrInfo;
		TCVClientStauts m_ClientStatus;
		map<long, struct CVOriginalOrder> m_mOriginalOrder;
		map<string, string> m_mBranchAccount;
		string m_strService;
		CCVHeartbeat* m_pHeartbeat;
		pthread_mutex_t m_MutexLockOnClientStatus;
	protected:
		void* Run();
		bool LogonAuth(char* pID, char* pPasswd, struct CV_StructLogonReply &logon_reply);
		void GetAccount(char* pID, char* pAgent, char* pVersion, vector<struct AccountMessage> &vAccountMessage);

	public:
		CCVClient(struct TCVClientAddrInfo &ClientAddrInfo, string strService, bool bIsProxy = false);
		TCVClientStauts GetStatus();
		virtual ~CCVClient();
		bool SendData(const unsigned char* pBuf, int nSize);
		bool SendAll(const unsigned char* pBuf, int nSize);
		bool RecvAll(unsigned char* pBuf, int nToRecv);
		void SetStatus(TCVClientStauts csStatus);
		void GetOriginalOrder(long nOrderNumber, int nOrderSize, union CV_ORDER_REPLY &cv_order_reply);
		int GetClientSocket();
};
#endif

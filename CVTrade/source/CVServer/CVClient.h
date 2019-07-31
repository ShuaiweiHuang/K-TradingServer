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
		friend class CCVHeartbeat;

		struct TCVClientAddrInfo m_ClientAddrInfo;
		TCVClientStauts m_ClientStatus;

		map<long, struct CVOriginalOrder> m_mOriginalOrder;
		map<string, string> m_mBranchAccount;

		string m_strService;

		CCVHeartbeat* m_pHeartbeat;

		int m_nLengthOfLogonMessage;
		int m_nLengthOfHeartbeatMessage;
		int m_nLengthOfOrderMessage;

		pthread_mutex_t m_MutexLockOnClientStatus;

		bool m_bIsProxy;
#ifdef MNTRMSG
		char m_userID[UIDLEN];
#endif
	protected:
		void* Run();
		bool Logon(char* pID, char* pPasswd, struct LogonReplyType &logon_reply);
		void GetAccount(char* pID, char* pAgent, char* pVersion, vector<struct AccountMessage> &vAccountMessage);

	public:
		CCVClient(struct TCVClientAddrInfo &ClientAddrInfo, string strService, bool bIsProxy = false);
		virtual ~CCVClient();

		bool SendData(const unsigned char* pBuf, int nSize);
		bool SendAll(const unsigned char* pBuf, int nSize);

		void SetStatus(TCVClientStauts csStatus);
		TCVClientStauts GetStatus();
#ifdef MNTRMSG
		struct LogMesssage m_LogMessageQueue;
#endif

		void GetOriginalOrder(long nOrderNumber, int nOrderSize, union CV_REPLY	&sk_reply);

		int GetClientSocket();

		int IsMessageComplete(unsigned char* pBuf, int nRecvedSize);
};
#endif

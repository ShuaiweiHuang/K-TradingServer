#ifndef CVTANDEMDAO_H_
#define CVTANDEMDAO_H_

#include <vector>

#include "CVCommon/CVThread.h"
#include "CVNet/CVSocket.h"
#include "CVInterface/ICVSocketCallback.h"
#include "CVHeartbeat.h"
#include "CVWriteQueueDAOs.h"
#include "../include/CVType.h"
#include "../include/CVGlobal.h"
#include <nlohmann/json.hpp>
#include <iomanip>

using json = nlohmann::json;

class CCVHeartbeat;
class CCVReply;

struct HEADRESP
{
	string remain;
	string epoch;
};

struct APIKEY
{
	string api_id;
	string api_secret;
};

enum TCVReplyDAOStatus
{
	tsNone,
	tsServiceOff,
	tsServiceOn,
	tsBreakdown,
	tsReconnecting,
	tsMsgReady,
};

class CCVReplyDAO : public CCVThread
{
private:
	friend CCVHeartbeat;
	friend CCVWriteQueueDAOs;

	vector<struct APIKEY> m_vApikeyTable_Bitmex;
	vector<struct APIKEY> m_vApikeyTable_Binance;

	CCVSocket* m_pSocket;
	CCVHeartbeat* m_pHeartbeat;

	string m_strHost;
	string m_strPort;
	string m_request_remain;
	string m_time_limit;
	TCVReplyDAOStatus m_TandemDAOStatus;
	CCVWriteQueueDAOs* m_pWriteQueueDAOs;
	CCVReply* m_pTandem;
	pthread_mutex_t m_MutexLockOnSetStatus;

	unsigned int m_nTandemNodeIndex;
	int  m_numberOfkey_Bitmex;
	int  m_numberOfkey_Binance;
	bool m_bInuse;
	char m_caTandemDAOID[4];
	struct CV_StructTSOrderReply m_trade_reply[MAXHISTORY];

private:
	char* GetReplyMsgOffsetPointer(const unsigned char *pMessageBuf);
	bool LogTranReplyDB_Bitmex(json*);
	bool UpdateOrderReplyDB_Binance(json*);

protected:
	void* Run();

public:
	CCVReplyDAO();
	CCVReplyDAO(int nTandemDAOID, int nNumberOfWriteQueueDAO, key_t kWriteQueueDAOStartKey, key_t kWriteQueueDAOEndKey);
	int HmacEncodeSHA256( const char * key, unsigned int key_length, const char * input, unsigned int input_length, unsigned char * &output, unsigned int &output_length);
	virtual ~CCVReplyDAO();

	TCVReplyDAOStatus GetStatus();
	void SetStatus(TCVReplyDAOStatus tsStatus);
	void RestoreStatus();
	void SendLogout();
	void Bitmex_Transaction_Update(int, string, struct APIKEY);
	void Bitmex_Order_Update(int, string, struct APIKEY);
	void Binance_Transaction_Update(int, string, struct APIKEY);
	void Binance_Order_Update(int, string, struct APIKEY);
	bool Bitmex_Getkey();
	bool Binance_Getkey();
	void SendNotify(char* pBuf);
	void ReconnectSocket();
	void CloseSocket();
	void SetInuse(bool bInuse);
	bool IsInuse();
};
#endif

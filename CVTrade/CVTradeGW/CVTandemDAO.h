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
class CCVTandem;

struct HEADRESP
{
	string remain;
	string epoch;
};

enum TYPE_SELECT
{
	OPT_ADD,
	OPT_DELETE
};

enum TCVTandemDAOStatus
{
	tsNone,
	tsServiceOff,
	tsServiceOn,
	tsBreakdown,
	tsReconnecting,
	tsMsgReady,
};

class CCVTandemDAO : public CCVThread
{
private:
	friend CCVHeartbeat;
	friend CCVWriteQueueDAOs;

	CCVSocket* m_pSocket;
	CCVHeartbeat* m_pHeartbeat;

	string m_strHost;
	string m_strPort;
	string m_request_remain;
	string m_time_limit;
	TCVTandemDAOStatus m_TandemDAOStatus;
	CCVWriteQueueDAOs* m_pWriteQueueDAOs;
	CCVTandem* m_pTandem;
	pthread_mutex_t m_MutexLockOnSetStatus;

	unsigned int m_nTandemNodeIndex;
	bool m_bInuse;
	char m_caTandemDAOID[4];
	struct CV_StructTSOrderReply m_tandem_reply;
	struct CV_StructTSOrderReply m_trade_reply[MAXHISTORY];
private:
	char* GetReplyMsgOffsetPointer(const unsigned char *pMessageBuf);

protected:
	void* Run();

public:
	CCVTandemDAO();
	CCVTandemDAO(int nTandemDAOID, int nNumberOfWriteQueueDAO, key_t kWriteQueueDAOStartKey, key_t kWriteQueueDAOEndKey, key_t kQueueDAOMonitorKey);
	int HmacEncodeSHA256( const char * key, unsigned int key_length, const char * input, unsigned int input_length, unsigned char * &output, unsigned int &output_length);
	virtual ~CCVTandemDAO();

	bool SendOrder(const unsigned char* pBuf, int nSize);
	bool OrderSubmit(const unsigned char* pBuf, int nSize);
	bool OrderSubmit_Bitmex(struct CV_StructTSOrder cv_ts_order, int nSize, int nSilent);
	bool OrderModify_Bitmex(struct CV_StructTSOrder cv_ts_order, int nSize);
	bool OCOSubmit_Bitmex(struct CV_StructTSOrder cv_ts_order, int nSize);
	bool OrderSubmit_Binance(struct CV_StructTSOrder cv_ts_order, int nSize);
	bool LogOrderReplyDB_Bitmex(json* jtable, struct CV_StructTSOrder* cv_ts_order, int option);
	bool LogOrderReplyDB_Binance(json* jtable, struct CV_StructTSOrder* cv_ts_order, int option);

	TCVTandemDAOStatus GetStatus();
	void SetStatus(TCVTandemDAOStatus tsStatus);
	void RestoreStatus();
	void SendLogout();
	void SendNotify(char* pBuf);

	void ReconnectSocket();
	void CloseSocket();

	void SetInuse(bool bInuse);
	bool IsInuse();
};
#endif

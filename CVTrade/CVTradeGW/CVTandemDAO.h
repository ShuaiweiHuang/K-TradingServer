#ifndef SKTANDEMDAO_H_
#define SKTANDEMDAO_H_

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

class CSKHeartbeat;
class CSKTandem;

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

enum TSKTandemDAOStatus
{
	tsNone,
	tsServiceOff,
	tsServiceOn,
	tsBreakdown,
	tsReconnecting,
	tsMsgReady,
};

class CSKTandemDAO : public CSKThread
{
private:
	friend CSKHeartbeat;
	friend CSKWriteQueueDAOs;

	CSKSocket* m_pSocket;
	CSKHeartbeat* m_pHeartbeat;

	string m_strHost;
	string m_strPort;
	string m_request_remain;
	string m_time_limit;
	struct CV_StructTSOrderReply m_tandemreply;
	TSKTandemDAOStatus m_TandemDAOStatus;
	bool m_bInuse;

	CSKWriteQueueDAOs* m_pWriteQueueDAOs;

	CSKTandem* m_pTandem;
	unsigned int m_nTandemNodeIndex;

	pthread_mutex_t m_MutexLockOnSetStatus;

	char m_caTandemDAOID[4];
private:
	char* GetReplyMsgOffsetPointer(const unsigned char *pMessageBuf);

protected:
	void* Run();

public:
	CSKTandemDAO();
	CSKTandemDAO(int nTandemDAOID, int nNumberOfWriteQueueDAO, key_t kWriteQueueDAOStartKey, key_t kWriteQueueDAOEndKey);
	int HmacEncodeSHA256( const char * key, unsigned int key_length, const char * input, unsigned int input_length, unsigned char * &output, unsigned int &output_length);
	virtual ~CSKTandemDAO();

	bool SendOrder(const unsigned char* pBuf, int nSize);
	bool OrderSubmit(const unsigned char* pBuf, int nSize);
	bool OrderSubmit_Bitmex(struct CV_StructTSOrder cv_ts_order, int nSize);
	bool OrderSubmit_Binance(struct CV_StructTSOrder cv_ts_order, int nSize);
	bool RiskControl();
	bool FillRiskMsg(const unsigned char* pBuf, int nSize);
	bool LogOrderReplyDB_Bitmex(json* jtable, int option);

	TSKTandemDAOStatus GetStatus();
	void SetStatus(TSKTandemDAOStatus tsStatus);
	void RestoreStatus();
	void SendLogout();
	void SendNotify(char* pBuf);

	void ReconnectSocket();
	void CloseSocket();

	void SetInuse(bool bInuse);
	bool IsInuse();
};
#endif

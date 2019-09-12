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

class CSKReplyDAO : public CSKThread
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
	TSKTandemDAOStatus m_TandemDAOStatus;
	CSKWriteQueueDAOs* m_pWriteQueueDAOs;
	CSKTandem* m_pTandem;
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
	CSKReplyDAO();
	CSKReplyDAO(int nNumberOfWriteQueueDAO, key_t kWriteQueueDAOStartKey, key_t kWriteQueueDAOEndKey);
	int HmacEncodeSHA256( const char * key, unsigned int key_length, const char * input, unsigned int input_length, unsigned char * &output, unsigned int &output_length);
	virtual ~CSKReplyDAO();

	TSKTandemDAOStatus GetStatus();
	void SetStatus(TSKTandemDAOStatus tsStatus);
	void RestoreStatus();
	void Bitmex_Transaction_Update();

	void ReconnectSocket();
	void CloseSocket();

	void SetInuse(bool bInuse);
	bool IsInuse();
};
#endif

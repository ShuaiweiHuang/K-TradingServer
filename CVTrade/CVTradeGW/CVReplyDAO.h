#ifndef SKTANDEMDAO_H_
#define SKTANDEMDAO_H_

#include "CVCommon/CVThread.h"
#include "CVHeartbeat.h"
#include "CVWriteQueueDAOs.h"
#include "../include/CVType.h"
#include "../include/CVGlobal.h"
#include <nlohmann/json.hpp>
#include <iomanip>

using json = nlohmann::json;

struct HEADRESP
{
	string remain;
	string epoch;
};

class CSKReplyDAO : public CSKThread
{
private:
	friend CSKHeartbeat;
	friend CSKWriteQueueDAOs;

	CSKHeartbeat* m_pHeartbeat;
	CSKWriteQueueDAOs* m_pWriteQueueDAOs;
	string m_request_remain;
	string m_time_limit;
	pthread_mutex_t m_MutexLockOnSetStatus;
	struct CV_StructTSOrderReply m_trade_reply[MAXHISTORY];
private:
	char* GetReplyMsgOffsetPointer(const unsigned char *pMessageBuf);

protected:
	void* Run();

public:
	CSKReplyDAO();
	CSKReplyDAO(int query_history);
	int HmacEncodeSHA256( const char * key, unsigned int key_length, const char * input, unsigned int input_length, unsigned char * &output, unsigned int &output_length);
	virtual ~CSKReplyDAO();
	void Bitmex_Transaction_Update();
};
#endif

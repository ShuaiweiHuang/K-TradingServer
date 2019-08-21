/*
 * SKTandemDAO.h
 *
 *  Created on: 2015年11月12日
 *      Author: alex
 */
#ifndef SKTANDEMDAO_H_
#define SKTANDEMDAO_H_

#include <vector>

#include "CVCommon/CVThread.h"
#include "CVNet/CVSocket.h"
#include "CVInterface/ICVSocketCallback.h"
#include "CVHeartbeat.h"
#include "CVWriteQueueDAOs.h"

class CSKHeartbeat;
class CSKTandem;

enum TSKTandemDAOStatus
{
	tsNone,
	tsServiceOff,
	tsServiceOn,
	tsBreakdown,
	tsReconnecting
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

	bool SendData(const unsigned char* pBuf, int nSize);
	bool SendAll(const unsigned char* pBuf, int nSize);

	TSKTandemDAOStatus GetStatus();
	void SetStatus(TSKTandemDAOStatus tsStatus);
	void RestoreStatus();
	void SendLogout();

	void ReconnectSocket();
	void CloseSocket();

	void SetInuse(bool bInuse);
	bool IsInuse();
};
#endif

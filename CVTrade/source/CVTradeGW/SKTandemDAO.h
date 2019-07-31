/*
 * SKTandemDAO.h
 *
 *  Created on: 2015年11月12日
 *      Author: alex
 */
#ifndef SKTANDEMDAO_H_
#define SKTANDEMDAO_H_

#include <vector>

#include "SKCommon/SKThread.h"
#include "SKNet/SKSocket.h"
#include "SKInterface/ISKSocketCallback.h"
#include "SKHeartbeat.h"
#include "SKWriteQueueDAOs.h"

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

class CSKTandemDAO : public CSKThread, public ISKSocketCallback
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

	void OnConnect();
	void OnDisconnect();
	void OnData(unsigned char* pBuf, int nSize);
	void OnControlMessage(const unsigned char* pBuf);

public:
	CSKTandemDAO();
	CSKTandemDAO(int nTandemDAOID, int nNumberOfWriteQueueDAO, key_t kWriteQueueDAOStartKey, key_t kWriteQueueDAOEndKey);
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

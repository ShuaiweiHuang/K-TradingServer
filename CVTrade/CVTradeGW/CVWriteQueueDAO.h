#ifndef SKWRITEQUEUEDAO_H_
#define SKWRITEQUEUEDAO_H_

#ifdef _WIN32
#include <Windows.h>
#endif

#include "CVPevents.h"

#include <string>

#include "CVCommon/CVQueue.h"
#include "CVCommon/CVThread.h"

using namespace neosmart;

enum TSKWriteQueueDAOStatus 
{
	wsNone,
	//wsSleeping,
	wsReadyForLooping,
	wsReadyForRunning,
	wsRunning,
	wsSendQFailed,
	wsSendQSuccessful,
	wsBreakdown,
};

class CSKWriteQueueDAO: public CSKThread
{
	private:
		char m_caWriteQueueDAOID[4];

		key_t m_kWriteKey;

		CSKQueue* m_pWriteQueue;

		neosmart_event_t m_PEvent;

		unsigned char m_uncaReplyMessage[BUFSIZE];
		int m_nReplyMessageSize;

		TSKWriteQueueDAOStatus m_WriteQueueDAOStatus;

		struct timeval m_timevalStart;//us

		pthread_mutex_t m_MutexLockOnSetStatus;

	protected:
		void* Run();

	public:
		CSKWriteQueueDAO(int nWriteQueueDAOID, key_t key);
		virtual ~CSKWriteQueueDAO();

		void SetReplyMessage(unsigned char* pReplyMessage, int nReplyMessageSize);

		void TriggerWakeUpEvent();

		char* GetWriteQueueDAOID();

		void SetStatus(TSKWriteQueueDAOStatus wsStatus);
		TSKWriteQueueDAOStatus GetStatus();

		struct timeval& GetStartTimeval();

		unsigned char* GetReplyMessage();
		int GetReplyMessageLength();
};
#endif

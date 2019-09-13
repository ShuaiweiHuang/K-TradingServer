#ifndef CVWRITEQUEUEDAO_H_
#define CVWRITEQUEUEDAO_H_

#ifdef _WIN32
#include <Windows.h>
#endif

#include "CVPevents.h"

#include <string>

#include "CVCommon/CVQueue.h"
#include "CVCommon/CVThread.h"

using namespace neosmart;

enum TCVWriteQueueDAOStatus 
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

class CCVWriteQueueDAO: public CCVThread
{
	private:
		char m_caWriteQueueDAOID[4];

		key_t m_kWriteKey;

		CCVQueue* m_pWriteQueue;

		neosmart_event_t m_PEvent;

		unsigned char m_uncaReplyMessage[BUFSIZE];
		int m_nReplyMessageSize;

		TCVWriteQueueDAOStatus m_WriteQueueDAOStatus;

		struct timeval m_timevalStart;//us

		pthread_mutex_t m_MutexLockOnSetStatus;

	protected:
		void* Run();

	public:
		CCVWriteQueueDAO(int nWriteQueueDAOID, key_t key);
		virtual ~CCVWriteQueueDAO();

		void SetReplyMessage(unsigned char* pReplyMessage, int nReplyMessageSize);

		void TriggerWakeUpEvent();

		char* GetWriteQueueDAOID();

		void SetStatus(TCVWriteQueueDAOStatus wsStatus);
		TCVWriteQueueDAOStatus GetStatus();

		struct timeval& GetStartTimeval();

		unsigned char* GetReplyMessage();
		int GetReplyMessageLength();
};
#endif

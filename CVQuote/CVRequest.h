#ifndef SKREQUEST_H_
#define SKREQUEST_H_

#ifdef _WIN32
#include <Windows.h>
#endif

#include "CVCommon/CVPevents.h"

#include "CVCommon/CVThread.h"
#include "CVCommon/ISKRequestCallback.h"

//#define MAX_TIG_DATA_LENGTH 8123

using namespace neosmart;

class ISKRequestCallback;

class CSKRequest: public CSKThread
{
	private:
		ISKRequestCallback* m_pRequestCallback;

		//unsigned char m_uncaRequestMessage[MAX_TIG_DATA_LENGTH];
		//int m_nRequestMessageLength;

		neosmart_event_t m_PEvent[2];

	protected:
		void* Run();

	public:
		CSKRequest(ISKRequestCallback* pRequestCallback);
		virtual ~CSKRequest();

		//void SetRequestMessage(union TIG_ORDER &tig_order);
		//void SetRequestMessage(unsigned char* pRequestMessage, int nRequestMessageLength);
		//void SetOrderMessageLength(int nOrderMessageLength);

		void TriggerWakeUpEvent();
		void TriggerTerminateEvent();
};
#endif

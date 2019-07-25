#ifndef SKREQUEST_H_
#define SKREQUEST_H_

#ifdef _WIN32
#include <Windows.h>
#endif

#include "CVCommon/CVPevents.h"

#include "CVCommon/CVThread.h"
#include "CVCommon/ISKRequestCallback.h"

using namespace neosmart;

class ISKRequestCallback;

class CSKRequest: public CSKThread
{
	private:
		ISKRequestCallback* m_pRequestCallback;
		neosmart_event_t m_PEvent[2];

	protected:
		void* Run();

	public:
		CSKRequest(ISKRequestCallback* pRequestCallback);
		virtual ~CSKRequest();

		void TriggerWakeUpEvent();
		void TriggerTerminateEvent();
};
#endif

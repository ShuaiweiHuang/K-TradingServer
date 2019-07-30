#ifndef CVREQUEST_H_
#define CVREQUEST_H_

#ifdef _WIN32
#include <Windows.h>
#endif

#include "CVCommon/CVPevents.h"

#include "CVCommon/CVThread.h"
#include "CVCommon/ICVRequestCallback.h"

using namespace neosmart;

class ICVRequestCallback;

class CCVRequest: public CCVThread
{
	private:
		ICVRequestCallback* m_pRequestCallback;
		neosmart_event_t m_PEvent[2];

	protected:
		void* Run();

	public:
		CCVRequest(ICVRequestCallback* pRequestCallback);
		virtual ~CCVRequest();

		void TriggerWakeUpEvent();
		void TriggerTerminateEvent();
};
#endif

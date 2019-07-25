#include <iostream>
#include <unistd.h>
#include <ctime>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <sys/time.h>
#include <fstream>

#include "CVRequest.h"

using namespace std;

extern void FprintfStderrLog(const char* pCause, int nError, int nData, const char* pFile, int nLine,
												unsigned char* pMessage1 = NULL, int nMessage1Length = 0, 
												unsigned char* pMessage2 = NULL, int nMessage2Length = 0);

extern int WaitForMultipleEvents(neosmart_event_t *events, int count, bool waitAll, uint64_t milliseconds);

CSKRequest::CSKRequest(ISKRequestCallback* pRequestCallback)
{
	m_pRequestCallback = pRequestCallback;

	//m_nRequestMessageLength = 0;

	m_PEvent[0] = CreateEvent(0, 0);
	m_PEvent[1] = CreateEvent(0, 0);

	Start();
}

CSKRequest::~CSKRequest() 
{
	DestroyEvent(m_PEvent[0]);
	DestroyEvent(m_PEvent[1]);
}

void* CSKRequest::Run()
{
	while(m_pRequestCallback)	
	{
		int nIndex = -1;
		int nResult = WaitForMultipleEvents(m_PEvent, 2, false, -1, nIndex);

		if(nResult == WAIT_TIMEOUT)
		{
			if(m_pRequestCallback)
				m_pRequestCallback->OnRequestError(0, "REQUEST_WAIT_TIMEOUT_ERROR");
		}
		else if(nResult != 0)
		{
			if(m_pRequestCallback)
				m_pRequestCallback->OnRequestError(nResult, "REQUEST_ERROR_IN_WAIT");
		}
		else if(nIndex == 0)
		{
			if(m_pRequestCallback)
				m_pRequestCallback->OnRequest();
		}
		else if(nIndex == 1)
		{
			if(m_pRequestCallback)
				m_pRequestCallback->OnRequestError(0, "TERMINATE_REQUEST");
			break;
		}
		else
		{
			if(m_pRequestCallback)
				m_pRequestCallback->OnRequestError(nIndex, "REQUEST_UNEXPECTED_INDEX_ERROR");
		}
	}

	return NULL;
}

/*void CSKRequest::SetRequestMessage(unsigned char* pRequestMessage, int nRequestMessageLength)
{
	try
	{
		if(nRequestMessageLength > sizeof(m_uncaRequestMessage))
		{
			throw "REQUEST_DATA_LENGTH_TOO_LARGE";
		}
		memset(m_uncaRequestMessage, 0, sizeof(m_uncaRequestMessage));
		memcpy(m_uncaRequestMessage, pRequestMessage, nRequestMessageLength);
	}
	catch(const char* pErrorMessage)
	{
		FprintfStderrLog(pErrorMessage, -1, nRequestMessageLength, NULL, 0, pRequestMessage, nRequestMessageLength);
	}
}*/

void CSKRequest::TriggerWakeUpEvent()
{
	SetEvent(m_PEvent[0]);
}

void CSKRequest::TriggerTerminateEvent()
{
	SetEvent(m_PEvent[1]);
}

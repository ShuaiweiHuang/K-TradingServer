#ifndef CVINTERFACE_ICVREQUESTCALLBACK_H_
#define CVINTERFACE_ICVREQUESTCALLBACK_H_

class ICVRequestCallback 
{
	public:
		//virtual void OnRequest(unsigned char* pRequestMessage, int nRequestMessageLength)=0;
		virtual void OnRequest()=0;

		virtual void OnRequestError(int nData, const char* pErrorMessage)=0;

		virtual ~ICVRequestCallback(){};
};

#endif

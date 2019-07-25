#ifndef SKINTERFACE_ISKREQUESTCALLBACK_H_
#define SKINTERFACE_ISKREQUESTCALLBACK_H_

class ISKRequestCallback 
{
	public:
		//virtual void OnRequest(unsigned char* pRequestMessage, int nRequestMessageLength)=0;
		virtual void OnRequest()=0;

		virtual void OnRequestError(int nData, const char* pErrorMessage)=0;

		virtual ~ISKRequestCallback(){};
};

#endif

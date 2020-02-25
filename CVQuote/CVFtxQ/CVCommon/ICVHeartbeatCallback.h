#ifndef CVINTERFACE_ICVHEARTBEATCALLBACK_H_
#define CVINTERFACE_ICVHEARTBEATCALLBACK_H_

class ICVHeartbeatCallback 
{
	public:
		virtual void OnHeartbeatLost()=0;
		virtual void OnHeartbeatRequest()=0;

		virtual void OnHeartbeatError(int nData, const char* pErrorMessage)=0;

		virtual ~ICVHeartbeatCallback(){};
};

#endif

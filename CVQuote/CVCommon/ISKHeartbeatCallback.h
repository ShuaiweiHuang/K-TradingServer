#ifndef SKINTERFACE_ISKHEARTBEATCALLBACK_H_
#define SKINTERFACE_ISKHEARTBEATCALLBACK_H_

class ISKHeartbeatCallback 
{
	public:
		virtual void OnHeartbeatLost()=0;
		virtual void OnHeartbeatRequest()=0;

		virtual void OnHeartbeatError(int nData, const char* pErrorMessage)=0;

		virtual ~ISKHeartbeatCallback(){};
};

#endif

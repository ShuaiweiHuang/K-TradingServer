#ifndef CVINTERFACE_ICVCLIENTSOCKETCALLBACK_H_
#define CVINTERFACE_ICVCLIENTSOCKETCALLBACK_H_

class ICVClientSocketCallback 
{
	public:
		virtual void OnConnect()=0;
		virtual void OnDisconnect()=0;
		virtual void OnData( unsigned char* pBuf, int nSize)=0;
		virtual ~ICVClientSocketCallback(){};
};

#endif /* CVINTERFACE_ICVSOCKETCALLBACK_H_ */

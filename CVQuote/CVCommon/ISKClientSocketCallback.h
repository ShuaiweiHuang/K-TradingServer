#ifndef SKINTERFACE_ISKCLIENTSOCKETCALLBACK_H_
#define SKINTERFACE_ISKCLIENTSOCKETCALLBACK_H_

class ISKClientSocketCallback 
{
	public:
		virtual void OnConnect()=0;
		virtual void OnDisconnect()=0;
		virtual void OnData( unsigned char* pBuf, int nSize)=0;
		virtual ~ISKClientSocketCallback(){};
};

#endif /* SKINTERFACE_ISKSOCKETCALLBACK_H_ */

#ifndef SKINTERFACE_ISKSOCKETCALLBACK_H_
#define SKINTERFACE_ISKSOCKETCALLBACK_H_

class ISKSocketCallback 
{
	public:
		virtual void OnConnect()=0;
		virtual void OnDisconnect()=0;
		virtual void OnData( unsigned char* pBuf, int nSize)=0;

		virtual ~ISKSocketCallback(){};
};

#endif /* SKINTERFACE_ISKSOCKETCALLBACK_H_ */

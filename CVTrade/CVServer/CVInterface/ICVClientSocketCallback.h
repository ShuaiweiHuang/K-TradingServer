#ifndef CVINTERFACE_ICVSOCKETCALLBACK_H_
#define CVINTERFACE_ICVSOCKETCALLBACK_H_

class ICVSocketCallback 
{
	public:
		virtual void OnConnect()=0;
		virtual void OnDisconnect()=0;
		virtual void OnData( unsigned char* pBuf, int nSize)=0;

		virtual ~ICVSocketCallback(){};
};

#endif /* CVINTERFACE_ICVSOCKETCALLBACK_H_ */

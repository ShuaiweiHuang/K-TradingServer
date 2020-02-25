#ifndef CVINTERFACE_ICVSOCKETCALLBACK_H_
#define CVINTERFACE_ICVSOCKETCALLBACK_H_

class ICVSocketCallback 
{
	public:
		virtual void OnListening()=0;
		virtual void OnShutdown()=0;

		virtual ~ICVSocketCallback(){};
};

#endif /* CVINTERFACE_ICVSOCKETCALLBACK_H_ */

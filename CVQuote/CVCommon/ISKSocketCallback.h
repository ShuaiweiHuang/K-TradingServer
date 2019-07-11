#ifndef SKINTERFACE_ISKSOCKETCALLBACK_H_
#define SKINTERFACE_ISKSOCKETCALLBACK_H_

class ISKSocketCallback 
{
	public:
		virtual void OnListening()=0;
		virtual void OnShutdown()=0;

		virtual ~ISKSocketCallback(){};
};

#endif /* SKINTERFACE_ISKSOCKETCALLBACK_H_ */

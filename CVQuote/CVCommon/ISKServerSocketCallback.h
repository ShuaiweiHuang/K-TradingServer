#ifndef SKINTERFACE_ISKServerSOCKETCALLBACK_H_
#define SKINTERFACE_ISKServerSOCKETCALLBACK_H_

class ISKServerSocketCallback 
{
	public:
		virtual void OnListening()=0;
		virtual void OnShutdown()=0;

		virtual ~ISKServerSocketCallback(){};
};

#endif 

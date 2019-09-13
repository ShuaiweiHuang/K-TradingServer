#ifndef CVTANDEMS_H_
#define CVTANDEMS_H_

#include "CVReply.h"

class CCVReplys
{
	private:
		CCVReplys();
		virtual ~CCVReplys();
		static CCVReplys* instance;
		static pthread_mutex_t ms_mtxInstance;

		CCVReply* m_pTandemBitmex;

	protected:

	public:
		static CCVReplys* GetInstance();

		CCVReply* GetTandemBitmex();
};
#endif

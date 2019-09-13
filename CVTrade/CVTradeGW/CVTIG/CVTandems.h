#ifndef CVTANDEMS_H_
#define CVTANDEMS_H_

#include "CVTandem.h"

class CCVTandems
{
	private:
		CCVTandems();
		virtual ~CCVTandems();
		static CCVTandems* instance;
		static pthread_mutex_t ms_mtxInstance;

		CCVTandem* m_pTandemBitmex;

	protected:

	public:
		static CCVTandems* GetInstance();

		CCVTandem* GetTandemBitmex();
};
#endif

#ifndef SKTANDEMS_H_
#define SKTANDEMS_H_

#include "CVTandem.h"

class CSKTandems
{
	private:
		CSKTandems();
		virtual ~CSKTandems();
		static CSKTandems* instance;
		static pthread_mutex_t ms_mtxInstance;

		CSKTandem* m_pTandemBitmex;

	protected:

	public:
		static CSKTandems* GetInstance();

		CSKTandem* GetTandemBitmex();
};
#endif

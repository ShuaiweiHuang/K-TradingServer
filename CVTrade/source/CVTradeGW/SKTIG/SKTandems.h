#ifndef SKTANDEMS_H_
#define SKTANDEMS_H_

#include "SKTandem.h"

class CSKTandems
{
	private:
		CSKTandems();
		virtual ~CSKTandems();
		static CSKTandems* instance;
		static pthread_mutex_t ms_mtxInstance;

		CSKTandem* m_pTandemTaiwanStock;
		CSKTandem* m_pTandemTaiwanFuture;
		CSKTandem* m_pTandemOverseasFuture;
		CSKTandem* m_pTandemOthers;

	protected:

	public:
		static CSKTandems* GetInstance();

		CSKTandem* GetTandemTaiwanStock();
		CSKTandem* GetTandemTaiwanFuture();
		CSKTandem* GetTandemOverseasFuture();
		CSKTandem* GetTandemOthers();
};
#endif

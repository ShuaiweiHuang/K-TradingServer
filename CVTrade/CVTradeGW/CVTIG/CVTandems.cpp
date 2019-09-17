#include <iostream>

#include "CVTandems.h"

using namespace std;

CCVTandems* CCVTandems::instance = NULL;
pthread_mutex_t CCVTandems::ms_mtxInstance = PTHREAD_MUTEX_INITIALIZER;

CCVTandems::CCVTandems()
{
	m_pTandemBitmex	= new CCVTandem();
}

CCVTandems::~CCVTandems()
{
	if(m_pTandemBitmex)
	{
		delete m_pTandemBitmex;
		m_pTandemBitmex = NULL;
	}
}

CCVTandem* CCVTandems::GetTandemBitmex()
{
	return m_pTandemBitmex;
}

CCVTandems* CCVTandems::GetInstance()
{
	if(instance == NULL)
	{
		pthread_mutex_lock(&ms_mtxInstance);//lock

		if(instance == NULL)
		{
			instance = new CCVTandems();
			cout << "Tandems One" << endl;
		}

		pthread_mutex_unlock(&ms_mtxInstance);//unlock
	}

	return instance;
}

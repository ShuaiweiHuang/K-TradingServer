#include <iostream>

#include "CVTandems.h"

using namespace std;

CSKTandems* CSKTandems::instance = NULL;
pthread_mutex_t CSKTandems::ms_mtxInstance = PTHREAD_MUTEX_INITIALIZER;

CSKTandems::CSKTandems()
{
	m_pTandemBitmex	= new CSKTandem();
}

CSKTandems::~CSKTandems()
{
	if(m_pTandemBitmex)
	{
		delete m_pTandemBitmex;
		m_pTandemBitmex = NULL;
	}
}

CSKTandem* CSKTandems::GetTandemBitmex()
{
	return m_pTandemBitmex;
}

CSKTandems* CSKTandems::GetInstance()
{
	if(instance == NULL)
	{
		pthread_mutex_lock(&ms_mtxInstance);//lock

		if(instance == NULL)
		{
			instance = new CSKTandems();
			cout << "Tandems One" << endl;
		}

		pthread_mutex_unlock(&ms_mtxInstance);//unlock
	}

	return instance;
}

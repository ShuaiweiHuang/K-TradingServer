#include <iostream>

#include "SKTandems.h"

using namespace std;

CSKTandems* CSKTandems::instance = NULL;
pthread_mutex_t CSKTandems::ms_mtxInstance = PTHREAD_MUTEX_INITIALIZER;

CSKTandems::CSKTandems()
{
	m_pTandemTaiwanStock 	= new CSKTandem("TIGNODWS", "TIGSVCWS");
	m_pTandemTaiwanFuture 	= new CSKTandem("TIGNODWU", "TIGSVCWU");
	m_pTandemOverseasFuture	= new CSKTandem("TIGNODWF", "TIGSVCWF");
	m_pTandemOthers 		= new CSKTandem("TIGNODWG", "TIGSVCWG");
}

CSKTandems::~CSKTandems()
{
	if(m_pTandemTaiwanStock)
	{
		delete m_pTandemTaiwanStock;
		m_pTandemTaiwanStock = NULL;
	}

	if(m_pTandemTaiwanFuture)
	{
		delete m_pTandemTaiwanFuture;
		m_pTandemTaiwanFuture = NULL;
	}

	if(m_pTandemOverseasFuture)
	{
		delete m_pTandemOverseasFuture;
		m_pTandemOverseasFuture = NULL;
	}

	if(m_pTandemOthers)
	{
		delete m_pTandemOthers;
		m_pTandemOthers = NULL;
	}
}

CSKTandem* CSKTandems::GetTandemTaiwanStock()
{
	return m_pTandemTaiwanStock;
}

CSKTandem* CSKTandems::GetTandemTaiwanFuture()
{
	return m_pTandemTaiwanFuture;
}

CSKTandem* CSKTandems::GetTandemOverseasFuture()
{
	return m_pTandemOverseasFuture;
}

CSKTandem* CSKTandems::GetTandemOthers()
{
	return m_pTandemOthers;
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

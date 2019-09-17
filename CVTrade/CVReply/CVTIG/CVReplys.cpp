#include <iostream>

#include "CVReplys.h"

using namespace std;

CCVReplys* CCVReplys::instance = NULL;
pthread_mutex_t CCVReplys::ms_mtxInstance = PTHREAD_MUTEX_INITIALIZER;

CCVReplys::CCVReplys()
{
	m_pTandemBitmex	= new CCVReply();
}

CCVReplys::~CCVReplys()
{
	if(m_pTandemBitmex)
	{
		delete m_pTandemBitmex;
		m_pTandemBitmex = NULL;
	}
}

CCVReply* CCVReplys::GetTandemBitmex()
{
	return m_pTandemBitmex;
}

CCVReplys* CCVReplys::GetInstance()
{
	if(instance == NULL)
	{
		pthread_mutex_lock(&ms_mtxInstance);//lock

		if(instance == NULL)
		{
			instance = new CCVReplys();
			cout << "Tandems One" << endl;
		}

		pthread_mutex_unlock(&ms_mtxInstance);//unlock
	}

	return instance;
}

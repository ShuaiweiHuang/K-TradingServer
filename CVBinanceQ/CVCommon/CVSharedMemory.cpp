#include <cstdio>
#include <cstddef>

#include "CVSharedMemory.h"

CCVSharedMemory::CCVSharedMemory(key_t kKey, int nSize): m_pSharedMemory(NULL), m_nDetached(0)
{
	m_nShmid = shmget(kKey, nSize, 0666 | IPC_CREAT);

	if(m_nShmid == -1)
	{
		perror("Get shared memory error");
	}
}

CCVSharedMemory::~CCVSharedMemory() 
{
	if ( m_nDetached == 0) 
	{
		shmdt(m_pSharedMemory);	
	}
}

void CCVSharedMemory::AttachSharedMemory()
{
	m_pSharedMemory = shmat(m_nShmid, NULL, 0);

	if(m_pSharedMemory == (void*)-1)
	{
		perror("Attach memory error");
	}
	else
	{
		m_nDetached = 0;
	}
}

int CCVSharedMemory::DetachSharedMemory()
{
	int nResult = shmdt(m_pSharedMemory);

	if(nResult == -1)
	{
		perror("Detach memory error");
	}
	else
	{
		m_nDetached = 1;
	}
}

int CCVSharedMemory::RemoveSharedMemory()
{
	int nResult = shmctl(m_nShmid, IPC_RMID, 0);

	if(nResult == -1)
	{
		perror("Remove memory error");
	}
}

void* CCVSharedMemory::GetSharedMemory()
{
	return m_pSharedMemory;
}

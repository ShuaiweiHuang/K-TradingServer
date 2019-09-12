#include <cstdio>
#include <cstddef>

#include "CVSharedMemory.h"

CSKSharedMemory::CSKSharedMemory(key_t kKey, int nSize): m_pSharedMemory(NULL), m_nDetached(0)
{
	m_nShmid = shmget(kKey, nSize, 0666 | IPC_CREAT);

	if(m_nShmid == -1)
	{
		perror("Get shared memory error");
	}
}

CSKSharedMemory::~CSKSharedMemory() 
{
	if ( m_nDetached == 0) 
	{
		shmdt(m_pSharedMemory);	
	}
}

void CSKSharedMemory::AttachSharedMemory()
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

int CSKSharedMemory::DetachSharedMemory()
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

int CSKSharedMemory::RemoveSharedMemory()
{
	int nResult = shmctl(m_nShmid, IPC_RMID, 0);

	if(nResult == -1)
	{
		perror("Remove memory error");
	}
}

void* CSKSharedMemory::GetSharedMemory()
{
	return m_pSharedMemory;
}

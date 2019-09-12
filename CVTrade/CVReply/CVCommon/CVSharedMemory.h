#ifndef SKCOMMON_SKSHAREDMEMORY_H_
#define SKCOMMON_SKSHAREDMEMORY_H_

#include <sys/shm.h>

class CSKSharedMemory
{
	private:
		void* 	m_pSharedMemory;
		int 	m_nShmid;
		int 	m_nDetached;

	public:
		CSKSharedMemory(key_t kKey, int nSize);
		virtual ~CSKSharedMemory();

		//int GetSharedMemory();
		void AttachSharedMemory();
		int DetachSharedMemory();
		int RemoveSharedMemory();

		void* GetSharedMemory();
};
#endif

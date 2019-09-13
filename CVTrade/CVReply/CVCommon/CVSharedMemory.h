#ifndef CVCOMMON_CVSHAREDMEMORY_H_
#define CVCOMMON_CVSHAREDMEMORY_H_

#include <sys/shm.h>

class CCVSharedMemory
{
	private:
		void* 	m_pSharedMemory;
		int 	m_nShmid;
		int 	m_nDetached;

	public:
		CCVSharedMemory(key_t kKey, int nSize);
		virtual ~CCVSharedMemory();

		//int GetSharedMemory();
		void AttachSharedMemory();
		int DetachSharedMemory();
		int RemoveSharedMemory();

		void* GetSharedMemory();
};
#endif

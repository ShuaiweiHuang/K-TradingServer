#ifndef SKREADQUEUEDAO_H_
#define SKREADQUEUEDAO_H_

#include <string>

#include "SKCommon/SKQueue.h"
#include "SKCommon/SKThread.h"

using namespace std;

class CSKTandem;

class CSKReadQueueDAO: public CSKThread
{
	private:
		CSKQueue* m_pReadQueue;

		key_t m_kReadKey;

		string m_strService;//to change TandemDAOs->GetService()

		string m_strOTSID;

		CSKTandem* m_pTandem;
		int m_nTandemServiceIndex;
		char m_caTandemService[20];

	protected:
		void* Run();

	public:
		CSKReadQueueDAO(key_t key, string strService, string strOTSID);
		virtual ~CSKReadQueueDAO();

		key_t GetReadKey();

		//void GetOrderNumber(unsigned char* pBuf, string strService, unsigned char* pOrderNumber);
};
#endif

#ifndef CVREADQUEUEDAO_H_
#define CVREADQUEUEDAO_H_

#include <string>

#include "CVCommon/CVQueue.h"
#include "CVCommon/CVThread.h"

using namespace std;

class CCVTandem;

class CCVReadQueueDAO: public CCVThread
{
	private:
		CCVQueue* m_pReadQueue;

		key_t m_kReadKey;

		string m_strService;//to change TandemDAOs->GetService()

		string m_strOTSID;

		CCVTandem* m_pTandem;
		int m_nTandemServiceIndex;
		char m_caTandemService[20];

	protected:
		void* Run();

	public:
		CCVReadQueueDAO(key_t key, string strService, string strOTSID);
		virtual ~CCVReadQueueDAO();

		key_t GetReadKey();

		//void GetOrderNumber(unsigned char* pBuf, string strService, unsigned char* pOrderNumber);
};
#endif

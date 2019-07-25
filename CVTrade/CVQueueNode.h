#ifndef SKQUEUEDAO_H_
#define SKQUEUEDAO_H_

#include <map>
#include <string>

#include"CVCommon/CVQueue.h"
#include"CVCommon/CVThread.h"

using namespace std;

class CSKQueueDAO: public CSKThread
{
	private:
		string m_strService;
		key_t m_kRecvKey;
		key_t m_kSendKey;
		CSKQueue* m_pSendQueue;
		CSKQueue* m_pRecvQueue;

	protected:
		void* Run();

	public:
		CSKQueueDAO(string strService, key_t kSendKey, key_t kRecvKey);
		virtual ~CSKQueueDAO();

		int SendData(char* pBuf, int nSize, long lType = 1,int nFlag = 0);

		key_t GetSendKey();
		key_t GetRecvKey();
};
#endif

#ifndef CVQUEUEDAO_H_
#define CVQUEUEDAO_H_

#include <map>
#include <string>

#include"CVCommon/CVQueue.h"
#include"CVCommon/CVThread.h"

using namespace std;

class CCVQueueDAO: public CCVThread
{
	private:
		string m_strService;
		key_t m_kRecvKey;
		key_t m_kSendKey;
		CCVQueue* m_pSendQueue;
		CCVQueue* m_pRecvQueue;

	protected:
		void* Run();

	public:
		CCVQueueDAO(string strService, key_t kSendKey, key_t kRecvKey);
		virtual ~CCVQueueDAO();

		int SendData(char* pBuf, int nSize, long lType = 1,int nFlag = 0);

		key_t GetSendKey();
		key_t GetRecvKey();
};
#endif

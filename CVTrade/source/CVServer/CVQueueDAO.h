#ifndef CVQUEUEDAO_H_
#define CVQUEUEDAO_H_

#include <map>
#include <string>

#include"CVCommon/CVQueue.h"
#include"CVCommon/CVThread.h"
//#include "CVInterface/ICVQueueDAOCallback.h"

using namespace std;

class CCVQueueDAO: public CCVThread
{
	private:
		CCVQueue* m_pSendQueue;
		CCVQueue* m_pRecvQueue;

		key_t m_kRecvKey;
		key_t m_kSendKey;

		string m_strService;

	protected:
		void* Run();

	public:
		CCVQueueDAO(string strService, key_t kSendKey, key_t kRecvKey);
		virtual ~CCVQueueDAO();

		int SendData(const unsigned char* pBuf, int nSize, long lType = 1,int nFlag = 0);

		key_t GetSendKey();
		key_t GetRecvKey();
};
#endif

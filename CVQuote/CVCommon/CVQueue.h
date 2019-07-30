#ifndef CVCOMMON_CVQUEUE_H_
#define CVCOMMON_CVQUEUE_H_

#include<sys/msg.h>

const int BUFSIZE = 1024;

struct TCVQueueMessage
{
	long lMessageType;
	unsigned char uncaMessageBuf[BUFSIZE];
};

class CCVQueue
{
	private:
		int  m_nID;

		struct TCVQueueMessage m_QueueMessage;

	public:
		CCVQueue();
		virtual ~CCVQueue();

		int GetMessage(char* pBuf, long lType = 0, int nFlag = 0);
		int SendMessage(char* pBuf, int nSize, long lType = 1, int nFlag = 0);

        int Create(key_t kKey);
        int Remove();
};
#endif

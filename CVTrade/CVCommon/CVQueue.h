#ifndef SKCOMMON_SKQUEUE_H_
#define SKCOMMON_SKQUEUE_H_

#include<sys/msg.h>

const int BUFSIZE = 1024;

struct TSKQueueMessage
{
	long lMessageType;
	unsigned char uncaMessageBuf[BUFSIZE];
};

class CSKQueue
{
	private:
		int  m_nID;

		struct TSKQueueMessage m_QueueMessage;

	public:
		CSKQueue();
		virtual ~CSKQueue();

		int GetMessage(char* pBuf, long lType = 0, int nFlag = 0);
		int SendMessage(char* pBuf, int nSize, long lType = 1, int nFlag = 0);

        int Create(key_t kKey);
        int Remove();
};
#endif

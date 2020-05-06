#include "CVQueue.h"
#include <cstring>

using namespace std;

CCVQueue::CCVQueue()
{
	m_nID = 0;
	memset(&m_QueueMessage,0,sizeof(struct TCVQueueMessage));
}

CCVQueue::~CCVQueue() 
{
	/*if (msgctl(m_nMsgid, IPC_RMID, 0) == -1) 
	{
		fprintf(stderr, "msgctl(IPC_RMID) failed\n");
		exit(EXIT_FAILURE);
	}*/
}

int CCVQueue::GetMessage(unsigned char* pBuf, long lType, int nFlag)
{
	int nGetMessage = msgrcv(m_nID, (void *)&m_QueueMessage, BUFSIZE, lType, nFlag);

	if(nGetMessage> 0)	
	{
		memcpy(pBuf, m_QueueMessage.uncaMessageBuf, nGetMessage);
	}
	return nGetMessage;
}

int CCVQueue::SendMessage(const unsigned char* pBuf, int nSize, long lType, int nFlag)
{
	m_QueueMessage.lMessageType = lType;
	memset(m_QueueMessage.uncaMessageBuf, 0, BUFSIZE);
	memcpy(m_QueueMessage.uncaMessageBuf, pBuf, nSize);

	return msgsnd(m_nID, (void *)&m_QueueMessage, nSize, nFlag);
}

int CCVQueue::Remove()
{
	if (msgctl(m_nID, IPC_RMID, (struct msqid_ds *)0) == -1) 
	{
		return -1;
	}

	return 0;
}

int CCVQueue::Create(key_t kKey)
{
    if( (m_nID = msgget(kKey,0666 | IPC_CREAT)) == -1 )
	{
		return -1;
	}

	return 0;
}

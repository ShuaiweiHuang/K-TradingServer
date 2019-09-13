#include "CVReply.h"
#include <cstring>
#include <fstream>
#include <assert.h>

#include<iostream>
using namespace std;

CCVReply::CCVReply() 
{
}

CCVReply::~CCVReply() 
{
}

const TCVTIGNode* CCVReply::GetNode(int nIndex)
{
	nIndex %= m_vNode.size();
	return m_vNode[nIndex];
}

const TCVTIGService* CCVReply::GetService(int nIndex, char* pService)//todo overhead
{
	int nCount = GetServiceCount(pService);
	int nFirstIndex = GetServiceFirstIndex(pService);

	return m_vService[nFirstIndex + nIndex % nCount];
}

int CCVReply::GetServiceCount(char* pService)
{
	int nCount = 0;

	for(vector<struct TCVTIGService*>::iterator iter = m_vService.begin(); iter != m_vService.end(); iter++)
	{
		if(memcmp((*iter)->uncaServiceID, pService, strlen(pService)) == 0)
		{
			nCount++;
		}
	}

	return nCount;
}

int CCVReply::GetServiceFirstIndex(char* pService)
{
	int nFirstIndex = 0;

	for(vector<struct TCVTIGService*>::iterator iter = m_vService.begin(); iter != m_vService.end(); iter++)
	{
		if(memcmp((*iter)->uncaServiceID, pService, strlen(pService)) == 0)
		{
			break;
		}

		nFirstIndex++;
	}

	return nFirstIndex;
}

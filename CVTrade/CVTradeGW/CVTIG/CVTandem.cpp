#include "CVTandem.h"
#include <cstring>
#include <fstream>
#include <assert.h>

#include<iostream>
using namespace std;

CCVTandem::CCVTandem() 
{
}

CCVTandem::~CCVTandem() 
{
}

const TCVTIGNode* CCVTandem::GetNode(int nIndex)
{
	nIndex %= m_vNode.size();
	return m_vNode[nIndex];
}

const TCVTIGService* CCVTandem::GetService(int nIndex, char* pService)//todo overhead
{
	int nCount = GetServiceCount(pService);
	int nFirstIndex = GetServiceFirstIndex(pService);

	return m_vService[nFirstIndex + nIndex % nCount];
}

int CCVTandem::GetServiceCount(char* pService)
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

int CCVTandem::GetServiceFirstIndex(char* pService)
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

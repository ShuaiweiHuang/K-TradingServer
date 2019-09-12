#include "CVTandem.h"
#include <cstring>
#include <fstream>
#include <assert.h>

#include<iostream>
using namespace std;

CSKTandem::CSKTandem() 
{
}

CSKTandem::~CSKTandem() 
{
}

const TSKTIGNode* CSKTandem::GetNode(int nIndex)
{
	nIndex %= m_vNode.size();
	return m_vNode[nIndex];
}

const TSKTIGService* CSKTandem::GetService(int nIndex, char* pService)//todo overhead
{
	int nCount = GetServiceCount(pService);
	int nFirstIndex = GetServiceFirstIndex(pService);

	return m_vService[nFirstIndex + nIndex % nCount];
}

int CSKTandem::GetServiceCount(char* pService)
{
	int nCount = 0;

	for(vector<struct TSKTIGService*>::iterator iter = m_vService.begin(); iter != m_vService.end(); iter++)
	{
		if(memcmp((*iter)->uncaServiceID, pService, strlen(pService)) == 0)
		{
			nCount++;
		}
	}

	return nCount;
}

int CSKTandem::GetServiceFirstIndex(char* pService)
{
	int nFirstIndex = 0;

	for(vector<struct TSKTIGService*>::iterator iter = m_vService.begin(); iter != m_vService.end(); iter++)
	{
		if(memcmp((*iter)->uncaServiceID, pService, strlen(pService)) == 0)
		{
			break;
		}

		nFirstIndex++;
	}

	return nFirstIndex;
}

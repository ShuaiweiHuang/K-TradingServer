#include "SKTandem.h"
#include <cstring>
#include <fstream>
#include <assert.h>

#include<iostream>
using namespace std;

CSKTandem::CSKTandem(string strNodeFileName, string strServiceFileName) 
{
	char caBuf[128];

	ifstream NodeFile(strNodeFileName.c_str());

	assert(NodeFile);

	while(NodeFile)//EOF
	{
		memset(caBuf, 0, sizeof(caBuf));
		NodeFile.getline(caBuf, sizeof(caBuf));

		if(strlen(caBuf) != 0)
		{
			struct TSKTIGNode* struNode = new struct TSKTIGNode();

			memset(struNode, 0, sizeof(struct TSKTIGNode));
			memcpy(struNode, caBuf, sizeof(struct TSKTIGNode));

			m_vNode.push_back(struNode);
		}
	}

	ifstream ServiceFile(strServiceFileName.c_str());

	assert(ServiceFile);

	while(ServiceFile)//EOF
	{
		memset(caBuf, 0, sizeof(caBuf));
		ServiceFile.getline(caBuf, sizeof(caBuf));

		if(strlen(caBuf) != 0)
		{
			TSKTIGService* struService = new struct TSKTIGService;

			memset(struService, 0, sizeof(struct TSKTIGService));
			memcpy(struService, caBuf, sizeof(struct TSKTIGService));

			m_vService.push_back(struService);
		}
	}
}

CSKTandem::~CSKTandem() 
{
	for(vector<struct TSKTIGNode*>::iterator iter = m_vNode.begin(); iter != m_vNode.end(); iter++)
	{
		delete *iter;
	}

	for(vector<struct TSKTIGService*>::iterator iter = m_vService.begin(); iter != m_vService.end(); iter++)
	{
		delete *iter;
	}
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

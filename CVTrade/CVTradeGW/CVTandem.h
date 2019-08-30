#ifndef SKTANDEM_H_
#define SKTANDEM_H_

#include "../include/CVTIGNode.h"
#include "../include/CVTIGService.h"
#include <vector>

class CSKTandem
{
	public:
		vector<TSKTIGNode> m_vStockNode;
		vector<TSKTIGNode> m_vFuturesNode;
		vector<TSKTIGNode> m_vForeignFuturesNode;
		vector<TSKTIGNode> m_vOthersNode;//re-consigned, bond, accounting

		vector<TSKTIGService> m_vStockService;
		vector<TSKTIGService> m_vFuturesService;
		vector<TSKTIGService> m_vForeignFuturesService;
		vector<TSKTIGService> m_vOthersService;//re-consigned, bond, accounting
};
#endif

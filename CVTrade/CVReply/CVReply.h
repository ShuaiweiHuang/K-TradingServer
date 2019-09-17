#ifndef CVTANDEM_H_
#define CVTANDEM_H_

#include "../include/CVTIGNode.h"
#include "../include/CVTIGService.h"
#include <vector>

class CCVReply
{
	public:
		vector<TCVTIGNode> m_vStockNode;
		vector<TCVTIGNode> m_vFuturesNode;
		vector<TCVTIGNode> m_vForeignFuturesNode;
		vector<TCVTIGNode> m_vOthersNode;//re-consigned, bond, accounting

		vector<TCVTIGService> m_vStockService;
		vector<TCVTIGService> m_vFuturesService;
		vector<TCVTIGService> m_vForeignFuturesService;
		vector<TCVTIGService> m_vOthersService;//re-consigned, bond, accounting
};
#endif

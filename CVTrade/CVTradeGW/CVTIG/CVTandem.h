#ifndef CVTANDEM_H_
#define CVTANDEM_H_

#include "../../include/CVTIGNode.h"
#include "../../include/CVTIGService.h"
#include <vector>
#include <string>

using namespace std;

class CCVTandem
{
	private:
		vector<struct TCVTIGNode*> m_vNode;
		vector<struct TCVTIGService*> m_vService;

	protected:
		int GetServiceCount(char* pService);
		int GetServiceFirstIndex(char* pService);

	public:
		CCVTandem();
		virtual ~CCVTandem();

		const TCVTIGNode* GetNode(int nIndex);
		const TCVTIGService* GetService(int nIndex, char* pService);
};
#endif

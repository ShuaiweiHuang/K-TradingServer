#ifndef CLEARVECTORCONTENT_H_
#define CLEARVECTORCONTENT_H_

#include<vector>

using namespace std;

template <typename T>
void ClearVectorContent(vector<T*>& vVectorToClear)
{
    while(!vVectorToClear.empty()) 
	{
        delete vVectorToClear.back();

		vVectorToClear.back() = NULL;

        vVectorToClear.pop_back();
    }

    vVectorToClear.clear();
}

#endif

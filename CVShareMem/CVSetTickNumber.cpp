#include <cstdlib>
#include <cstring>
#include <assert.h>
#include "CVSharedMemory.h"

using namespace std;

int main(int argc, char **argv)
{
	CCVSharedMemory* pSharedMemory = new CCVSharedMemory(atoi(argv[1]), sizeof(long));//key, size

	pSharedMemory->AttachSharedMemory();

	long *pSerial = (long*)pSharedMemory->GetSharedMemory();

	assert(strlen(argv[2]) == 13);

	*pSerial = atol(argv[2]);

	pSharedMemory->DetachSharedMemory();

	//S->RemoveSharedMemory();
}

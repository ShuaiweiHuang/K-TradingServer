#ifndef SKERRORMESSAGE_H_
#define SKERRORMESSAGE_H_

#include <map>
#include <string>

using namespace std;

class CSKErrorMessage
{
	private:
		map<int, string> m_mErrorMessage;

	protected:

	public:
		CSKErrorMessage();
		virtual ~CSKErrorMessage();

		const char* GetErrorMessage(int nError);
};
#endif

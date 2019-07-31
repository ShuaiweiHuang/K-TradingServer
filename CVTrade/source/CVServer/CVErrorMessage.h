#ifndef CVERRORMESSAGE_H_
#define CVERRORMESSAGE_H_

#include <map>
#include <string>

using namespace std;

class CCVErrorMessage
{
	private:
		map<int, string> m_mErrorMessage;

	protected:

	public:
		CCVErrorMessage();
		virtual ~CCVErrorMessage();

		const char* GetErrorMessage(int nError);
};
#endif

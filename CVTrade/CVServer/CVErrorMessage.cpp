#include "CVErrorMessage.h"

CCVErrorMessage::CCVErrorMessage()
{
	string str1100 = "Trade type error";
	string str1101 = "Agent ID error";
	string str1102 = "Order Offset error";
	string str1103 = "Order Dayoff error";
	string str1104 = "Order type error";
	string str1105 = "Buysell field error";
	string str1106 = "Order condition error";
	string str1107 = "Order mark error";
	string str1108 = "Price mark error";
	string str1109 = "Price error";
	string str1110 = "Touch price error";
	string str1111 = "quantity mark error";
	string str1112 = "quantity error";
	string str1113 = "order kind error";
	string str1114 = "please login first";

	m_mErrorMessage.insert(std::pair<int, string>(1100, str1100));
	m_mErrorMessage.insert(std::pair<int, string>(1101, str1101));
	m_mErrorMessage.insert(std::pair<int, string>(1102, str1102));
	m_mErrorMessage.insert(std::pair<int, string>(1103, str1103));
	m_mErrorMessage.insert(std::pair<int, string>(1104, str1104));
	m_mErrorMessage.insert(std::pair<int, string>(1105, str1105));
	m_mErrorMessage.insert(std::pair<int, string>(1106, str1106));
	m_mErrorMessage.insert(std::pair<int, string>(1107, str1107));
	m_mErrorMessage.insert(std::pair<int, string>(1108, str1108));
	m_mErrorMessage.insert(std::pair<int, string>(1109, str1109));
	m_mErrorMessage.insert(std::pair<int, string>(1110, str1110));
	m_mErrorMessage.insert(std::pair<int, string>(1111, str1111));
	m_mErrorMessage.insert(std::pair<int, string>(1112, str1112));
	m_mErrorMessage.insert(std::pair<int, string>(1113, str1113));
	m_mErrorMessage.insert(std::pair<int, string>(1114, str1114));
}

CCVErrorMessage::~CCVErrorMessage()
{
}

const char* CCVErrorMessage::GetErrorMessage(int nError)
{
	return m_mErrorMessage[-nError].c_str();
}

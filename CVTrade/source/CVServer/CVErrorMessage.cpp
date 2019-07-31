#include "CVErrorMessage.h"

CCVErrorMessage::CCVErrorMessage()
{
	string str1100 = "trade_type欄位錯誤！";
	string str1101 = "委託數量欄位錯誤！";
	string str1102 = "委託數量欄位須帶正負號！";
	string str1103 = "改價單，委託數量須為0！";
	string str1105 = "委託價格欄位錯誤！";
	string str1106 = "委託價格欄位須帶正負號！";
	string str1107 = "委託價格分子欄位錯誤！";
	string str1108 = "委託價格分母欄位錯誤！";
	string str1109 = "觸發價欄位錯誤！";
	string str1110 = "觸發價欄位須帶正負號！";
	string str1111 = "觸發價分子欄位錯誤！";
	string str1112 = "觸發價分母欄位錯誤！";
	string str1113 = "履約價格1欄位錯誤！";
	string str1114 = "履約價格2欄位錯誤！";
	string str1115 = "市價單、保護市價單，委託價格與委託價格分子須為0！";
	string str1116 = "市價單、停損市價單，委託價格須為0！";
	string str1117 = "市價單、限價單，觸發價與觸發價分子須為0！";
	string str1121 = "買賣別欄位錯誤！";
	string str1122 = "買賣別1欄位錯誤！";
	string str1123 = "買賣別2欄位錯誤！";
	string str1124 = "分公司、帳號或子帳欄位錯誤！";
	string str1125 = "盤別欄位錯誤！";
	string str1126 = "下單ID欄位錯誤！";
	string str1127 = "新平倉欄位錯誤！";
	string str1128 = "商品型態欄位錯誤！";
	string str1129 = "當沖欄位錯誤！";
	string str1130 = "委託別欄位錯誤！";
	string str1131 = "停損單記號欄位錯誤";
	string str1132 = "價差單不接受停損市價單或停損限價單！";
	string str1133 = "委託條件錯誤！";
	string str1134 = "價格類別錯誤！";

	string str1200 = "請輸入服務名稱！";

	m_mErrorMessage.insert(std::pair<int, string>(1100, str1100));
	m_mErrorMessage.insert(std::pair<int, string>(1101, str1101));
	m_mErrorMessage.insert(std::pair<int, string>(1102, str1102));
	m_mErrorMessage.insert(std::pair<int, string>(1103, str1103));
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
	m_mErrorMessage.insert(std::pair<int, string>(1115, str1115));
	m_mErrorMessage.insert(std::pair<int, string>(1116, str1116));
	m_mErrorMessage.insert(std::pair<int, string>(1117, str1117));
	m_mErrorMessage.insert(std::pair<int, string>(1121, str1121));
	m_mErrorMessage.insert(std::pair<int, string>(1122, str1122));
	m_mErrorMessage.insert(std::pair<int, string>(1123, str1123));
	m_mErrorMessage.insert(std::pair<int, string>(1124, str1124));
	m_mErrorMessage.insert(std::pair<int, string>(1125, str1125));
	m_mErrorMessage.insert(std::pair<int, string>(1126, str1126));
	m_mErrorMessage.insert(std::pair<int, string>(1127, str1127));
	m_mErrorMessage.insert(std::pair<int, string>(1128, str1128));
	m_mErrorMessage.insert(std::pair<int, string>(1129, str1129));
	m_mErrorMessage.insert(std::pair<int, string>(1130, str1130));
	m_mErrorMessage.insert(std::pair<int, string>(1131, str1131));
	m_mErrorMessage.insert(std::pair<int, string>(1132, str1132));
	m_mErrorMessage.insert(std::pair<int, string>(1133, str1133));
	m_mErrorMessage.insert(std::pair<int, string>(1134, str1134));

	m_mErrorMessage.insert(std::pair<int, string>(1200, str1200));
}

CCVErrorMessage::~CCVErrorMessage()
{
}

const char* CCVErrorMessage::GetErrorMessage(int nError)
{
	//return const_cast<char*>(m_mErrorMessage[-nError].c_str());
	return m_mErrorMessage[-nError].c_str();
}

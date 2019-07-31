#include "CVErrorMessage.h"

CCVErrorMessage::CCVErrorMessage()
{
	string str1100 = "trade_type�����~�I";
	string str1101 = "�e�U�ƶq�����~�I";
	string str1102 = "�e�U�ƶq��춷�a���t���I";
	string str1103 = "�����A�e�U�ƶq����0�I";
	string str1105 = "�e�U���������~�I";
	string str1106 = "�e�U������춷�a���t���I";
	string str1107 = "�e�U������l�����~�I";
	string str1108 = "�e�U������������~�I";
	string str1109 = "Ĳ�o�������~�I";
	string str1110 = "Ĳ�o����춷�a���t���I";
	string str1111 = "Ĳ�o�����l�����~�I";
	string str1112 = "Ĳ�o�����������~�I";
	string str1113 = "�i������1�����~�I";
	string str1114 = "�i������2�����~�I";
	string str1115 = "������B�O�@������A�e�U����P�e�U������l����0�I";
	string str1116 = "������B���l������A�e�U���涷��0�I";
	string str1117 = "������B������AĲ�o���PĲ�o�����l����0�I";
	string str1121 = "�R��O�����~�I";
	string str1122 = "�R��O1�����~�I";
	string str1123 = "�R��O2�����~�I";
	string str1124 = "�����q�B�b���Τl�b�����~�I";
	string str1125 = "�L�O�����~�I";
	string str1126 = "�U��ID�����~�I";
	string str1127 = "�s���������~�I";
	string str1128 = "�ӫ~���A�����~�I";
	string str1129 = "��R�����~�I";
	string str1130 = "�e�U�O�����~�I";
	string str1131 = "���l��O�������~";
	string str1132 = "���t�椣�������l������ΰ��l������I";
	string str1133 = "�e�U������~�I";
	string str1134 = "�������O���~�I";

	string str1200 = "�п�J�A�ȦW�١I";

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

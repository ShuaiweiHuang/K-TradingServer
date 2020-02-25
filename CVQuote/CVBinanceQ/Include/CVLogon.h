#ifndef INCLUDE_LOGON_H_
#define INCLUDE_LOGON_H_

struct LogonType
{
	char LogonID[10];
	//char SellerID[6];
	//char InvAcno[7];
	char Passwd[50];
	char Agent[2];
	char Version[10];
};

struct LogonReplyType
{
	char StatusCode[4];//XXXX
	char ErrorCode[4];//MXXX
	char ErrorMessage[512];
	char BackupIP[20];
};

#endif

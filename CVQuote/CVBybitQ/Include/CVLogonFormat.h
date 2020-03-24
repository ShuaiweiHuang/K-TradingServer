#ifndef INCLUDE_CVLOGONFORMAT_H_
#define INCLUDE_CVLOGONFORMAT_H_

struct TCVLogon
{
	char logon_id[10];
	//char SellerID[6];
	//char InvAcno[7];
	char password[50];
	char agent[2];
	char version[10];
};

struct TCVLogonReply
{
	char status_code[4];//XXXX
	char error_code[4];//MXXX
	char error_message[512];
	char backup_ip[20];
};

#endif

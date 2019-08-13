#ifndef INCLUDE_LOGON_H_
#define INCLUDE_LOGON_H_

struct LogonType
{
	char control_bit[2];
	char LogonID[10];
	char Passwd[40];
	char Agent[2];
	char Version[10];
};

struct LogonReplyType
{
	char control_bit[2];
	char StatusCode[2];
	char BackupIP[40];
	char ErrorCode[4];
	char ErrorMessageCHT[60];
	char ErrorMessageENG[60];
};

#endif

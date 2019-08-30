#ifndef INCLUDE_SKTIGSERVICE_H_
#define INCLUDE_SKTIGSERVICE_H_

/*
Service ID         (20Bytes)
Comma   		   (1Byte)
PATHMON Name       (16Bytes)
Comma              (1Byte)
PATHWAY Server Name(16Bytes)
Comma              (1Byte)
Comment            (20Bytes)
Comma              (1Byte)
Prefix             (2Bytes)
EOL                (2Bytes)
*/

struct TSKTIGService{
	unsigned char uncaServiceID[20];
	unsigned char uncaComma1[1];
	unsigned char uncaPATHMONName[16];
	unsigned char uncaComma2[1];
	unsigned char uncaPATHWAYServerName[16];
	unsigned char uncaComma3[1];
	unsigned char uncaComment[20];
	unsigned char uncaComma4[1];
	unsigned char uncaPrefix[2];
};

#endif

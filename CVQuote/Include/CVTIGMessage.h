#ifndef INCLUDE_CVTIGMESSAGE_H_
#define INCLUDE_CVTIGMESSAGE_H_


struct TCVTIGMessage 
{
	char caLength[4];
	char caType[4];
	char caServiceId[20];//for this, unsigned -> signed
	char caOTSId[12];
	char caTag[20];
	char caError[8];
};

#endif /* INCLUDE_CVTIGMESSAGE_H_ */

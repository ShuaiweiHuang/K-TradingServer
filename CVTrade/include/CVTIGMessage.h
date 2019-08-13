/*
 * SKTIGMessage.h
 *
 *  Created on: 2015年11月24日
 *      Author: alex
 */

#ifndef INCLUDE_SKTIGMESSAGE_H_
#define INCLUDE_SKTIGMESSAGE_H_

/*
Tx/Rx Message Format
Message Length (4Bytes)
Message Type   (4Bytes)
Service ID     (20Bytes)
OTS ID         (12Bytes)
Message Tag    (20Bytes)
Error code     (8Bytes)
USER DATA      (MAX 8123Bytes)
*/
/*
①	Message Length – Total data size including the message text (Maximum 8191 bytes)
②	Message Type – Message type
  Kind	Type	Description	              Data Flow
------------------------------------------------------------------------------
Control  HTBT  Heartbeat message		OTS <--> Tandem
Message	HTBR	Heartbeat Reply		OTS <--> Tandem
       	SVON	Service on	      	Tandem -> OTS
      	SVOF	Service off	      	Tandem -> OTS
Data	   DARQ	Request Data	         OTS -> Tandem
      	DARP	Replay Data	         Tandem -> OTS

-------------------------------------------------------------------------------
③	Service ID: Data necessary to route the corresponding data from the Tandem. Used only when the message type is DARQ or DARP.
DARQ	Sets the transaction number in the OTS.
DARP	Sets the transaction number previously set in the Tandem for the DARQ message.

④	OTS ID: Hostname of each OTS.  Used only when the message type is DARQ or DARP.
DARQ	Sets the hostname in the OTS.
DARP	Sets the hostname previously set in the Tandem for the DARQ message.

⑤	Message Tag: Message tag generated by OTS. That is unique in one OTS’s outstanding messages (suggest unique in one Day). Used only when the message type is DARQ or DARP.
DARQ	Sets the message tag in the OTS.
DARP	Sets the message tag previously set in the Tandem for the DARQ message.

⑥	Error code: The Left 4 digits indicate the PATHSEND error; the Right 4 digits indicate the file-system error. Error code “00000000” means send/recv Pathway server successful (application may be reply error that will be indicated in USER DATA). If send/recv Pathway server failed, the error code will > 0, for example “09040201”. Used only when the message type is DARQ or DARP.
DARQ	Sets “00000000” as the default in the OTS.
DARP	Sets the Left 4 digits form PATHSEND error and sets the Right 4 digits from the file-system error.

⑦	User data: Actual user data. Used only when the message type is DARQ or DARP.
*/

/*struct TSKTIGMessage {
	unsigned char uncaLength[4];
	unsigned char uncaType[4];
	unsigned char uncaServiceId[20];
	unsigned char uncaOTSId[12];
	unsigned char uncaTag[20];
	unsigned char uncaError[8];
};*/

struct TSKTIGMessage 
{
	char caLength[4];
	char caType[4];
	char caServiceId[20];//for this, unsigned -> signed
	char caOTSId[12];
	char caTag[20];
	char caError[8];
};

#endif /* INCLUDE_SKTIGMESSAGE_H_ */
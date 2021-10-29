/*
 * UdpStatusMessage.h
 *
 *  Created on: 4 de ago de 2020
 *      Author: user
 */

#ifndef MESSAGES_H_
#define MESSAGES_H_

#include "globals.h"

extern size_t maxMessageSize;
// header[1]:	0 - don't notify to cloud
//				1 - notify to cloud
#define MSG_DONT_NOTIFY_CLOUD 0
#define MSG_NOTIFY_CLOUD 1
// header[0]:	0 - status request
//				1 - status reply
//				2 - button change
#define MSG_STATUSREQ 0
#define MSG_STATUSRES 1
typedef struct {
	uint8_t header[4];
    char nodeName[MAX_NAME_LEN];
    char relays[MAX_NAME_LEN * 3];
    char buttons[MAX_NAME_LEN * 3];
    uint8_t relayStatus[4];
    // uint8 nodetype
    // np status, alarm status, etc.
} MsgStatus;
typedef struct {
	uint8_t header[4];
    char nodeName[MAX_NAME_LEN];
    char relays[MAX_NAME_LEN * 3];
    char buttons[MAX_NAME_LEN * 3];
    uint8_t relayStatus[4];
    uint8_t nodetype;
    // np status, alarm status, etc.
    uint8_t npProgram;
    uint8_t colorR;
    uint8_t colorG;
    uint8_t colorB;
} MsgStatusNeoPixel;

#define MSG_BTNCLICK 2
typedef struct {
	uint8_t header[4];
    char nodeName[MAX_NAME_LEN];
    char button[MAX_NAME_LEN];
} MsgButtonClick;

#define MSG_RELAYSTATECHANGE 3

#define MSG_RELAYSTATECHANGE_TOGGLE 2
#define MSG_RELAYSTATECHANGE_ON 1
#define MSG_RELAYSTATECHANGE_OFF 0
typedef struct {
	uint8_t header[4];
    char nodeName[MAX_NAME_LEN];
    char relay[MAX_NAME_LEN];
    uint8_t state;
} MsgRelayStateChange;

#define MSG_NEOPIXELSTATECHANGE 3
typedef struct {
	uint8_t header[4];
    char nodeName[MAX_NAME_LEN];
	uint8_t flags[4]; // byte0 - program
	uint32_t  startTime; // quantidade de minutos após meia noite OU a data de início UnixEpoch seconds
	uint16_t duration; // seconds
	uint8_t intensityR;
	uint8_t intensityG;
	uint8_t intensityB;
} MsgNeoPixelStateChange;

// notifica que o estado mudou!!!! ---- usado para ALEXA ou para atualizar uma central
#define MSG_RELAYSTATECHANGE_REPORT 4

void buildMsgStatusReq(MsgStatus * msg);
void buildMsgStatusRes(MsgStatus * msg);
/**
 * btnNum : 0, 1, 2
 */
void buildMsgButtonClick(MsgButtonClick * msg, uint8_t btnNum, bool notifyCloud);
/**
 * relayNum : 0, 1
 */
void buildMsgRelayStateChange(MsgRelayStateChange * msg, uint8_t relayNum, uint8_t state, bool notifyCloud);
void buildMsgRelayStateChange(MsgRelayStateChange * msg, const char * relayName, uint8_t state, bool notifyCloud);
void buildMsgRelayStateChangeReport(MsgRelayStateChange * msg, uint8_t relayNum, uint8_t state);
void buildMsgRelayStateChangeReport(MsgRelayStateChange * msg, const char * relayName, uint8_t state);

#endif /* MESSAGES_H_ */

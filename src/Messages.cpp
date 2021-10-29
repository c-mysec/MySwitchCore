/*
 * UdpStatusMessage.cpp
 *
 *  Created on: 4 de ago de 2020
 *      Author: user
 */

#include <mySwitchConfig.h>
#include "Messages.h"


size_t maxMessageSize = sizeof(MsgStatus);

void buildMsgStatusReq(MsgStatus * msg) {
	memset(msg, 0, sizeof(msg));
	msg->header[0] = MSG_STATUSREQ;
	strncpy(msg->nodeName, configValues.nodeName, MAX_NAME_LEN);
	memcpy(msg->relays, configValues.relays, MAX_NAME_LEN * NUM_RELAYS);
	memcpy(msg->buttons, configValues.buttons, MAX_NAME_LEN * NUM_BUTTONS);
	if (configValues.relayPins[0] != 255) {
		msg->relayStatus[0] = digitalRead(configValues.relayPins[0])?1:0;
	} else {
		msg->relayStatus[0] = 255;
	}
	Serial.print(F("Buildmsg status relay 0 pin "));
	Serial.print(configValues.relayPins[0]);
	Serial.print(F(" value "));
	Serial.println(msg->relayStatus[0]);
	if (configValues.relayPins[1] != 255) {
		msg->relayStatus[1] = digitalRead(configValues.relayPins[1]);
	} else {
		msg->relayStatus[1] = 255;
	}
	Serial.print(F("Buildmsg status relay 1 pin "));
	Serial.print(configValues.relayPins[1]);
	Serial.print(F(" value "));
	Serial.println(msg->relayStatus[1]);
	if (configValues.relayPins[2] != 255) {
		msg->relayStatus[2] = digitalRead(configValues.relayPins[2]);
	} else {
		msg->relayStatus[2] = 255;
	}
	Serial.print(F("Buildmsg status relay 2 pin "));
	Serial.print(configValues.relayPins[2]);
	Serial.print(F(" value "));
	Serial.println(msg->relayStatus[2]);
}
void buildMsgStatusRes(MsgStatus * msg) {
	buildMsgStatusReq(msg);
	msg->header[0] = MSG_STATUSRES;
}
/**
 * btnNum : 0, 1, 2
 */
void buildMsgButtonClick(MsgButtonClick * msg, uint8_t btnNum, bool notifyCloud) {
	memset(msg, 0, sizeof(msg));
	msg->header[0] = MSG_BTNCLICK;
	msg->header[1] = notifyCloud ? MSG_NOTIFY_CLOUD : MSG_DONT_NOTIFY_CLOUD;
	strncpy(msg->nodeName, configValues.nodeName, MAX_NAME_LEN);
	strncpy(msg->button, getConfigButtonName(btnNum), MAX_NAME_LEN);
}
/**
 * relayNum : 0, 1
 */
void buildMsgRelayStateChange(MsgRelayStateChange * msg, uint8_t relayNum, uint8_t state, bool notifyCloud) {
	memset(msg, 0, sizeof(msg));
	msg->header[0] = MSG_RELAYSTATECHANGE;
	strncpy(msg->nodeName, configValues.nodeName, MAX_NAME_LEN);
	strncpy(msg->relay, getConfigRelayName(relayNum), MAX_NAME_LEN);
	msg->state = state;
}
void buildMsgRelayStateChange(MsgRelayStateChange * msg, const char * relayName, uint8_t state, bool notifyCloud) {
	memset(msg, 0, sizeof(msg));
	msg->header[0] = MSG_RELAYSTATECHANGE;
	strncpy(msg->nodeName, configValues.nodeName, MAX_NAME_LEN);
	strncpy(msg->relay, relayName, MAX_NAME_LEN);
	msg->state = state;
}
void buildMsgRelayStateChangeReport(MsgRelayStateChange * msg, uint8_t relayNum, uint8_t state) {
	buildMsgRelayStateChange(msg, relayNum, state, true);
	msg->header[0] = MSG_RELAYSTATECHANGE_REPORT;
}
void buildMsgRelayStateChangeReport(MsgRelayStateChange * msg, const char * relayName, uint8_t state) {
	buildMsgRelayStateChange(msg, relayName, state, true);
	msg->header[0] = MSG_RELAYSTATECHANGE_REPORT;
}

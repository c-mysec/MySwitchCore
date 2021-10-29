/*
 * UdpBroadcast.h
 *
 *  Created on: 5 de ago de 2020
 *      Author: user
 */

#ifndef UDPBROADCAST_H_
#define UDPBROADCAST_H_
#include "globals.h"

void udpSetup();
void udpSendStatusReq();
void udpSendStatusRes();
void udpSendButtonClick(uint8_t btnNum, bool notifyCloud);
void udpSendRelayChange(const char * name, uint8_t state, bool notifyCloud);
void udpSendRelayChangeReport(const char * name, uint8_t state);
void udpLoop();
#endif /* UDPBROADCAST_H_ */

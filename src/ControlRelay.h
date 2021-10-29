/*
 * ControlRelay.h
 *
 *  Created on: 5 de ago de 2020
 *      Author: user
 */

#ifndef CONTROLRELAY_H_
#define CONTROLRELAY_H_

#include <mySwitchConfig.h>

bool turnOnRelay(const char * relayName, bool notifyToCloud);
bool turnOffRelay(const char * relayName, bool notifyToCloud);
bool toggleRelay(const char * relayName, bool notifyToCloud);

#endif /* CONTROLRELAY_H_ */

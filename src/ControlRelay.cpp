/*
 * ControlRelay.cpp
 *
 *  Created on: 5 de ago de 2020
 *      Author: user
 */

#include "ControlRelay.h"
#include "Web.h"
#include "globals.h"
#include "UdpBroadcast.h"
#include "IotCore.h"

int isOurRelay(const char * relayName) {
	Serial.print(relayName);
	Serial.println(strlen(relayName));
	if (configValues.relayPins[0] != 255 && strncmp(relayName, getConfigRelayName(0), MAX_NAME_LEN) == 0) {
		// yeah, we have the relay
		Serial.print(F(" Relay 0 found "));
		Serial.println(getConfigRelayName(0));
		return 0;
	} else if (configValues.relayPins[1] != 255 && strncmp(relayName, getConfigRelayName(1), MAX_NAME_LEN) == 0) {
		// yeah, we have the relay
		Serial.print(F(" Relay 1 found "));
		Serial.println(getConfigRelayName(1));
		return 1;
	} else if (configValues.relayPins[2] != 255 && strncmp(relayName, getConfigRelayName(2), MAX_NAME_LEN) == 0) {
		// yeah, we have the relay
		Serial.print(F(" Relay 2 found "));
		Serial.println(getConfigRelayName(2));
		return 2;
	}
	Serial.print(F(" Relay 0: "));
	Serial.print(getConfigRelayName(0));
	Serial.print(strlen(getConfigRelayName(0)));
	Serial.print(F(" Relay 1: "));
	Serial.print(getConfigRelayName(1));
	Serial.print(strlen(getConfigRelayName(1)));
	Serial.print(F(" Relay 2: "));
	Serial.print(getConfigRelayName(2));
	Serial.print(strlen(getConfigRelayName(2)));
	Serial.println(F(" Not found"));
	return -1;
}
bool turnOnRelay(const char * relayName, bool notifyToCloud) {
	int relay = isOurRelay(relayName);
	if (relay == -1) {
		return false;
	}
	Serial.print(F("switch on relay: "));
	Serial.print(relay);
	Serial.print(F(" pin "));
	Serial.println(configValues.relayPins[relay]);
	digitalWrite(configValues.relayPins[relay], HIGH);
	if (notifyToCloud) {
		// se estiver configurado, envia para cloud
		if (!sendRelayChangeMessageIotCore(relayName, 0)) {
			// se não estiver configurado, envia por UDP para outro dispositivo
			udpSendRelayChangeReport(relayName, 1);
		}
	}
	return true;
}
bool turnOffRelay(const char * relayName, bool notifyToCloud) {
	int relay = isOurRelay(relayName);
	if (relay == -1) {
		return false;
	}
	Serial.print(F("switch off relay: "));
	Serial.print(relay);
	Serial.print(F(" pin "));
	Serial.println(configValues.relayPins[relay]);
	digitalWrite(configValues.relayPins[relay], LOW);
	if (notifyToCloud) {
		// se estiver configurado, envia para cloud
		if (!sendRelayChangeMessageIotCore(relayName, 0)) {
			// se não estiver configurado, envia por UDP para outro dispositivo
			udpSendRelayChangeReport(relayName, 0);
		}
	}
	return true;
}
bool toggleRelay(const char * relayName, bool notifyToCloud) {
	int relay = isOurRelay(relayName);
	if (relay == -1) {
		return false;
	}
	Serial.print(F("switch toggle relay: "));
	Serial.print(relay);
	Serial.print(F(" pin "));
	Serial.println(configValues.relayPins[relay]);
	uint8_t state = 1;
	if (digitalRead(configValues.relayPins[relay])) {
		state = 0;
	}
	digitalWrite(configValues.relayPins[relay], state);
	if (notifyToCloud) {
		// se estiver configurado, envia para cloud
		if (!sendRelayChangeMessageIotCore(relayName, state)) {
			// se não estiver configurado, envia por UDP para outro dispositivo
			udpSendRelayChangeReport(relayName, state);
		}
	}
	return true;
}

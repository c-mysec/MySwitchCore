/*
 * IotCore.cpp
 *
 *  Created on: Sep 29, 2021
 *      Author: clovis
 */

#include "IotCore.h"
#include "FS.h"
#include "SPIFFS.h"
#ifdef ARDUINO_ARCH_ESP32
#include <WiFi.h>
#endif
#ifdef ARDUINO_ARCH_ESP8266
#include <ESP8266WiFi.h>
#endif

#include <mySwitchConfig.h>
#include <PubSubClient.h>

#include "ControlRelay.h"
#include "UdpBroadcast.h"
#include "NTPSync.h"

String outTopic;
String outMsg;

//const char *AWS_endpoint = "a2g89lg5mw80c9-ats.iot.us-east-1.amazonaws.com";
//"xxxxxxxxxxxxxx-ats.iot.us-west-2.amazonaws.com"; //MQTT broker ip

void callbackIotCore(char *topic, byte *payload, unsigned int length) {
	Serial.print(F("Message arrived ["));
	Serial.print(topic);
	Serial.print(F("] "));
	for (unsigned int i = 0; i < length; i++) {
		Serial.print((char) payload[i]);
	}
	Serial.println();
	//1;4;0;home;niche
	char *ptr = NULL;
	ptr = strtok((char *)payload, ";");
	// ptr -> 1
	ptr = strtok(NULL, ";");
	// ptr -> 4
	int msgType = atoi(ptr);
	ptr = strtok(NULL, ";");
	// ptr -> 0
	int nextState = atoi(ptr);
	ptr = strtok(NULL, ";");
	// ptr -> home
	ptr = strtok(NULL, ";");
	// ptr -> niche
	if (nextState == 0) {
		if (!turnOffRelay(ptr, false)) {
			Serial.print(F("Sending UDP button pressed"));
			udpSendRelayChange(ptr, nextState, false);
		}
	} else {
		if (!turnOnRelay(ptr, false)) {
			Serial.print(F("Sending UDP button pressed"));
			udpSendRelayChange(ptr, nextState, false);
		}
	}
}
WiFiClientSecure wifiSecureClient;
PubSubClient *pubSubClient;
long lastReconnect = 0;
bool configured = false;

long lastMsg = 0;
char msg[50];
int value = 0;

void reconnect() {
// Loop until we're reconnected
	if (!pubSubClient->connected()) {
		Serial.print(F("Attempting MQTT connection..."));
// Attempt to connect
		if (pubSubClient->connect(cloudValues.username)) {
			Serial.println(F("connected"));
// Once connected, publish an announcement...
			//client.publish("outTopic", "hello world");
// ... and resubscribe
			String inTopic(F("myswitch/"));
			inTopic.concat(cloudValues.username);
			inTopic.concat(F("/control"));
			pubSubClient->subscribe(inTopic.c_str());
			Serial.print(F("Subscribing to: "));
			Serial.print(inTopic.c_str());

			// topicos para cada usuário
			// topic/myswitch/userid/data
			// topic/myswitch/userid/control
			// topic/myswitch/userid/config

		} else {
			Serial.print(F("failed, rc="));
			Serial.print(pubSubClient->state());
			Serial.println(F(" try again in 5 seconds"));

			char buf[256];
			wifiSecureClient.getLastSSLError(buf, 256);
			Serial.print(F("WiFiClientSecure SSL error: "));
			Serial.println(buf);

// Wait 5 seconds before retrying
		}
	}
}
void setupIotCore() {
	configured = false;
	if (cloudValues.config[0] != 1) return;
	// Load certificate file
	File cert = SPIFFS.open("/cert.der", "r"); //replace cert.crt eith your uploaded file name
	if (!cert) {
		Serial.println(F("Failed to open cert file"));
		return;
	} else
		Serial.println(F("Success to open cert file"));
	// Load private key file
	File private_key = SPIFFS.open("/private.der", "r"); //replace private eith your uploaded file name
	if (!private_key) {
		Serial.println(F("Failed to open private cert file"));
		return;
	} else
		Serial.println(F("Success to open private cert file"));
	// Load CA file
	File ca = SPIFFS.open("/ca.der", "r"); //replace ca eith your uploaded file name
	if (!ca) {
		Serial.println(F("Failed to open ca "));
		return;
	} else
		Serial.println(F("Success to open ca"));

	wifiSecureClient.setBufferSizes(512, 512);

	wifiSecureClient.setX509Time(timeClient.getEpochTime());
	Serial.print(F("Heap: "));
	Serial.println(ESP.getFreeHeap());

	if (wifiSecureClient.loadCertificate(cert))
		Serial.println(F("cert loaded"));
	else {
		Serial.println(F("cert not loaded"));
		return;
	}

	if (wifiSecureClient.loadPrivateKey(private_key))
		Serial.println(F("private key loaded"));
	else {
		Serial.println(F("private key not loaded"));
		return;
	}

	if (wifiSecureClient.loadCACert(ca))
		Serial.println(F("ca loaded"));
	else {
		Serial.println(F("ca failed"));
		return;
	}
	pubSubClient = new PubSubClient(cloudValues.endpoint, 8883, callbackIotCore, wifiSecureClient); //set MQTT port number to 8883 as per //standard
	Serial.print(F("Heap: "));
	Serial.println(ESP.getFreeHeap());
	outTopic.clear();
	outTopic.concat(F("myswitch/"));
	outTopic.concat(cloudValues.username);
	outTopic.concat(F("/data"));
	configured = true;
}
void loopIotCore() {
	if (!configured) return;
	if (!pubSubClient->connected()) {
		long now = millis();
		if (now - lastReconnect > 5000) {
			reconnect();
		}
	} else {
		pubSubClient->loop();
		long now = millis();
		if (now - lastMsg > 0) {
			lastMsg = now + 36000000;
			++value;
			snprintf(msg, 75, "{\"message\": \"hello world #%ld\"}", value);
			Serial.print(F("Publish message: "));
			Serial.println(msg);
			pubSubClient->publish("outTopic", msg);
			Serial.print(F("Heap: "));
			Serial.println(ESP.getFreeHeap()); //Low heap can cause problems
		}
	}
}
bool sendRelayChangeMessageIotCore(const char * name, uint8_t state) {
	if (!configured) return false;
	outMsg.clear();
	outMsg.reserve(100);
	outMsg.concat(F("{\"username\":\""));
	outMsg.concat(cloudValues.username);
	outMsg.concat(F("\",\"relay\":\""));
	outMsg.concat(name);
	outMsg.concat(F("\",\"state\":"));
	outMsg.concat(state);
	outMsg.concat(F(",\"password\":\""));
	outMsg.concat(cloudValues.pass);
	outMsg.concat(F("\"}"));
	pubSubClient->publish(outTopic.c_str(), outMsg.c_str());
	Serial.print(F("outTopic: "));
	Serial.print(outTopic.c_str());
	Serial.print(F("outMsg: "));
	Serial.print(outMsg.c_str());
	Serial.print(F("Heap: "));
	Serial.println(ESP.getFreeHeap()); //Low heap can cause problems
	return true;
}
/* Mensagem envio para IOT CORE
{
     "username" : "home",
     "relay" : "niche",
     "state" : 1,
     "password" : "Lockheed!"
}
*/
/* Mensagem que vem do IOT CORE ao acionar alexa
    // protocol 1 ; relay state change; powerNextState; home; niche
     * 1;4;0;home;niche
    var buffer = "1;4;" + powerNextState + ";" + request.directive.endpoint.cookie.account + ";" + request.directive.endpoint.cookie.name;
    var params = {
        topic: 'myswitch/'+request.directive.endpoint.cookie.account+'/control',
        payload: buffer,
        qos: 0
    }
 Provavelmente só chega o campo payload
*/


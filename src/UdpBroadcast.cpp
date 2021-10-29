/*
 * UdpBroadcast.cpp
 *
 *  Created on: 5 de ago de 2020
 *      Author: user
 */
#include <ESP8266WiFi.h>
#include <mySwitchConfig.h>
#include <WiFiUdp.h>
#include "UdpBroadcast.h"
#include "Messages.h"
#include "ControlRelay.h"
#include "crypto.h"
#include "IotCore.h"

unsigned int brodcastPort = 5577;
IPAddress broadcastIp = 0xFFFFFFFF;
WiFiUDP udp;
uint32_t hbTimeout;
uint32_t hbNext;
void sendPackage(const char* packet, size_t packetSize) {
	AES aes(keyEncrypt, iv, AES::AES_MODE_128, AES::CIPHER_ENCRYPT);
	int encryptedSize = aes.calcSizeAndPad(packetSize);
	int ivEncryptedSize = encryptedSize + AES_KEY_LENGTH;
	int ivEncryptedHmacSize = ivEncryptedSize + SHA256HMAC_SIZE;
	uint8_t ivEncryptedHmac[ivEncryptedHmacSize];
	// copy IV to our final message buffer
	memcpy(ivEncryptedHmac, iv, AES_KEY_LENGTH);

	// encrypted is a pointer that points to the encypted messages position in our final message buffer
	uint8_t* encrypted = ivEncryptedHmac + AES_KEY_LENGTH;

	printBlock((uint8_t*)packet, packetSize);
	// AES 128 CBC and pkcs7 padding
	aes.process((uint8_t*)packet, encrypted, packetSize);

	Serial.printf("Encrypted (%d bytes)", encryptedSize);

	// computedHmac is a pointer which points to the HMAC position in our final message buffer
	uint8_t* computedHmac = encrypted + encryptedSize;

	// compute HMAC/SHA-256 with keyHmac
	SHA256HMAC hmac(keyHmac, HMAC_KEY_LENGTH);
	hmac.doUpdate(ivEncryptedHmac, ivEncryptedSize);
	hmac.doFinal(computedHmac);

	Serial.print(F("Computed HMAC ")); Serial.println(SHA256HMAC_SIZE);
	printBlock(computedHmac, SHA256HMAC_SIZE);

	Serial.print(F("IV | encrypted | HMAC ")); Serial.println(ivEncryptedHmacSize);
	printBlock(ivEncryptedHmac, ivEncryptedHmacSize);

	udp.beginPacket(broadcastIp, brodcastPort);
	udp.write((const char*)&ivEncryptedHmac, ivEncryptedHmacSize);
	udp.endPacket();

}
void sendHeartBeat() {
	IPAddress ip = WiFi.localIP();
	char packet[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, ip[0], ip[1], ip[2], ip[3]};
	udp.beginPacket(broadcastIp, brodcastPort);
	udp.write(packet, 14);
	udp.endPacket();
	hbNext = millis() + 60000; // 1 minuto
}

uint32_t  calcBroadcastAddress(uint32_t ip, uint32_t subnet) {
	uint32_t broadcastAddress = ip | (~ subnet);
	return broadcastAddress;
}
void udpSetup() {
	Serial.println(F("Starting UDP"));
	udp.begin(brodcastPort);
	IPAddress ip = WiFi.localIP();
	uint32_t lip = ip;
	ip = WiFi.subnetMask();
	uint32_t netmask = ip;
	broadcastIp = calcBroadcastAddress(lip, netmask);
	hbTimeout = millis() + 241000; // 181 segundos
}
void udpSendStatusReq() {
	MsgStatus msg;
	buildMsgStatusReq(&msg);
	sendPackage((const char*)&msg,sizeof(msg));
}
void udpSendStatusRes() {
	MsgStatus msg;
	buildMsgStatusRes(&msg);
	sendPackage((const char*)&msg,sizeof(msg));
}
void udpSendButtonClick(uint8_t btnNum, bool notifyCloud) {
	MsgButtonClick msg;
	buildMsgButtonClick(&msg, btnNum, notifyCloud);
	sendPackage((const char*)&msg,sizeof(msg));
}
void udpSendRelayChange(const char * name, uint8_t state, bool notifyCloud) {
	MsgRelayStateChange msg;
	buildMsgRelayStateChange(&msg, name, state, notifyCloud);
	sendPackage((const char*)&msg,sizeof(msg));
}
void udpSendRelayChangeReport(const char * name, uint8_t state) {
	MsgRelayStateChange msg;
	buildMsgRelayStateChangeReport(&msg, name, state);
	sendPackage((const char*)&msg,sizeof(msg));
}
void udpLoop() {
	if ((long) (millis() - hbTimeout) >= 0) {
		// para isto funcionar devem existir mais de 1 ESP na rede.
		Serial.println("Restart hbTimeout no wifi");
		delay(1000);
		ESP.restart();
	}
	if ((long) (millis() - hbNext) >= 0) {
		sendHeartBeat();
		hbNext = millis() + 60000; // envia a cada minuto
	}
	int cb = udp.parsePacket();
	if (cb)	{
		char packetBuffer[maxMessageSize + AES_KEY_LENGTH + AES_KEY_LENGTH + SHA256HMAC_SIZE]; // 16 padding, 32 hmac, 16 iv
		// We've received a UDP packet, send it to serial
		memset(packetBuffer, 0, sizeof(maxMessageSize));
		int bytesRead = udp.read(packetBuffer, maxMessageSize + AES_KEY_LENGTH + AES_KEY_LENGTH + SHA256HMAC_SIZE);
		if (bytesRead == 14) {
			if (packetBuffer[0] == 0 && packetBuffer[1] == 1 && packetBuffer[2] == 2 && packetBuffer[3] == 3
					 && packetBuffer[4] == 4 && packetBuffer[5] == 5 && packetBuffer[6] == 6 && packetBuffer[7] == 7
					 && packetBuffer[8] == 8 && packetBuffer[9] == 9) {
				hbTimeout = millis() + 241000; // 241 segundos
			}
			return;
		}
		if (bytesRead > 0) {
			Serial.printf("Received %d bytes\n", bytesRead);
			printBlock((unsigned char *) packetBuffer, bytesRead);
			// receivedHmac is a pointer which points to the received HMAC in our decoded message
			uint8_t* receivedHmac = ((uint8_t*)packetBuffer) + bytesRead - SHA256HMAC_SIZE;

			// compute HMAC/SHA-256 with keyHmac
			uint8_t remote_computedHmac[SHA256HMAC_SIZE];
			SHA256HMAC remote_hmac(keyHmac, HMAC_KEY_LENGTH);
			remote_hmac.doUpdate(packetBuffer, bytesRead - SHA256HMAC_SIZE);
			remote_hmac.doFinal(remote_computedHmac);

			Serial.printf("Computed HMAC (%d bytes)", SHA256HMAC_SIZE);
			printBlock(remote_computedHmac, SHA256HMAC_SIZE);

			if (memcmp(receivedHmac, remote_computedHmac, SHA256HMAC_SIZE) == 0) {
				uint8_t iv2[AES_KEY_LENGTH];
				memcpy(iv2, packetBuffer, AES_KEY_LENGTH);
			    Serial.printf("Received IV (%d bytes)", AES_KEY_LENGTH);
			    printBlock(iv2, AES_KEY_LENGTH);



			    // decrypt
			    int decryptedSize = bytesRead - AES_KEY_LENGTH - SHA256HMAC_SIZE;
			    char decrypted[decryptedSize];
			    AES aesDecryptor(keyEncrypt, iv2, AES::AES_MODE_128, AES::CIPHER_DECRYPT);
			    aesDecryptor.process((uint8_t*)packetBuffer + AES_KEY_LENGTH, (uint8_t*)decrypted, decryptedSize);

			    Serial.printf("Decrypted Packet HEX (%d bytes)", decryptedSize);
			    printBlock((uint8_t*)decrypted, decryptedSize);

				uint8_t msgType = decrypted[0];
				Serial.print(F("Received UDP "));
				switch (msgType) {
				case MSG_STATUSREQ: {
					Serial.println(F("MSG_STATUSREQ"));
						// we don't need to know other nodes. Just reply
						udpSendStatusRes();
						break;
					}
				case MSG_STATUSRES: {
						// we don't need to know other nodes statuses.
					Serial.println(F("MSG_STATUSRES"));
						break;
					}
				case MSG_BTNCLICK: {
					Serial.println(F("MSG_BTNCLICK"));
						// look if we have a switch related to the changed button
						MsgButtonClick * btnMsg = (MsgButtonClick *)decrypted;
						toggleRelay(btnMsg->button, btnMsg->header[1] == MSG_NOTIFY_CLOUD);
						break;
					}
				case MSG_RELAYSTATECHANGE: {
					Serial.println(F("MSG_RELAYSTATECHANGE"));
						// look if we have a switch related to the changed button
						MsgRelayStateChange * relayMsg = (MsgRelayStateChange *)decrypted;
						if (relayMsg->state == MSG_RELAYSTATECHANGE_TOGGLE) {
							toggleRelay(relayMsg->relay, relayMsg->header[1] == MSG_NOTIFY_CLOUD);
						} else if (relayMsg->state == MSG_RELAYSTATECHANGE_ON) {
							turnOnRelay(relayMsg->relay, relayMsg->header[1] == MSG_NOTIFY_CLOUD);
						} else {
							turnOffRelay(relayMsg->relay, relayMsg->header[1] == MSG_NOTIFY_CLOUD);
						}
						break;
					}
				case MSG_RELAYSTATECHANGE_REPORT: {
					Serial.println(F("MSG_RELAYSTATECHANGE_REPORT"));
						// look if we have a switch related to the changed button
						MsgRelayStateChange * relayMsg = (MsgRelayStateChange *)decrypted;
						if (relayMsg->state == MSG_RELAYSTATECHANGE_ON) {
							// se estiver configurado, envia para cloud
							sendRelayChangeMessageIotCore(relayMsg->relay, 1);
						} else {
							// se estiver configurado, envia para cloud
							sendRelayChangeMessageIotCore(relayMsg->relay, 0);
						}
						break;
					}
				}
			}
			return;
		}
	}
}

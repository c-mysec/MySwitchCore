/*
 * globals.cpp
 *
 *  Created on: 31 de jul de 2020
 *      Author: user
 */

#include <Arduino.h>
#include <FS.h>
#include <mySwitchConfig.h>



uint8_t keyHash[SHA256_SIZE];
uint8_t* keyEncrypt;
uint8_t* keyHmac;
uint8_t iv[AES_KEY_LENGTH];

SHA256 sha256;

// prints given block of given length in HEX
void printBlock(uint8_t* block, int length) {
  Serial.print(" { ");
  for (int i=0; i<length; i++) {
		if (block[i] < 16) Serial.print('0');
		Serial.print(block[i], HEX);
  }
  Serial.println("}");
}

ConfigValues configValues;
CloudValues cloudValues;

void println(const __FlashStringHelper* parname, const char* buf) {
	Serial.print(parname);
	Serial.println(buf);
}
void println(const __FlashStringHelper* parname, uint8_t num) {
	Serial.print(parname);
	Serial.println(num);
}
void println(const __FlashStringHelper* parname, uint8_t num1, uint8_t num2) {
	Serial.print(parname);
	Serial.print(num1);
	Serial.print(',');
	Serial.println(num2);
}
void println(const __FlashStringHelper* parname, uint8_t num1, uint8_t num2, uint8_t num3) {
	Serial.print(parname);
	Serial.print(num1);
	Serial.print(',');
	Serial.print(num2);
	Serial.print(',');
	Serial.println(num3);
}
void println(const __FlashStringHelper* parname, const char* buf1, const char* buf2) {
	Serial.print(parname);
	Serial.print(buf1);
	Serial.print(',');
	Serial.println(buf2);
}
void println(const __FlashStringHelper* parname, const char* buf1, const char* buf2, const char* buf3) {
	Serial.print(parname);
	Serial.print(buf1);
	Serial.print(',');
	Serial.print(buf2);
	Serial.print(',');
	Serial.println(buf3);
}
bool loadAllConfigs() {
	if (!loadConfig()) return false;
	createUdpKeys();
	loadCloudConfig(); // se retornar false não tem cloud mas tudo funciona normal.
	return true;
}
bool loadConfig() {
	// load config
	memset(&configValues, 0, sizeof(ConfigValues));
	File f1 = SPIFFS.open(F("/config.dat"), "r");
	if (!f1) {
		Serial.println(F("file open failed"));
		strcpy(configValues.passwd, "12345678");
		configValues.buttonPins[0] = 255;
		configValues.buttonPins[1] = 255;
		configValues.buttonPins[2] = 255;
		configValues.buttonPins[3] = 255;
		configValues.relayPins[0] = 255;
		configValues.relayPins[1] = 255;
		configValues.relayPins[2] = 255;
		configValues.relayPins[3] = 255;
  		return false;
	}
	Serial.println(F("Reading file"));
	//int version = f1.read();
	f1.readBytes((char*)&configValues, sizeof(ConfigValues));
	println(F("Router SSID: "), configValues.routerSsid);
	println(F("Router Pass: "), configValues.routerPass);
	println(F("nodeName: "), configValues.nodeName);
	println(F("passwd: "), configValues.passwd);
	println(F("relays: "), configValues.relays, &configValues.relays[MAX_NAME_LEN], &configValues.relays[MAX_NAME_LEN*2]);
	println(F("buttons: "), configValues.buttons, &configValues.buttons[MAX_NAME_LEN], &configValues.buttons[MAX_NAME_LEN*2]);
	println(F("relay pins: "), configValues.relayPins[0], configValues.relayPins[1], configValues.relayPins[2]);
	println(F("button pins: "), configValues.buttonPins[0], configValues.buttonPins[1], configValues.buttonPins[2]);
	return true;
}
bool loadCloudConfig() {
	// load config
	File f1 = SPIFFS.open(F("/cloud.dat"), "r");
	memset(&cloudValues, 0, sizeof(cloudValues));
	if (!f1) {
		Serial.println(F("cloud file open failed"));
//		cloudValues.username[0] = 0;
//		cloudValues.pass[0] = 0;
//		cloudValues.endpoint[0] = 0; // a2g89lg5mw80c9-ats.iot.us-west-2.amazonaws.com
		return false;
	}
	Serial.println(F("Reading cloud file"));
	if (f1.available() < sizeof(cloudValues)) return false; // this is a slave
	// this is a master
	//int version = f1.read();
	f1.readBytes((char*)&cloudValues, sizeof(cloudValues));
	println(F("uname: "), cloudValues.username);
	println(F("pass: "), cloudValues.pass);
	println(F("endpoint: "), cloudValues.endpoint);
	return true;
}
void createUdpKeys() {
	SHA256 sha256;
	uint8_t key[AES_KEY_LENGTH];
	uint8_t key2[AES_KEY_LENGTH] = { 0x1C,0x3E,0x4B,0xAF,0x13,0x4A,0x89,0xC3,0xF3,0x87,0x4F,0xBC,0xD7,0xF3, 0x31, 0x31 };
	// get SHA-256 hash of our secret key to create 256 bits of "key material"
	memcpy(key, key2, AES_KEY_LENGTH);
	memcpy(key, configValues.passwd, strlen(configValues.passwd));
	sha256.doUpdate(key, AES_KEY_LENGTH);
	Serial.print(F("passkey"));
	printBlock(key, AES_KEY_LENGTH);
	sha256.doFinal(keyHash);
	Serial.print(("keyHash"));
	printBlock(keyHash, AES_KEY_LENGTH * 2);
	// keyEncrypt is a pointer pointing to the first 128 bits bits of "key material" stored in keyHash
	// keyHmac is a pointer poinging to the second 128 bits of "key material" stored in keyHashMAC
	keyEncrypt = keyHash;
	keyHmac = keyHash + AES_KEY_LENGTH;
	RNG::fill(iv, AES_KEY_LENGTH);
}
const char * getConfigButtonName(uint8_t btnNum) {
	return &configValues.buttons[MAX_NAME_LEN * btnNum];
}
const char * getConfigRelayName(uint8_t btnNum) {
	return &configValues.relays[MAX_NAME_LEN * btnNum];
}

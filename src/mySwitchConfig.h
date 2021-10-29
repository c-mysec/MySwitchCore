/*
 * globals.h
 *
 *  Created on: 31 de jul de 2020
 *      Author: user
 *
 *      https://c-mysec.github.io/
 *      myswitch/home/data
 *      myswitch/home/control
 *
{
      	"username": "home",
      	"relay":"niche",
      	"state":0
}
 */
#ifndef CONFIG_H_
#define CONFIG_H_
#include "globals.h"
#include "crypto.h"

#define HMAC_KEY_LENGTH 16
#define AES_KEY_LENGTH 16
#define NUM_RELAYS 3
#define NUM_BUTTONS 3

#define BUTTON_TYPE_LATCH 0
#define BUTTON_TYPE_MOMENTARY 1

typedef struct ConfigValues {
	char routerSsid[MAX_ROUTER_STR];
	char routerPass[MAX_ROUTER_STR];
    char nodeName[MAX_NAME_LEN];
	char passwd[MAX_NAME_LEN];
    char relays[MAX_NAME_LEN * NUM_RELAYS];
    char buttons[MAX_NAME_LEN * NUM_BUTTONS];
    uint8_t relayPins[4];
    uint8_t buttonPins[4];
    uint8_t buttonTypes[4];
} ConfigValues;
typedef struct CloudValues {
	uint8_t config[4]; // 1 0 0 0 => this node is the main, else, this is a slave
	char username[MAX_ROUTER_STR];
	char pass[MAX_ROUTER_STR];
	char endpoint[MAX_ROUTER_STR * 2];
} CloudValues;

void printBlock(uint8_t* block, int length);

extern ConfigValues configValues;
extern CloudValues cloudValues;
extern uint8_t* keyEncrypt;
extern uint8_t* keyHmac;
extern uint8_t iv[AES_KEY_LENGTH];

extern SHA256 sha256;

bool loadAllConfigs();
void createUdpKeys();
bool loadConfig();
bool loadCloudConfig();
const char * getConfigButtonName(uint8_t btnNum);
const char * getConfigRelayName(uint8_t btnNum);

#endif /* GLOBALS_H_ */

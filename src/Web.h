/*
 * Web.h
 *
 *  Created on: 7 de ago de 2020
 *      Author: user
 */

#ifndef WEB_H_
#define WEB_H_
#include <Arduino.h>
#include <ESPAsyncWebServer.h>

void webSetup();
void webBegin();
void webLoop();
void verifyReset();
extern int fwupdt;
extern AsyncWebServer httpServer;
extern const char *update_username;
extern char _BINARY[];
extern char _TEXTPLAIN[];
extern char _TEXTHTML[];

#endif /* WEB_H_ */

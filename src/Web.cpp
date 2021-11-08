/*
 * Web.cpp
 *
 *  Created on: 7 de ago de 2020
 *      Author: user
 */

#include "Web.h"
#include "FS.h"
#ifdef ARDUINO_ARCH_ESP32
#include "SPIFFS.h"
#include <WiFi.h>
#endif
#ifdef ARDUINO_ARCH_ESP8266
#include <ESP8266WiFi.h>
#endif
#include <AsyncElegantOTA.h>

#include "Messages.h"
#include "ControlRelay.h"
#include "UdpBroadcast.h"

char _TEXTPLAIN[] PROGMEM = "text/plain";
char _TEXTHTML[] PROGMEM = "text/html";
static const char _BRHOME[] PROGMEM = "<br><a href=\"/\">home</a>";
char _BINARY[] PROGMEM = "binary/octet-stream";
static const char _UPLOADFORM[] PROGMEM
		= "<form method=\"POST\" action=\"/fupload\" enctype=\"multipart/form-data\"><input type=\"file\" name=\"data\" /><input type=\"submit\" name=\"upload\" value=\"Upload\" title=\"Upload Files\"></form><br><a href=\"/\">home</a>";
static const char _RELAYNAME[] PROGMEM = "relayName";
static const char _BUTTONNUM[] PROGMEM = "buttonNum";
static const char _STATE[] PROGMEM = "state";
int fwupdt = 0;
// teste
const char *update_path = "/firmware";
const char *update_username = "admin";
//const char *update_password = "lockheed";
bool reset = false;

//callback functions
void relay1Changed(uint8_t brightness);
void relay2Changed(uint8_t brightness);
void relay3Changed(uint8_t brightness);

AsyncWebServer httpServer(80);

String getContentType(String filename) { // convert the file extension to the MIME type
	if (filename.endsWith(F(".html")))
		return FPSTR(_TEXTHTML);
	else if (filename.endsWith(F(".css")))
		return F("text/css");
	else if (filename.endsWith(F(".js")))
		return F("application/javascript");
	else if (filename.endsWith(F(".ico")))
		return F("image/x-icon");
	else if (filename.endsWith(F(".gz")))
		return F("application/x-gzip");
	return String(FPSTR(_TEXTPLAIN));
}

void handleUpload(AsyncWebServerRequest *request, String filename, size_t index,
		uint8_t *data, size_t len, bool final) {
	if (!request->authenticate(update_username, configValues.passwd))
		return;
	if (!index) {
		Serial.printf(String(F("UploadStart: %s\n")).c_str(), filename.c_str());
		// open the file on first call and store the file handle in the request object
		if (filename.endsWith(F(".dat")) ||
				filename.endsWith(F(".der"))) {
			request->_tempFile = SPIFFS.open("/" + filename, "w");
		} else {
			request->_tempFile = SPIFFS.open(String(F("/www/")) + filename, "w");
		}
	}
	if (len) {
		// stream the incoming chunk to the opened file
		request->_tempFile.write(data, len);
	}
	if (final) {
		Serial.printf(String(F("UploadEnd: %s, %u B\n")).c_str(),
				filename.c_str(), index + len);
		// close the file handle as the upload is now done
		request->_tempFile.close();
		request->send(200, FPSTR(_TEXTHTML), F("File Uploaded !. <a href=\"fupload\">voltar</a>"));
		if (filename.equals(F("config.dat"))) loadConfig();
	}
}
void webSetup() {
	httpServer.serveStatic("/", SPIFFS, "/www/").setCacheControl("max-age=600").setDefaultFile("index.html").setAuthentication(
			update_username, configValues.passwd);

//	httpServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
//		request->redirect(F("/index.html"));
//		delay(100);
//	});
	httpServer.on("/ver", HTTP_GET, [](AsyncWebServerRequest *request) {
		String t(F("1.0.1"));
		request->send(200, FPSTR(_TEXTHTML),t);
	});
	httpServer.on("/dir", HTTP_GET, [](AsyncWebServerRequest *request) {
		File dir = SPIFFS.open("/");
		String t;
		t.reserve(500);
		File f = dir.openNextFile();
		while (f) {
			t.concat(F("<br><a href=\"/delete?f="));
		    t.concat(f.name());
		    t.concat(F("\">"));
		    t.concat(f.name());
		    t.concat(F("</a>"));
		    t.concat(F("<br>"));
			f = dir.openNextFile();
		}
		t.concat(F("<br><a href=\"/format\">format</a>"));
		t.concat(F("<br><a href=\"/\">home1</a>"));
		request->send(200, FPSTR(_TEXTHTML),t);
	});
	httpServer.on(String(F("/delete")).c_str(), HTTP_GET,
			[](AsyncWebServerRequest *request) {
				if (!request->authenticate(update_username, configValues.passwd))
					return request->requestAuthentication();
				String f = request->arg("f");

				bool deleted = SPIFFS.remove(f);
				String t;
				t.reserve(500);
				if(deleted){
					t.concat(F("Success deleting "));
					t.concat(f);
				}else{
					t.concat(F("Error deleting "));
					t.concat(f);
				}
				t.concat(FPSTR(_BRHOME));
				request->send(200, FPSTR(_TEXTHTML),
						t);
			}).setAuthentication(update_username, configValues.passwd);
	httpServer.on(String(F("/format")).c_str(), HTTP_GET,
			[](AsyncWebServerRequest *request) {
				if (!request->authenticate(update_username, configValues.passwd))
					return request->requestAuthentication();
				bool formatted = SPIFFS.format();
				String t;
				t.reserve(500);
				if(formatted){
					t.concat(F("Success formatting"));
				}else{
					t.concat(F("Error formatting"));
				}
				t.concat(FPSTR(_BRHOME));
				request->send(200, FPSTR(_TEXTHTML),
						t);
			}).setAuthentication(update_username, configValues.passwd);

	httpServer.on(String(F("/heap")).c_str(), HTTP_GET,
			[](AsyncWebServerRequest *request) {
				if (!request->authenticate(update_username, configValues.passwd))
					return request->requestAuthentication();
				String t;
				t.reserve(500);
				t.concat(ESP.getFreeHeap());
				t.concat(FPSTR(_BRHOME));
				request->send(200, FPSTR(_TEXTHTML),
						t);
			}).setAuthentication(update_username, configValues.passwd);

	//used to force a reset of the device
	httpServer.on(String(F("/reset")).c_str(), HTTP_GET,
			[](AsyncWebServerRequest *request) {
				if (!request->authenticate(update_username, configValues.passwd))
					return request->requestAuthentication();
				request->send(200, FPSTR(_TEXTHTML), F("resetting device. <script>setTimeout(function(){ location.href='/index.html' }, 3000)</script>"));
				reset = true;
			}).setAuthentication(update_username, configValues.passwd);

	httpServer.on(String(F("/resetwifi")).c_str(), HTTP_GET,
			[](AsyncWebServerRequest *request) {
				if (!request->authenticate(update_username, configValues.passwd))
					return request->requestAuthentication();
				request->send(200, FPSTR(_TEXTPLAIN),
						F("resetting wifi settings ..."));
				delay(1000);
				WiFi.disconnect(true);
				ESP.restart();
			}).setAuthentication(update_username, configValues.passwd);
	httpServer.on(String(F("/fupload")).c_str(), HTTP_GET,
			[](AsyncWebServerRequest *request) {
				if (!request->authenticate(update_username, configValues.passwd))
					return request->requestAuthentication();
				request->send(200, FPSTR(_TEXTHTML), FPSTR(_UPLOADFORM));
			});
	httpServer.on(String(F("/fupload")).c_str(), HTTP_POST,
			[](AsyncWebServerRequest *request) {
				if (!request->authenticate(update_username, configValues.passwd))
					return request->requestAuthentication();
				request->send(200, FPSTR(_TEXTPLAIN), F("OK..."));
			},handleUpload).setAuthentication(update_username, configValues.passwd);

	// Simple Firmware Update Form
	httpServer.on(update_path, HTTP_GET,
			[](AsyncWebServerRequest *request) {
				if (!request->authenticate(update_username, configValues.passwd))
					return request->requestAuthentication();
				request->send(200, FPSTR(_TEXTHTML),
						"<form method='POST' action='/firmware' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form><br><a href=\"/\">home</a>");
			}).setAuthentication(update_username, configValues.passwd);
	httpServer.on(String(F("/config.dat")).c_str(), HTTP_GET, [](AsyncWebServerRequest *request) {
		if (!request->authenticate(update_username, configValues.passwd))
			return request->requestAuthentication();
		AsyncResponseStream *response = request->beginResponseStream(FPSTR(_BINARY));
		response->write((uint8_t*)&configValues, sizeof(ConfigValues));
		request->send(response);
	}).setAuthentication(update_username, configValues.passwd);

	httpServer.on(String(F("/relay")).c_str(), HTTP_GET, [](AsyncWebServerRequest *request) {
//		if (!request->authenticate(update_username, update_password,NULL,false))
//			return request->requestAuthentication(NULL,false);
		if(request->hasParam(FPSTR(_RELAYNAME)) && request->hasParam(FPSTR(_STATE))) {
			String relayName = request->arg(FPSTR(_RELAYNAME));
			String state = request->arg(FPSTR(_STATE));
			uint8_t i =state.toInt();
			Serial.print(F("/relay: "));
			Serial.print(relayName);
			Serial.print(F(", "));
			Serial.print(state);
			Serial.print(F(", "));
			Serial.print(i);
			Serial.print(F(", "));
			switch (i) {
				case MSG_RELAYSTATECHANGE_ON: {
					if (!turnOnRelay(relayName.c_str(), true)) {
						Serial.print(F("Sending UDP turn on relay "));
						Serial.print(relayName);
						udpSendRelayChange(relayName.c_str(), 1, true);
					}
					Serial.println(F("on"));
					break;
				}
				case MSG_RELAYSTATECHANGE_OFF: {
					if (!turnOffRelay(relayName.c_str(), true)) {
						Serial.print(F("Sending UDP turn off relay"));
						Serial.print(relayName);
						udpSendRelayChange(relayName.c_str(), 0, true);
					}
					Serial.println(F("off"));
					break;
				}
				case MSG_RELAYSTATECHANGE_TOGGLE: {
					toggleRelay(relayName.c_str(), true);
					Serial.print(relayName);
					Serial.println(F("toggle"));
					break;
				}
			}
		}
		MsgStatus msg;
		buildMsgStatusRes(&msg);
		AsyncResponseStream *response = request->beginResponseStream(FPSTR(_BINARY));
		response->write((uint8_t*)&msg, sizeof(MsgStatus));
		request->send(response);
	}).setAuthentication(update_username, configValues.passwd);
	httpServer.on(String(F("/button")).c_str(), HTTP_GET, [](AsyncWebServerRequest *request) {
		if (!request->authenticate(update_username, configValues.passwd))
			return request->requestAuthentication();
		if(request->hasParam(FPSTR(_BUTTONNUM))) {
			String buttonNum = request->arg(FPSTR(_BUTTONNUM));
			uint8_t i = buttonNum.toInt();
			Serial.print(F("/button: "));
			Serial.print(buttonNum);
			Serial.print(F(", "));
			Serial.println(i);
			Serial.print(F(", "));
			if (configValues.buttonPins[i] != 255) {
				if (toggleRelay(getConfigButtonName(i), true)) {
					Serial.println(F("toggle"));
					request->send(200, FPSTR(_TEXTHTML),
							F("Ok"));
				} else {
					Serial.println(F("remote toggle"));
					request->send(200, FPSTR(_TEXTHTML),
							F("Command sent"));
					udpSendButtonClick(i, true);
				}
			} else {
				Serial.println(F("Button not found"));
				request->send(200, FPSTR(_TEXTHTML),
						F("Button not found"));
			}
		}
	}).setAuthentication(update_username, configValues.passwd);
	httpServer.onNotFound([](AsyncWebServerRequest *request) {
		  if (request->method() == HTTP_OPTIONS) {
		    request->send(200);
		  } else {
			  request->send(404, FPSTR(_TEXTPLAIN), F("404: Not Found")); // otherwise, respond with a 404 (Not Found) error
		  }
	});


	DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Origin"), F("*"));
	//DefaultHeaders::Instance().addHeader(F("Access-Control-Max-Age"), F("600"));
	DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Methods"), F("PUT,POST,GET,OPTIONS"));
	DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Headers"), F("*"));
	DefaultHeaders::Instance().addHeader(F("Access-Control-Expose-Headers"), F("www-authenticate, WWW-Authenticate"));
}
void webBegin() {
	httpServer.begin(); // a linha abaixo faz isto
}
void relay1Changed(uint8_t brightness) {
	Serial.printf(String(F("[MAIN] led1 state: %s value: %d\n")).c_str(),
			brightness ? "ON" : "OFF", brightness);
	digitalWrite(configValues.relayPins[0], brightness ? HIGH : LOW);
	// todo: send UDP status update
}
void relay2Changed(uint8_t brightness) {
	Serial.printf(String(F("[MAIN] led2 state: %s value: %d\n")).c_str(),
			brightness ? "ON" : "OFF", brightness);
	digitalWrite(configValues.relayPins[1], brightness ? HIGH : LOW);
	// todo: send UDP status update
}
void relay3Changed(uint8_t brightness) {
	Serial.printf(String(F("[MAIN] led3 state: %s value: %d\n")).c_str(),
			brightness ? "ON" : "OFF", brightness);
	digitalWrite(configValues.relayPins[2], brightness ? HIGH : LOW);
	// todo: send UDP status update
}
void verifyReset() {
	if (reset || fwupdt == 3) {
		delay(1000);
		ESP.restart();
	}
}
void webLoop() {
}

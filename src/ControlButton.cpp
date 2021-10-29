/*
 * ControlButton.cpp
 *
 *  Created on: 9 de ago de 2020
 *      Author: user
 */
#include <Arduino.h>
#include <mySwitchConfig.h>
#include "ControlRelay.h"
#include "UdpBroadcast.h"
#include "ControlButton.h"
// global will default to Zero
unsigned long lastDebounceTime[NUM_BUTTONS];
uint8_t buttonState[NUM_BUTTONS];
uint8_t lastButtonState[NUM_BUTTONS];
bool firstRun = true;
void readButtonMomentary(uint8_t num) {
	uint8_t pin = configValues.buttonPins[num];
	if (pin != 255) {
		int reading = digitalRead(pin);
		if (reading != lastButtonState[num]) {
			//Serial.print("c");
			//Serial.print(reading);
			// reset the debouncing timer
			lastDebounceTime[num] = millis();
			lastButtonState[num] = reading;
		} else if ((millis() - lastDebounceTime[num]) > 20) {
			// whatever the reading is at, it's been there for longer than the debounce
			// delay, so take it as the actual current state:
			// if the button state has changed:
			if (reading != buttonState[num]) {
				buttonState[num] = reading;
				//Serial.print("n");
				//Serial.println(reading);
				// only toggle the LED if the new button state is HIGH when using momentary
				// else toggle on every state change
				if (buttonState[num] == HIGH) {
					if (!toggleRelay(getConfigButtonName(num), true)) {
						Serial.print(F("Sending UDP button pressed"));
						udpSendButtonClick(num, true);
					}
				}
			}
		}
	}
}
void readButtons() {
	for (uint8_t num = 0; num < NUM_BUTTONS; num++) {
		if (configValues.buttonTypes[num] == BUTTON_TYPE_MOMENTARY) readButtonMomentary(num);
		uint8_t pin = configValues.buttonPins[num];
		if (pin != 255) {
			int reading = digitalRead(pin);
			if (reading != lastButtonState[num]) {
				if (!firstRun) {
					// if the button state has changed: pressed
					if (!toggleRelay(getConfigButtonName(num), true)) {
						Serial.print(F("Sending UDP button pressed"));
						udpSendButtonClick(num, true);
					}
				}
				lastButtonState[num] = reading;
			}
			firstRun = false;
		}
	}
}

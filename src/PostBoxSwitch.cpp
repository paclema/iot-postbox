#include "PostBoxSwitch.h"

#ifdef ESP32
void ISR_FLAG PostBoxSwitch::isr(){
#elif defined(ESP8266)
void PostBoxSwitch::isr() {
#endif
	// Serial.printf(" \n\n----  isr() for GPIO: %d\n", pin);
	int pinState = digitalRead(pin);
	// Serial.printf(" ----  pinState: %d\n", pinState);

	// Ignore dupe readings
	if ( state == pinState) {
		// Serial.printf(" ----  Ignore dupe readings\n");
		return;
		};

	// Ignore events faster than debounceMs
	if (millis() - lastChange < debounceMs) {
		// Serial.printf(" ----  Ignore events faster than debounceMs\n");
		return;
		};

	lastChange = millis();
	state = pinState;

	// Increase counter if pin status change from close to open
	if (state && lastState != state ){
		count++;
		newChange = true;
		// Serial.printf(" ----  Increase counter if pin status change from close to open: %d\n", count);

	}

	// Handle LED strip on/off if for Switch_2:
	// if (switches[i].name == "Switch_2"){
	// 	if(state && !lastState){
	// 	strip.fill(strip.Color(255,255,255), 0, LED_COUNT);
	// 	strip.setBrightness(BRIGHTNESS);
	// 	strip.show();
	// 	} else if (!state && lastState){
	// 	strip.clear();
	// 	strip.show();
	// 	}
	// }
	
	Serial.printf(" -- GPIO %d state: %s lastState: %s count: %d\n", pin, state ? "true": "false", lastState ? "true": "false", count);
	// lastState = state;


}
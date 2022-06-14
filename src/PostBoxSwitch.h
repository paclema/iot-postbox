
// PostBox:
// struct  PostBoxSwitch{
//   String name;
//   int pin;
//   volatile int state;
//   volatile int lastState;
//   int count;
//   volatile unsigned long lastChange;
// };
#ifndef PostBoxSwitch_H
#define PostBoxSwitch_H
#include <Arduino.h>
#include <FunctionalInterrupt.h>


#ifdef ESP32
  #define ISR_FLAG  ARDUINO_ISR_ATTR
#elif defined(ESP8266)
  #define ISR_FLAG  ICACHE_RAM_ATTR
//   #define ISR_FLAG  IRAM_ATTR
#endif

class PostBoxSwitch {
public:
	PostBoxSwitch(uint8_t reqpin, String initName) : pin(reqpin), name(initName){
		pinMode(pin, INPUT);
		if(digitalRead(pin) == HIGH){
			state = true;
			lastState = state;
			count++;
		} 
		attachInterrupt(pin, std::bind(&PostBoxSwitch::isr,this), CHANGE);
	};
	
	~PostBoxSwitch() {
		detachInterrupt(pin);
	}


	#ifdef ESP32
	void ISR_FLAG isr();
	#elif defined(ESP8266)
	void isr();
	#endif


	uint8_t getPin() { return pin; }
	bool getState() { return state; }
	bool getLastState() { return lastState; }
	uint32_t getCount() { return count; }

	bool checkChange() {
		if (newChange) {
            Serial.printf("Button on pin %u changed %u times\n", pin, count);
            newChange = false;
            return true;
		}
        return false;
	}

  	void readCurrentState() { 
		state = digitalRead(pin);
  		updateLastState();
  	}
  	void updateLastState() { lastState = state; }


    String name;
private:
	const uint8_t pin;
	volatile bool state = false;
	volatile bool lastState = false;
	volatile uint32_t count = 0;
	volatile unsigned long lastChange;

	volatile bool newChange;

	int debounceMs = 50;          // To ignore button signals changes faster than this debounce ms

};

#endif
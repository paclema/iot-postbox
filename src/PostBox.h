#ifndef PostBox_H
#define PostBox_H

#include <Arduino.h>

#ifdef ESP32
#include <esp_wifi.h>
#include <WiFi.h>
#endif


#include <PubSubClient.h>


// WS2812B LED strip
// -----------------
#include <Adafruit_NeoPixel.h>

#ifdef ARDUINO_IOTPOSTBOX_V1
  #define LED_PIN    PIN_NEOPIXEL 	//GPIO17
#else
  #define LED_PIN    15 			//GPIO15
#endif

#define LED_COUNT  4
#define BRIGHTNESS 150


// PostBoxSwitch sensors
// ---------------------
#include "PostBoxSwitch.h"
#ifndef ARDUINO_IOTPOSTBOX_V1
  #define SW1_PIN         12
  #define SW2_PIN         13
  #define KEEP_WAKE_PIN   14    // Pin which is used to Keep ESP awake until job is finished
#endif


// PostBox sleep configs
// ---------------------
#define BUTTON_PIN_BITMASK 0x10 // GPIO4 --> 2^4 in hex
RTC_DATA_ATTR static int bootCount = 0;


// Battery charger feedback
// ------------------------
#ifdef ARDUINO_IOTPOSTBOX_V1
//TODO: BATTERY, USB POWER AND CHARGING SENSE

#else
//Using tp4056 charger with battery fedback to ADC0 (max 1v! for ESP8266) and tp4056 chrg and stdby pins feedback:
  #define USE_TP4056
  #define NO_SLEEP_WHILE_CHARGING    // While charging the ESP won't go to sleep. Comment out this line if you want to avoid that

  #ifdef USE_TP4056
    #define CHRG_PIN    4    // Pin for charge battery. LOW when usb is connected and baterry is being charged.
    #define STDBY_PIN   5    // Pin for stdby battery charge. LOW for battery charge termination.
  #else
    ADC_MODE(ADC_VCC);          // Allows to monitor the internal VCC level; it varies with WiFi load
  #endif
#endif




class PostBox {
public:
	Adafruit_NeoPixel ledStrip;

	PostBoxSwitch sw1;
	PostBoxSwitch sw2;


	String MQTTBaseTopic;
	PubSubClient *mqtt;




	PostBox(void);
	~PostBox(void);

	void setup(void);
	void init(void);
	void loop(void);
	void powerOff(void);

	float readVoltage(void);

	void updateLedStrip(void);

	void printWakeupReason(void);
	void setupDeepSleep(void);
	void turnOffDevice(void);

	void publishWakeUp(String topic_end);

	void setMQTTBaseTopic(String topic) { MQTTBaseTopic = topic; }
	void setMQTTClient(PubSubClient * client) { mqtt = client; }



private:

	bool wakeUpPublished = false;
	int button = -1;              // Variable to store the button which triggered the bootup


};

#endif
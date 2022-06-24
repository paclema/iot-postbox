#ifndef PostBox_H
#define PostBox_H

#include <Arduino.h>

#ifdef ESP32
#include <esp_wifi.h>
#include <WiFi.h>

#pragma once
#include "driver/adc.h"
#include "esp_adc_cal.h"

//ADC Attenuation
#define ADC_EXAMPLE_ATTEN	ADC_ATTEN_DB_6
// #define ADC_WIDTH_BAT_SENSE	ADC_WIDTH_BIT_12

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

enum class PowerStatus {
    Unknown = 0,
    BatteryPowered = 1,
    USBPowered = 2,
    BatteryAndUSBPowered = 3,
};

enum class ChargingStatus {
    Unknown = 0,
    Charging = 1,
    NotCharging = 2,
};


// #define VBUS_SENSE          1
// #define VBAT_SENSE          2
// #define VBAT_STAT_SENSE     3

// #define SW1_PIN             4
// #define SW2_PIN             5

// #define RFM95W_DIO0         6
// #define RFM95W_DIO1         7
// #define RFM95W_DIO2         8
// #define RFM95W_DIO3         9
// #define RFM95W_NSS          10
// #define RFM95W_MOSI         11
// #define RFM95W_SCK          12
// #define RFM95W_MISO         13
// #define RFM95W_RESET        14
// #define RFM95W_DIO4         15
// #define RFM95W_DIO5         16

class PostBox {
public:
	Adafruit_NeoPixel ledStrip;

	PostBoxSwitch sw1;
	PostBoxSwitch sw2;


	String MQTTBaseTopic;
	PubSubClient *mqtt;

	//States
	PowerStatus powerStatus;
	ChargingStatus chargingStatus;

	// int VBUSState;
	// int VBAT_STATUS_State;
	float batVoltage = 0;
	esp_adc_cal_characteristics_t *adc_chars = new esp_adc_cal_characteristics_t;



	PostBox(void);
	~PostBox(void);

	void setup(void);
	void init(void);
	void loop(void);
	void powerOff(void);

	void adc_calibrate(void);
	int analogRead_cal(uint8_t channel, adc_atten_t attenuation);
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
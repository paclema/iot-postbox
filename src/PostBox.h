#ifndef PostBox_H
#define PostBox_H

#pragma once
#include <Arduino.h>

#ifdef ESP32
#include <esp_wifi.h>
#include <WiFi.h>


//ADC Channels
// -----------
#include "ADCSense.h"
#define VBUS_ADC_CHANNEL	ADC1_CHANNEL_0
#define VBAT_ADC_CHANNEL	ADC1_CHANNEL_1

//ADC Attenuation
#define VBUS_ADC_ATTEN	ADC_ATTEN_DB_11
#define VBAT_ADC_ATTEN	ADC_ATTEN_DB_6

// Voltage divider coefficients:
#define VBUS_R9		3.3		// R9 = 3.3KOhm
#define VBUS_R10	2		// R10 = 2KOhm
#define VBUS_VOLTAGE_DIVIDER_COEFICIENT		(VBUS_R9 + VBUS_R10) / VBUS_R10

#define VBAT_R6		442		// R6 = 442KOhm
#define VBAT_R7		160		// R7 = 160KOhm
#define VBAT_VOLTAGE_DIVIDER_COEFICIENT 	(VBAT_R6 + VBAT_R7) / VBAT_R7

#define ADC_SAMPLES 30

#endif


#include <PubSubClient.h>


// WS2812B LED strip
// -----------------
#include <FastLED.h>

#ifdef ARDUINO_IOTPOSTBOX_V1
  #define LED_PIN    PIN_NEOPIXEL 	//GPIO17
#else
  #define LED_PIN    15 			//GPIO15
#endif

#define NUM_LEDS  4
#define BRIGHTNESS 10


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
#define WAKE_UP_BITMASK ( GPIO_SEL_4 | GPIO_SEL_5 )
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
    Charged = 3,
};



class PostBox {
public:
	CRGB leds[NUM_LEDS];

	PostBoxSwitch sw1;
	PostBoxSwitch sw2;


	String MQTTBaseTopic;
	PubSubClient *mqtt;

	//States
	PowerStatus powerStatus = PowerStatus::Unknown;
	PowerStatus lastPowerStatus = PowerStatus::Unknown;
	ChargingStatus chargingStatus = ChargingStatus::Unknown;
	ChargingStatus lastChargingStatus = ChargingStatus::Unknown;

	ADCSense vBatSense;

	float vBat = 0;		// Values in mV
	float vBus = 0;		// Values in mV
	int vBatStat = 0;	// Charging = 0, Not charging = 1
	esp_adc_cal_characteristics_t *VBUS_adc_chars = new esp_adc_cal_characteristics_t;



	PostBox(void);
	~PostBox(void);

	void setup(void);
	void init(void);
	void loop(void);
	void powerOff(void);

	void initADC(void);
	void updatedADC(void);
	void updatePowerStatus(void);
	float readVoltage(void);
	float getVBus(void) { return vBus;};
	float getVBat(void) { return vBat;};
	float getVBatNow(void) { return vBatSense.getLastmV(); };
	float getVBatStat(void) { return vBatStat;};


	void ISR_FLAG isrCharging(){
		vBatStat = digitalRead(VBAT_STAT_SENSE_PIN);

		// if (pinState == HIGH) chargingStatus = ChargingStatus::NotCharging;
		// else if (pinState == LOW) chargingStatus = ChargingStatus::Charging;
		// else chargingStatus = ChargingStatus::Unknown;

		Serial.printf(" -- VBAT_STAT_SENSE_PIN: %s --> ChargingStatus: %d\n",  vBatStat ? "true": "false", chargingStatus);
	};

	ChargingStatus getChargingStatus(void) { return chargingStatus;};
	PowerStatus getPowerStatus(void) { return powerStatus;};
	bool isWakeUpPublished(void) { return wakeUpPublished;};



	void updateLedStrip(void);

	void printWakeupReason(void);
	void setupDeepSleep(void);
	void turnOffDevice(void);

	void publishWakeUp(String topic_end);

	void setMQTTBaseTopic(String topic) { MQTTBaseTopic = topic; }
	void setMQTTClient(PubSubClient * client) { mqtt = client; }



private:

	bool wakeUpPublished = false;
	int wakeUpGPIO = -1;              // Variable to store the GPIO which triggered the wake from Deep Sleep


};

#endif
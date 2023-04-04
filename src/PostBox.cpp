#include "PostBox.h"

PostBox::PostBox(void) : 
	// sw1(0, "Switch_1"),
	sw1(SW1_PIN, "Switch_1"),
	sw2(SW2_PIN, "Switch_2")
  {


}

PostBox::~PostBox(void) {
}

void PostBox::setup(void) {
  #ifndef ARDUINO_IOTPOSTBOX_V1
  // ESP wake Pin, the pin which keeps CH_PD HIGH, a requirement for normal functioning of ESP8266
  pinMode(KEEP_WAKE_PIN, OUTPUT);
  digitalWrite(KEEP_WAKE_PIN, HIGH);
  #else
  pinMode(LDO2_EN_PIN, OUTPUT);
  digitalWrite(LDO2_EN_PIN, HIGH);

  power.setup();
  #endif

  #ifdef USE_TP4056
  pinMode(CHRG_PIN, INPUT);
  pinMode(STDBY_PIN, INPUT);
  #endif

  setupDeepSleep();

  // Setup WS2812B LED
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  fill_solid( leds, NUM_LEDS, CRGB::White);
  FastLED.show();

  //Check which button was pressed
  // int countSwitches = sizeof switches / sizeof *switches;
  // for(int i=0; i< countSwitches; i++) {
  //   pinMode(switches[i].pin, INPUT);
  //   if(digitalRead(switches[i].pin) == HIGH) {
  //     wakeUpGPIO = switches[i].pin;
  //     switches[i].count++;
  //     break;
  //   }
  // }
  // Serial.printf("\n -- Pin booter wakeUpGPIO: %d\n", wakeUpGPIO);
  


  // long bootDelay = millis() - connectionTime;
  // long bootDelay = millis();
  // Serial.printf("\tbootDelay: %ld - pin booter: %d\n", bootDelay, wakeUpGPIO);

  // Disconnect wifi if the reboot was not originated by GPIOs
  // but probably caused by updating firmware by UART
  // if (wakeUpGPIO == -1) WiFi.disconnect();

  updateLedStrip();
}


void PostBox::init(void) {

  // Recheck switches state
  sw1.readCurrentState();
  sw2.readCurrentState();

  //Check Power Management and ADC sense inputs
  updatePowerStatus();
  readVoltage();



  publishWakeUp("wakeup");

  Serial.printf(" -- VBAT_STAT_SENSE_PIN: %s --> ChargingStatus: %d\n",  power.getVBatStat() ? "true": "false", chargingStatus);
  Serial.printf(" -- GPIO %d state: %s lastState: %s count: %d\n", sw1.getPin(), sw1.getState() ? "true": "false", sw1.getLastState() ? "true": "false", sw1.getCount());
  Serial.printf(" -- GPIO %d state: %s lastState: %s count: %d\n", sw2.getPin(), sw2.getState() ? "true": "false", sw2.getLastState() ? "true": "false", sw2.getCount());
  Serial.printf(" -- vBus %1.3fmV vBat: %1.3fmV\n", power.vBusSense.mV, power.vBatSense.mV);

}


void PostBox::loop(void) {

  updatePowerStatus();
  
  // Check if there was an interrupt casued by one PostBoxSwitch
  if ( sw1.checkChange() || sw2.checkChange() ) {
    publishWakeUp("wakeup");
    // publish2TTN();
  }

  // Check the PostBoxSwitch sw2 to tur on/off the LED strip
  if (sw2.getState() != sw2.getLastState()){
    if(sw2.getState() && !sw2.getLastState()) fill_solid( leds, NUM_LEDS, CRGB::White);
    else if (!sw2.getState() && sw2.getLastState()) fill_solid( leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
  }


  if (powerStatus != power.getLastPowerStatus()) {
    if (chargingStatus == ChargingStatus::Charging) leds[3] = CRGB::Red;
    else if (chargingStatus == ChargingStatus::Charged) leds[3] = CRGB::Green;
    else if (chargingStatus == ChargingStatus::NotCharging) leds[3] = CRGB::Blue;
    else if (chargingStatus == ChargingStatus::Unknown) leds[3] = CRGB::Yellow;
    else leds[3] = CRGB::Orange;

    if (powerStatus == PowerStatus::BatteryPowered) leds[2] = CRGB::Red;
    else if (powerStatus == PowerStatus::USBPowered) leds[2] = CRGB::Green;
    else if (powerStatus == PowerStatus::BatteryAndUSBPowered) leds[2] = CRGB::Blue;
    else if (powerStatus == PowerStatus::Unknown) leds[2] = CRGB::Yellow;
    else leds[2] = CRGB::Orange;
    FastLED.show();
  }
  

  // Update PostBoxSwitch states for next loop
  sw1.updateLastState();
  sw2.updateLastState();

}


void PostBox::powerOff(void) {

  fill_solid( leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
  digitalWrite(LDO2_EN_PIN, LOW);
  Serial.println("CH_PD disabled");
  delay(10);

  //TODO: RUTINE FOR ESP32S2 
  #ifndef ARDUINO_IOTPOSTBOX_V1
    digitalWrite(KEEP_WAKE_PIN, LOW); //Turns the ESP OFF
  #endif
  esp_wifi_stop();
  delay(100);
  esp_deep_sleep_start();
  
}


void PostBox::updatePowerStatus(void){
  power.update();
  powerStatus = power.getPowerStatus();
  chargingStatus = power.getChargingStatus();
}


float PostBox::readVoltage(void) {
	#ifdef ARDUINO_IOTPOSTBOX_V1
	  return power.vBatSense.mV/1000;
	#else
	#ifdef USE_TP4056
		int sensorValue = analogRead(A0);
		float volts = sensorValue * (4.333 / 1023.0);
		Serial.printf("The internal VCC reads %1.3f volts. CHRG: %d - STDBY: %d\n", volts , !digitalRead(CHRG_PIN), !digitalRead(STDBY_PIN));
	#else
		float volts = ESP.getVcc();
		Serial.printf("The internal VCC reads %1.1f volts\n", volts / 1000);
		return volts;
	#endif
	#endif
	return -1;
}


void PostBox::updateLedStrip(void) {
  // If Switch_2 (used while opening the postbox) is on, turn on the lights
  // if(digitalRead(switches[1].pin)){
  bool lightMode = false;
  if(digitalRead(sw2.getPin()) == HIGH){
    fill_solid( leds, NUM_LEDS, CRGB::White);
    FastLED.show();
    lightMode = true;
  } else {
    fill_solid( leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
    lightMode = false;
  }

  // Serial.printf("updateLedStrip() done!: lightMode = %s\n", lightMode ? "ON": "OFF");
}

void PostBox::printWakeupReason(void) {
	esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();
  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : {
      uint64_t GPIO_reason = esp_sleep_get_ext1_wakeup_status();
      int gpioWake = log(GPIO_reason)/log(2);
      wakeUpGPIO = gpioWake;
      Serial.printf("Wakeup caused by external signal using RTC_CNTL. GPIO: %d\n", gpioWake);
      break;
      }
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

void PostBox::setupDeepSleep(void) {
	  //Increment boot number and print it every reboot
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));
  printWakeupReason();

  /*
  First we configure the wake up source
  We set our ESP32 to wake up for an external trigger.
  There are two types for ESP32, ext0 and ext1 .
  ext0 uses RTC_IO to wakeup thus requires RTC peripherals
  to be on while ext1 uses RTC Controller so doesnt need
  peripherals to be powered on.
  Note that using internal pullups/pulldowns also requires
  RTC peripherals to be turned on.
  */

  // esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON); // all RTC Peripherals are powered on
  // esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF); // all RTC Peripherals are powered off

  // esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
  // esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
  // esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);
  // esp_sleep_pd_config(ESP_PD_DOMAIN_RTC8M, ESP_PD_OPTION_OFF);
  // esp_sleep_pd_config(ESP_PD_DOMAIN_VDDSDIO, ESP_PD_OPTION_OFF);
  // esp_sleep_pd_config(ESP_PD_DOMAIN_MAX, ESP_PD_OPTION_OFF);

  // esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR); // ESP32 wakes up every 5 seconds
  // esp_sleep_enable_ext0_wakeup(GPIO_NUM_4,1); //1 = High, 0 = Low
  // esp_sleep_enable_ext0_wakeup(GPIO_NUM_4,1); //1 = High, 0 = Low
  esp_sleep_enable_ext1_wakeup(WAKE_UP_BITMASK,ESP_EXT1_WAKEUP_ANY_HIGH);
}

void PostBox::turnOffDevice(void) {
  fill_solid( leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
  digitalWrite(LDO2_EN_PIN, LOW);
  Serial.println("CH_PD disabled");
  delay(10);

  //TODO: RUTINE FOR ESP32S2 
  #ifndef ARDUINO_IOTPOSTBOX_V1
    digitalWrite(KEEP_WAKE_PIN, LOW); //Turns the ESP OFF
  #else degined(ARDUINO_IOTPOSTBOX_V1)
  // TODO: add this functionality to WebConfigServer
    esp_wifi_stop();
    delay(100);
    esp_deep_sleep_start();
  #endif
}

void PostBox::publishWakeUp(String topic_end) {
  String topic = MQTTBaseTopic + topic_end;

  if (topic_end == "wakeup"){
    publish2TTN();
  }
  
  String msg_pub ="{\"wake_up_pin\": ";
  msg_pub += String(wakeUpGPIO);
  msg_pub = msg_pub + " ,\"boot_counter\": " + String(bootCount);
  msg_pub = msg_pub + " ,\"vcc\": " + String(readVoltage(), 3);
  msg_pub = msg_pub + " ,\"vBat\": " + String((float)power.vBatSense.mV/1000, 3);
  msg_pub = msg_pub + " ,\"vBus\": " + String((float)power.vBusSense.mV/1000, 3);
  msg_pub = msg_pub + " ,\"rssi\": " + String(WiFi.RSSI());

  // int countSwitches = sizeof switches / sizeof *switches;
  // for(int i=0; i< countSwitches; i++) {
  //   msg_pub = msg_pub + " ,\"GPIO_" + switches[i].pin + "_counter\": " + String(switches[i].count);
  //   msg_pub = msg_pub + " ,\"GPIO_" + switches[i].pin + "_state\": " + (switches[i].state ? "true" : "false");
  // }

  msg_pub = msg_pub + " ,\"GPIO_" + sw1.getPin() + "_counter\": " + String(sw1.getCount());
  msg_pub = msg_pub + " ,\"GPIO_" + sw1.getPin() + "_state\": " + (sw1.getState() ? "true" : "false");
  msg_pub = msg_pub + " ,\"GPIO_" + sw2.getPin() + "_counter\": " + String(sw2.getCount());
  msg_pub = msg_pub + " ,\"GPIO_" + sw2.getPin() + "_state\": " + (sw2.getState() ? "true" : "false");

  msg_pub = msg_pub + " ,\"PowerStatus\": " + String((int)powerStatus);
  msg_pub = msg_pub + " ,\"ChargingStatus\": " + String((int)chargingStatus);


  #ifdef USE_TP4056
  msg_pub = msg_pub + " ,\"chrg\": " + !digitalRead(CHRG_PIN);
  msg_pub = msg_pub + " ,\"stdby\": " + !digitalRead(STDBY_PIN);
  #endif
  msg_pub +=" }";

  Serial.println(msg_pub);

  mqtt->setBufferSize((uint16_t)(msg_pub.length() + 100));
  wakeUpPublished = mqtt->publish(topic.c_str(), msg_pub.c_str());
}

void PostBox::publish2TTN(void) {

  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND) {
      Serial.printf("*** OP_TXRXPEND, not sending\n");
  } else {
    // Prepare upstream data transmission at the next possible time.

    int index = 0;

    // wakeUpGPIO starts with -1 by default. If it is not changed, it means that the device was not woken up by a GPIO
    // it needs to be incremented by 1 to convert it to uint8_t
    payload[index++] = wakeUpGPIO + 1;

    uint16_t bootCount_uint16_t = bootCount;
    payload[index++] = (bootCount_uint16_t >> 8) & 0xFF;
    payload[index++] = bootCount_uint16_t & 0xFF;

    uint16_t vBat = power.vBatSense.mV;
    payload[index++] = (vBat >> 8) & 0xFF;
    payload[index++] = vBat & 0xFF;

    uint16_t vBus = power.vBusSense.mV;
    payload[index++] = (vBus >> 8) & 0xFF;
    payload[index++] = vBus & 0xFF;

    uint32_t sw1_uint16_t = sw1.getCount();
    payload[index++] = (sw1_uint16_t >> 24) & 0xFF;
    payload[index++] = (sw1_uint16_t >> 16) & 0xFF;
    payload[index++] = (sw1_uint16_t >> 8) & 0xFF;
    payload[index++] = sw1_uint16_t & 0xFF;

    payload[index++] = sw1.getState();

    uint32_t sw2_uint16_t = sw2.getCount();
    payload[index++] = (sw2_uint16_t >> 24) & 0xFF;
    payload[index++] = (sw2_uint16_t >> 16) & 0xFF;
    payload[index++] = (sw2_uint16_t >> 8) & 0xFF;
    payload[index++] = sw2_uint16_t & 0xFF;

    payload[index++] = sw2.getState();

    payload[index++] = (uint8_t)powerStatus;
    payload[index++] = (uint8_t)chargingStatus;



    // Print the payload and its size to the serial port
    Serial.printf("Payload[%d]: ",index);
    for (int i = 0; i < sizeof(payload); i++) {
        Serial.print(payload[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
    LMIC_setTxData2(1, payload, index, 0);
  }
  // Next TX is scheduled after TX_COMPLETE event.
}

#include "PostBox.h"

PostBox::PostBox(void) : 
	ledStrip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800), 
	// sw1(0, "Switch_1"),
	sw1(SW1_PIN, "Switch_1"),
	sw2(SW2_PIN, "Switch_2") {

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

  // Sense inputs
  pinMode(VBUS_SENSE, INPUT);
  pinMode(VBAT_SENSE, INPUT);
  pinMode(VBAT_STAT_SENSE, INPUT);

  #endif

  #ifdef USE_TP4056
  pinMode(CHRG_PIN, INPUT);
  pinMode(STDBY_PIN, INPUT);
  #endif

  setupDeepSleep();

  ledStrip.begin();

  //Check which button was pressed
  // int countSwitches = sizeof switches / sizeof *switches;
  // for(int i=0; i< countSwitches; i++) {
  //   pinMode(switches[i].pin, INPUT);
  //   if(digitalRead(switches[i].pin) == HIGH) {
  //     button = switches[i].pin;
  //     switches[i].count++;
  //     break;
  //   }
  // }
  // Serial.printf("\n -- Pin booter button: %d\n", button);
  


  // long bootDelay = millis() - connectionTime;
  // long bootDelay = millis();
  // Serial.printf("\tbootDelay: %ld - pin booter: %d\n", bootDelay, button);

  // Disconnect wifi if the reboot was not originated by GPIOs
  // but probably caused by updating firmware by UART
  // if (button == -1) WiFi.disconnect();

  updateLedStrip();
}

#include "driver/adc.h"

void PostBox::init(void) {

  // Recheck switches state
  sw1.readCurrentState();
  sw2.readCurrentState();

  //Check sense inputs
  // batVoltage = analogRead(VBAT_SENSE);

  ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_13));
  ESP_ERROR_CHECK(adc1_config_channel_atten(ADC1_CHANNEL_1, ADC_ATTEN_DB_6));
  // adc1_config_width(ADC_WIDTH_BAT_SENSE);
  // adc1_config_channel_atten(ADC1_CHANNEL_1, ADC_EXAMPLE_ATTEN);

  // analogReadResolution(13);

  adc_calibrate();
  readVoltage();

  
  int vbus = digitalRead(VBUS_SENSE);
  int vbat_stat = digitalRead(VBAT_STAT_SENSE);
  Serial.printf(" -- VBUS_SENSE: %d VBAT_STAT_SENSE: %d batVoltage: %d\n", vbus, vbat_stat );

  if( vbus == HIGH && vbat_stat == HIGH) powerStatus = PowerStatus::BatteryAndUSBPowered;
  else if( vbus == HIGH) powerStatus = PowerStatus::USBPowered;
  else if( vbat_stat == HIGH) powerStatus = PowerStatus::BatteryPowered;
  else powerStatus = PowerStatus::Unknown;

  int sume = 0;
  sume = vbat_stat + 2 *vbus;
  PowerStatus sumePower = (PowerStatus)sume;
  Serial.printf(" -- PowerStatus: %d Sume: %d sumePower: %d\n", vbus, sume, sumePower );



  

  publishWakeUp("wakeup");

  Serial.printf(" -- GPIO %d state: %s lastState: %s count: %d\n", sw1.getPin(), sw1.getState() ? "true": "false", sw1.getLastState() ? "true": "false", sw1.getCount());
  Serial.printf(" -- GPIO %d state: %s lastState: %s count: %d\n", sw2.getPin(), sw2.getState() ? "true": "false", sw2.getLastState() ? "true": "false", sw2.getCount());
}


void PostBox::loop(void) {
  #if defined(USE_TP4056) && defined(NO_SLEEP_WHILE_CHARGING)
    if ( (!wakeUpPublished ) || !digitalRead(CHRG_PIN) == true ) config.services.deep_sleep.enabled = false;    
    else config.services.deep_sleep.enabled = true;  
    // TODO: save that deep sleep enabled within the config file instead hardcoding the variable
  #else
    // if ( !wakeUpPublished ) config.services.deep_sleep.enabled = false;    
    // else config.services.deep_sleep.enabled = true; 
    // TODO: save that deep sleep enabled within the config file instead hardcoding the variable

  #endif

  
  // Check if there was an interrupt casued by one PostBoxSwitch
  if ( sw1.checkChange() || sw2.checkChange() ) publishWakeUp("wakeup");

  // Check the PostBoxSwitch sw2 to tur on/off the LED strip
  if (sw2.getState() != sw2.getLastState() ){  	
    if(sw2.getState() && !sw2.getLastState()){
  	ledStrip.fill(ledStrip.Color(255,255,255), 0, LED_COUNT);
  	ledStrip.setBrightness(BRIGHTNESS);
  	ledStrip.show();
  	} else if (!sw2.getState() && sw2.getLastState()){
  	ledStrip.clear();
  	ledStrip.show();
  	}
  }

  // Update PostBoxSwitch states for next loop
  sw1.updateLastState();
  sw2.updateLastState();

}


void PostBox::powerOff(void) {

  ledStrip.clear();
  ledStrip.show();
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


void PostBox::adc_calibrate(void){
esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1,ADC_ATTEN_DB_6, ADC_WIDTH_BIT_13, 0, adc_chars);
}

int PostBox::analogRead_cal(uint8_t channel, adc_atten_t attenuation) {
  adc1_channel_t channelNum;

  adc1_config_channel_atten(channelNum, attenuation);
  return esp_adc_cal_raw_to_voltage(analogRead(channel), adc_chars);
}

// int PostBox::analogRead_no_cal(void) {

//   adc1_config_channel_atten(ADC1_CHANNEL_1, ADC_ATTEN_DB_6);

//   int adcVal = adc1_get_raw(ADC1_CHANNEL_1);
//   float vout = adcVal * 1300 /
//   return esp_adc_cal_raw_to_voltage(analogRead(channel), adc_chars);
// }

float PostBox::readVoltage(void) {
	// #ifdef ARDUINO_IOTPOSTBOX_V1
	//TODO: BATTERY, USB POWER AND CHARGING SENSE
  // batVoltage = analogRead(VBAT_SENSE);

  float volts = -1;
  //Characterize ADC at particular atten
  // esp_adc_cal_characteristics_t *adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));

  esp_err_t ret;
  ret = esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP);
  if (ret == ESP_ERR_NOT_SUPPORTED) {
      Serial.printf( "Calibration scheme not supported, skip software calibration\n");
  } else if (ret == ESP_ERR_INVALID_VERSION) {
      Serial.printf( "eFuse not burnt, skip software calibration\n");
  } else if (ret == ESP_OK) {
      // Serial.printf( "eFuse burnt, software calibration can proceed\n");

      // cali_enable = true;
      // esp_adc_cal_characterize(ADC_UNIT_1, ADC_EXAMPLE_ATTEN, ADC_WIDTH_BIT_DEFAULT, 0, &adc1_chars);
      // esp_adc_cal_characterize(ADC_UNIT_2, ADC_EXAMPLE_ATTEN, ADC_WIDTH_BIT_DEFAULT, 0, &adc2_chars);
  


      /*
      esp_adc_cal_characteristics_t adc_chars;
      esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_6, ADC_WIDTH_BIT_DEFAULT, ESP_ADC_CAL_VAL_DEFAULT_VREF, &adc_chars);
      //Check type of calibration value used to characterize ADC
      if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
          Serial.printf("eFuse Vref\n");
      } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
          Serial.printf("Two Point\n");
      } else {
          Serial.printf("Default\n");
      }
      */
  }

  int read_raw = adc1_get_raw(ADC1_CHANNEL_1);
  // int adcv =  analogRead(VBAT_SENSE);
  // int analogVolts = analogReadMilliVolts(VBAT_SENSE);
  // int analogVolts = analogReadMilliVolts(VBAT_SENSE);
  uint32_t analogVolts = esp_adc_cal_raw_to_voltage(read_raw, adc_chars);
  // volts =  adcv;
  
  float vbat = analogVolts * (442 + 160)/160;

  // Serial.printf("The internal VCC reads %1.3f volts. adcv: %d read_raw: %d. analogVolts: %d\n", volts/1000, adcv, read_raw, analogVolts);
  Serial.printf("The internal VCC reads %dmV --> %1.3fV VoltageDivider: %1.3fV - read_raw: %d\n", analogVolts, (float)analogVolts/1000, vbat/1000, read_raw);
	// return volts;
	return vbat/1000;

/*
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
	return -1;*/
}


void PostBox::updateLedStrip(void) {
  // If Switch_2 (used while opening the postbox) is on, turn on the lights
  // if(digitalRead(switches[1].pin)){
  bool lightMode = false;
  if(digitalRead(sw2.getPin()) == HIGH){
    ledStrip.fill(ledStrip.Color(255,255,255), 0, LED_COUNT);
    ledStrip.setBrightness(BRIGHTNESS);
    ledStrip.show();
    lightMode = true;
  } else {
    ledStrip.clear();
    ledStrip.show();
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
      uint64_t GPIO_reason = esp_sleep_get_wakeup_cause();
      int gpioWake = log(GPIO_reason)/log(2);
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
  esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK,ESP_EXT1_WAKEUP_ANY_HIGH);
}

void PostBox::turnOffDevice(void) {
  ledStrip.clear();
  ledStrip.show();
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
  
  String msg_pub ="{\"wake_up_pin\": ";
  msg_pub += String(button);
  msg_pub = msg_pub + " ,\"vcc\": " + String(readVoltage());
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


  #ifdef USE_TP4056
  msg_pub = msg_pub + " ,\"chrg\": " + !digitalRead(CHRG_PIN);
  msg_pub = msg_pub + " ,\"stdby\": " + !digitalRead(STDBY_PIN);
  #endif
  msg_pub +=" }";
  
  wakeUpPublished = mqtt->publish(topic.c_str(), msg_pub.c_str());
}
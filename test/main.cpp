#include <Arduino.h>


// Main variables:
// #define DEBUG_ESP_CORE

// Enable wifi diagnostic using platformio build_glag: -D ENABLE_SERIAL_DEBUG:
#define ENABLE_SERIAL_DEBUG true


// Device configurations
unsigned long currentLoopMillis = 0;
unsigned long previousPublishMillis = 0;
unsigned long previousMainLoopMillis = 0;


// WebConfigServer Configuration
// -----------------------------
#include "WebConfigServer.h"
WebConfigServer config;   // <- global configuration object


#include <PubSubClient.h>
PubSubClient * mqttClient;


// WS2812B LED strip
// -----------------
#include <Adafruit_NeoPixel.h>

#ifdef ARDUINO_IOTPOSTBOX_V1
  #define LED_PIN    PIN_NEOPIXEL //GPIO17
#else
  #define LED_PIN    15 //GPIO15
#endif

#define LED_COUNT  4
#define BRIGHTNESS 150
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);


// PostBoxSwitch sensors
// ---------------------
#include "PostBoxSwitch.h"
#ifndef ARDUINO_IOTPOSTBOX_V1
  #define SW1_PIN         12
  #define SW2_PIN         13
  #define KEEP_WAKE_PIN   14    // Pin which is used to Keep ESP awake until job is finished
#endif

// PostBoxSwitch sw1(0, "Switch_1");
PostBoxSwitch sw1(SW1_PIN, "Switch_1");
PostBoxSwitch sw2(SW2_PIN, "Switch_2");


int button = -1;              // Variable to store the button which triggered the bootup
bool wakeUpPublished = false;
bool setupDone = false;


// PostBox sleep configs
// ---------------------
#define BUTTON_PIN_BITMASK 0x10 // GPIO4 --> 2^4 in hex
RTC_DATA_ATTR int bootCount = 0;


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


float readVoltage() {
  #ifdef ARDUINO_IOTPOSTBOX_V1
    //TODO: BATTERY, USB POWER AND CHARGING SENSE
    return -1;
  #else
    #ifdef USE_TP4056
      int sensorValue = analogRead(A0);
      float volts = sensorValue * (4.333 / 1023.0);
      Serial.printf("The internal VCC reads %1.3f volts. CHRG: %d - STDBY: %d\n", volts , !digitalRead(CHRG_PIN), !digitalRead(STDBY_PIN));
    #else
      float volts = ESP.getVcc();
      Serial.printf("The internal VCC reads %1.3f volts\n", volts / 1000);
      return volts;
    #endif
  #endif
  return -1;
}


void print_wakeup_reason(){
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


void setupDeepSleep(){
  //Increment boot number and print it every reboot
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));
  print_wakeup_reason();

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


void turnESPOff (void){
  strip.clear();
  strip.show();
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


void publishWakeUp(String topic_end){

  String topic = config.getDeviceTopic() + topic_end;
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

  wakeUpPublished = mqttClient->publish(topic.c_str(), msg_pub.c_str());
}

void updateLights(void){
  // If Switch_2 (used while opening the postbox) is on, turn on the lights
  // if(digitalRead(switches[1].pin)){
  bool lightMode = false;
  if(digitalRead(sw2.getPin()) == HIGH){
    strip.fill(strip.Color(255,255,255), 0, LED_COUNT);
    strip.setBrightness(BRIGHTNESS);
    strip.show();
    lightMode = true;
  } else {
    strip.clear();
    strip.show();
    lightMode = false;
  }

  // Serial.printf("updateLights() done!: lightMode = %s\n", lightMode ? "ON": "OFF");
}


void setupPostBox(void){

  #ifndef ARDUINO_IOTPOSTBOX_V1
  // ESP wake Pin, the pin which keeps CH_PD HIGH, a requirement for normal functioning of ESP8266
  pinMode(KEEP_WAKE_PIN, OUTPUT);
  digitalWrite(KEEP_WAKE_PIN, HIGH);
  #else
  pinMode(LDO2_EN_PIN, OUTPUT);
  digitalWrite(LDO2_EN_PIN, HIGH);
  #endif

  #ifdef USE_TP4056
  pinMode(CHRG_PIN, INPUT);
  pinMode(STDBY_PIN, INPUT);
  #endif

  setupDeepSleep();

  strip.begin();

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
  long bootDelay = millis();
  // Serial.printf("\tbootDelay: %ld - pin booter: %d\n", bootDelay, button);

  // Disconnect wifi if the reboot was not originated by GPIOs
  // but probably caused by updating firmware by UART
  // if (button == -1) WiFi.disconnect();

  updateLights();
}



// Websocket functions to publish:
String getLoopTime(){ return String(currentLoopMillis - previousMainLoopMillis);}
String getRSSI(){ return String(WiFi.RSSI());}
String getHeapFree(){ return String((float)GET_FREE_HEAP/1000);}
String getVCC(){ return String((float)readVoltage());}




void setup() {
  Serial.begin(115200);
  // while (!Serial);  // Wait until the Serial is available
  #ifdef ENABLE_SERIAL_DEBUG
    Serial.setDebugOutput(true);
  #endif

  setupPostBox();
  config.begin();
 
  config.addDashboardObject("heap_free", getHeapFree);
  config.addDashboardObject("loop", getLoopTime);
  config.addDashboardObject("RSSI", getRSSI);
  config.addDashboardObject("VCC", getVCC);

  config.setPreSleepRoutine(turnESPOff);

  mqttClient = config.getMQTTClient();

  // Recheck switches state:
  sw1.readCurrentState();
  sw2.readCurrentState();

  updateLights();


  publishWakeUp("wakeup");
  setupDone = true;

  Serial.printf(" -- GPIO %d state: %s lastState: %s count: %d\n", sw1.getPin(), sw1.getState() ? "true": "false", sw1.getLastState() ? "true": "false", sw1.getCount());
  Serial.printf(" -- GPIO %d state: %s lastState: %s count: %d\n", sw2.getPin(), sw2.getState() ? "true": "false", sw2.getLastState() ? "true": "false", sw2.getCount());

  Serial.println("###  Looping time\n");
}

void loop() {

  currentLoopMillis = millis();

  #if defined(USE_TP4056) && defined(NO_SLEEP_WHILE_CHARGING)
    if ( (!wakeUpPublished ) || !digitalRead(CHRG_PIN) == true ) config.services.deep_sleep.enabled = false;    
    else config.services.deep_sleep.enabled = true;  
    // TODO: save that deep sleep enabled within the config file instead hardcoding the variable
  #else
    // if ( !wakeUpPublished ) config.services.deep_sleep.enabled = false;    
    // else config.services.deep_sleep.enabled = true; 
    // TODO: save that deep sleep enabled within the config file instead hardcoding the variable

  #endif

  config.loop();

  // Reconnection loop:
  // if (WiFi.status() != WL_CONNECTED) {
  //   config.begin();
  //   networkRestart();
  //   config.configureServer(&server);
  // }

  // Check if there was an interrupt casued by one PostBoxSwitch
  if ( sw1.checkChange() || sw2.checkChange() ) publishWakeUp("wakeup");

  // Check the PostBoxSwitch sw2 to tur on/off the LED strip
  if (sw2.getState() != sw2.getLastState() ){  	
    if(sw2.getState() && !sw2.getLastState()){
  	strip.fill(strip.Color(255,255,255), 0, LED_COUNT);
  	strip.setBrightness(BRIGHTNESS);
  	strip.show();
  	} else if (!sw2.getState() && sw2.getLastState()){
  	strip.clear();
  	strip.show();
  	}
  }



  // Main Loop:
  if(mqttClient->connected() && (config.device.publish_time_ms != 0) &&
      (currentLoopMillis - previousPublishMillis > (unsigned)config.device.publish_time_ms)) {
    previousPublishMillis = currentLoopMillis;
    // Here starts the MQTT publish loop configured:

    if (config.services.deep_sleep.enabled) {
      String topic = config.getDeviceTopic() + "sleepCountdown";
      String msg_pub ="{\"time_to_sleep\": ";
      msg_pub += String((config.services.deep_sleep.sleep_delay*1000 + config.getDeviceSetupTime() - (currentLoopMillis))/1000);
      msg_pub +=" }";
      mqttClient->publish(topic.c_str(), msg_pub.c_str());
    }

    publishWakeUp("data");
  }


  // Update PostBoxSwitch states for next loop
  sw1.updateLastState();
  sw2.updateLastState();
  
  previousMainLoopMillis = currentLoopMillis;
}

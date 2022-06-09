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
#include "WebConfigServer.h"
WebConfigServer config;   // <- global configuration object


#include <PubSubClient.h>
PubSubClient * mqttClient;


// WS2812B LED strip
#include <Adafruit_NeoPixel.h>
#define LED_PIN    15 //GPIO15
#define LED_COUNT  4
#define BRIGHTNESS 150
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);


// PostBox:
struct  PostBoxSwitch{
  String name;
  int pin;
  volatile int state;
  volatile int lastState;
  int count;
  volatile unsigned long lastChange;
};

PostBoxSwitch switches[2] = { { "Switch_1", 12, 0, 0, 0, 0}, 
                              { "Switch_2", 13, 0, 0, 0, 0}};
int debounceMs = 50;          // To ignore button signals changes faster than this debounce ms
int button = -1;              // Variable to store the button which triggered the bootup
int wake = 14;                // Pin which is used to Keep ESP awake until job is finished
bool wakeUpPublished = false;
bool setupDone = false;

//Using tp4056 charger with battery fedback to ADC0 (max 1v!) and tp4056 chrg and stdby pins feedback:
#define USE_TP4056
#define NO_SLEEP_WHILE_CHARGING    // While charging the ESP won't go to sleep. Comment out this line if you want to avoid that


#ifdef USE_TP4056
  int chrg = 4;               // Pin for charge battery. LOW when usb is connected and baterry is being charged.
  int stdby = 5;              // Pin for stdby battery charge. LOW for battery charge termination.
#else
  ADC_MODE(ADC_VCC);          // Allows to monitor the internal VCC level; it varies with WiFi load
#endif

float readVoltage() {
  #ifdef USE_TP4056
    int sensorValue = analogRead(A0);
    float volts = sensorValue * (4.333 / 1023.0);
    Serial.printf("The internal VCC reads %1.3f volts. CHRG: %d - STDBY: %d\n", volts , !digitalRead(chrg), !digitalRead(stdby));
  #else
    float volts = ESP.getVcc();
    Serial.printf("The internal VCC reads %1.3f volts\n", volts / 1000);
  #endif
  return volts;
}

void publishWakeUp(String topic_end){

  String topic = config.getDeviceTopic() + topic_end;
  String msg_pub ="{\"wake_up_pin\": ";
  msg_pub += String(button);
  msg_pub = msg_pub + " ,\"vcc\": " + String(readVoltage());
  msg_pub = msg_pub + " ,\"rssi\": " + String(WiFi.RSSI());

  int countSwitches = sizeof switches / sizeof *switches;
  for(int i=0; i< countSwitches; i++) {
    msg_pub = msg_pub + " ,\"GPIO_" + switches[i].pin + "_counter\": " + String(switches[i].count);
    msg_pub = msg_pub + " ,\"GPIO_" + switches[i].pin + "_state\": " + (switches[i].state ? "true" : "false");
  }

  #ifdef USE_TP4056
  msg_pub = msg_pub + " ,\"chrg\": " + !digitalRead(chrg);
  msg_pub = msg_pub + " ,\"stdby\": " + !digitalRead(stdby);
  #endif
  msg_pub +=" }";


  wakeUpPublished = mqttClient->publish(topic.c_str(), msg_pub.c_str());
}

void updateLights(void){
  // If Switch_2 (used while opening the postbox) is on, turn on the lights
  if(digitalRead(switches[1].pin)){
    strip.fill(strip.Color(255,255,255), 0, LED_COUNT);
    strip.setBrightness(BRIGHTNESS);
    strip.show();
  } else {
    strip.clear();
    strip.show();
  }
}

ICACHE_RAM_ATTR void detectsChange(int pin) {

  int pinState = digitalRead(pin);
  int countSwitches = sizeof switches / sizeof *switches;
  for(int i=0; i< countSwitches; i++) {
    if (pin == switches[i].pin){

      // Ignore dupe readings
      if ( switches[i].state == pinState) return;

      // Ignore events faster than debounceMs
      if (millis() - switches[i].lastChange < debounceMs) return;

      switches[i].lastChange = millis();
      switches[i].state = pinState;

      // Increase counter if pin status change from close to open
      if (switches[i].state && switches[i].lastState != switches[i].state ){
        switches[i].count++;
      }

      // Handle LED strip on/off if for Switch_2:
      if (switches[i].name == "Switch_2"){
        if(switches[i].state && !switches[i].lastState){
          strip.fill(strip.Color(255,255,255), 0, LED_COUNT);
          strip.setBrightness(BRIGHTNESS);
          strip.show();
        } else if (!switches[i].state && switches[i].lastState){
          strip.clear();
          strip.show();
        }
      }
      
      Serial.printf(" -- GPIO %d state: %s lastState: %s count: %d\n", switches[i].pin, switches[i].state ? "true": "false", switches[i].lastState ? "true": "false", switches[i].count);
      switches[i].lastState = switches[i].state;
      
      if (setupDone) publishWakeUp("wakeup");
    return;
    }
  }

}

ICACHE_RAM_ATTR void pin12ISR() {detectsChange(switches[0].pin);}
ICACHE_RAM_ATTR void pin13ISR() {detectsChange(switches[1].pin);}


void setupPostBox(void){

  // ESP Awake Pin, the pin which keeps CH_PD HIGH, a requirement for normal functioning of ESP8266
  pinMode(wake, OUTPUT);
  digitalWrite(wake, HIGH);

  #ifdef USE_TP4056
  pinMode(chrg, INPUT);
  pinMode(stdby, INPUT);
  #endif

  strip.begin();

  //Check which button was pressed
  int countSwitches = sizeof switches / sizeof *switches;
  for(int i=0; i< countSwitches; i++) {
    pinMode(switches[i].pin, INPUT);
    if(digitalRead(switches[i].pin) == HIGH) {
      button = switches[i].pin;
      switches[i].count++;
      break;
    }
  }

  Serial.printf("\n -- Pin booter button: %d\n", button);

  attachInterrupt(digitalPinToInterrupt(switches[0].pin), pin12ISR , CHANGE);
  attachInterrupt(digitalPinToInterrupt(switches[1].pin), pin13ISR , CHANGE);

  // long bootDelay = millis() - connectionTime;
  long bootDelay = millis();
  Serial.printf("\tbootDelay: %ld - pin booter: %d\n", bootDelay, button);

  // Disconnect wifi if the reboot was not originated by GPIOs
  // but probably caused by updating firmware by UART
  if (button == -1) WiFi.disconnect();

  updateLights();
}

void turnESPOff (void){
    strip.clear();
    strip.show();
    Serial.println("CH_PD disabled");
    delay(10);
    digitalWrite(wake, LOW); //Turns the ESP OFF
}



// Websocket functions to publish:
String getLoopTime(){ return String(currentLoopMillis - previousMainLoopMillis);}
String getRSSI(){ return String(WiFi.RSSI());}
String getHeapFree(){ return String((float)GET_FREE_HEAP/1000);}
String getVCC(){ return String((float)readVoltage());}




void setup() {
  Serial.begin(115200);
  
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
  switches[0].state = digitalRead(switches[0].pin);
  switches[0].lastState = switches[0].state;
  switches[1].state = digitalRead(switches[1].pin);
  switches[1].lastState = switches[1].state;
  updateLights();


  publishWakeUp("wakeup");
  setupDone = true;
  Serial.println("###  Looping time\n");

  Serial.printf(" -- GPIO %d state: %s lastState: %s count: %d\n", switches[1].pin, switches[1].state ? "true": "false", switches[1].lastState ? "true": "false", switches[1].count);

}

void loop() {

  currentLoopMillis = millis();

  #if defined(USE_TP4056) && defined(NO_SLEEP_WHILE_CHARGING)
    if ( (!wakeUpPublished ) || !digitalRead(chrg) == true ) config.services.deep_sleep.enabled = false;    
    else config.services.deep_sleep.enabled = true;  
  #else
    if ( !wakeUpPublished ) config.services.deep_sleep.enabled = false;    
    else config.services.deep_sleep.enabled = true;  
  #endif

  config.loop();

  // Reconnection loop:
  // if (WiFi.status() != WL_CONNECTED) {
  //   config.begin();
  //   networkRestart();
  //   config.configureServer(&server);
  // }

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

  previousMainLoopMillis = currentLoopMillis;
}

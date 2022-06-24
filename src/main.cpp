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
PubSubClient * mainClientMqtt;


// PostBox object
// ---------------------
#include "PostBox.h"
PostBox postbox;



void turnESPOff (void){

  postbox.powerOff();

  // digitalWrite(LDO2_EN_PIN, LOW);
  // Serial.println("CH_PD disabled");
  // delay(10);

  // //TODO: RUTINE FOR ESP32S2 
  // #ifndef ARDUINO_IOTPOSTBOX_V1
  //   digitalWrite(KEEP_WAKE_PIN, LOW); //Turns the ESP OFF
  // #endif
  // esp_wifi_stop();
  // delay(100);
  // esp_deep_sleep_start();

}

// #include "driver/adc.h"
// #include "esp_adc_cal.h"
// #include "driver/soc_caps.h"

#include <driver/adc.h>

// Websocket functions to publish:
String getLoopTime(){ return String(currentLoopMillis - previousMainLoopMillis);}
String getRSSI(){ return String(WiFi.RSSI());}
String getHeapFree(){ return String((float)GET_FREE_HEAP/1000);}
String getVCC(){ return String((float)postbox.readVoltage());}
String getCalVCC(){ return String((float)postbox.analogRead_cal(ADC1_CHANNEL_1 , ADC_ATTEN_DB_6));}



void setup() {
  Serial.begin(115200);
  while (!Serial);  // Wait until the Serial is available
  #ifdef ENABLE_SERIAL_DEBUG
    Serial.setDebugOutput(true);
  #endif

  //   ESP_ERROR_CHECK(adc1_config_width(SOC_ADC_MAX_BITWIDTH));
  // ESP_ERROR_CHECK(adc1_config_channel_atten(ADC1_CHANNEL_1, ADC_EXAMPLE_ATTEN));

  adc1_config_width(ADC_WIDTH_BIT_13);
  adc1_config_channel_atten(ADC1_CHANNEL_1, ADC_ATTEN_DB_6);



  postbox.setup();
  config.begin();

  config.setPreSleepRoutine(turnESPOff);
  
  
  mainClientMqtt = config.getMQTTClient();
  postbox.setMQTTClient(config.getMQTTClient());
  postbox.setMQTTBaseTopic(config.getDeviceTopic());
  postbox.init();
 
  config.addDashboardObject("heap_free", getHeapFree);
  config.addDashboardObject("loop", getLoopTime);
  config.addDashboardObject("RSSI", getRSSI);
  config.addDashboardObject("VCC", getVCC);
  config.addDashboardObject("Cal_VCC", getCalVCC);


  Serial.println("###  Looping time\n");


}

void loop() {

  currentLoopMillis = millis();

  config.loop();

  postbox.loop();


  // Reconnection loop:
  // if (WiFi.status() != WL_CONNECTED) {
  //   config.begin();
  //   networkRestart();
  //   config.configureServer(&server);
  // }

  // // Check if there was an interrupt casued by one PostBoxSwitch
  // if ( sw1.checkChange() || sw2.checkChange() ) publishWakeUp("wakeup");

  // // Check the PostBoxSwitch sw2 to tur on/off the LED strip
  // if (sw2.getState() != sw2.getLastState() ){  	
  //   if(sw2.getState() && !sw2.getLastState()){
  // 	strip.fill(strip.Color(255,255,255), 0, LED_COUNT);
  // 	strip.setBrightness(BRIGHTNESS);
  // 	strip.show();
  // 	} else if (!sw2.getState() && sw2.getLastState()){
  // 	strip.clear();
  // 	strip.show();
  // 	}
  // }



  // Main Loop:
  if(mainClientMqtt->connected() && (config.device.publish_time_ms != 0) &&
      (currentLoopMillis - previousPublishMillis > (unsigned)config.device.publish_time_ms)) {
    previousPublishMillis = currentLoopMillis;
    // Here starts the MQTT publish loop configured:

    if (config.services.deep_sleep.enabled) {
      String topic = config.getDeviceTopic() + "sleepCountdown";
      String msg_pub ="{\"time_to_sleep\": ";
      msg_pub += String((config.services.deep_sleep.sleep_delay*1000 + config.getDeviceSetupTime() - (currentLoopMillis))/1000);
      msg_pub +=" }";
      mainClientMqtt->publish(topic.c_str(), msg_pub.c_str());
    }

    postbox.publishWakeUp("data");
  }
 
  previousMainLoopMillis = currentLoopMillis;
}

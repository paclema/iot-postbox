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



// LoRaWAN network
// ---------------
#pragma once
#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

#define hal_init LMICHAL_init
// #ifdef COMPILE_REGRESSION_TEST
// # define FILLMEIN 0
// #else
// # warning "You must replace the values marked FILLMEIN with real values from the TTN control panel!"
// # define FILLMEIN (#dont edit this, edit the lines that use FILLMEIN)
// #endif



// This EUI must be in little-endian format, so least-significant-byte
// first. When copying an EUI from ttnctl output, this means to reverse
// the bytes. For TTN issued EUIs the last bytes should be 0xD5, 0xB3,
// 0x70.
static const u1_t PROGMEM APPEUI[8]= { 0xAA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}

// This should also be in little endian format, see above.
static const u1_t PROGMEM DEVEUI[8]= { 0xEF, 0xCD, 0xAB, 0x89, 0x67, 0x45, 0x23, 0x01 };
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}

// This key should be in big endian format (or, since it is not really a
// number but a block of memory, endianness does not really apply). In
// practice, a key taken from ttnctl can be copied as-is.
static const u1_t PROGMEM APPKEY[16] = { 0xAA, 0xDE, 0x3A, 0xC4, 0xE7, 0x33, 0x27, 0x8C, 0xD2, 0xEA, 0x4B, 0x3E, 0xDE, 0xF3, 0x09, 0x0B };

void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}

static uint8_t mydata[] = "Hello, world!";
static osjob_t sendjob;

// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 60;



// Pin mapping
const lmic_pinmap lmic_pins = {
    .nss = RFM95W_NSS_PIN,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = RFM95W_RESET_PIN,
    .dio = {RFM95W_DIO0_PIN,
            RFM95W_DIO1_PIN,
            RFM95W_DIO2_PIN
            },
};

// const lmic_pinmap lmic_pins = {
//     .nss = 10,
//     .rxtx = LMIC_UNUSED_PIN,
//     .rst = 14,
//     .dio = {2, 3, 4},
// };

void printHex2(unsigned v) {
    v &= 0xff;
    if (v < 16)
        Serial.print('0');
    Serial.print(v, HEX);
}

void do_send(osjob_t* j){
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else {
        // Prepare upstream data transmission at the next possible time.
        LMIC_setTxData2(1, mydata, sizeof(mydata)-1, 0);
        Serial.println(F("Packet queued"));
    }
    // Next TX is scheduled after TX_COMPLETE event.
}
void onEvent (ev_t ev) {
    Serial.print(os_getTime());
    Serial.print(": ");
    switch(ev) {
        case EV_SCAN_TIMEOUT:
            Serial.println(F("EV_SCAN_TIMEOUT"));
            break;
        case EV_BEACON_FOUND:
            Serial.println(F("EV_BEACON_FOUND"));
            break;
        case EV_BEACON_MISSED:
            Serial.println(F("EV_BEACON_MISSED"));
            break;
        case EV_BEACON_TRACKED:
            Serial.println(F("EV_BEACON_TRACKED"));
            break;
        case EV_JOINING:
            Serial.println(F("EV_JOINING"));
            break;
        case EV_JOINED:
            Serial.println(F("EV_JOINED"));
            {
              u4_t netid = 0;
              devaddr_t devaddr = 0;
              u1_t nwkKey[16];
              u1_t artKey[16];
              LMIC_getSessionKeys(&netid, &devaddr, nwkKey, artKey);
              Serial.print("netid: ");
              Serial.println(netid, DEC);
              Serial.print("devaddr: ");
              Serial.println(devaddr, HEX);
              Serial.print("AppSKey: ");
              for (size_t i=0; i<sizeof(artKey); ++i) {
                if (i != 0)
                  Serial.print("-");
                printHex2(artKey[i]);
              }
              Serial.println("");
              Serial.print("NwkSKey: ");
              for (size_t i=0; i<sizeof(nwkKey); ++i) {
                      if (i != 0)
                              Serial.print("-");
                      printHex2(nwkKey[i]);
              }
              Serial.println();
            }
            // Disable link check validation (automatically enabled
            // during join, but because slow data rates change max TX
	    // size, we don't use it in this example.
            LMIC_setLinkCheckMode(0);
            break;
        /*
        || This event is defined but not used in the code. No
        || point in wasting codespace on it.
        ||
        || case EV_RFU1:
        ||     Serial.println(F("EV_RFU1"));
        ||     break;
        */
        case EV_JOIN_FAILED:
            Serial.println(F("EV_JOIN_FAILED"));
            break;
        case EV_REJOIN_FAILED:
            Serial.println(F("EV_REJOIN_FAILED"));
            break;
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
            if (LMIC.txrxFlags & TXRX_ACK)
              Serial.println(F("Received ack"));
            if (LMIC.dataLen) {
              Serial.print(F("Received "));
              Serial.print(LMIC.dataLen);
              Serial.println(F(" bytes of payload"));
            }
            // Schedule next transmission
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            break;
        case EV_LOST_TSYNC:
            Serial.println(F("EV_LOST_TSYNC"));
            break;
        case EV_RESET:
            Serial.println(F("EV_RESET"));
            break;
        case EV_RXCOMPLETE:
            // data received in ping slot
            Serial.println(F("EV_RXCOMPLETE"));
            break;
        case EV_LINK_DEAD:
            Serial.println(F("EV_LINK_DEAD"));
            break;
        case EV_LINK_ALIVE:
            Serial.println(F("EV_LINK_ALIVE"));
            break;
        /*
        || This event is defined but not used in the code. No
        || point in wasting codespace on it.
        ||
        || case EV_SCAN_FOUND:
        ||    Serial.println(F("EV_SCAN_FOUND"));
        ||    break;
        */
        case EV_TXSTART:
            Serial.println(F("EV_TXSTART"));
            break;
        case EV_TXCANCELED:
            Serial.println(F("EV_TXCANCELED"));
            break;
        case EV_RXSTART:
            /* do not print anything -- it wrecks timing */
            break;
        case EV_JOIN_TXCOMPLETE:
            Serial.println(F("EV_JOIN_TXCOMPLETE: no JoinAccept"));
            break;

        default:
            Serial.print(F("Unknown event: "));
            Serial.println((unsigned) ev);
            break;
    }
}





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

bool configuredSleep = false;

// Websocket functions to publish:
String getLoopTime(){ return String(currentLoopMillis - previousMainLoopMillis);}
String getRSSI(){ return String(WiFi.RSSI());}
String getHeapFree(){ return String((float)GET_FREE_HEAP/1000);}
String getVCC(){ return String((float)postbox.readVoltage(),3);}
String getVBat(){ return String((float)postbox.getVBat()/1000,4);}
String getVBatNow(){ return String((float)postbox.getVBatNow()/1000,3);}
String getVBus(){ return String((float)postbox.getVBus()/1000,3);}
String getVBatStat(){ return String(postbox.getVBatStat() ? "true" : "false");}
String getChargingStatus(){ return String((int)postbox.getChargingStatus());}
String getPowerStatus(){ return String((int)postbox.getPowerStatus());}
String getConfiguredSleep(){ return String(configuredSleep ? "true" : "false");}


void setup() {
  Serial.begin(115200);
  while (!Serial);  // Wait until the Serial is available
  #ifdef ENABLE_SERIAL_DEBUG
    Serial.setDebugOutput(true);
  #endif

  SPI.begin(RFM95W_SCK_PIN, RFM95W_MISO_PIN, RFM95W_MOSI_PIN, RFM95W_NSS_PIN);


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
  config.addDashboardObject("VBat", getVBat);
  config.addDashboardObject("VBatNow", getVBatNow);
  config.addDashboardObject("VBus", getVBus);
  config.addDashboardObject("VBatStat", getVBatStat);
  config.addDashboardObject("ChargingStatus", getChargingStatus);
  config.addDashboardObject("PowerStatus", getPowerStatus);
  config.addDashboardObject("configuredSleep", getConfiguredSleep);

    // LMIC init
    os_init();
    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();

    // Start job (sending automatically starts OTAA too)
    do_send(&sendjob);


  Serial.println("###  Looping time\n");

}

void loop() {
    os_runloop_once();
  currentLoopMillis = millis();

  config.loop();

  postbox.loop();


  // Reconnection loop:
  // if (WiFi.status() != WL_CONNECTED) {
  //   config.begin();
  //   networkRestart();
  //   config.configureServer(&server);
  // }



  // TODO: clean defined process
  // #if defined(USE_TP4056) && defined(NO_SLEEP_WHILE_CHARGING)
    PowerStatus powerNow = postbox.getPowerStatus();
    if ( !postbox.isWakeUpPublished() || powerNow == PowerStatus::USBPowered || powerNow == PowerStatus::BatteryAndUSBPowered ){
      config.services.deep_sleep.enabled = false;
      // Serial.println("Deep sleep   ---> OFF");
      configuredSleep = false;
      } 
    else {
      config.services.deep_sleep.enabled = true;
      // Serial.println("Deep sleep   ---> ON");
      configuredSleep = true;
    }
    // TODO: save that deep sleep enabled within the config file instead hardcoding the variable
  // #else
    // if ( !wakeUpPublished ) config.services.deep_sleep.enabled = false;    
    // else config.services.deep_sleep.enabled = true; 
    // TODO: save that deep sleep enabled within the config file instead hardcoding the variable

  // #endif


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

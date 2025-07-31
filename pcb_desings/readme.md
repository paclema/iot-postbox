## IoT-PostBox  PCB desings: ##

Here you will find some KiCad desings for this project. There are several versions and the main differences are:

* [**IoT-postbox**](iot-postbox/)

In this version, the microcontroller wakes-up when one of the 2 SW inputs triggers the enable pin of the microcontroller. 
This design is based in an ESP8266 MCU. Within the folder you can find 2 versions:

  
| v0.0                                                            | v0.3          |
|:---------------------------------------------------------------:|:-------------:|
| ![Alt text](iot-postbox/docs/v0.0/pcb_3d.PNG?raw=true "v0.0")   | ![Alt text](iot-postbox/docs/v0.3/3dview.png?raw=true "v0.3 ") |
|  *  First version                                               |  * Added WS2812B led strip<br>  * Updated battery sense resistors<br> * Changed Switch pinouts   |


* [**IoT-postbox v1**](iot-postbox_v1/):

In this version, the CH_PU pin of the MCU is not triggered but ESP32S2 sleep functions using digital triggers are used.
This design is based in an ESP32-S2 MCU. It also provides Lora support with an RFM95W module. It can be added a 16MB flash and 8MB PSRAM.

| v1.2                                                            |
|:---------------------------------------------------------------:|
| ![Alt text](iot-postbox_v1/docs/v1.2/iot-postbox_top_xray.png "v1.2")   |
| ![Alt text](iot-postbox_v1/docs/v1.2/iot-postbox_bottom_xray.png "v1.2")   |


### Submodules installation for KiCad:
Under the folder /libs you can find several symbols and footprints from other repositories.
To pull KiCad lib submodules run: `git submodule update --init --recursive`

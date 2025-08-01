# iot_button

## Features:

Web application:
* Angular 10 and Bootstrap 4 web application to configure the device and visualize data.
* Nebular UI Angular lib.
* ng2-charts to visualize WS data graphs on dashboard web server endpoint.
* Gzipped compression to store and serve the Webapp.

Firmware:
* ESP32 support.
* Improved platformio targets and ESP partitions for Wemos D1 mini and Lolin D32 pro and OTA updates.
* Store configs as JSON and certs on SPI Flash File System (SPIFFS).
* AP+STA wifi mode with wifimulti and mDNS support
* FTP server to easily modify files stored on SPIFFS.
* WrapperOTA class to handle OTA updates.
* WrapperWebSockets class to handle WS communication between device and webserver dashboard endpoint.
* MQTT client as QoS2 to detect disconnection and enabled connection using user&pass and/or certificates.
* Added configurable DeepSleep modes for ESP8266.
* Webserver or AsyncWebserver support.
* Enabled lwip NAT features for ESP8266 and ESP32.
* Added configurable NTP server.

## Requirements:

* Wemos D1 mini (ESP8266) or Lolin D32 pro (ESP32).
* push button on RST --> this will triger the push action after restart the board.
* D0 (GPIO16) connected to RST so Deep Sleep can wake up after the sleep time configured.
* CHPD (or EN) to VCC.
* Wemos D1 mini can use USB power or 3v3-5v on 5v pin.

### Add a new configuration object:
1. Update the new object in the configuration file _/data/config.json_
2. Add the struct object in the configuration class _/lib/WebConfigServer/WebConfigServer.h_
3. Update the parse function to link the json object with the class struct in the configuration class _/lib/WebConfigServer/WebConfigServer.cpp_

### Web Dashboard

The static web dashboard served by the device is provided by the [WebConfigServer](https://github.com/paclema/WebConfigServer) library.
For this project, the default dashboard is sufficient and no local `/webserver` folder is included.

If you need to update or customize the dashboard:
- Go to the WebConfigServer repository and follow the instructions to build the Angular webserver.
- Copy the generated static files to the `/data/` folder in this project.
- Upload `/data/` to the device SPIFFS as described below.

### Upload _/data_ folder to ESP SPIFFS File System:

Using PlatformIO, run:
```sh
pio run -t uploadfs
```

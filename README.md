# IoT-PostBox

IoT-PostBox is a complete smart mailbox sensor solution that monitors mailbox activity and sends wireless notifications when events occur. The project consists of custom hardware designs with integrated battery management and charging system, and firmware that monitors switch sensors to detect mailbox events. Initially developed with ESP8266 (v0.x hardware), the project has evolved to ESP32-S2 (v1.x hardware) with added LoRaWAN capabilities for extended range communication beyond WiFi and a optimized power management system. The device is designed for battery-powered operation with onboard charging circuitry. When an event is detected, the device wakes from deep sleep, connects wirelessly (WiFi or LoRa), publishes the event via MQTT or LoRaWAN, and returns to sleep to maximize battery life. The system includes device configuration and management through the WebConfigServer library, which provides a web-based dashboard to visualize data and handle device configurations easily using a config.json file.

**Installation and use:** The device is installed inside the mailbox with switches positioned to detect when the mail slot or door is opened. One switch can detect mail insertion (when the slot is opened to insert mail), while another can detect mail collection (when the mailbox door is opened to retrieve mail). This allows monitoring both mail delivery and pickup events.

**Hardware evolution:**
- **v0.x (ESP8266):** WiFi-only communication, basic power management using CH_PD pin to disable the microcontroller, battery charging system, switch sensor inputs.
- **v1.x (ESP32-S2):** Enhanced with LoRaWAN capability, improved power management with deep sleep modes managed by interrupts (allowing identification of wake-up source via WAKE_UP_BITMASK), advanced battery management, native USB programming, and expanded I/O options.

> [!NOTE]  
> For detailed hardware specifications, BOMs, and pinouts, see [`pcb_desings/readme.md`](pcb_desings/readme.md).**

---

## 📁 Repository Structure

- `/src/` — Main firmware source code for IoT-PostBox
- `/pcb_desings/` — Hardware designs, documentation, BOMs, gerbers, and 3D models
- `/data/` — SPIFFS filesystem data (configs, static webapp, certificates)
- `/platformio.ini` — PlatformIO project configuration

---

## ⚡ Getting Started

1. **Select and configure your hardware:** 
   - **For IoT-PostBox PCBs:** Choose between IoT-PostBox v0.x (ESP8266-based) or IoT-PostBox v1.x (ESP32-S2-based) versions. See [`pcb_desings/readme.md`](pcb_desings/readme.md) for detailed specifications.
   - **For development boards:** The firmware also supports various ESP8266/ESP32 development boards (see available targets in `platformio.ini`), but full compatibility and functionality is only guaranteed with the custom IoT-PostBox PCBs designed for this purpose.
   - Configure the appropriate target in `platformio.ini` for your hardware version (e.g., `IoT-PostBox_v1` for IoT-PostBox v1.x PCB).
   - Edit `data/config/config.json` with your initial configuration (WiFi credentials, MQTT settings, etc.) following the WebConfigServer library standard.

2. **Upload SPIFFS data:**
   ```sh
   pio run -t uploadfs
   ```

3. **Build and upload firmware:** Use PlatformIO to compile and flash the firmware to your device.

4. **Configure the device:** Access the web dashboard hosted by the device for WiFi, MQTT, and power management setup, or update the configuration by editing `config.json` and re-uploading SPIFFS data.

---

## 📈 Web Dashboard

The static web dashboard is provided by the [WebConfigServer](https://github.com/paclema/WebConfigServer) library and served directly from the device.

If you need to customize the dashboard:
- Go to the WebConfigServer repository and follow the build instructions
- Copy the generated static files to the `/data/` folder
- Upload `/data/` to the device SPIFFS

---

## 📚 Dependencies

This project uses the following main libraries:
- [WebConfigServer](https://github.com/paclema/WebConfigServer) - Web dashboard and device configuration management
- [PowerManagement](https://github.com/paclema/PowerManagement) - Battery monitoring and power management

---

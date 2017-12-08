### Aquacontrol32

Aquacontrol32 is software used to program and control 5 led strips to create more natural sunrises and sunsets in your aquarium.

Aquacontrol32 runs on hardware based on a [MH-ET LIVE MiniKit ESP32](http://mh.nodebb.com/topic/8/new-mh-et-live-minikit-for-esp32).

#### The hardware:

<a href="https://user-images.githubusercontent.com/24290108/33763793-1df0fe98-dc12-11e7-82a5-853e5a1d07d1.JPG"><img src="https://user-images.githubusercontent.com/24290108/33763798-2385a69c-dc12-11e7-81c4-2429f2fb88fd.JPG" height="320" width="512" ></a>

#### Features:

- 5 channels led dimming at 1.22kHz with 16 bit ( 65535 steps ) resolution.

- 50 timers per channel.

- Web interface to control the device.

- Automatic NTP timekeeping.

- Timezone support.

- 1-Wire Maxim ( Dallas ) DS18B20 sensor support.

- I2C 128x64 SSD1306 OLED support.

- SPI 320x240 ILI9341 TFT support.

- XPT2046 touchscreen support.

- SPIFFS storage support.

- Temperature logging on SPIFFS.

- All settings are saved in NVS.

- Easily connect your controller to WiFi with the [ESP8266 SmartConfig Android app](https://play.google.com/store/apps/details?id=com.cmmakerclub.iot.esptouch&hl=nl).


#### You will need:

To compile or install Aquacontrol32, you will need the latest version [Arduino IDE](https://arduino.cc/), with the latest [ESP32 Arduino Core](https://github.com/espressif/arduino-esp32) installed.

Aquacontrol32 can run happily with or without OLED or TFT display.
